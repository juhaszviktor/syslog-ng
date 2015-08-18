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

#include "java-reader.h"
#include "java-preferences.h"
#include "proxies/java-reader-proxy.h"
#include "mainloop-io-worker.h"
#include "mainloop-call.h"
#include "ack_tracker.h"

struct _JavaReader
{
  LogSource super;
  JavaPreferences *preferences;
  gboolean immediate_check;
  LogPipe *control;
  JavaReaderOptions *options;
  JavaReaderProxy *proxy;

  struct iv_task restart_task;
  struct iv_event schedule_wakeup;
  MainLoopIOWorkerJob io_job;
  gint notify_code;

};

static gboolean java_reader_fetch_log(JavaReader *self);

static void java_reader_stop_watches(JavaReader *self);
static void java_reader_update_watches(JavaReader *self);

static void
java_reader_work_perform(void *s)
{
  JavaReader *self = (JavaReader *) s;


  self->notify_code = java_reader_fetch_log(self);
}

static void
java_reader_work_finished(void *s)
{
  JavaReader *self = (JavaReader *) s;

  if (self->notify_code)
    {
      gint notify_code = self->notify_code;

      self->notify_code = 0;
      log_pipe_notify(self->control, notify_code, self);
    }
  log_pipe_unref(&self->super.super);
}

static void
java_reader_wakeup_triggered(gpointer s)
{
  JavaReader *self = (JavaReader *) s;

  self->immediate_check = TRUE;
  if (!self->io_job.working)
    {
      java_reader_update_watches(self);
    }
}

/* NOTE: may be running in the destination's thread, thus proper locking must be used */
static void
java_reader_wakeup(LogSource *s)
{
  JavaReader *self = (JavaReader *) s;

  if (self->super.super.flags & PIF_INITIALIZED)
    iv_event_post(&self->schedule_wakeup);
}

static void
java_reader_io_process_input(gpointer s)
{
  JavaReader *self = (JavaReader *) s;

  if (!main_loop_worker_job_quit())
    {
      log_pipe_ref(&self->super.super);
      if ((self->options->flags & LR_THREADED))
        {
          main_loop_io_worker_job_submit(&self->io_job);
        }
      else
        {
          java_reader_work_perform(s);
          java_reader_work_finished(s);
        }
    }
}

static void
java_reader_init_watches(JavaReader *self)
{
  IV_EVENT_INIT(&self->schedule_wakeup);
  self->schedule_wakeup.cookie = self;
  self->schedule_wakeup.handler = java_reader_wakeup_triggered;

  IV_TASK_INIT(&self->restart_task);
  self->restart_task.cookie = self;
  self->restart_task.handler = java_reader_io_process_input;

  main_loop_io_worker_job_init(&self->io_job);
  self->io_job.user_data = self;
  self->io_job.work = (void (*)(void *)) java_reader_work_perform;
  self->io_job.completion = (void (*)(void *)) java_reader_work_finished;
}

static void
java_reader_stop_watches(JavaReader *self)
{
  if (iv_task_registered(&self->restart_task))
    iv_task_unregister(&self->restart_task);
}

static void
java_reader_start_watches(JavaReader *self)
{
 if (iv_task_registered(&self->restart_task))
     iv_task_register(&self->restart_task);

}

static gboolean
java_reader_is_opened(JavaReader *self)
{
  return TRUE;
}

static void
java_reader_update_watches(JavaReader *self)
{
  gboolean free_to_send;

  main_loop_assert_main_thread();

  free_to_send = log_source_free_to_send(&self->super);
  if (!free_to_send)
    {
      self->immediate_check = FALSE;
      return;
    }

  if (self->immediate_check)
    {
      self->immediate_check = FALSE;
      if (!iv_task_registered(&self->restart_task))
        {
          iv_task_register(&self->restart_task);
        }
      return;
    }
}

static gboolean
java_reader_handle_line(JavaReader *self)
{
  msg_debug("Incoming log entry", NULL);

  //FETCH LOG
LogMessage *msg = log_msg_new_empty();
LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;
msg->flags = LF_LOCAL;
msg->pri = 15;
log_msg_refcache_start_producer(msg);
log_pipe_queue(&self->super.super, msg, &path_options);
log_msg_refcache_stop();
log_source_free_to_send(&self->super);
log_source_post(&self->super, msg);
  return log_source_free_to_send(&self->super);
}

