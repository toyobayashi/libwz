package com.toyobayashi.libwz;

public class WzDoubleProperty extends WzImageProperty {
    WzDoubleProperty(long ptr) { super(ptr); }

    private static native double nativeGetValue(long ptr);

    public double getValue() { return nativeGetValue(nativePtr); }
}
