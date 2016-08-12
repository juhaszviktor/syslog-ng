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

#include "java-source-proxy.h"
#include "java_machine.h"
#include "java-logmsg-proxy.h"
#include "messages.h"

#include <jni.h>


typedef struct _JavaMethod
{
  jmethodID method_id;
  const gchar *name;
  const gchar *signature;
  gboolean optional;
} JavaMethod;


typedef struct _JavaSourceImpl
{
  struct {
    JavaMethod constructor;
    JavaMethod init;
    JavaMethod deinit;
    JavaMethod read_message;
    JavaMethod open;
    JavaMethod close;
    JavaMethod ack;
    JavaMethod nack;
    JavaMethod is_readable;
    JavaMethod get_stats_instance;
    JavaMethod get_persist_name;
    JavaMethod get_cursor;
    JavaMethod seek_to_cursor;
  } methods;
  jobject source_object;
} JavaSourceImpl;

struct _JavaSourceProxy
{
  JavaVMSingleton *java_machine;
  gchar *class_name;
  gchar *stats_instance;
  gchar *persist_name;
  jclass loaded_class;
  JavaSourceImpl source_impl;
  JavaLogMessageProxy *msg_builder;
};

#define USE_JAVA_ENVIRONMENT(jvm)                   \
  JNIEnv *env = NULL;                               \
  env = java_machine_get_env(jvm, &env);

#define JAVA_FUNCTION(function, ...) CALL_JAVA_FUNCTION(env, function, __VA_ARGS__)

static void
_init_java_source_impl_instance(JavaSourceImpl *self)
{
  self->methods.constructor = (JavaMethod){.name = "<init>", .signature = "(J)V", .optional = FALSE};
  self->methods.init = (JavaMethod){.name = "initProxy", .signature = "()Z", .optional = FALSE};
  self->methods.deinit = (JavaMethod){.name = "deinitProxy", .signature = "()V", .optional = FALSE};
  self->methods.read_message = (JavaMethod){.name = "readMessageProxy", .signature = "(Lorg/syslog_ng/LogMessage;)I", .optional = FALSE};
  self->methods.open = (JavaMethod){.name = "openProxy",  .signature = "()Z", .optional = FALSE};
  self->methods.close = (JavaMethod){.name = "closeProxy", .signature = "()V", .optional = FALSE};
  self->methods.ack = (JavaMethod){.name = "ackProxy", .signature = "(Lorg/syslog_ng/LogMessage;)V", .optional = TRUE};
  self->methods.nack = (JavaMethod){.name = "nackProxy", .signature = "(Lorg/syslog_ng/LogMessage;)V", .optional = TRUE};
  self->methods.is_readable = (JavaMethod){.name = "isReadableProxy", .signature = "()Z", .optional = FALSE};
  self->methods.get_stats_instance = (JavaMethod){.name = "getStatsInstanceProxy", .signature = "()Ljava/lang/String;", .optional = FALSE};
  self->methods.get_persist_name = (JavaMethod){.name = "getPersistNameProxy", .signature = "()Ljava/lang/String;", .optional = FALSE};
  self->methods.get_cursor = (JavaMethod){.name = "getCursorProxy", .signature = "()Ljava/lang/String;", .optional = TRUE};
  self->methods.seek_to_cursor = (JavaMethod){.name = "seekToCursorProxy", .signature = "(Ljava/lang/String;)Z", .optional = TRUE};
}

static jmethodID
_load_source_method(JNIEnv *env, jclass loaded_class, JavaMethod *method)
{
  method->method_id = JAVA_FUNCTION(GetMethodID, loaded_class, method->name, method->signature);
  return method->method_id;
}

static gboolean
_load_source_methods(JNIEnv *env, JavaSourceProxy *self)
{
  gint i = 0;
  gboolean result = TRUE;
  JavaMethod *methods = (JavaMethod *)&self->source_impl.methods;
  gint number_of_methods = sizeof(self->source_impl.methods) / sizeof(JavaMethod);
  for (i = 0; i < number_of_methods; i++)
    {
      if (!_load_source_method(env, self->loaded_class, &methods[i]) && &methods[i].optional == FALSE)
        {
          msg_error("Can't find method in class",
                    evt_tag_str("class_name", self->class_name),
                    evt_tag_str("method_name", methods[i].name),
                    evt_tag_str("method_signature", methods[i].signature),
                    NULL);
          result = FALSE;
          break;
        }
    }
  return result;
}

