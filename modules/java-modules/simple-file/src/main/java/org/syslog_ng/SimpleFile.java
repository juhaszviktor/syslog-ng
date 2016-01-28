/*
 * Copyright (c) 2016 Balabit
 * Copyright (c) 2016 Viktor Juhasz <viktor.juhasz@balabit.com>
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

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

public class SimpleFile extends TextLogDestination {

  private BufferedWriter writer;
  private String fileName; 
  private boolean isOpen;

  public SimpleFile(long cHandle) {
    super(cHandle);
    isOpen = false;
  }

  public void deinit() {
    close();
  }

  public boolean init() {
    fileName = getOption("file");
    if (fileName == null) {
      InternalMessageSender.error("File is a required option for this destination");
      return false;
    }
    return true;
  }

  public boolean open() {
    InternalMessageSender.debug("open");
    if (isOpen) {
       close();
    }
    File file = new File(fileName);
    try {
        if (!file.exists()) {
            file.createNewFile();
        }
        writer = new BufferedWriter(new FileWriter(fileName, true));
        isOpen = true;
    }
    catch(IOException e) {
    	InternalMessageSender.error("Can't open file: '" +fileName + "' error: " + e);	
    }
    return isOpen;
  }
  
  public void close() {
	  try {
		  if (isOpen) {
			  writer.close();
			  isOpen = false;
		  }
	  } catch (IOException e) {
		  InternalMessageSender.error("Can't close file: '" +fileName + "' error: " + e);
	  }
  }

  public boolean isOpened() {
    return isOpen;
  }

  public boolean send(String message) {
	boolean result = false;
	try {
		writer.write(message);
		result = true;
	} catch (IOException e) {
		InternalMessageSender.error("Can't write to file: '" +fileName + "' error: " + e);
	}
    return result;
  }

  public String getNameByUniqOptions() {
    return "SimpleFile";
  }
}
