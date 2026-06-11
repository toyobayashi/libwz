package com.toyobayashi.libwz;

public class WzDirectory extends WzObject {
    WzDirectory(long ptr) { super(ptr); }

    private static native void nativeDispose(long ptr);
    private static native String nativeName(long ptr);
    private static native int nativeCountDirectories(long ptr);
    private static native int nativeCountImagesTotal(long ptr);
    private static native long nativeGetImage(long ptr, int index);
    private static native long nativeGetImageByName(long ptr, String name);
    private static native long nativeGetDirectory(long ptr, int index);
    private static native long nativeGetDirectoryByName(long ptr, String name);
    private static native int nativeBlockSize(long ptr);
    private static native int nativeChecksum(long ptr);
    private static native long nativeOffset(long ptr);

    @Override public String getName() { return nativeName(nativePtr); }
    public int countImages() { return nativeCountImagesTotal(nativePtr); }
    public int countDirectories() { return nativeCountDirectories(nativePtr); }

    public WzImage getImage(int index) {
        long p = nativeGetImage(nativePtr, index);
        return p == 0 ? null : new WzImage(p);
    }

    public WzImage getImageByName(String name) {
        long p = nativeGetImageByName(nativePtr, name);
        return p == 0 ? null : new WzImage(p);
    }

    public WzDirectory getDirectory(int index) {
        long p = nativeGetDirectory(nativePtr, index);
        return p == 0 ? null : new WzDirectory(p);
    }

    public WzDirectory getDirectoryByName(String name) {
        long p = nativeGetDirectoryByName(nativePtr, name);
        return p == 0 ? null : new WzDirectory(p);
    }

    public int getBlockSize() { return nativeBlockSize(nativePtr); }
    public int getChecksum() { return nativeChecksum(nativePtr); }
    public long getOffset() { return nativeOffset(nativePtr); }

    @Override
    protected void dispose() { nativeDispose(nativePtr); nativePtr = 0; }
}
