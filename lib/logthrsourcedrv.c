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

#include "logthrsourcedrv.h"
#include "ack_tracker.h"
#include "persistable-state-header.h"

#include <stdlib.h>

#define FETCH_LIMIT 10

struct _LogThreadedSource
{
  LogSource super;
  LogThrSourceDriver *driver;
};

typedef struct _ThreadedSourceState
{
  PersistableStateHeader header;
  gchar cursor[MAX_CURSOR_LENGTH];
} ThreadedSourceState;

typedef struct _ThreadedSourceBookmarkData
{
  PersistEntryHandle persist_handle;
  gchar *cursor;
} ThreadedSourceBookmarkData;

static void _update_watches(LogThrSourceDriver *self);
static gboolean _open(LogThrSourceDriver *self);

static const gchar *
_get_stats_instance(LogThrSourceDriver *self)
{
  const gchar *stats_instance = "LogThreadedSource";
  if (self->get_stats_instance)
    {
      stats_instance = self->get_stats_instance(self);
    }
  return stats_instance;
}

static gboolean
_is_readable(LogThrSourceDriver *self)
{
  gboolean result = TRUE;
  if (self->worker.is_readable)
    {
      result = self->worker.is_readable(self);
    }
  return result;
}

static void
_close(LogThrSourceDriver *self)
{
  self->worker_opened = FALSE;
  if (self->worker.close)
    {
      self->worker.close(self);
    }
}

static void
_wake_up(LogSource *s)
{
  LogThreadedSource *source = (LogThreadedSource *)s;
  iv_event_post(&source->driver->scheduled_wakeup);
}

static void
_source_custom_ack(LogMessage *const msg, AckType ack_type)
{
  AckTracker *ack_tracker = msg->ack_record->tracker;
  LogThreadedSource *source = (LogThreadedSource *)ack_tracker->source;
  LogThrSourceDriver *self = source->driver;

  switch (ack_type)
  {
  case AT_PROCESSED:
    if (self->worker.msg_ack)
      self->worker.msg_ack(self, msg);
    break;

  case AT_ABORTED:
    if (self->worker.msg_nack)
      self->worker.msg_nack(self, msg);
    break;

  default:
    break;
  }

  ack_tracker_manage_msg_ack(ack_tracker, msg, ack_type);
}

static void
_source_post(LogThreadedSource *source, LogMessage *const msg)
{
  LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;
  gint old_window_size;

  ack_tracker_track_msg(source->super.ack_tracker, msg);

  path_options.ack_needed = TRUE;
  log_msg_ref(msg);
  log_msg_add_ack(msg, &path_options);
  msg->ack_func = _source_custom_ack;

  old_window_size = g_atomic_counter_exchange_and_add(&source->super.window_size, -1);

  g_assert(old_window_size > 0);
  log_pipe_queue(&source->super.super, msg, &path_options);
}

static void
_stop_watches(LogThrSourceDriver *self)
{
  self->watches_running = FALSE;
  if (iv_task_registered(&self->do_work))
    {
      iv_task_unregister(&self->do_work);
    }
  if (iv_timer_registered(&self->timer_reopen))
    {
      iv_timer_unregister(&self->timer_reopen);
    }
  if (self->poll_events)
    {
      poll_events_stop_watches(self->poll_events);
    }
}

static void
_start_watches(LogThrSourceDriver *self)
{
  self->watches_running = TRUE;
  _update_watches(self);
}

static void
_shutdown(gpointer data)
{
  LogThrSourceDriver *self = (LogThrSourceDriver *)data;
  msg_debug("Shutting down this source!");
  _stop_watches(self);
  iv_quit();
}

static void
_suspend(LogThrSourceDriver *self)
{
  self->suspended = TRUE;
}

static void
_arm_reopen_timer(LogThrSourceDriver *self)
{
  msg_info("Try to open source again after time reopen",
          evt_tag_str("source", _get_stats_instance(self)),
          evt_tag_int("time_reopen", self->time_reopen));
  iv_validate_now();
  self->timer_reopen.expires = iv_now;
  self->timer_reopen.expires.tv_sec += self->time_reopen;
  if (!iv_timer_registered(&self->timer_reopen))
    {
      iv_timer_register(&self->timer_reopen);
    }
}

