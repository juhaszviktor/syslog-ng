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


#ifndef JAVA_SD_H_
#define JAVA_SD_H_

#include "driver.h"
#include "logsource.h"
#include "java-reader.h"
#include "java-preferences.h"

typedef struct _JavaSourceDriver
{
  LogSrcDriver super;
  JavaReaderOptions reader_options;
  JavaReader *reader;
  JavaPreferences *preferences;
} JavaSourceDriver;

LogDriver *java_sd_new(GlobalConfig *cfg);

LogSourceOptions *java_sd_get_source_options(LogDriver *s);
JavaReaderOptions *java_sd_get_reader_options(LogDriver *s);

JavaPreferences *java_sd_get_preferences(LogDriver *s);

#endif
