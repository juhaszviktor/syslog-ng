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

#include "journald-mock.h"
#include "journald-helper.c"
#include "journal-reader.c"
#include "testutils.h"
#include "apphook.h"

#define JOURNALD_TESTCASE(testfunc, ...) { testcase_begin("%s(%s)", #testfunc, #__VA_ARGS__); testfunc(__VA_ARGS__); testcase_end(); }

static gboolean task_called;
static gboolean poll_triggered;

void
add_mock_entry(gpointer user_data)
{
  Journald *journald = user_data;
  MockEntry *entry = mock_entry_new("test_data3");
  mock_entry_add_data(entry, "MESSAGE=test message3");
  mock_entry_add_data(entry, "KEY=VALUE3");
  mock_entry_add_data(entry, "HOST=testhost3");
  journald_mock_add_entry(journald, entry);

  entry = mock_entry_new("test_data4");
  mock_entry_add_data(entry, "MESSAGE=test message4");
  mock_entry_add_data(entry, "KEY=VALUE4");
  mock_entry_add_data(entry, "HOST=testhost4");
  journald_mock_add_entry(journald, entry);
}

void
handle_new_entry(gpointer user_data)
{
  Journald *journald = user_data;
  journald_process(journald);
  assert_false(poll_triggered, ASSERTION_ERROR("Should called only once"));
  poll_triggered = TRUE;
}

void
stop_timer_expired(gpointer user_data)
{
  iv_quit();
}

void
__test_seeks(Journald *journald)
{
  gint result = journald_seek_head(journald);
  assert_gint(result, 0, ASSERTION_ERROR("Can't seek in empty journald mock"));
  result = journald_next(journald);
  assert_gint(result, 0, ASSERTION_ERROR("Bad next step result"));

  MockEntry *entry = mock_entry_new("test_data1");
  mock_entry_add_data(entry, "MESSAGE=test message");
  mock_entry_add_data(entry, "KEY=VALUE");
  mock_entry_add_data(entry, "HOST=testhost");

  journald_mock_add_entry(journald, entry);

  entry = mock_entry_new("test_data2");
  mock_entry_add_data(entry, "MESSAGE=test message2");
  mock_entry_add_data(entry, "KEY=VALUE2");
  mock_entry_add_data(entry, "HOST=testhost2");

  journald_mock_add_entry(journald, entry);

  result = journald_seek_head(journald);
  assert_gint(result, 0, ASSERTION_ERROR("Can't seek in journald mock"));
  result = journald_next(journald);
  assert_gint(result, 1, ASSERTION_ERROR("Bad next step result"));
}

void
__test_cursors(Journald *journald)
{
  gchar *cursor;
  gint result = journald_get_cursor(journald, &cursor);
  assert_string(cursor, "test_data1", ASSERTION_ERROR("Bad cursor fetched"));

  result = journald_next(journald);
  assert_gint(result, 1, ASSERTION_ERROR("Bad next step result"));
  result = journald_get_cursor(journald, &cursor);
  assert_string(cursor, "test_data2", ASSERTION_ERROR("Bad cursor fetched"));

  result = journald_next(journald);
  assert_gint(result, 0, ASSERTION_ERROR("Should not contain more elements"));

  result = journald_seek_cursor(journald, "test_data1");
  assert_gint(result, 0, ASSERTION_ERROR("Should find cursor"));
  result = journald_next(journald);
  assert_gint(result, 1, ASSERTION_ERROR("Bad next step result"));
  result = journald_get_cursor(journald, &cursor);
  assert_string(cursor, "test_data1", ASSERTION_ERROR("Bad cursor fetched"));

  result = journald_seek_cursor(journald, "test_data2");
  assert_gint(result, 0, ASSERTION_ERROR("Should find cursor"));
  result = journald_next(journald);
  assert_gint(result, 1, ASSERTION_ERROR("Bad next step result"));
  result = journald_get_cursor(journald, &cursor);
  assert_string(cursor, "test_data2", ASSERTION_ERROR("Bad cursor fetched"));
}

void
__test_enumerate(Journald *journald)
{
  const void *data;
  const void *prev_data;
  gsize length;
  gsize prev_len;
  gint result;

  journald_restart_data(journald);
  result = journald_enumerate_data(journald, &data, &length);
  assert_gint(result, 1, ASSERTION_ERROR("Data should exist"));

  prev_data = data;
  prev_len = length;

  result = journald_enumerate_data(journald, &data, &length);
  assert_gint(result, 1, ASSERTION_ERROR("Data should exist"));
  result = journald_enumerate_data(journald, &data, &length);
  assert_gint(result, 1, ASSERTION_ERROR("Data should exist"));
  result = journald_enumerate_data(journald, &data, &length);
  assert_gint(result, 0, ASSERTION_ERROR("Data should not exist"));

  journald_restart_data(journald);

  result = journald_enumerate_data(journald, &data, &length);
  assert_gint(result, 1, ASSERTION_ERROR("Data should exist"));
  assert_gpointer((gpointer )data, (gpointer )prev_data, ASSERTION_ERROR("restart data should seek the start of the data"));
  assert_gint(length, prev_len, ASSERTION_ERROR("Bad length after restart data"));

  result = journald_next(journald);
  assert_gint(result, 0, ASSERTION_ERROR("Should not contain more elements"));
}

