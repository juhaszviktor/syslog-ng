/*
 * Copyright (c) 2010-2014 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 2010-2014 Viktor Juhasz <viktor.juhasz@balabit.com>
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
#include "syslog-ng.h"
#include "logmsg.h"
#include "logpipe.h"
#include "messages.h"
#include "poll-fd-events.h"
#include "mainloop-io-worker.h"
#include "persist-state.h"
#include "persistable-state-header.h"
#include "ack_tracker.h"
#include "parse-number.h"
#include "journal-reader.h"
#include "journald-helper.h"

#include <stdlib.h>

#define JR_THREADED 0x0001

#define MAX_CURSOR_LENGTH 1024

typedef struct _JournalReaderState {
  PersistableStateHeader header;
  gchar cursor[MAX_CURSOR_LENGTH];
} JournalReaderState;

typedef struct _JournalBookmarkData
{
  PersistEntryHandle persist_handle;
  gchar *cursor;
} JournalBookmarkData;

struct _JournalReader {
  LogSource super;
  LogPipe *control;
  JournalReaderOptions *options;
  Journald *journal;
  PollEvents *poll_events;
  struct iv_event schedule_wakeup;
  struct iv_task restart_task;
  MainLoopIOWorkerJob io_job;
  gboolean watches_running:1, suspended:1;
  gint notify_code;
  gboolean immediate_check;

  PersistState *persist_state;
  PersistEntryHandle persist_handle;
  gchar *persist_name;
};

static void
journal_reader_start_watches_if_stopped(JournalReader *self)
{
  if (!self->watches_running)
    {
      poll_events_start_watches(self->poll_events);
      self->watches_running = TRUE;
    }
}

static void
journal_reader_suspend_until_awoken(JournalReader *self)
{
  self->immediate_check = FALSE;
  poll_events_suspend_watches(self->poll_events);
  self->suspended = TRUE;
}

static void
journal_reader_force_check_in_next_poll(JournalReader *self)
{
  self->immediate_check = FALSE;
  poll_events_suspend_watches(self->poll_events);
  self->suspended = FALSE;

  if (!iv_task_registered(&self->restart_task))
    {
      iv_task_register(&self->restart_task);
    }
}

static void
journal_reader_update_watches(JournalReader *self)
{
  gboolean free_to_send;

  main_loop_assert_main_thread();

  journal_reader_start_watches_if_stopped(self);

  free_to_send = log_source_free_to_send(&self->super);
  if (!free_to_send)
    {
      journal_reader_suspend_until_awoken(self);
      return;
    }

  if (self->immediate_check)
    {
      journal_reader_force_check_in_next_poll(self);
      return;
    }
  poll_events_update_watches(self->poll_events, G_IO_IN);
}

static void
journal_reader_wakeup_triggered(gpointer s)
{
  JournalReader *self = (JournalReader *) s;

  if (!self->io_job.working && self->suspended)
    {
      self->immediate_check = TRUE;
      journal_reader_update_watches(self);
    }
}

static void
journal_reader_wakeup(LogSource *s)
{
  JournalReader *self = (JournalReader *) s;

  if (self->super.super.flags & PIF_INITIALIZED)
    iv_event_post(&self->schedule_wakeup);
}

static void
journal_reader_handle_data(gchar *key, gchar *value, gpointer user_data)
{
  gpointer *args = user_data;

  LogMessage *msg = args[0];
  gchar *prefix = args[1];

  if (strcmp(key, "MESSAGE") == 0)
    {
      log_msg_set_value(msg, LM_V_MESSAGE, value, -1);
    }
  else if (strcmp(key, "_HOSTNAME") == 0)
    {
      log_msg_set_value(msg, LM_V_HOST, value, -1);
    }
  else if (strcmp(key, "_PID") == 0)
    {
      log_msg_set_value(msg, LM_V_PID, value, -1);
    }
  else if (strcmp(key, "_SOURCE_REALTIME_TIMESTAMP") == 0)
    {
      guint64 ts;
      parse_number(value, (gint64 *) &ts);
      msg->timestamps[LM_TS_STAMP].tv_sec = ts / 1000000;
      msg->timestamps[LM_TS_STAMP].tv_usec = ts % 1000000;
      msg->timestamps[LM_TS_STAMP].zone_offset = get_local_timezone_ofs(msg->timestamps[LM_TS_STAMP].tv_sec);
    }
  else if (strcmp(key, "_COMM") == 0)
    {
      log_msg_set_value(msg, LM_V_PROGRAM, value, -1);
    }
  else if (strcmp(key, "SYSLOG_FACILITY") == 0)
    {
      msg->pri = (msg->pri & 7) | atoi(value) << 3;
    }
  else if (strcmp(key, "PRIORITY") == 0)
    {
      msg->pri = (msg->pri & ~7) | atoi(value);
    }
  else
    {
      NVHandle handle = log_msg_get_value_handle(key);
      log_msg_set_value(msg, handle, value, -1);
      if (prefix)
        {
          gchar *prefixed_key = g_strdup_printf("%s%s", prefix,
              key);
          NVHandle prefixed_handle = log_msg_get_value_handle(prefixed_key);
          log_msg_set_value_indirect(msg, prefixed_handle, handle, 0, 0, strlen(value));
        }
    }
}

static gboolean
journal_reader_handle_message(JournalReader *self)
{
  LogMessage *msg = log_msg_new_empty();
  LogPathOptions lpo = LOG_PATH_OPTIONS_INIT;

  msg->pri = self->options->default_pri;

  gpointer args[] = {msg, self->options->prefix};

  journald_foreach_data(self->journal, journal_reader_handle_data, args);

  log_pipe_queue(&self->super.super, msg, &lpo);
  return log_source_free_to_send(&self->super);
}

void
journal_reader_set_persist_name(JournalReader *self, gchar *persist_name)
{
  g_free(self->persist_name);
  self->persist_name = g_strdup(persist_name);
}

static gboolean
journal_reader_load_state(JournalReader *self)
{
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super);

  PersistEntryHandle state_handle;
  gsize state_size;
  guint8 persist_version;
  gboolean result = TRUE;

  self->persist_state = cfg->state;
  state_handle = persist_state_lookup_entry(self->persist_state, self->persist_name, &state_size, &persist_version);
  if (!state_handle)
    {
      state_handle = persist_state_alloc_entry(self->persist_state, self->persist_name, sizeof(JournalReaderState));
      if (state_handle)
        {
          JournalReaderState *state = persist_state_map_entry(self->persist_state, state_handle);

          state->header.version = 0;
          state->header.big_endian = (G_BYTE_ORDER == G_BIG_ENDIAN);

          persist_state_unmap_entry(self->persist_state, state_handle);
        }
      result = FALSE;
    }
  self->persist_handle = state_handle;
  return result;
}

static gboolean
journal_reader_set_starting_position(JournalReader *self)
{
  gint rc;
  if (!journal_reader_load_state(self))
    {
      rc = journald_seek_head(self->journal);
      if (rc != 0)
        {
          msg_error("Failed to seek to head of journal", evt_tag_errno("error", errno), NULL);
          return FALSE;
        }
    }
  else
    {
      JournalReaderState *state = persist_state_map_entry(self->persist_state, self->persist_handle);
      rc = journald_seek_cursor(self->journal, state->cursor);
      persist_state_unmap_entry(self->persist_state, self->persist_handle);
      if (rc != 0)
        {
          msg_warning("Failed to seek to the cursor", evt_tag_str("cursor", state->cursor), evt_tag_errno("error", errno), NULL);
          rc = journald_seek_head(self->journal);
          if (rc != 0)
            {
              msg_error("Failed to seek to head of journal", evt_tag_errno("error", errno), NULL);
              return FALSE;
            }
        }
      else
        {
          journald_next(self->journal);
        }
    }
  return TRUE;
}

static gchar *
journal_reader_get_cursor(JournalReader *self)
{
  gchar *cursor;
  journald_get_cursor(self->journal, &cursor);
  return cursor;
}

static void
journal_reader_save_state(Bookmark *bookmark)
{
  JournalBookmarkData *bookmark_data = (JournalBookmarkData *)(&bookmark->container);
  JournalReaderState *state = persist_state_map_entry(bookmark->persist_state, bookmark_data->persist_handle);
  strcpy(state->cursor, bookmark_data->cursor);
  persist_state_unmap_entry(bookmark->persist_state, bookmark_data->persist_handle);
}

static void
journal_reader_destroy_bookmark(Bookmark *bookmark)
{
  JournalBookmarkData *bookmark_data = (JournalBookmarkData *)(&bookmark->container);
  free(bookmark_data->cursor);
}

static void
journal_reader_fill_bookmark(JournalReader *self, Bookmark *bookmark)
{
  JournalBookmarkData *bookmark_data = (JournalBookmarkData *)(&bookmark->container);
  bookmark_data->cursor = journal_reader_get_cursor(self);
  bookmark_data->persist_handle = self->persist_handle;
  bookmark->save = journal_reader_save_state;
  bookmark->destroy = journal_reader_destroy_bookmark;
}

static gint
journal_reader_fetch_log(JournalReader *self)
{
  gint msg_count = 0;
  gboolean has_eof = FALSE;
  while (msg_count < self->options->fetch_limit && !main_loop_worker_job_quit())
    {
      gint rc = journald_next(self->journal);
      if (rc < 0)
        {
          msg_error("Error occured while getting next message from journal", evt_tag_errno("error", errno), NULL);
          return NC_READ_ERROR;
        }
      if (rc == 0)
        {
          fprintf(stderr, "EOF\n");
          has_eof = TRUE;
          break;
        }
      Bookmark *bookmark = ack_tracker_request_bookmark(self->super.ack_tracker);
      journal_reader_fill_bookmark(self, bookmark);
      msg_count++;
      if (!journal_reader_handle_message(self))
        {
          break;
        }
   }
  if (!has_eof)
    {
      self->immediate_check = TRUE;
    }
  return 0;
}

static void
journal_reader_work_finished(gpointer s)
{
  JournalReader *self = (JournalReader *) s;
  if (self->notify_code)
    {
      gint notify_code = self->notify_code;

      self->notify_code = 0;
      log_pipe_notify(self->control, notify_code, self);
    }
  if (self->super.super.flags & PIF_INITIALIZED)
    {
      /* reenable polling the source assuming that we're still in
       * business (e.g. the reader hasn't been uninitialized) */
      journal_reader_update_watches(self);
    }
  log_pipe_unref(&self->super.super);
}

