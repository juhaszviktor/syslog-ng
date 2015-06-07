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

import org.syslog_ng.InternalMessageSender;

public abstract class FilterExprNode {

    private long handle;

    public FilterExprNode(long handle) {
        this.handle = handle;
    }

    protected abstract boolean init();
    protected abstract boolean eval(LogMessage msg);

    public String getOption(String key) {
        return getOption(handle, key);
    }

    private native String getOption(long ptr, String key);

    public boolean initProxy() {
        try {
            return init();
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return false;
        }
    }

    public boolean evalProxy(LogMessage msg) {
        try {
            return eval(msg);
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return false;
        }
        finally {
            msg.release();
        }
    }

}
