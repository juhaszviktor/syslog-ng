/*
 * Copyright (c) 2010-2014 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 2010-2014 Viktor Juhasz <viktor.juhasz@balabit.com>
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

#ifndef JAVA_FILTER_PROXY_H_
#define JAVA_Filter_PROXY_H_

#include <jni.h>
#include <syslog-ng.h>
#include "filter/filter-expr.h"
#include "java_machine.h"

typedef struct _JavaFilterProxy JavaFilterProxy;

JavaFilterProxy* java_filter_proxy_new(const gchar *class_name, const gchar *class_path, gpointer handle);
void java_filter_set_class_path(FilterExprNode *s, const gchar *class_path);
void java_filter_set_class_name(FilterExprNode *s, const gchar *class_name);

gboolean java_filter_proxy_init(JavaFilterProxy *self);
gboolean java_filter_proxy_eval(JavaFilterProxy *self, LogMessage *msg);

// WTF?
//void java_filter_proxy_set_env(JavaFilterProxy *self, JNIEnv *env);

void java_filter_proxy_free(JavaFilterProxy *self);

#endif
