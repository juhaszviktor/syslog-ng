#include "systemd-journal.h"
#include "journald-subsystem.h"
#include "systemd-journal.h"

#include <stdlib.h>

struct _SystemdJournalSourceDriver
{
  LogSrcDriver super;
  JournalReaderOptions reader_options;
  JournalReader *reader;
  Journald *journald;
};

JournalReaderOptions *
systemd_journal_get_reader_options(LogDriver *s)
{
  SystemdJournalSourceDriver *self = (SystemdJournalSourceDriver *)s;
  return &self->reader_options;
}

static gchar *
__generate_persist_name(SystemdJournalSourceDriver *self)
{
  return g_strdup_printf("journald_source_%s_%s", self->super.super.group, self->super.super.id);
}

static gboolean
__init(LogPipe *s)
{
  SystemdJournalSourceDriver *self = (SystemdJournalSourceDriver *)s;
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);
  gchar *persist_name = __generate_persist_name(self);
  self->reader = journal_reader_new(cfg, self->journald);

  journal_reader_options_init(&self->reader_options, cfg, self->super.super.group);

  journal_reader_set_options((LogPipe *)self->reader, &self->super.super.super,  &self->reader_options, 0, SCS_JOURNALD, self->super.super.id, "journal");

  journal_reader_set_persist_name(self->reader, persist_name);
  g_free(persist_name);

  log_pipe_append((LogPipe *)self->reader, &self->super.super.super);
  if (!log_pipe_init((LogPipe *)self->reader))
    {
      msg_error("Error initializing journal_reader",
                NULL);
      log_pipe_unref((LogPipe *) self->reader);
      self->reader = NULL;
      return FALSE;
    }
  return TRUE;
}

static gboolean
__deinit(LogPipe *s)
{
  SystemdJournalSourceDriver *self = (SystemdJournalSourceDriver *)s;
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
__free(LogPipe *s)
{
  SystemdJournalSourceDriver *self = (SystemdJournalSourceDriver *)s;
  journal_reader_options_destroy(&self->reader_options);
  journald_free(self->journald);
  log_src_driver_free(s);
}

LogDriver *
systemd_journal_sd_new(GlobalConfig *cfg)
{
  SystemdJournalSourceDriver *self = g_new0(SystemdJournalSourceDriver, 1);
  log_src_driver_init_instance(&self->super, cfg);
  self->super.super.super.init = __init;
  self->super.super.super.deinit = __deinit;
  self->super.super.super.free_fn = __free;
  journal_reader_options_defaults(&self->reader_options);
  self->journald = journald_new();
  return &self->super.super;
}
