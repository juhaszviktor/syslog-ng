/*
 * Copyright (c) 2016 Balabit
 * Copyright (c) 2016 Viktor Juhasz <viktor.juhasz@balabit.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

package org.syslog_ng;

public abstract class LogSource extends LogPipe implements Optionable {

	public static int SUCCESS = 0;
	public static int NOTHING_TO_READ = 1;
	public static int NOT_CONNECTED = 2;
	
	public LogSource(long pipeHandle) {
		super(pipeHandle);
	}

	public String getOption(String key) {
		return getOption(getHandle(), key);
	}

	protected abstract boolean open();

	protected abstract void close();

	protected abstract int readMessage(LogMessage msg);

	protected abstract void ack(LogMessage msg);
	
	protected abstract void nack(LogMessage msg);
	
	protected abstract boolean isReadable();
	
	protected abstract String getStatsInstance();
	
	protected abstract String getPersistName();
	
	protected abstract String getCursor();
	
	protected abstract boolean seekToCursor(String new_cursor);

	private native String getOption(long ptr, String key);

	public boolean openProxy() {
		try {
			return open();
		}
		catch (Exception e) {
			sendExceptionMessage(e);
			return false;
		}
	}

	public void closeProxy() {
		try {
			close();
		}
		catch (Exception e) {
			sendExceptionMessage(e);
		}
	}

	public int readMessageProxy(LogMessage msg) {
		try {
			int r = readMessage(msg);
			if (r == SUCCESS) return 0;
			if (r == NOTHING_TO_READ) return 1;
			if (r == NOT_CONNECTED) return 2;
			else return 1;
		}
		catch (Exception e) {
			sendExceptionMessage(e);
			return 1;
		}
	}

	public void ackProxy(LogMessage msg) {
		try {
			ack(msg);
		}
		catch (Exception e) {
			sendExceptionMessage(e);
		}
	}
	
	public void nackProxy(LogMessage msg) {
		try {
			nack(msg);
		}
		catch (Exception e) {
			sendExceptionMessage(e);
		}
	}
	
	public boolean isReadableProxy() {
		try {
			return isReadable();
		}
		catch (Exception e) {
			sendExceptionMessage(e);
			return false;
		}
	}
	
	public String getStatsInstanceProxy() {
		try {
			return getStatsInstance();
		}
		catch (Exception e) {
			sendExceptionMessage(e);
			return null;
		}
	}
	
	public String getPersistNameProxy() {
		try {
			return getPersistName();
		}
		catch (Exception e) {
			sendExceptionMessage(e);
			return null;
		}
	}
	
	public String getCursorProxy()	{
		try {
			return getCursor();
		}
		catch (Exception e) {
			sendExceptionMessage(e);
			return null;
		}
	}
	
	public boolean seekToCursorProxy(String new_cursor) {
		try {
			return seekToCursor(new_cursor);
		}
		catch (Exception e) {
			sendExceptionMessage(e);
			return false;
		}
	}
}
