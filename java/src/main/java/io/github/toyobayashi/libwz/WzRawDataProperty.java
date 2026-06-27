package io.github.toyobayashi.libwz;

public class WzRawDataProperty extends WzImageProperty {
    WzRawDataProperty(long ptr) { super(ptr); }

    private static native byte[] nativeGetBytes(long ptr);
    private static native int nativeType(long ptr);

    public byte[] getBytes() { return nativeGetBytes(nativePtr); }
    public int getRawType() { return nativeType(nativePtr); }

    @Override
    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, false);
    }
}
