package com.toyobayashi.libwz;

public class WzRawDataProperty extends WzImageProperty {
    WzRawDataProperty(long ptr) { super(ptr); }

    private static native byte[] nativeGetBytes(long ptr);
    private static native int nativeType(long ptr);
    private static native boolean nativeSaveToFile(long ptr, String path);

    public byte[] getBytes() { return nativeGetBytes(nativePtr); }
    public int getRawType() { return nativeType(nativePtr); }
    public boolean saveToFile(String path) { return nativeSaveToFile(nativePtr, path); }

    @Override
    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, false);
    }
}
