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

#include <iv.h>
#include <iv_event.h>

#include "driver.h"
#include "logsource.h"
#include "ack_tracker.h"
#include "mainloop-worker.h"
#include "poll-events.h"

#define MAX_CURSOR_LENGTH 1024

typedef struct _LogThrSourceDriver LogThrSourceDriver;

typedef struct _LogThreadedSource LogThreadedSource;

typedef enum
{
  WORKER_READ_RESULT_SUCCESS = 0,
  WORKER_READ_RESULT_NOTHING_TO_READ,
  WORKER_READ_RESULT_NOT_CONNECTED
} worker_read_result_t;

struct _LogThrSourceDriver
{
  LogSrcDriver super;

  struct
  {
    void (* thread_init)(LogThrSourceDriver *self);
    void (* thread_deinit)(LogThrSourceDriver *self);
    gboolean (*open)(LogThrSourceDriver *self);
    gboolean (*is_opened)(LogThrSourceDriver *self);
    void (*close)(LogThrSourceDriver *self);
    worker_read_result_t (*read_message)(LogThrSourceDriver *self, LogMessage *msg);
    void (*msg_ack)(LogThrSourceDriver *self, LogMessage *msg);
    void (*msg_nack)(LogThrSourceDriver *self, LogMessage *msg);
    gboolean (*is_readable)(LogThrSourceDriver *self);
  } worker;

  const gchar *(*get_stats_instance) (LogThrSourceDriver *s);

  /*
   * This has to be set in constructor time,
   * otherwise it drives to undefined behavior
   */
  gboolean is_pos_tracked;

  /*
   * These functions has to be implemented if the source is position tracked,
   * otherwise no need to implement these functions
   */
  struct
  {
    const gchar *(*get_persist_name)(LogThrSourceDriver *s);
    const gchar *(*get_cursor)(LogThrSourceDriver *s);
    gboolean (*seek_to_cursor)(LogThrSourceDriver *self, const gchar *cursor);
  } pos_tracking;

  /*
   * The following members should not be modified by the children
   */
  struct
  {
    PersistState *state;
    PersistEntryHandle handle;
  } persist;

  LogThreadedSource *source;
  LogSourceOptions source_options;

  WorkerOptions worker_options;

  struct iv_event shutdown_event;
  struct iv_event scheduled_wakeup;
  struct iv_task do_work;
  struct iv_timer timer_reopen;
  PollEvents *poll_events;

  gint32 time_reopen;

  gboolean suspended;
  gboolean watches_running;
  gboolean worker_opened;
};

void log_threaded_source_driver_init_instance(LogThrSourceDriver *self, GlobalConfig *cfg);
gboolean log_threaded_source_driver_deinit_method(LogPipe *s);
gboolean log_threaded_source_driver_init_method(LogPipe *s);
void log_threaded_source_driver_free(LogPipe *s);
void log_threaded_source_wakeup(LogThrSourceDriver *self);
void log_threaded_source_set_poll_events(LogThrSourceDriver *self, PollEvents *poll_events);
void log_threaded_source_set_cursor(LogThrSourceDriver *self, const gchar *new_cursor);
