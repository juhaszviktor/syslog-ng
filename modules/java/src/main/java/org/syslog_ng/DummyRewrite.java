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

public class DummyRewrite extends LogRewrite {

    public DummyRewrite(long pipeHandle) {
        super(pipeHandle);
    }

    public void deinit() {
        InternalMessageSender.debug("Deinit");
    }

    public boolean init() {
        InternalMessageSender.debug("Init");
        return true;
    }

    @Override
    public boolean process(LogMessage msg) {
        InternalMessageSender.debug("Set fruit to banana");
        msg.setValue("fruit", "banana");

        InternalMessageSender.debug("Add NewTag");
        msg.setTag(new LogTag("NewTag"), true);

        return true;
    }
}

