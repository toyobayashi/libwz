package io.github.toyobayashi.libwz;

public class WzVectorProperty extends WzImageProperty {
    WzVectorProperty(long ptr) { super(ptr); }
    WzVectorProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native int nativeGetX(long ptr);
    private static native int nativeGetY(long ptr);

    public int getX() { return nativeGetX(nativePtr); }
    public int getY() { return nativeGetY(nativePtr); }
}
