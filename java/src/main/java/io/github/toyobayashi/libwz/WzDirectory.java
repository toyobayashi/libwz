package io.github.toyobayashi.libwz;

import java.util.AbstractList;
import java.util.List;

public class WzDirectory extends WzObject {
    WzDirectory(long ptr) { super(ptr); }

    private static native void nativeDispose(long ptr);
    private static native String nativeName(long ptr);
    private static native int nativeCountImages(long ptr);
    private static native int nativeCountDirectories(long ptr);
    private static native int nativeCountImagesTotal(long ptr);
    private static native long nativeGetImage(long ptr, int index);
    private static native long nativeGetImageByName(long ptr, String name);
    private static native long nativeGetDirectory(long ptr, int index);
    private static native long nativeGetDirectoryByName(long ptr, String name);
    private static native int nativeBlockSize(long ptr);
    private static native int nativeChecksum(long ptr);
    private static native long nativeOffset(long ptr);
    private static native long nativeAddDirectory(long ptr, String name);
    private static native long nativeAddImage(long ptr, String name);
    private static native void nativeRemoveDirectory(long ptr, long childPtr);
    private static native void nativeRemoveImage(long ptr, long childPtr);

    @Override public String getName() { return nativeName(nativePtr); }
    public int countImages() { return nativeCountImagesTotal(nativePtr); }

    private static final class WzImageList extends AbstractList<WzImage> {
        private final long nativePtr;
        WzImageList(long ptr) { nativePtr = ptr; }
        @Override public int size() { return nativeCountImages(nativePtr); }
        @Override public WzImage get(int index) {
            long p = nativeGetImage(nativePtr, index);
            return p == 0 ? null : new WzImage(p);
        }
    }

    private static final class WzDirectoryList extends AbstractList<WzDirectory> {
        private final long nativePtr;
        WzDirectoryList(long ptr) { nativePtr = ptr; }
        @Override public int size() {
            return nativeCountDirectories(nativePtr);
        }
        @Override public WzDirectory get(int index) {
            long p = nativeGetDirectory(nativePtr, index);
            return p == 0 ? null : new WzDirectory(p);
        }
    }

    public List<WzImage> wzImages() {
        return new WzImageList(nativePtr);
    }

    public List<WzDirectory> wzDirectories() {
        return new WzDirectoryList(nativePtr);
    }

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

    public WzDirectory addDirectory(String name) {
        long p = nativeAddDirectory(nativePtr, name);
        return p == 0 ? null : new WzDirectory(p);
    }

    public WzImage addImage(String name) {
        long p = nativeAddImage(nativePtr, name);
        return p == 0 ? null : new WzImage(p);
    }

    public void removeDirectory(WzDirectory directory) {
        nativeRemoveDirectory(nativePtr, directory.nativePtr());
    }

    public void removeImage(WzImage image) {
        nativeRemoveImage(nativePtr, image.nativePtr());
    }

    @Override
    protected void dispose() {
        if (ownsNative() && nativePtr != 0) {
            nativeDispose(nativePtr);
            nativePtr = 0;
        }
    }
}
