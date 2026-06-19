package io.github.toyobayashi.libwz;

public abstract class WzObject implements AutoCloseable {
    static { NativeLibraryLoader.load(); }

    protected long nativePtr;
    private boolean ownsNative;

    protected WzObject(long ptr) {
        this(ptr, false);
    }

    protected WzObject(long ptr, boolean ownsNative) {
        this.nativePtr = ptr;
        this.ownsNative = ownsNative;
    }

    public long nativePtr() { return nativePtr; }

    protected static native int nativeObjectType(long ptr);
    private static native String nativeName(long ptr);
    private static native long nativeParent(long ptr);
    private static native String nativeFullPath(long ptr);
    private static native long nativeWzFileParent(long ptr);
    private static native long nativeGetTopMostWzDirectory(long ptr);
    private static native long nativeGetTopMostWzImage(long ptr);
    private static native long nativeAt(long ptr, String name);
    private static native void nativeSetName(long ptr, String name);
    private static native void nativeRemove(long ptr);

    public WzEnums.ObjectType getObjectType() {
        int v = nativeObjectType(nativePtr);
        for (var t : WzEnums.ObjectType.values()) if (t.value == v) return t;
        return WzEnums.ObjectType.FILE;
    }

    public String getName() { return nativeName(nativePtr); }

    public void setName(String name) { nativeSetName(nativePtr, name); }

    public void remove() { nativeRemove(nativePtr); }

    public WzObject getParent() {
        long p = nativeParent(nativePtr);
        if (p == 0) return null;
        int type = nativeObjectType(p);
        return WzObjectFactory.wrap(type, p);
    }

    public String getFullPath() { return nativeFullPath(nativePtr); }

    public WzFile getWzFileParent() {
        long p = nativeWzFileParent(nativePtr);
        return p == 0 ? null : new WzFile(p);
    }

    public WzObject getTopMostWzDirectory() {
        long p = nativeGetTopMostWzDirectory(nativePtr);
        if (p == 0) return null;
        int type = nativeObjectType(p);
        return WzObjectFactory.wrap(type, p);
    }

    public WzObject getTopMostWzImage() {
        long p = nativeGetTopMostWzImage(nativePtr);
        if (p == 0) return null;
        int type = nativeObjectType(p);
        return WzObjectFactory.wrap(type, p);
    }

    public WzObject at(String name) {
        long p = nativeAt(nativePtr, name);
        if (p == 0) return null;
        int type = nativeObjectType(p);
        return WzObjectFactory.wrap(type, p);
    }

    void updateNativePtr(long ptr) { this.nativePtr = ptr; }
    void releaseNativeOwnership() { this.ownsNative = false; }
    protected boolean ownsNative() { return ownsNative; }

    @Override
    public void close() { dispose(); }
    protected void dispose() {}
}
