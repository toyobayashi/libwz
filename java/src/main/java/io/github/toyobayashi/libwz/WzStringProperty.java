package io.github.toyobayashi.libwz;

public class WzStringProperty extends WzImageProperty {
    WzStringProperty(long ptr) { super(ptr); }
    WzStringProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native String nativeGetValue(long ptr);
    private static native void nativeSetValue(long ptr, String value);
    private static native boolean nativeSaveToFile(long ptr, String path);

    public String getValue() { return nativeGetValue(nativePtr); }
    public void setValue(String value) { nativeSetValue(nativePtr, value); }
    public boolean saveToFile(String path) { return nativeSaveToFile(nativePtr, path); }
}
