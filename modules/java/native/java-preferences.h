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

#ifndef JAVA_PREFERENCES_H_INCLUDED
#define JAVA_PREFERENCES_H_INCLUDED

typedef struct
{
    GString *class_path;
    gchar *class_name;
    GHashTable *options;
} JavaPreferences;

JavaPreferences* java_preferences_new();
void java_preferences_set_class_path(JavaPreferences *self, const gchar *class_path);
void java_preferences_set_class_name(JavaPreferences *self, const gchar *class_name);
void java_preferences_set_option(JavaPreferences *self, const gchar* key, const gchar* value);
void java_preferences_free(JavaPreferences *self);
JavaPreferences* java_preferences_clone(JavaPreferences *self);

#endif
