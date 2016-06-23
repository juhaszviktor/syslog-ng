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

#pragma once

#include "logthrsourcedrv.h"
#include "proxies/java-source-proxy.h"

typedef struct
{
  LogThrSourceDriver super;
  JavaSourceProxy *proxy;
  GString *class_path;
  gchar *class_name;
  GHashTable *options;
} JavaSourceDriver;

LogDriver *java_sd_new(GlobalConfig *cfg);
void java_sd_set_class_path(LogDriver *s, const gchar *class_path);
void java_sd_set_class_name(LogDriver *s, const gchar *class_name);
void java_sd_set_option(LogDriver *s, const gchar *key, const gchar *value);
