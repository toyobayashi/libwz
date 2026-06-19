package io.github.toyobayashi.libwz;

public class WzImage extends WzObject {
    WzImage(long ptr) { super(ptr); }

    private static native void nativeDispose(long ptr);
    private static native String nativeName(long ptr);
    private static native boolean nativeParsed(long ptr);
    private static native boolean nativeChanged(long ptr);
    private static native int nativeBlockSize(long ptr);
    private static native int nativeChecksum(long ptr);
    private static native long nativeOffset(long ptr);
    private static native boolean nativeIsLuaWzImage(long ptr);
    private static native void nativeParseImage(long ptr);
    static native int nativeCountProperties(long ptr);
    static native long nativeGetProperty(long ptr, int index);
    private static native long nativeGetFromPath(long ptr, String path);
    private static native void nativeAddProperty(long ptr, long propPtr);
    private static native void nativeRemoveProperty(long ptr, long propPtr);
    private static native void nativeClearProperties(long ptr);

    @Override public String getName() { return nativeName(nativePtr); }
    public boolean isParsed() { return nativeParsed(nativePtr); }
    public boolean isChanged() { return nativeChanged(nativePtr); }
    public int getBlockSize() { return nativeBlockSize(nativePtr); }
    public int getChecksum() { return nativeChecksum(nativePtr); }
    public long getOffset() { return nativeOffset(nativePtr); }
    public boolean isLuaWzImage() { return nativeIsLuaWzImage(nativePtr); }
    public void parseImage() { nativeParseImage(nativePtr); }

    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, true);
    }

    public WzImageProperty getFromPath(String path) {
        long p = nativeGetFromPath(nativePtr, path);
        return p == 0 ? null : WzPropertyFactory.wrap(p);
    }

    public void addProperty(WzImageProperty property) {
        nativeAddProperty(nativePtr, property.nativePtr());
        property.releaseNativeOwnership();
    }

    public void removeProperty(WzImageProperty property) {
        long ptr = property.nativePtr();
        nativeRemoveProperty(nativePtr, ptr);
        invalidateNativePtr(ptr);
    }

    public void clearProperties() {
        int count = nativeCountProperties(nativePtr);
        long[] childPtrs = new long[count];
        for (int i = 0; i < count; i++) {
            childPtrs[i] = nativeGetProperty(nativePtr, i);
        }
        nativeClearProperties(nativePtr);
        for (long ptr : childPtrs) invalidateNativePtr(ptr);
    }

    @Override
    protected void dispose() {
        if (ownsNative() && nativePtr != 0) {
            long ptr = nativePtr;
            nativeDispose(ptr);
            invalidateNativePtr(ptr);
        }
    }
}
