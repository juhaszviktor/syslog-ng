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

#include "java-source.h"

#include <stdlib.h>


JavaReaderOptions *
java_sd_get_reader_options(LogDriver *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return &self->reader_options;
}

JavaPreferences *
java_sd_get_preferences(LogDriver *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  return self->preferences;
}

static gboolean
java_sd_init(LogPipe *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  msg_debug("Java sd init", NULL);
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);
  self->reader = java_reader_new(cfg);

  java_reader_options_init(&self->reader_options, cfg, self->super.super.group);

  java_reader_set_options((LogPipe *)self->reader, &self->super.super.super,  &self->reader_options, 0, SCS_JOURNALD, self->super.super.id, "java");

  log_pipe_append((LogPipe *)self->reader, &self->super.super.super);
  if (!log_pipe_init((LogPipe *)self->reader))
    {
      msg_error("Error initializing java_reader", NULL);
      log_pipe_unref((LogPipe *) self->reader);
      self->reader = NULL;
      return FALSE;
    }
  return TRUE;
}

static gboolean
java_sd_deinit(LogPipe *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  msg_debug("Java sd deinit", NULL);
  if (self->reader)
    {
      log_pipe_deinit((LogPipe *)self->reader);
      log_pipe_unref((LogPipe *)self->reader);
      self->reader = NULL;
    }

  log_src_driver_deinit_method(s);
  return TRUE;
}

static void
java_sd_free(LogPipe *s)
{
  JavaSourceDriver *self = (JavaSourceDriver *)s;
  java_reader_options_destroy(&self->reader_options);
  log_src_driver_free(s);
}

LogDriver *
java_sd_new(GlobalConfig *cfg)
{
  JavaSourceDriver *self = g_new0(JavaSourceDriver, 1);

  log_src_driver_init_instance(&self->super, cfg);

  self->super.super.super.init = java_sd_init;
  self->super.super.super.deinit = java_sd_deinit;
  self->super.super.super.free_fn = java_sd_free;

  java_reader_options_defaults(&self->reader_options);
  self->preferences = java_preferences_new();

  return &self->super.super;
}
