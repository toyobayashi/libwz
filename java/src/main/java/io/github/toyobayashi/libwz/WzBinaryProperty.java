package io.github.toyobayashi.libwz;

import io.github.toyobayashi.libwz.WzEnums.*;

public class WzBinaryProperty extends WzImageProperty {
    WzBinaryProperty(long ptr) { super(ptr); }

    private static native byte[] nativeGetBytes(long ptr);
    private static native byte[] nativeGetWAVPlayback(long ptr);
    private static native int nativeLength(long ptr);
    private static native int nativeFrequency(long ptr);
    private static native int nativeType(long ptr);
    private static native boolean nativeHeaderEncrypted(long ptr);
    private static native boolean nativeSaveToFile(long ptr, String path);

    public byte[] getBytes() { return nativeGetBytes(nativePtr); }
    public byte[] getWAVPlayback() { return nativeGetWAVPlayback(nativePtr); }
    public int getLength() { return nativeLength(nativePtr); }
    public int getFrequency() { return nativeFrequency(nativePtr); }

    public BinaryType getType() {
        int v = nativeType(nativePtr);
        for (BinaryType t : BinaryType.values()) if (t.value == v) return t;
        return BinaryType.RAW;
    }

    public boolean isHeaderEncrypted() { return nativeHeaderEncrypted(nativePtr); }
    public boolean saveToFile(String path) { return nativeSaveToFile(nativePtr, path); }
}
