/*
 * Copyright (c) 2002-2016 BalaBit
 * Copyright (c) 2009-2016 Viktor Juhász
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "java-source.h"
#include "java_machine.h"
#include "str-utils.h"
#include "org_syslog_ng_LogSource.h"


JNIEXPORT jstring JNICALL
Java_org_syslog_1ng_LogSource_getOption(JNIEnv *env, jobject obj, jlong s, jstring key)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  gchar *value;
  const char *key_str = (*env)->GetStringUTFChars(env, key, NULL);
  if (key_str == NULL)
    {
      return NULL;
    }
  gchar *normalized_key = g_strdup(key_str);
  normalized_key = __normalize_key(normalized_key);
  value = g_hash_table_lookup(self->options, normalized_key);
  (*env)->ReleaseStringUTFChars(env, key, key_str);  // release resources
  g_free(normalized_key);

  if (value && value[0] != '\0')
    {
      return (*env)->NewStringUTF(env, value);
    }
  else
    {
      return NULL;
    }
}

static gboolean
_init(LogPipe *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;

  if (self->proxy)
    {
      java_source_proxy_free(self->proxy);
    }
  self->proxy = java_source_proxy_new(self->class_name, self->class_path->str, self);
  if (!self->proxy || !java_source_proxy_init(self->proxy))
    {
      return FALSE;
    }
  return log_threaded_source_driver_init_method(s);
}

static void
_free_fn(LogPipe *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  java_source_proxy_free(self->proxy);
  g_string_free(self->class_path, TRUE);
  g_free(self->class_name);
  g_hash_table_unref(self->options);
  log_threaded_source_driver_free(s);
}

static gboolean
_deinit(LogPipe *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  java_source_proxy_deinit(self->proxy);
  return log_threaded_source_driver_deinit_method(s);
}

static void
_thread_deinit(LogThrSourceDriver *s)
{
  java_machine_detach_thread();
}

static gboolean
_open(LogThrSourceDriver *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return java_source_proxy_open(self->proxy);
}

static void
_close(LogThrSourceDriver *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  java_source_proxy_close(self->proxy);
}

static worker_read_result_t
_read_message(LogThrSourceDriver *s, LogMessage *msg)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return java_source_proxy_read_message(self->proxy, msg);
}

static void
_msg_ack(LogThrSourceDriver *s, LogMessage *msg)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  java_source_proxy_msg_ack(self->proxy, msg);
}

static void
_msg_nack(LogThrSourceDriver *s, LogMessage *msg)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  java_source_proxy_msg_ack(self->proxy, msg);
}

static gboolean
_is_readable(LogThrSourceDriver *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return java_source_proxy_is_readable(self->proxy);
}

static const gchar *
_get_stats_instance(LogThrSourceDriver *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return java_source_proxy_get_stats_instance(self->proxy);
}

static const gchar *
_get_persist_name(LogThrSourceDriver *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return java_source_proxy_get_persist_name(self->proxy);
}

static const gchar *
_get_cursor(LogThrSourceDriver *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return java_source_proxy_get_cursor(self->proxy);
}

static gboolean
_seek_to_cursor(LogThrSourceDriver *s, const gchar *new_cursor)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return java_source_proxy_seek_to_cursor(self->proxy, new_cursor);
}

LogDriver *
java_sd_new(GlobalConfig *cfg)
{
  JavaSourceDriver *self = g_new0(JavaSourceDriver, 1);
  log_threaded_source_driver_init_instance(&self->super, cfg);

  self->class_path = g_string_sized_new(1024);
  self->options = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  self->super.super.super.super.init = _init;
  self->super.super.super.super.deinit = _deinit;
  self->super.super.super.super.free_fn = _free_fn;

  self->super.worker.thread_deinit = _thread_deinit;
  self->super.worker.open = _open;
  self->super.worker.close = _close;
  self->super.worker.read_message = _read_message;
  self->super.worker.msg_ack = _msg_ack;
  self->super.worker.msg_nack = _msg_nack;
  self->super.worker.is_readable = _is_readable;

  self->super.get_stats_instance = _get_stats_instance;
  self->super.pos_tracking.get_persist_name = _get_persist_name;
  self->super.pos_tracking.get_cursor = _get_cursor;
  self->super.pos_tracking.seek_to_cursor = _seek_to_cursor;

  return &self->super.super.super;
}

void
java_sd_set_class_path(LogDriver *s, const gchar *class_path)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  g_string_assign(self->class_path, class_path);
}

void
java_sd_set_class_name(LogDriver *s, const gchar *class_name)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  g_free(self->class_name);
  self->class_name = g_strdup(class_name);
}

void
java_sd_set_option(LogDriver *s, const gchar *key, const gchar *value)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  gchar *normalized_key = g_strdup(key);
  normalized_key = __normalize_key(normalized_key);
  g_hash_table_insert(self->options, normalized_key, g_strdup(value));
}
