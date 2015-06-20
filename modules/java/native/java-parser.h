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

#ifndef JAVA_PARSER_H_INCLUDED
#define JAVA_PARSER_H_INCLUDED

#include "parser/parser-expr.h"
#include "proxies/java-parser-proxy.h"


typedef struct
{
  LogParser super;
  JavaParserProxy *proxy;
  GString *class_path;
  gchar *class_name;
  GHashTable *options;
} JavaParser;

LogParser *java_parser_new(GlobalConfig *cfg);
void java_parser_set_class_path(LogParser *s, const gchar *class_path);
void java_parser_set_class_name(LogParser *s, const gchar *class_name);
void java_parser_set_option(LogParser *s, const gchar* key, const gchar* value);

#endif
