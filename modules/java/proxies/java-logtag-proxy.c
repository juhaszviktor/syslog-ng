/*
 * Copyright (c) 2010-2015 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 2010-2015 Viktor Juhasz <viktor.juhasz@balabit.com>
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


#include "java-logtag-proxy.h"
#include "messages.h"

JNIEXPORT jlong
JNICALL Java_org_syslog_1ng_LogTag_log_1tags_1get_1by_1name(JNIEnv *env, jobject obj, jstring name)
{
    const char *name_str = (*env)->GetStringUTFChars(env, name, NULL);
    if (name_str == NULL)
      {
        return NULL;
      }

    LogTagId id = log_tags_get_by_name(name_str);

    (*env)->ReleaseStringUTFChars(env, name, name_str);
    return (jlong)id;
}