void
__test_fd_handling(Journald *journald)
{
  gint fd = journald_get_fd(journald);
  journald_process(journald);

  task_called = FALSE;
  poll_triggered = FALSE;
  struct iv_task add_entry_task;
  struct iv_fd fd_to_poll;
  struct iv_timer stop_timer;

  IV_TASK_INIT(&add_entry_task);
  add_entry_task.cookie = journald;
  add_entry_task.handler = add_mock_entry;

  IV_FD_INIT(&fd_to_poll);
  fd_to_poll.fd = fd;
  fd_to_poll.cookie = journald;
  fd_to_poll.handler_in = handle_new_entry;

  iv_validate_now();
  IV_TIMER_INIT(&stop_timer);
  stop_timer.cookie = NULL;
  stop_timer.expires = iv_now;
  stop_timer.expires.tv_sec++;
  stop_timer.handler = stop_timer_expired;

  iv_task_register(&add_entry_task);
  iv_fd_register(&fd_to_poll);
  iv_timer_register(&stop_timer);

  iv_main();

  assert_true(poll_triggered, ASSERTION_ERROR("Poll event isn't triggered"));
}

void
test_journald_mock()
{
  Journald *journald = journald_mock_new();
  gint result = journald_open(journald, 0);

  assert_gint(result, 0, ASSERTION_ERROR("Can't open journald mock"));

  __test_seeks(journald);

  __test_cursors(journald);

  __test_fd_handling(journald);

  journald_close(journald);
  journald_free(journald);
}

void
__helper_test(gchar *key, gchar *value, gpointer user_data)
{
  GHashTable *result = user_data;
  g_hash_table_insert(result, key, value);
  return;
}


void
test_journald_helper()
{
  Journald *journald = journald_mock_new();
  journald_open(journald, 0);

  MockEntry *entry = mock_entry_new("test_data1");
  mock_entry_add_data(entry, "MESSAGE=test message");
  mock_entry_add_data(entry, "KEY=VALUE");
  mock_entry_add_data(entry, "HOST=testhost");

  journald_mock_add_entry(journald, entry);
  journald_seek_head(journald);
  journald_next(journald);

  GHashTable *result = g_hash_table_new(g_str_hash, g_str_equal);
  journald_foreach_data(journald, __helper_test, result);

  gchar *message = g_hash_table_lookup(result, "MESSAGE");
  gchar *key = g_hash_table_lookup(result, "KEY");
  gchar *host = g_hash_table_lookup(result, "HOST");

  assert_string(message, "test message", ASSERTION_ERROR("Bad item"));
  assert_string(key, "VALUE", ASSERTION_ERROR("Bad item"));
  assert_string(host, "testhost", ASSERTION_ERROR("Bad item"));

  journald_close(journald);
  journald_free(journald);
}

typedef struct _TestSource {
  LogPipe super;
  JournalReaderOptions options;
  JournalReader *reader;
  Journald *journald_mock;
  struct iv_task start_task;
  struct iv_task stop_task;
  struct iv_task test_task;
} TestSource;

gboolean
test_source_init(LogPipe *s)
{
  TestSource *self = (TestSource *)s;
  iv_task_register(&self->test_task);
  return TRUE;
}

gboolean
test_source_deinit(LogPipe *s)
{
  TestSource *self = (TestSource *)s;
  return TRUE;
}

void
test_source_free(LogPipe *s)
{
  TestSource *self = (TestSource *)s;

}

void
test_source_queue(LogPipe *s, LogMessage *msg, const LogPathOptions *path_options, gpointer user_data)
{
  TestSource *self = (TestSource *)s;
}

TestSource *
test_source_new(GlobalConfig *cfg)
{
  TestSource *self = g_new0(TestSource, 1);
  log_pipe_init_instance(&self->super, cfg);
  self->super.init = test_source_init;
  self->super.deinit = test_source_deinit;
  self->super.free_fn = test_source_free;
  self->super.queue = test_source_queue;
  self->journald_mock = journald_mock_new();
  journal_reader_options_defaults(&self->options);
  return self;
}

void
start_source(gpointer user_data)
{
  TestSource *self = (TestSource *)user_data;
  log_pipe_init(&self->super);
}

void
stop_source(gpointer user_data)
{
  TestSource *self = (TestSource *)user_data;
  log_pipe_deinit(&self->super);
  iv_quit();
}

void
first_test(gpointer user_data)
{
  TestSource *self = (TestSource *)user_data;
  iv_task_register(&self->stop_task);
}

void
test_journal_reader()
{
  TestSource *src = test_source_new(configuration);
  IV_TASK_INIT(&src->start_task);
  src->start_task.cookie = src;
  src->start_task.handler = start_source;
  IV_TASK_INIT(&src->stop_task);
  src->stop_task.cookie = src;
  src->stop_task.handler = stop_source;

  IV_TASK_INIT(&src->test_task);
  src->test_task.cookie = src;
  src->test_task.handler = first_test;
  iv_task_register(&src->start_task);
  iv_main();
}

int
main(int argc, char **argv)
{
  app_startup();
  configuration = cfg_new(0x306);
  configuration->threaded = FALSE;
  JOURNALD_TESTCASE(test_journald_mock);
  JOURNALD_TESTCASE(test_journald_helper);
  JOURNALD_TESTCASE(test_journal_reader);
  app_shutdown();
  return 0;
}
