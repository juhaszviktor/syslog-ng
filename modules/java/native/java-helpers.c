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

#include "java-helpers.h"
#include "messages.h"

gchar*
normalize_key(const gchar *buffer)
{
  const gchar from = '-';
  const gchar to = '_';
  gchar *p;
  gchar *normalized_key = g_strdup(buffer);
  p = normalized_key;
  while (*p)
    {
      if (*p == from)
        *p = to;
      p++;
    }
  return normalized_key;
}

inline gboolean
load_class_method(JNIEnv *java_env, jclass loaded_class, const gchar *method_name, const gchar *signature, jmethodID *method_id)
{
  *method_id = CALL_JAVA_FUNCTION(java_env, GetMethodID, loaded_class, method_name, signature);
  if (!*method_id)
    {
      msg_error("Can't find method in class",
                evt_tag_str("method", method_name),
                evt_tag_str("signature", signature),
                NULL);
      return FALSE;
    }
  return TRUE;
}

static inline void
__copy_hash_table_iterator(gpointer key, gpointer val, gpointer dest){
  g_hash_table_insert((GHashTable*) dest, key, val);
}

void
clone_java_options(GHashTable *src, GHashTable *dest) {
  g_hash_table_foreach(src, __copy_hash_table_iterator, dest);
}