static void
journal_reader_work_perform(gpointer s)
{
  JournalReader *self = (JournalReader *) s;
  self->notify_code = journal_reader_fetch_log(self);
}

static void
journal_reader_stop_watches(JournalReader *self)
{
  if (self->watches_running)
    {
      poll_events_stop_watches(self->poll_events);

      if (iv_task_registered(&self->restart_task))
        iv_task_unregister(&self->restart_task);
      self->watches_running = FALSE;
    }
}

static void
journal_reader_io_process_input(gpointer s)
{
  JournalReader *self = (JournalReader *) s;

  journal_reader_stop_watches(self);
  log_pipe_ref(&self->super.super);
  if ((self->options->flags & JR_THREADED))
    {
      main_loop_io_worker_job_submit(&self->io_job);
    }
  else
    {
      /* Checking main_loop_io_worker_job_quit() helps to speed up the
       * reload process.  If reload/shutdown is requested we shouldn't do
       * anything here, outstanding messages will be processed by the new
       * configuration.
       *
       * Our current understanding is that it doesn't prevent race
       * conditions of any kind.
       */
      if (!main_loop_worker_job_quit())
        {
          journal_reader_work_perform(s);
          journal_reader_work_finished(s);
        }
    }
}

static void
journal_reader_io_process_async_input(gpointer s)
{
  JournalReader *self = (JournalReader *)s;
  journald_process(self->journal);
  journal_reader_io_process_input(s);
}

