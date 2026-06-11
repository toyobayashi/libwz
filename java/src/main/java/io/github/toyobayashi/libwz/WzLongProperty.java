package io.github.toyobayashi.libwz;

public class WzLongProperty extends WzImageProperty {
    WzLongProperty(long ptr) { super(ptr); }

    private static native long nativeGetValue(long ptr);

    public long getValue() { return nativeGetValue(nativePtr); }
}
