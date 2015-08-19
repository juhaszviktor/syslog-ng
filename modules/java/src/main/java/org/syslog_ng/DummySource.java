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

package org.syslog_ng;

public class DummySource extends LogReader {

    long last_msg_sent;
    int msg_count;
    final static long TIMEOUT = 5000;

    public DummySource(long pipeHandle) {
        super(pipeHandle);
        last_msg_sent = 0;
    }

    public void deinit() {
        InternalMessageSender.debug("Deinit");
    }

    public boolean init() {
        InternalMessageSender.debug("Init");
        return true;
    }

    public void close() {
        InternalMessageSender.debug("Close");
    }

    public boolean open() {
        InternalMessageSender.debug("Open");
        return true;
    }

    public boolean isOpened() {
        InternalMessageSender.debug("isOpened");
        return true;
    }

    public boolean isReadable() {
        InternalMessageSender.debug("isReadable");
        if (last_msg_sent == 0 || System.currentTimeMillis() > last_msg_sent + TIMEOUT)
          {
            return true;
          }
        return false;
    }

    public String getBookmark() {
        return Integer.toString(msg_count);
    }

    public boolean seekToBookmark(String bookmark) {
        try {
            msg_count = Integer.parseInt(bookmark);
            return true;
        } catch (NumberFormatException e) {
            InternalMessageSender.error("Bad bookmark: " + bookmark);
            return false;
        }
    }

    public boolean fetch(LogMessage msg) {
        long t = System.currentTimeMillis();
        if (last_msg_sent == 0 || t > last_msg_sent + TIMEOUT)
          {
            msg.setValue("MSG", "Hello from Java! " + msg_count);
            last_msg_sent = t;
            msg_count++;
            return true;
          }
        return false;
    }

    public String getNameByUniqOptions() {
        return "DUMMY";
    }
}

