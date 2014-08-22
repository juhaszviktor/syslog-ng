#include "systemd-journal.h"
#include "journal-interface.h"
#include "systemd-journal.h"

#include <gmodule.h>
#include <stdlib.h>

static GModule *journald_module;


#define LOAD_SYMBOL(library, symbol) g_module_symbol(library, #symbol, (gpointer*)&symbol)

#define JOURNAL_LIBRARY_NAME "libsystemd-journal.so.0"

gboolean
load_journald_subsystem()
{
  if (!journald_module)
    {
      journald_module = g_module_open(JOURNAL_LIBRARY_NAME, 0);
      if (!journald_module)
        {
          return FALSE;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_open))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_close))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_seek_head))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_get_cursor))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_next))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_restart_data))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_enumerate_data))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_seek_cursor))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_get_fd))
        {
          goto error;
        }
      if (!LOAD_SYMBOL(journald_module, sd_journal_process))
        {
          goto error;
        }
    }
  return TRUE;
error:
  g_module_close(journald_module);
  return FALSE;
}

struct _SystemdJournalSourceDriver
{
  LogSrcDriver super;
  JournalReaderOptions reader_options;
  JournalReader *reader;
};

JournalReaderOptions *
systemd_journal_get_reader_options(LogDriver *s)
{
  SystemdJournalSourceDriver *self = (SystemdJournalSourceDriver *)s;
  return &self->reader_options;
}

static gboolean
systemd_journal_sd_init(LogPipe *s)
{
  SystemdJournalSourceDriver *self = (SystemdJournalSourceDriver *)s;
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);
  gchar *persist_name = g_strdup_printf("journald_source_%s_%s", self->super.super.group, self->super.super.id);
  self->reader = journal_reader_new(cfg);

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
systemd_journal_sd_deinit(LogPipe *s)
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
systemd_journal_sd_free(LogPipe *s)
{
  SystemdJournalSourceDriver *self = (SystemdJournalSourceDriver *)s;
  journal_reader_options_destroy(&self->reader_options);
  log_src_driver_free(s);
}

LogDriver *
systemd_journal_sd_new(GlobalConfig *cfg)
{
  SystemdJournalSourceDriver *self = g_new0(SystemdJournalSourceDriver, 1);
  log_src_driver_init_instance(&self->super, cfg);
  self->super.super.super.init = systemd_journal_sd_init;
  self->super.super.super.deinit = systemd_journal_sd_deinit;
  self->super.super.super.free_fn = systemd_journal_sd_free;
  journal_reader_options_defaults(&self->reader_options);
  return &self->super.super;
}
