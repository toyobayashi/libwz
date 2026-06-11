package com.toyobayashi.libwz;

public class WzFloatProperty extends WzImageProperty {
    WzFloatProperty(long ptr) { super(ptr); }

    private static native float nativeGetValue(long ptr);

    public float getValue() { return nativeGetValue(nativePtr); }
}
