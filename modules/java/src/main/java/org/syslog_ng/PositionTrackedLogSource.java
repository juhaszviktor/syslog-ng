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

public abstract class PositionTrackedLogSource extends LogSource {

	public PositionTrackedLogSource(long pipeHandle) {
		super(pipeHandle);
	}

  protected abstract String getCursor();

  protected abstract boolean seekToCursor(String new_cursor);

  public String getCursorProxy()  {
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
