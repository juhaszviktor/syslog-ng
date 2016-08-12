/*
 * Copyright (c) 2014 Balabit
 * Copyright (c) 2014 Viktor Juhasz <viktor.juhasz@balabit.com>
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

public class DummySource extends PositionTrackedLogSource {
	
	public DummySource(long handle) {
		super(handle);
	}
	
	protected void deinit() {
		InternalMessageSender.debug("deinit");
	}
	
	protected boolean init() {
		InternalMessageSender.debug("Init");
		return true;
	}
	
	protected boolean open() {
		InternalMessageSender.debug("Open");
		return true;
	}

	protected void close() {
		InternalMessageSender.debug("Close");
	}

	protected int readMessage(LogMessage msg) {
		InternalMessageSender.debug("readMessage");
		msg.setValue("MSG", "THIS IS A MESSAGE");
		return 0;
	}

	protected void ack(LogMessage msg)	{
		InternalMessageSender.debug("ack");
	}
	
	protected void nack(LogMessage msg) {
		InternalMessageSender.debug("nack");
	}
	
	protected boolean isReadable() {
		InternalMessageSender.debug("isReadable");
		return true;
	}
	
	protected String getStatsInstance(){
		InternalMessageSender.debug("getStatsInstance");
		return "DummySource";
	}
	
	protected String getPersistName() {
		InternalMessageSender.debug("getPersistName");
		return "DummySourcePersistName";
	}
	
	protected String getCursor() {
		InternalMessageSender.debug("getCursor");
		return "CursorForDummySource";
	}
	
	protected boolean seekToCursor(String new_cursor){
		InternalMessageSender.debug("seekToCursor: " + new_cursor);
		return true;
	}
};
