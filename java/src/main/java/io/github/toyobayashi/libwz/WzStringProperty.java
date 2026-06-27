package io.github.toyobayashi.libwz;

public class WzStringProperty extends WzImageProperty {
    WzStringProperty(long ptr) { super(ptr); }
    WzStringProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native String nativeGetValue(long ptr);
    private static native void nativeSetValue(long ptr, String value);

    public String getValue() { return nativeGetValue(nativePtr); }
    public void setValue(String value) { nativeSetValue(nativePtr, value); }
}
