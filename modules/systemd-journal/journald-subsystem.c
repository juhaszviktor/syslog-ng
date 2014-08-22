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

#include "journald-subsystem.h"
#include <glib.h>
#include <gmodule.h>

#define LOAD_SYMBOL(library, symbol) g_module_symbol(library, #symbol, (gpointer*)&symbol)

#define JOURNAL_LIBRARY_NAME "libsystemd-journal.so.0"

static GModule *journald_module;

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
