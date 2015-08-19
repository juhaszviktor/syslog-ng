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

public abstract class LogReader extends LogPipe {

    public LogReader(long pipeHandle) {
        super(pipeHandle);
    }

    public String getOption(String key) {
        return getOption(getHandle(), key);
    }

    protected abstract boolean open();

    protected abstract void close();

    protected abstract boolean isOpened();

    protected abstract boolean fetch(LogMessage msg);

    protected abstract boolean isReadable();

    protected abstract String getBookmark();

    protected abstract boolean seekToBookmark(String bookmark);

    protected abstract String getNameByUniqOptions();

    private native String getOption(long ptr, String key);

    public boolean fetchProxy(LogMessage msg) {
        try {
            return fetch(msg);
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return false;
        }
    }

    public boolean openProxy() {
        try {
            return open();
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return false;
        }
    }

    public void closeProxy() {
        try {
            close();
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
        }
    }

    public boolean isOpenedProxy() {
        try {
            return isOpened();
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return false;
        }
    }

    public boolean isReadableProxy() {
        try {
            return isReadable();
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return false;
        }
    }

    public String getBookmarkProxy() {
        try {
            return getBookmark();
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return null;
        }
    }

    public boolean seekToBookmarkProxy(String bookmark) {
        try {
            return seekToBookmark(bookmark);
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return false;
        }
    }

    public String getNameByUniqOptionsProxy() {
        try {
            return getNameByUniqOptions();
        }
        catch (Exception e) {
            InternalMessageSender.sendExceptionMessage(e);
            return null;
        }
    }
}