/* returns: notify_code (NC_XXXX) or 0 for success */
static gint
java_reader_fetch_log(JavaReader *self)
{
  gint msg_count = 0;
  gint result = 0;
  self->immediate_check = TRUE;
  while (msg_count < self->options->fetch_limit && !main_loop_worker_job_quit())
    {
      Bookmark *bookmark = ack_tracker_request_bookmark(self->super.ack_tracker);
      msg_count++;
      if (!java_reader_handle_line(self))
        {
          break;
        }
   }
  return result;
}

static gboolean
java_reader_init(LogPipe *s)
{
  JavaReader *self = (JavaReader *) s;

  if (!log_source_init(s))
    return FALSE;

  self->proxy = java_reader_proxy_new(self->preferences->class_name, self->preferences->class_path->str, self);
  if (!self->proxy)
    return FALSE;

  if (!java_reader_proxy_init(self->proxy))
    return FALSE;

  iv_event_register(&self->schedule_wakeup);
  java_reader_start_watches(self); 

  msg_debug("Java source initialized", NULL);
  return TRUE;
}

static gboolean
java_reader_deinit(LogPipe *s)
{
  JavaReader *self = (JavaReader *) s;

  main_loop_assert_main_thread();

  iv_event_unregister(&self->schedule_wakeup);
  java_reader_stop_watches(self);

  if(!java_reader_proxy_deinit(self->proxy))
    return FALSE;

  if (!log_source_deinit(s))
    return FALSE;

  return TRUE;
}

static void
java_reader_free(LogPipe *s)
{
  JavaReader *self = (JavaReader *) s;

  if (self->proxy)
    java_reader_proxy_free(self->proxy);

  log_pipe_unref(self->control);
  log_source_free(s);
}

JavaPreferences *
java_reader_get_preferences(JavaReader *s)
{
  JavaReader *self = (JavaReader *) s;
  return self->preferences;
}

void
java_reader_set_options(JavaReader *s, LogPipe *control, JavaReaderOptions *options, gint stats_level, gint stats_source, const gchar *stats_id, const gchar *stats_instance)
{
  JavaReader *self = (JavaReader *) s;

  log_source_set_options(&self->super, &options->super, stats_level, stats_source, stats_id, stats_instance, (options->flags & LR_THREADED), FALSE, control->expr_node);

  log_pipe_unref(self->control);
  log_pipe_ref(control);
  self->control = control;

  self->options = options;
}

JavaReader *
java_reader_new(GlobalConfig *cfg)
{
  JavaReader *self = g_new0(JavaReader, 1);

  log_source_init_instance(&self->super, cfg);
  self->super.super.init = java_reader_init;
  self->super.super.deinit = java_reader_deinit;
  self->super.super.free_fn = java_reader_free;
  self->super.wakeup = java_reader_wakeup;
  self->immediate_check = FALSE;
  java_reader_init_watches(self);
  return self;
}

void 
java_reader_set_immediate_check(JavaReader *s)
{
  JavaReader *self = (JavaReader*) s;

  self->immediate_check = TRUE;
}

void
java_reader_options_defaults(JavaReaderOptions *options)
{
  log_source_options_defaults(&options->super);
  options->fetch_limit = 10;
}

void
java_reader_options_init(JavaReaderOptions *options, GlobalConfig *cfg, const gchar *group_name)
{
  if (options->initialized)
    return;

  log_source_options_init(&options->super, cfg, group_name);

  if (cfg->threaded)
    options->flags |= LR_THREADED;
  options->initialized = TRUE;
}

void
java_reader_options_destroy(JavaReaderOptions *options)
{
  log_source_options_destroy(&options->super);
  options->initialized = FALSE;
}

//CfgFlagHandler java_reader_flag_handlers[] =
//{
//  /* NOTE: underscores are automatically converted to dashes */
//
//  /* JavaReaderOptions */
//  { "kernel",                     CFH_SET, offsetof(JavaReaderOptions, flags),               LR_KERNEL },
//  { "empty-lines",                CFH_SET, offsetof(JavaReaderOptions, flags),               LR_EMPTY_LINES },
//  { "threaded",                   CFH_SET, offsetof(JavaReaderOptions, flags),               LR_THREADED },
//  { NULL },
//};

//gboolean
//java_reader_options_process_flag(JavaReaderOptions *options, gchar *flag)
//{
//  if (!msg_format_options_process_flag(&options->parse_options, flag))
//    return cfg_process_flag(java_reader_flag_handlers, options, flag);
//  return TRUE;
//}
