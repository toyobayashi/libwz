package io.github.toyobayashi.libwz;

public class WzLongProperty extends WzImageProperty {
    WzLongProperty(long ptr) { super(ptr); }
    WzLongProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native long nativeGetValue(long ptr);
    private static native void nativeSetValue(long ptr, long value);

    public long getValue() { return nativeGetValue(nativePtr); }
    public void setValue(long value) { nativeSetValue(nativePtr, value); }
}
