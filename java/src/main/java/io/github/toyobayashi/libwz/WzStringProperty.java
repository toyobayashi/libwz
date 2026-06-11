package io.github.toyobayashi.libwz;

public class WzStringProperty extends WzImageProperty {
    WzStringProperty(long ptr) { super(ptr); }

    private static native String nativeGetValue(long ptr);
    private static native boolean nativeSaveToFile(long ptr, String path);

    public String getValue() { return nativeGetValue(nativePtr); }
    public boolean saveToFile(String path) { return nativeSaveToFile(nativePtr, path); }
}