static void
_close_and_suspend(LogThrSourceDriver *self)
{
  _close(self);
  _suspend(self);
  _arm_reopen_timer(self);
}

static gboolean
_check_worker_opened(LogThrSourceDriver *self)
{
  gboolean opened = self->worker_opened;
  if (self->worker.is_opened)
    {
      opened = self->worker.is_opened(self);
    }
  return opened;
}

static void
_update_watches(LogThrSourceDriver *self)
{
  g_assert(self->watches_running);
  self->suspended = FALSE;

  self->worker_opened = _check_worker_opened(self);

  if (!self->worker_opened)
      {
        self->worker_opened = _open(self);
      }
  if (self->worker_opened)
    {

      if (!log_source_free_to_send(&self->source->super))
        {
          /* wait for message acknowledgment */
          _suspend(self);
          return;
        }
      if (_is_readable(self))
        {
          if (!iv_task_registered(&self->do_work))
            {
              iv_task_register(&self->do_work);
            }
          return;
        }
      if (self->poll_events)
        {
          poll_events_update_watches(self->poll_events, G_IO_IN);
        }
      else
        {
          _arm_reopen_timer(self);
        }
    }
  else
    {
      _arm_reopen_timer(self);
    }
}

static void
_handle_unsuccessfull_read(LogThrSourceDriver *self, worker_read_result_t read_result)
{
  switch(read_result)
    {
      case WORKER_READ_RESULT_NOT_CONNECTED:
        msg_error("Read error, source not opened",
            evt_tag_str("source", _get_stats_instance(self)));
        _close_and_suspend(self);
        break;
      case WORKER_READ_RESULT_NOTHING_TO_READ:
        msg_debug("No more messages to read",
            evt_tag_str("source", _get_stats_instance(self)));
        _suspend(self);
        break;
      default:
        g_assert_not_reached();
    }
}

static void
_save_state(Bookmark *bookmark)
{
  ThreadedSourceBookmarkData *bookmark_data = (ThreadedSourceBookmarkData *)(&bookmark->container);
  ThreadedSourceState *state = persist_state_map_entry(bookmark->persist_state, bookmark_data->persist_handle);
  strncpy(state->cursor, bookmark_data->cursor, MAX_CURSOR_LENGTH);
  persist_state_unmap_entry(bookmark->persist_state, bookmark_data->persist_handle);
}

static void
_destroy_bookmark(Bookmark *bookmark)
{
  ThreadedSourceBookmarkData *bookmark_data = (ThreadedSourceBookmarkData *)(&bookmark->container);
  g_free(bookmark_data->cursor);
}

static void
_fill_bookmark(LogThrSourceDriver *self, Bookmark *bookmark)
{
  ThreadedSourceBookmarkData *bookmark_data = (ThreadedSourceBookmarkData *)(&bookmark->container);
  bookmark_data->cursor = g_strndup(self->pos_tracking.get_cursor(self), MAX_CURSOR_LENGTH);
  bookmark_data->persist_handle = self->persist.handle;
  bookmark->save = _save_state;
  bookmark->destroy = _destroy_bookmark;
}

static void
_fetch_single_log(LogThrSourceDriver *self, LogMessage *msg)
{
  worker_read_result_t read_result = self->worker.read_message(self, msg);
  if (read_result == WORKER_READ_RESULT_SUCCESS)
    {
      msg_debug("Incoming message",
          evt_tag_str("source", _get_stats_instance(self)),
          evt_tag_str("message", log_msg_get_value(msg, LM_V_MESSAGE, NULL)));
      if (self->is_pos_tracked)
        {
          Bookmark *bookmark = ack_tracker_request_bookmark(self->source->super.ack_tracker);
          _fill_bookmark(self, bookmark);
        }
      msg_set_context(msg);
      log_msg_refcache_start_producer(msg);
      _source_post(self->source, msg);
      log_msg_refcache_stop();
      self->suspended = !log_source_free_to_send(&self->source->super);
    }
  else
    {
      log_msg_unref(msg);
      _handle_unsuccessfull_read(self, read_result);
    }
}

static void
_fetch_logs(LogThrSourceDriver *self)
{
  guint32 read_messages = 0;
  while (!self->suspended && read_messages < FETCH_LIMIT)
    {
      LogMessage *msg = log_msg_new_empty();
      _fetch_single_log(self, msg);
      read_messages++;
    }
}

