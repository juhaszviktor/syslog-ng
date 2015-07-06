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

#ifndef JAVA_REWRITE_H_INCLUDED
#define JAVA_REWRITE_H_INCLUDED

#include "rewrite/rewrite-expr.h"
#include "proxies/java-rewrite-proxy.h"


typedef struct
{
  LogRewrite super;
  JavaRewriteProxy *proxy;
  GString *class_path;
  gchar *class_name;
  GHashTable *options;
} JavaRewrite;

LogRewrite *java_rewrite_new(GlobalConfig *cfg);
void java_rewrite_set_class_path(LogRewrite *s, const gchar *class_path);
void java_rewrite_set_class_name(LogRewrite *s, const gchar *class_name);
void java_rewrite_set_option(LogRewrite *s, const gchar* key, const gchar* value);

#endif
