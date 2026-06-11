package com.toyobayashi.libwz;

public class WzShortProperty extends WzImageProperty {
    WzShortProperty(long ptr) { super(ptr); }

    private static native short nativeGetValue(long ptr);

    public short getValue() { return nativeGetValue(nativePtr); }
}