static gboolean
journal_reader_init(LogPipe *s)
{
  JournalReader *self = (JournalReader *)s;

  if (!log_source_init(s))
    return FALSE;

  gint res = journald_open(self->journal, SD_JOURNAL_LOCAL_ONLY);
  if (res < 0)
    {
      msg_error("Can't open journal", evt_tag_errno("error", errno), NULL);
      return FALSE;
    }

  gint fd = journald_get_fd(self->journal);
  if (fd < 0)
    {
      msg_error("Can't get fd from journal",
                evt_tag_errno("error", errno),
                NULL);
      journald_close(self->journal);
      return FALSE;
    }

  if (!journal_reader_set_starting_position(self))
    {
      msg_error("Can't set starting position to the journal", NULL);
      journald_close(self->journal);
      return FALSE;
    }

  self->poll_events = poll_fd_events_new(fd);

  poll_events_set_callback(self->poll_events, journal_reader_io_process_async_input, self);
  self->immediate_check = TRUE;
  journal_reader_update_watches(self);
  iv_event_register(&self->schedule_wakeup);
  return TRUE;
}

static gboolean
journal_reader_deinit(LogPipe *s)
{
  JournalReader *self = (JournalReader *)s;
  journal_reader_stop_watches(self);
  journald_close(self->journal);
  poll_events_free(self->poll_events);
  return TRUE;
}

