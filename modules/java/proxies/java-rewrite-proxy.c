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


#include "java-rewrite-proxy.h"
#include "java-logmsg-proxy.h"
#include "java-class-loader.h"
#include "java-helpers.h"
#include "messages.h"
#include <string.h>


typedef struct _JavaRewriteImpl
{
  jobject rewrite_object;
  jmethodID mi_constructor;
  jmethodID mi_init;
  jmethodID mi_process;
} JavaRewriteImpl;

struct _JavaRewriteProxy
{
  JavaVMSingleton *java_machine;
  jclass loaded_class;
  JavaRewriteImpl rewrite_impl;
  JavaLogMessageProxy *msg_builder;
};

static gboolean
__load_rewrite_object(JavaRewriteProxy *self, const gchar *class_name, const gchar *class_path, gpointer handle)
{
  JNIEnv *java_env = NULL;
  gboolean result = TRUE;
  java_env = java_machine_get_env(self->java_machine, &java_env);
  self->loaded_class = java_machine_load_class(self->java_machine, class_name, class_path);
  if (!self->loaded_class) {
      msg_error("Can't find class",
                evt_tag_str("class_name", class_name),
                NULL);
      return FALSE;
  }

  result &= load_class_method(java_env, self->loaded_class, "<init>", "(J)V", &self->rewrite_impl.mi_constructor);
  result &= load_class_method(java_env, self->loaded_class, "initProxy", "()Z", &self->rewrite_impl.mi_init);
  result &= load_class_method(java_env, self->loaded_class, "processProxy", "(Lorg/syslog_ng/LogMessage;)Z", &self->rewrite_impl.mi_process);

  self->rewrite_impl.rewrite_object = CALL_JAVA_FUNCTION(java_env, NewObject, self->loaded_class, self->rewrite_impl.mi_constructor, handle);
  if (!self->rewrite_impl.rewrite_object)
    {
      msg_error("Can't create object",
                evt_tag_str("class_name", class_name),
                NULL);
      return FALSE;
    }
  return result;
}

gboolean
java_rewrite_proxy_process(JavaRewriteProxy *self, LogMessage *msg)
{
  jboolean result;
  JNIEnv *env = java_machine_get_env(self->java_machine, &env);
  jobject jmsg = java_log_message_proxy_create_java_object(self->msg_builder, msg);

  result = CALL_JAVA_FUNCTION(env,
                              CallBooleanMethod,
                              self->rewrite_impl.rewrite_object,
                              self->rewrite_impl.mi_process,
                              jmsg);

  return !!(result);
}

void
java_rewrite_proxy_free(JavaRewriteProxy *self)
{
  JNIEnv *env = NULL;
  env = java_machine_get_env(self->java_machine, &env);
  if (self->rewrite_impl.rewrite_object)
    {
      CALL_JAVA_FUNCTION(env, DeleteLocalRef, self->rewrite_impl.rewrite_object);
    }

  if (self->loaded_class)
    {
      CALL_JAVA_FUNCTION(env, DeleteLocalRef, self->loaded_class);
    }
  if (self->msg_builder)
    {
      java_log_message_proxy_free(self->msg_builder);
    }
  java_machine_unref(self->java_machine);
  g_free(self);
}

JavaRewriteProxy *
java_rewrite_proxy_new(const gchar *class_name, const gchar *class_path, gpointer handle)
{
  JavaRewriteProxy *self = g_new0(JavaRewriteProxy, 1);
  self->java_machine = java_machine_ref();

  if (!java_machine_start(self->java_machine))
      goto error;

  if (!__load_rewrite_object(self, class_name, class_path, handle))
    {
      goto error;
    }

  self->msg_builder = java_log_message_proxy_new();
  if (!self->msg_builder)
    {
      goto error;
    }

  return self;
error:
  java_rewrite_proxy_free(self);
  return NULL;
}

gboolean
java_rewrite_proxy_init(JavaRewriteProxy *self)
{
  jboolean result;
  JNIEnv *env = java_machine_get_env(self->java_machine, &env);

  result = CALL_JAVA_FUNCTION(env, CallBooleanMethod, self->rewrite_impl.rewrite_object, self->rewrite_impl.mi_init);

  return !!(result);
}