static void
_do_work(gpointer s)
{
  LogThrSourceDriver *self = (LogThrSourceDriver *)s;

  _stop_watches(self);
  _fetch_logs(self);
  _start_watches(self);
}

static void
_wakeup_handler(gpointer s)
{
  LogThrSourceDriver *self = (LogThrSourceDriver *)s;
  _start_watches(self);
}

static gboolean
_load_persist_state(LogThrSourceDriver *self, const gchar *persist_name)
{
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);

  gsize state_size;
  guint8 persist_version;

  self->persist.state = cfg->state;
  self->persist.handle = persist_state_lookup_entry(self->persist.state, persist_name, &state_size, &persist_version);
  return !!(self->persist.handle);
}

static void
_create_persist_state(LogThrSourceDriver *self, const gchar *persist_name)
{
  self->persist.handle = persist_state_alloc_entry(self->persist.state, persist_name, sizeof(ThreadedSourceState));
  ThreadedSourceState *state = persist_state_map_entry(self->persist.state, self->persist.handle);

  state->header.version = 0;
  state->header.big_endian = (G_BYTE_ORDER == G_BIG_ENDIAN);

  persist_state_unmap_entry(self->persist.state, self->persist.handle);
}

static gboolean
_seek_to_cursor(LogThrSourceDriver *self, const gchar *cursor)
{
  gboolean result = TRUE;
  if (self->pos_tracking.seek_to_cursor)
    {
      result = self->pos_tracking.seek_to_cursor(self, cursor);
    }
  return result;
}

static void
_seek_to_saved_state(LogThrSourceDriver *self)
{
  ThreadedSourceState *state = persist_state_map_entry(self->persist.state, self->persist.handle);
  gboolean seek_result = _seek_to_cursor(self, state->cursor);
  persist_state_unmap_entry(self->persist.state, self->persist.handle);
  if (!seek_result)
    {
      msg_warning("Failed to seek to the saved cursor position",
          evt_tag_str("source", _get_stats_instance(self)),
          evt_tag_str("cursor", state->cursor),
          evt_tag_errno("error", errno));
      _seek_to_cursor(self, NULL);
    }
  else
    {
      msg_debug("Seeking the to the last cursor position",
          evt_tag_str("source", _get_stats_instance(self)),
          evt_tag_str("cursor", state->cursor));
    }
}


static void
_init_persist(LogThrSourceDriver *self)
{
  gchar *persist_name = NULL;
  if (self->pos_tracking.get_persist_name)
    {
      persist_name = g_strdup(self->pos_tracking.get_persist_name(self));
    }
  else
    {
      persist_name = g_strdup_printf("%s:%s:LogThreadedSource", self->super.super.group, self->super.super.id);
    }
  if (!_load_persist_state(self, persist_name))
    {
      _create_persist_state(self, persist_name);
      (void)(_seek_to_cursor(self, NULL));
    }
  else
    {
      _seek_to_saved_state(self);
    }
  g_free(persist_name);
}

static gboolean
_open(LogThrSourceDriver *self)
{
  gboolean result = TRUE;
  msg_debug("Open source", evt_tag_str("source", _get_stats_instance(self)));
  if (self->worker.open)
    {
      result = self->worker.open(self);
    }
  if (result)
    {
      msg_debug("Source opened",
          evt_tag_str("source", _get_stats_instance(self)));
      if (self->is_pos_tracked)
        {
          _init_persist(self);
        }
    }
  else
    {
      _arm_reopen_timer(self);
    }
  return result;
}

static void
_init_logsource_instance(LogThrSourceDriver *self)
{
  log_source_set_options(&self->source->super,
                           &self->source_options,
                           0,
                           SCS_INTERNAL,
                           self->super.super.id,
                           _get_stats_instance(self),
                           TRUE,
                           self->is_pos_tracked,
                           self->super.super.super.expr_node);


    log_pipe_append(&self->source->super.super, &self->super.super.super);
    log_pipe_init(&self->source->super.super);
}