static gboolean
_load_source_object(JavaSourceProxy *self, const gchar *class_path, gpointer handle)
{
  JNIEnv *java_env = NULL;
  java_env = java_machine_get_env(self->java_machine, &java_env);
  self->loaded_class = java_machine_load_class(self->java_machine, self->class_name, class_path);
  if (!self->loaded_class)
    {
      msg_error("Can't find class", evt_tag_str("class_name", self->class_name), NULL);
      return FALSE;
    }
  if (!_load_source_methods(java_env, self))
    {
      return FALSE;
    }

  self->source_impl.source_object = CALL_JAVA_FUNCTION(java_env, NewObject, self->loaded_class, self->source_impl.methods.constructor.method_id, handle);
  if (!self->source_impl.source_object)
    {
      msg_error("Can't create object", evt_tag_str("class_name", self->class_name), NULL);
      return FALSE;
    }
  return TRUE;
}

gboolean
java_source_proxy_init(JavaSourceProxy *self)
{
  USE_JAVA_ENVIRONMENT(self->java_machine);
  return !!(JAVA_FUNCTION(CallBooleanMethod, self->source_impl.source_object, self->source_impl.methods.init.method_id));
}

void
java_source_proxy_deinit(JavaSourceProxy *self)
{
  USE_JAVA_ENVIRONMENT(self->java_machine);
  JAVA_FUNCTION(CallVoidMethod, self->source_impl.source_object, self->source_impl.methods.deinit.method_id);
}

gboolean
java_source_proxy_open(JavaSourceProxy *self)
{
  USE_JAVA_ENVIRONMENT(self->java_machine);
  return !!(JAVA_FUNCTION(CallBooleanMethod, self->source_impl.source_object, self->source_impl.methods.open.method_id));
}

void
java_source_proxy_close(JavaSourceProxy *self)
{
  USE_JAVA_ENVIRONMENT(self->java_machine);
  JAVA_FUNCTION(CallVoidMethod, self->source_impl.source_object, self->source_impl.methods.close.method_id);
}

worker_read_result_t
java_source_proxy_read_message(JavaSourceProxy *self, LogMessage *msg)
{
  USE_JAVA_ENVIRONMENT(self->java_machine);
  jobject jmsg = java_log_message_proxy_create_java_object(self->msg_builder, msg);
  jint res = JAVA_FUNCTION(CallIntMethod,
      self->source_impl.source_object,
      self->source_impl.methods.read_message.method_id,
      jmsg);

  JAVA_FUNCTION(DeleteLocalRef, jmsg);
  return res;
}

void
java_source_proxy_msg_ack(JavaSourceProxy *self, LogMessage *msg)
{
  if (self->source_impl.methods.ack.method_id)
    {
      USE_JAVA_ENVIRONMENT(self->java_machine);
      jobject jmsg = java_log_message_proxy_create_java_object(self->msg_builder, msg);
      JAVA_FUNCTION(CallVoidMethod,
            self->source_impl.source_object,
            self->source_impl.methods.ack.method_id,
            jmsg);
      JAVA_FUNCTION(DeleteLocalRef, jmsg);
    }
}

void
java_source_proxy_msg_nack(JavaSourceProxy *self, LogMessage *msg)
{
  if (self->source_impl.methods.nack.method_id)
    {
      USE_JAVA_ENVIRONMENT(self->java_machine);
      jobject jmsg = java_log_message_proxy_create_java_object(self->msg_builder, msg);
      JAVA_FUNCTION(CallVoidMethod,
              self->source_impl.source_object,
              self->source_impl.methods.nack.method_id,
              jmsg);
      JAVA_FUNCTION(DeleteLocalRef, jmsg);
    }
}

gboolean
java_source_proxy_is_readable(JavaSourceProxy *self)
{
  USE_JAVA_ENVIRONMENT(self->java_machine);
  return !!(JAVA_FUNCTION(CallBooleanMethod, self->source_impl.source_object, self->source_impl.methods.is_readable.method_id));
}

