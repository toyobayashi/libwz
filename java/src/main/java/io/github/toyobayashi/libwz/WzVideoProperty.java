package io.github.toyobayashi.libwz;

public class WzVideoProperty extends WzImageProperty {
    WzVideoProperty(long ptr) { super(ptr); }

    private static native byte[] nativeGetBytes(long ptr);

    public byte[] getData() { return nativeGetBytes(nativePtr); }

    @Override
    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, false);
    }
}
