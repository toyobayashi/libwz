package com.toyobayashi.libwz;

public class WzUOLProperty extends WzImageProperty {
    WzUOLProperty(long ptr) { super(ptr); }

    private static native String nativeGetValue(long ptr);

    public String getValue() { return nativeGetValue(nativePtr); }
}