static gchar *
_java_str_dup(JNIEnv *env, jstring java_string)
{
  gchar *result = NULL;
  gchar *c_string = NULL;

  c_string = (gchar *) CALL_JAVA_FUNCTION(env, GetStringUTFChars, java_string, NULL);
  if (strlen(c_string) == 0)
    goto exit;

  result = strdup(c_string);
  exit:
  CALL_JAVA_FUNCTION(env, ReleaseStringUTFChars, java_string, c_string);
  return result;
}

const gchar *
java_source_proxy_get_stats_instance(JavaSourceProxy *self)
{
  if (!self->stats_instance)
    {
      USE_JAVA_ENVIRONMENT(self->java_machine);
      jstring java_string = (jstring)JAVA_FUNCTION(CallObjectMethod, self->source_impl.source_object, self->source_impl.methods.get_stats_instance.method_id);
      if (!java_string)
        {
          msg_error("Can't get stats instance", NULL);
        }
      else
        {
          self->stats_instance = _java_str_dup(env, java_string);
        }
    }
  return self->stats_instance;
}

const gchar *
java_source_proxy_get_persist_name(JavaSourceProxy *self)
{
  if (!self->persist_name)
    {
      USE_JAVA_ENVIRONMENT(self->java_machine);
      jstring java_string = (jstring)JAVA_FUNCTION(CallObjectMethod, self->source_impl.source_object, self->source_impl.methods.get_persist_name.method_id);
      if (!java_string)
        {
          msg_error("Can't get persist_name", NULL);
        }
      else
        {
          self->persist_name = _java_str_dup(env, java_string);
        }
    }
  return self->persist_name;
}

const gchar *
java_source_proxy_get_cursor(JavaSourceProxy *self)
{
  if (self->source_impl.methods.get_persist_name.method_id)
    {
      USE_JAVA_ENVIRONMENT(self->java_machine);
      jstring java_string = (jstring)JAVA_FUNCTION(CallObjectMethod, self->source_impl.source_object, self->source_impl.methods.get_persist_name.method_id);
      if (!java_string)
        {
          msg_error("Can't get cursor", NULL);
          return NULL;
        }
      return _java_str_dup(env, java_string);
    }
  return NULL;
}

gboolean
java_source_proxy_seek_to_cursor(JavaSourceProxy *self, const gchar *cursor)
{
  if (self->source_impl.methods.seek_to_cursor.method_id)
    {
      USE_JAVA_ENVIRONMENT(self->java_machine);
      jstring jcursor = (*env)->NewStringUTF(env, cursor);
      return !!(JAVA_FUNCTION(CallBooleanMethod, self->source_impl.source_object, self->source_impl.methods.seek_to_cursor.method_id, jcursor));
    }
  return TRUE;
}

gboolean
java_source_proxy_is_pos_tracked(JavaSourceProxy *self)
{
  return !!(self->source_impl.methods.seek_to_cursor.method_id);
}

void
java_source_proxy_free(JavaSourceProxy *self)
{
  USE_JAVA_ENVIRONMENT(self->java_machine);
  if (self->source_impl.source_object)
    {
      JAVA_FUNCTION(DeleteLocalRef, self->source_impl.source_object);
    }
  if (self->loaded_class)
    {
      JAVA_FUNCTION(DeleteLocalRef, self->loaded_class);
    }
  if (self->msg_builder)
    {
      java_log_message_proxy_free(self->msg_builder);
    }
  g_free(self->stats_instance);
  g_free(self->persist_name);
  g_free(self->class_name);
  java_machine_unref(self->java_machine);
  g_free(self);
}

JavaSourceProxy *
java_source_proxy_new(const gchar *class_name, const gchar *class_path, gpointer handle)
{
  JavaSourceProxy *self = g_new0(JavaSourceProxy, 1);
  self->java_machine = java_machine_ref();

  if (!java_machine_start(self->java_machine))
    {
      goto error;
    }

  _init_java_source_impl_instance(&self->source_impl);
  self->class_name = g_strdup(class_name);

  if (!_load_source_object(self, class_path, handle))
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
  java_source_proxy_free(self);
  return NULL;
}
