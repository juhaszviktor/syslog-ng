/*
 * Copyright (c) 2015 BalaBit IT Ltd, Budapest, Hungary
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "java-rewrite.h"
#include "java-helpers.h"

JNIEXPORT jstring JNICALL
Java_org_syslog_1ng_LogRewrite_getOption(JNIEnv *env, jobject obj, jlong s, jstring key)
{
    JavaRewrite *self = (JavaRewrite *)s;
    gchar *value;
    const char *key_str = (*env)->GetStringUTFChars(env, key, NULL);
    if (key_str == NULL)
    {
        return NULL;
    }

    gchar *normalized_key = normalize_key(key_str);
    value = g_hash_table_lookup(self->options, normalized_key);
    (*env)->ReleaseStringUTFChars(env, key, key_str);
    g_free(normalized_key);

    if (value)
    {
        return (*env)->NewStringUTF(env, value);
    }
    else
    {
      return NULL;
    }
}

void java_rewrite_set_option(LogRewrite *s, const gchar *key, const gchar *value)
{
  JavaRewrite *self = (JavaRewrite *)s;
  gchar *normalized_key = normalize_key(key);
  g_hash_table_insert(self->options, normalized_key, g_strdup(value));
}

void
java_rewrite_set_class_path(LogRewrite *s, const gchar *class_path)
{
  JavaRewrite *self = (JavaRewrite *)s;
  g_string_assign(self->class_path, class_path);
}

void
java_rewrite_set_class_name(LogRewrite *s, const gchar *class_name)
{
  JavaRewrite *self = (JavaRewrite *)s;
  g_free(self->class_name);
  self->class_name = g_strdup(class_name);
}

static gboolean
java_rewrite_init(LogPipe *s)
{
  JavaRewrite *self = (JavaRewrite *)s;

  self->proxy = java_rewrite_proxy_new(self->class_name, self->class_path->str, self);

  java_rewrite_proxy_init(self->proxy);
  return TRUE;
};

void
java_rewrite_process(LogRewrite *s, LogMessage **pmsg, const LogPathOptions *path_options)
{
  JavaRewrite *self = (JavaRewrite *)s;
  LogMessage *msg = log_msg_make_writable(pmsg, path_options);

  java_rewrite_proxy_process(self->proxy, msg);
  java_machine_detach_thread();
}

static LogPipe *
java_rewrite_clone(LogPipe *s)
{
  JavaRewrite *self = (JavaRewrite *) s;

  JavaRewrite *cloned = (JavaRewrite *) java_rewrite_new(log_pipe_get_config(&self->super.super));

  g_free(cloned->class_name);
  cloned->class_name = g_strdup(self->class_name);
  g_string_assign(cloned->class_path, self->class_path->str);
  clone_java_options(self->options, cloned->options);

  if (self->super.condition)
    cloned->super.condition = filter_expr_ref(self->super.condition);

  return &cloned->super.super;
};

static void
java_rewrite_free(LogPipe *s)
{
  JavaRewrite *self = (JavaRewrite *)s;

  if (self->proxy)
    java_rewrite_proxy_free(self->proxy);

  log_rewrite_free_method(s);
  g_free(self->class_name);
  g_string_free(self->class_path, TRUE);
  g_hash_table_unref(self->options);
};

LogRewrite *
java_rewrite_new(GlobalConfig *cfg)
{
  JavaRewrite *self = g_new0(JavaRewrite, 1);
  log_rewrite_init_instance(&self->super, cfg);
  self->super.super.init = java_rewrite_init;
  self->super.process = java_rewrite_process;
  self->super.super.clone = java_rewrite_clone;
  self->super.super.free_fn = java_rewrite_free;

  self->class_path = g_string_new(".");
  self->options = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  return &self->super;
};
