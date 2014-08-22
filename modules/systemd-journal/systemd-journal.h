#ifndef SYSTEMD_JOURNAL_H_
#define SYSTEMD_JOURNAL_H_

#include "driver.h"
#include "logsource.h"

typedef struct _SystemdJournalSourceDriver SystemdJournalSourceDriver;

typedef struct _JournalReaderOptions {
  LogSourceOptions super;
  gboolean initialized;
  gint fetch_limit;
  guint16 default_pri;
  guint32 flags;
  gchar *recv_time_zone;
  gchar *prefix;
  guint32 max_field_size;
} JournalReaderOptions;

LogDriver *systemd_journal_sd_new(GlobalConfig *cfg);

gboolean load_journald_subsystem();
LogSourceOptions *systemd_journal_get_source_options(LogDriver *s);
JournalReaderOptions *systemd_journal_get_reader_options(LogDriver *s);

#endif /* SYSTEMD_JOURNAL_H_ */
