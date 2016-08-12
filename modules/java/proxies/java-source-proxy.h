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

#include "syslog-ng.h"
#include "logmsg/logmsg.h"
#include "logthrsourcedrv.h"

typedef struct _JavaSourceProxy JavaSourceProxy;

JavaSourceProxy *java_source_proxy_new(const gchar *class_name, const gchar *class_path, gpointer handle);

gboolean java_source_proxy_init(JavaSourceProxy *self);
void java_source_proxy_deinit(JavaSourceProxy *self);
gboolean java_source_proxy_open(JavaSourceProxy *self);
void java_source_proxy_close(JavaSourceProxy *self);
worker_read_result_t java_source_proxy_read_message(JavaSourceProxy *self, LogMessage *msg);
void java_source_proxy_msg_ack(JavaSourceProxy *self, LogMessage *msg);
void java_source_proxy_msg_nack(JavaSourceProxy *self, LogMessage *msg);
gboolean java_source_proxy_is_readable(JavaSourceProxy *self);
const gchar *java_source_proxy_get_stats_instance(JavaSourceProxy *self);
const gchar *java_source_proxy_get_persist_name(JavaSourceProxy *self);
const gchar *java_source_proxy_get_cursor(JavaSourceProxy *self);
gboolean java_source_proxy_seek_to_cursor(JavaSourceProxy *self, const gchar *cursor);
gboolean java_source_proxy_is_pos_tracked(JavaSourceProxy *self);

void java_source_proxy_free(JavaSourceProxy *self);