static void
journal_reader_free(LogPipe *s)
{
  JournalReader *self = (JournalReader *) s;
  g_free(self->persist_name);
  return;
}

void
journal_reader_set_options(LogPipe *s, LogPipe *control, JournalReaderOptions *options, gint stats_level, gint stats_source, const gchar *stats_id, const gchar *stats_instance)
{
  JournalReader *self = (JournalReader *) s;

  log_source_set_options(&self->super, &options->super, stats_level, stats_source, stats_id, stats_instance, (options->flags & JR_THREADED), TRUE);

  log_pipe_unref(self->control);
  log_pipe_ref(control);
  self->control = control;

  self->options = options;
}

static void
journal_reader_init_watches(JournalReader *self)
{
  IV_EVENT_INIT(&self->schedule_wakeup);
  self->schedule_wakeup.cookie = self;
  self->schedule_wakeup.handler = journal_reader_wakeup_triggered;
  iv_event_register(&self->schedule_wakeup);

  IV_TASK_INIT(&self->restart_task);
  self->restart_task.cookie = self;
  self->restart_task.handler = journal_reader_io_process_input;

  main_loop_io_worker_job_init(&self->io_job);
  self->io_job.user_data = self;
  self->io_job.work = (void (*)(void *)) journal_reader_work_perform;
  self->io_job.completion = (void (*)(void *)) journal_reader_work_finished;
}

JournalReader *
journal_reader_new(GlobalConfig *cfg, Journald *journal)
{
  JournalReader *self = g_new0(JournalReader, 1);
  log_source_init_instance(&self->super, cfg);
  self->super.wakeup = journal_reader_wakeup;
  self->super.super.init = journal_reader_init;
  self->super.super.deinit = journal_reader_deinit;
  self->super.super.free_fn = journal_reader_free;
  self->persist_name = g_strdup("systemd-journal");
  self->journal = journal;
  journal_reader_init_watches(self);
  return self;
}

void
journal_reader_options_init(JournalReaderOptions *options, GlobalConfig *cfg, const gchar *group_name)
{
  if (options->initialized)
    return;

  log_source_options_init(&options->super, cfg, group_name);
  if (cfg->threaded)
    options->flags |= JR_THREADED;
  options->initialized = TRUE;
}

void
journal_reader_options_defaults(JournalReaderOptions *options)
{
  log_source_options_defaults(&options->super);
  options->fetch_limit = 10;
  options->default_pri = (16 << 3) | LOG_NOTICE;
  options->max_field_size = 64 * 1024;
}

void
journal_reader_options_destroy(JournalReaderOptions *options)
{
  log_source_options_destroy(&options->super);
  options->initialized = FALSE;
}
