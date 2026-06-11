package io.github.toyobayashi.libwz;

public class WzIntProperty extends WzImageProperty {
    WzIntProperty(long ptr) { super(ptr); }

    private static native int nativeGetValue(long ptr);
    private static native void nativeSetValue(long ptr, int value);

    public int getValue() { return nativeGetValue(nativePtr); }
    public void setValue(int value) { nativeSetValue(nativePtr, value); }
}
