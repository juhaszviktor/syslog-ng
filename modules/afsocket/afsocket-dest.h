/*
 * Copyright (c) 2002-2012 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 1998-2012 Balázs Scheidler
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

#ifndef AFSOCKET_DEST_H_INCLUDED
#define AFSOCKET_DEST_H_INCLUDED

#include "afsocket.h"
#include "socket-options.h"
#include "transport-mapper.h"
#include "driver.h"
#include "logwriter.h"
#if BUILD_WITH_SSL
#include "tlscontext.h"
#endif

#include <iv.h>

typedef struct _AFSocketDestDriver AFSocketDestDriver;

struct _AFSocketDestDriver
{
  LogDestDriver super;

  gboolean
    connections_kept_alive_accross_reloads:1,
    syslog_protocol:1,
    require_tls:1;
  gint fd;
  LogPipe *writer;
  LogWriterOptions writer_options;
  LogProtoClientFactory *proto_factory;
#if BUILD_WITH_SSL
  TLSContext *tls_context;
#endif
  gchar *hostname;

  GSockAddr *bind_addr;
  GSockAddr *dest_addr;
  gint time_reopen;
  struct iv_fd connect_fd;
  struct iv_timer reconnect_timer;
  SocketOptions *socket_options;
  TransportMapper *transport_mapper;

  gboolean (*setup_addresses)(AFSocketDestDriver *s);
  const gchar *(*get_dest_name)(AFSocketDestDriver *s);
};


#if BUILD_WITH_SSL
void afsocket_dd_set_tls_context(LogDriver *s, TLSContext *tls_context);
#else
#define afsocket_dd_set_tls_context(s, t)
#endif

static inline gboolean
afsocket_dd_setup_addresses(AFSocketDestDriver *s)
{
  return s->setup_addresses(s);
}

static inline const gchar *
afsocket_dd_get_dest_name(AFSocketDestDriver *s)
{
  return s->get_dest_name(s);
}

gboolean afsocket_dd_setup_addresses_method(AFSocketDestDriver *self);
void afsocket_dd_set_keep_alive(LogDriver *self, gint enable);
void afsocket_dd_init_instance(AFSocketDestDriver *self, SocketOptions *socket_options, TransportMapper *transport_mapper, const gchar *hostname);
gboolean afsocket_dd_init(LogPipe *s);
void afsocket_dd_free(LogPipe *s);

#endif
