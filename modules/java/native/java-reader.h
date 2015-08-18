/*
 * Copyright (c) 2015 BalaBit IT Ltd, Budapest, Hungary
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
  
#ifndef JAVA_READER_H_
#define JAVA_READER_H_

#include "logsource.h"
#include "poll-events.h"
#include "timeutils.h"
#include "java-preferences.h"

/* flags */
#define LR_KERNEL          0x0002
#define LR_EMPTY_LINES     0x0004
#define LR_IGNORE_TIMEOUT  0x0008
#define LR_SYSLOG_PROTOCOL 0x0010
#define LR_PREEMPT         0x0020
#define LR_THREADED        0x0040

typedef struct _JavaReaderOptions
{
  LogSourceOptions super;
  gboolean initialized;
  guint32 flags;
  gint fetch_limit;
  const gchar *group_name;
} JavaReaderOptions;

typedef struct _JavaReader JavaReader;

void java_reader_set_options(JavaReader *s, LogPipe *control, JavaReaderOptions *options, gint stats_level, gint stats_source, const gchar *stats_id, const gchar *stats_instance);
void java_reader_set_immediate_check(JavaReader *s);
void java_reader_reopen(JavaReader *s);
JavaReader *java_reader_new(GlobalConfig *cfg, JavaPreferences *preferences);

void java_reader_options_defaults(JavaReaderOptions *options);
void java_reader_options_init(JavaReaderOptions *options, GlobalConfig *cfg, const gchar *group_name);
void java_reader_options_destroy(JavaReaderOptions *options);
//gint java_reader_options_lookup_flag(const gchar *flag);
//gboolean java_reader_options_process_flag(JavaReaderOptions *options, gchar *flag);

#endif