static void
_init_watches(LogThrSourceDriver* self)
{
  IV_EVENT_INIT(&self->shutdown_event);
  self->shutdown_event.cookie = self;
  self->shutdown_event.handler = _shutdown;
  iv_event_register(&self->shutdown_event);

  IV_EVENT_INIT(&self->scheduled_wakeup);
  self->scheduled_wakeup.cookie = self;
  self->scheduled_wakeup.handler = _wakeup_handler;
  iv_event_register(&self->scheduled_wakeup);

  IV_TASK_INIT(&self->do_work);
  self->do_work.cookie = self;
  self->do_work.handler = _do_work;

  IV_TIMER_INIT(&self->timer_reopen);
  self->timer_reopen.cookie = self;
  self->timer_reopen.handler = (void (*)(void*))_update_watches;
}

static void
_worker_thread_init(LogThrSourceDriver *self)
{
  _init_watches(self);
  _init_logsource_instance(self);

  if (self->worker.thread_init)
    {
      self->worker.thread_init(self);
    }

  _start_watches(self);
}

static void
_worker_thread_deinit(LogThrSourceDriver* self)
{
  _close(self);
  if (self->worker.thread_deinit)
    {
      self->worker.thread_deinit(self);
    }
}

static void
_worker_thread_main(gpointer arg)
{
  LogThrSourceDriver *self = (LogThrSourceDriver *)arg;
  iv_init();
  msg_debug("Worker thread started",
            evt_tag_str("driver", self->super.super.id));
  _worker_thread_init(self);
  iv_main();
  _worker_thread_deinit(self);
  iv_deinit();
}

static void
_worker_thread_stop(gpointer arg)
{
  LogThrSourceDriver *self = (LogThrSourceDriver *)arg;
  iv_event_post(&self->shutdown_event);
}

gboolean
log_threaded_source_driver_init_method(LogPipe *s)
{
  LogThrSourceDriver *self = (LogThrSourceDriver *)s;

  log_src_driver_init_method(s);
  main_loop_create_worker_thread(_worker_thread_main,
                                 _worker_thread_stop,
                                 self,
                                 &self->worker_options);
  log_source_options_init(&self->source_options,
                          log_pipe_get_config(s),
                          self->super.super.group);
  return TRUE;
}

void
log_threaded_source_driver_free(LogPipe *s)
{
  LogThrSourceDriver *self = (LogThrSourceDriver *)s;

  log_pipe_unref(&self->source->super.super);
  log_source_options_destroy(&self->source_options);
  log_src_driver_free(s);
};

void
log_threaded_source_wakeup(LogThrSourceDriver *self)
{
  iv_event_post(&self->scheduled_wakeup);
}

gboolean
log_threaded_source_driver_deinit_method(LogPipe *s)
{
  return log_src_driver_deinit_method(s);
}

void
log_threaded_source_set_poll_events(LogThrSourceDriver *self, PollEvents *poll_events)
{
  if (self->poll_events)
    {
      poll_events_stop_watches(self->poll_events);
      poll_events_free(self->poll_events);
    }
  self->poll_events = poll_events;
  if (self->poll_events)
    {
      poll_events_set_callback(self->poll_events, (PollCallback)_do_work, self);
    }
}

void
log_threaded_source_driver_init_instance(LogThrSourceDriver *self, GlobalConfig *cfg)
{
  log_src_driver_init_instance(&self->super, cfg);

  self->source = g_new0(LogThreadedSource, 1);

  log_source_init_instance(&self->source->super, cfg);
  self->source->driver = self;
  self->source->super.wakeup = _wake_up;

  log_source_options_defaults(&self->source_options);

  self->worker_options.is_external_input = TRUE;

  self->worker_opened = FALSE;
  self->time_reopen = cfg->time_reopen;

  self->super.super.super.init = log_threaded_source_driver_init_method;
  self->super.super.super.deinit = log_threaded_source_driver_deinit_method;
  self->super.super.super.free_fn = log_threaded_source_driver_free;
}

void
log_threaded_source_set_cursor(LogThrSourceDriver *self, const gchar *new_cursor)
{
  _close(self);
  ThreadedSourceState *state = persist_state_map_entry(self->persist.state, self->persist.handle);
  strncpy(state->cursor, new_cursor, MAX_CURSOR_LENGTH);
  persist_state_unmap_entry(self->persist.state, self->persist.handle);
  _open(self);
}
