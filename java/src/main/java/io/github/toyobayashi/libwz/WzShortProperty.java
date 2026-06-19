package io.github.toyobayashi.libwz;

public class WzShortProperty extends WzImageProperty {
    WzShortProperty(long ptr) { super(ptr); }
    WzShortProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native short nativeGetValue(long ptr);
    private static native void nativeSetValue(long ptr, short value);

    public short getValue() { return nativeGetValue(nativePtr); }
    public void setValue(short value) { nativeSetValue(nativePtr, value); }
}
