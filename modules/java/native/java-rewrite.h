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
#include "java-preferences.h"

typedef struct
{
  LogRewrite super;
  JavaRewriteProxy *proxy;
  JavaPreferences *preferences;
} JavaRewrite;

LogRewrite *java_rewrite_new(GlobalConfig *cfg, JavaPreferences *preferences);
void java_rewrite_set_options(LogRewrite *s, const JavaPreferences *preferences);

#endif
