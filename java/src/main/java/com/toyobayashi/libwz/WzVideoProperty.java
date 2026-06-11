package com.toyobayashi.libwz;

public class WzVideoProperty extends WzImageProperty {
    WzVideoProperty(long ptr) { super(ptr); }

    private static native byte[] nativeGetBytes(long ptr);
    private static native boolean nativeSaveToFile(long ptr, String path);

    public byte[] getData() { return nativeGetBytes(nativePtr); }
    public boolean saveToFile(String path) { return nativeSaveToFile(nativePtr, path); }

    @Override
    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, false);
    }
}
