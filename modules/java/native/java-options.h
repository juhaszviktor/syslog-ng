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

#ifndef JAVA_OPTIONS_H_INCLUDED
#define JAVA_OPTIONS_H_INCLUDED

typedef struct
{
    GString *class_path;
    gchar *class_name;
    GHashTable *options;
} JavaOptions;

JavaOptions* java_options_new();
void java_options_set_class_path(JavaOptions *self, const gchar *class_path);
void java_options_set_class_name(JavaOptions *self, const gchar *class_name);
void java_options_set_option(JavaOptions *self, const gchar* key, const gchar* value);
void java_options_free(JavaOptions *self);
JavaOptions* java_options_clone(JavaOptions *self);

#endif
