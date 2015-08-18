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


#include "java-reader-proxy.h"
#include "java-logmsg-proxy.h"
#include "java-class-loader.h"
#include "java-helpers.h"
#include "messages.h"
#include <string.h>


typedef struct _JavaReaderImpl
{
  jobject reader_object;
  jmethodID mi_constructor;
  jmethodID mi_init;
  jmethodID mi_deinit;
  jmethodID mi_fetch;
  jmethodID mi_open;
  jmethodID mi_close;
  jmethodID mi_is_opened;
} JavaReaderImpl;

struct _JavaReaderProxy
{
  JavaVMSingleton *java_machine;
  jclass loaded_class;
  JavaReaderImpl reader_impl;
};

static gboolean
__load_reader_object(JavaReaderProxy *self, const gchar *class_name, const gchar *class_path, gpointer handle)
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

  result &= load_class_method(java_env, self->loaded_class, "<init>", "(J)V", &self->reader_impl.mi_constructor);
  result &= load_class_method(java_env, self->loaded_class, "initProxy", "()Z", &self->reader_impl.mi_init);
  result &= load_class_method(java_env, self->loaded_class, "deinitProxy", "()Z", &self->reader_impl.mi_init);
  result &= load_class_method(java_env, self->loaded_class, "fetchProxy", "(Lorg/syslog_ng/LogMessage;)Z", &self->reader_impl.mi_fetch);
  result &= load_class_method(java_env, self->loaded_class, "openProxy", "()Z", &self->reader_impl.mi_open);
  result &= load_class_method(java_env, self->loaded_class, "closeProxy", "()V", &self->reader_impl.mi_close);
  result &= load_class_method(java_env, self->loaded_class, "isOpenedProxy", "()Z", &self->reader_impl.mi_is_opened);

  self->reader_impl.reader_object = CALL_JAVA_FUNCTION(java_env, NewObject, self->loaded_class, self->reader_impl.mi_constructor, handle);
  if (!self->reader_impl.reader_object)
    {
      msg_error("Can't create object",
                evt_tag_str("class_name", class_name),
                NULL);
      return FALSE;
    }
  return result;
}

gboolean
java_reader_proxy_fetch(JavaReaderProxy *self, LogMessage *msg)
{
  jboolean result;
  JNIEnv *env = java_machine_get_env(self->java_machine, &env);
  JavaLogMessageProxy *jmsg = java_log_message_proxy_new(msg);
  if (!jmsg)
    {
      return FALSE;
    }

  result = CALL_JAVA_FUNCTION(env,
                              CallBooleanMethod,
                              self->reader_impl.reader_object,
                              self->reader_impl.mi_fetch,
                              java_log_message_proxy_get_java_object(jmsg));

  java_log_message_proxy_free(jmsg);

  return !!(result);
}

void
java_reader_proxy_free(JavaReaderProxy *self)
{
  JNIEnv *env = NULL;
  env = java_machine_get_env(self->java_machine, &env);
  if (self->reader_impl.reader_object)
    {
      CALL_JAVA_FUNCTION(env, DeleteLocalRef, self->reader_impl.reader_object);
    }

  if (self->loaded_class)
    {
      CALL_JAVA_FUNCTION(env, DeleteLocalRef, self->loaded_class);
    }
  java_machine_unref(self->java_machine);
  g_free(self);
}

JavaReaderProxy *
java_reader_proxy_new(const gchar *class_name, const gchar *class_path, gpointer handle)
{
  JavaReaderProxy *self = g_new0(JavaReaderProxy, 1);
  self->java_machine = java_machine_ref();

  if (!java_machine_start(self->java_machine))
      goto error;

  if (!__load_reader_object(self, class_name, class_path, handle))
    {
      goto error;
    }

  return self;
error:
  java_reader_proxy_free(self);
  return NULL;
}

gboolean
java_reader_proxy_init(JavaReaderProxy *self)
{
  jboolean result;
  JNIEnv *env = java_machine_get_env(self->java_machine, &env);

  result = CALL_JAVA_FUNCTION(env, CallBooleanMethod, self->reader_impl.reader_object, self->reader_impl.mi_init);

  return !!(result);
}

gboolean
java_reader_proxy_deinit(JavaReaderProxy *self)
{
  jboolean result;
  JNIEnv *env = java_machine_get_env(self->java_machine, &env);

  result = CALL_JAVA_FUNCTION(env, CallBooleanMethod, self->reader_impl.reader_object, self->reader_impl.mi_deinit);

  return !!(result);
}

gboolean
java_reader_proxy_open(JavaReaderProxy *self)
{
  jboolean result;
  JNIEnv *env = java_machine_get_env(self->java_machine, &env);

  result = CALL_JAVA_FUNCTION(env, CallBooleanMethod, self->reader_impl.reader_object, self->reader_impl.mi_open);

  return !!(result);
}

void
java_reader_proxy_close(JavaReaderProxy *self)
{
  JNIEnv *env = java_machine_get_env(self->java_machine, &env);

  CALL_JAVA_FUNCTION(env, CallBooleanMethod, self->reader_impl.reader_object, self->reader_impl.mi_close);
}

gboolean
java_reader_proxy_is_opened(JavaReaderProxy *self)
{
  jboolean result;
  JNIEnv *env = java_machine_get_env(self->java_machine, &env);

  result = CALL_JAVA_FUNCTION(env, CallBooleanMethod, self->reader_impl.reader_object, self->reader_impl.mi_is_opened);

  return !!(result);
}
