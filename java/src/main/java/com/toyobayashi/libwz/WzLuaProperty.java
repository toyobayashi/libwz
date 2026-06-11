package com.toyobayashi.libwz;

public class WzLuaProperty extends WzImageProperty {
    WzLuaProperty(long ptr) { super(ptr); }

    private static native byte[] nativeGetData(long ptr);
    private static native String nativeGetString(long ptr);
    private static native boolean nativeSaveToFile(long ptr, String path);

    public byte[] getData() { return nativeGetData(nativePtr); }
    public String getString() { return nativeGetString(nativePtr); }
    public boolean saveToFile(String path) { return nativeSaveToFile(nativePtr, path); }
}
