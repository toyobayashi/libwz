package io.github.toyobayashi.libwz;

public abstract class WzObject implements AutoCloseable {
    static { NativeLibraryLoader.load(); }

    protected long nativePtr;

    protected WzObject(long ptr) { this.nativePtr = ptr; }

    public long nativePtr() { return nativePtr; }

    protected static native int nativeObjectType(long ptr);
    private static native String nativeName(long ptr);
    private static native long nativeParent(long ptr);
    private static native String nativeFullPath(long ptr);
    private static native long nativeWzFileParent(long ptr);
    private static native long nativeGetTopMostWzDirectory(long ptr);
    private static native long nativeGetTopMostWzImage(long ptr);
    private static native long nativeAt(long ptr, String name);

    public WzEnums.ObjectType getObjectType() {
        int v = nativeObjectType(nativePtr);
        for (var t : WzEnums.ObjectType.values()) if (t.value == v) return t;
        return WzEnums.ObjectType.FILE;
    }

    public String getName() { return nativeName(nativePtr); }

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
        return p == 0 ? null : WzObjectFactory.wrap(WzEnums.ObjectType.DIRECTORY.value, p);
    }

    public WzObject getTopMostWzImage() {
        long p = nativeGetTopMostWzImage(nativePtr);
        return p == 0 ? null : WzObjectFactory.wrap(WzEnums.ObjectType.IMAGE.value, p);
    }

    public WzObject at(String name) {
        long p = nativeAt(nativePtr, name);
        if (p == 0) return null;
        int type = nativeObjectType(p);
        return WzObjectFactory.wrap(type, p);
    }

    void updateNativePtr(long ptr) { this.nativePtr = ptr; }

    @Override
    public void close() { dispose(); }
    protected void dispose() {}
}
