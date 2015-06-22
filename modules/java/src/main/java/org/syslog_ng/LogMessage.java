package org.syslog_ng;

public class LogMessage {
  private long handle;

  public LogMessage(long handle) {
    this.handle = handle;
  }

  public String getValue(String name) {
    return getValue(handle, name);
  }

  public void setValue(String name, String value) {
    if (name == null || name.isEmpty()) {
      throw new IllegalArgumentException("Name cannot be empty");
    }
    if (value == null) {
      throw new IllegalArgumentException("Value cannot be null");
    }

    setValue(handle, name, value);
  }


  public void release() {
    unref(handle);
    handle = 0;
  }

  protected long getHandle() {
    return handle;
  }

  private native void unref(long handle);
  private native String getValue(long ptr, String name);
  private native void setValue(long ptr, String name, String value);
}
