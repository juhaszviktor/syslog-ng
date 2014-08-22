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

#ifndef JOURNAL_SOURCE_INTERFACE_H_
#define JOURNAL_SOURCE_INTERFACE_H_

#include <stdlib.h>
#include <glib.h>

typedef struct sd_journal sd_journal;

/* Open flags */
enum {
        SD_JOURNAL_LOCAL_ONLY = 1,
        SD_JOURNAL_RUNTIME_ONLY = 2,
        SD_JOURNAL_SYSTEM_ONLY = 4
};

typedef int (*SD_JOURNAL_OPEN)(sd_journal **ret, int flags);
typedef void (*SD_JOURNAL_CLOSE)(sd_journal *j);
typedef int (*SD_JOURNAL_SEEK_HEAD)(sd_journal *j);
typedef int (*SD_JOURNAL_GET_CURSOR)(sd_journal *j, char **cursor);
typedef int (*SD_JOURNAL_NEXT)(sd_journal *j);
typedef void (*SD_JOURNAL_RESTART_DATA)(sd_journal *j);
typedef int (*SD_JOURNAL_ENUMERATE_DATA)(sd_journal *j, const void **data, size_t *length);
typedef int (*SD_JOURNAL_SEEK_CURSOR)(sd_journal *j, const char *cursor);
typedef int (*SD_JOURNAL_GET_FD)(sd_journal *j);
typedef int (*SD_JOURNAL_PROCESS)(sd_journal *j);


SD_JOURNAL_OPEN sd_journal_open;
SD_JOURNAL_CLOSE sd_journal_close;
SD_JOURNAL_SEEK_HEAD sd_journal_seek_head;
SD_JOURNAL_GET_CURSOR sd_journal_get_cursor;
SD_JOURNAL_NEXT sd_journal_next;
SD_JOURNAL_RESTART_DATA sd_journal_restart_data;
SD_JOURNAL_ENUMERATE_DATA sd_journal_enumerate_data;
SD_JOURNAL_SEEK_CURSOR sd_journal_seek_cursor;
SD_JOURNAL_GET_FD sd_journal_get_fd;
SD_JOURNAL_PROCESS sd_journal_process;

gboolean load_journald_subsystem();

#endif /* JOURNAL_SOURCE_INTERFACE_H_ */
