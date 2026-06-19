package io.github.toyobayashi.libwz;

public abstract class WzImageProperty extends WzObject {
    WzImageProperty(long ptr) { super(ptr); }
    WzImageProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native int nativePropertyType(long ptr);
    private static native String nativeName(long ptr);
    static native int nativeCountChildren(long ptr);
    static native long nativeGetChild(long ptr, int index);
    private static native long nativeGetChildByName(long ptr, String name);
    private static native long nativeGetFromPath(long ptr, String path);
    private static native void nativeFree(long ptr);
    private static native void nativeAddProperty(long ptr, long childPtr);
    private static native void nativeRemoveProperty(long ptr, long childPtr);
    private static native void nativeClearProperties(long ptr);
    private static native int nativeGetInt(long ptr);
    private static native short nativeGetShort(long ptr);
    private static native long nativeGetLong(long ptr);
    private static native float nativeGetFloat(long ptr);
    private static native double nativeGetDouble(long ptr);
    private static native String nativeGetString(long ptr);
    private static native byte[] nativeGetBytes(long ptr);

    public WzEnums.PropertyType getPropertyType() {
        int v = nativePropertyType(nativePtr);
        for (WzEnums.PropertyType t : WzEnums.PropertyType.values()) if (t.value == v) return t;
        return WzEnums.PropertyType.NULL;
    }

    @Override public String getName() { return nativeName(nativePtr); }

    public WzPropertyCollection wzProperties() { return null; }

    public WzImageProperty getChild(int index) {
        long p = nativeGetChild(nativePtr, index);
        return p == 0 ? null : WzPropertyFactory.wrap(p);
    }

    public WzImageProperty getChildByName(String name) {
        long p = nativeGetChildByName(nativePtr, name);
        return p == 0 ? null : WzPropertyFactory.wrap(p);
    }

    public WzImageProperty getFromPath(String path) {
        long p = nativeGetFromPath(nativePtr, path);
        return p == 0 ? null : WzPropertyFactory.wrap(p);
    }

    private static native long nativeGetLinkedWzImageProperty(long ptr);

    public WzImageProperty getLinkedWzImageProperty() {
        long p = nativeGetLinkedWzImageProperty(nativePtr);
        return p == 0 ? null : WzPropertyFactory.wrap(p);
    }

    public int getInt() { return nativeGetInt(nativePtr); }
    public short getShort() { return nativeGetShort(nativePtr); }
    public long getLong() { return nativeGetLong(nativePtr); }
    public float getFloat() { return nativeGetFloat(nativePtr); }
    public double getDouble() { return nativeGetDouble(nativePtr); }
    public String getString() { return nativeGetString(nativePtr); }
    public byte[] getBytes() { return nativeGetBytes(nativePtr); }

    public void addProperty(WzImageProperty property) {
        nativeAddProperty(nativePtr, property.nativePtr());
        property.releaseNativeOwnership();
    }

    public void removeProperty(WzImageProperty property) {
        nativeRemoveProperty(nativePtr, property.nativePtr());
    }

    public void clearProperties() { nativeClearProperties(nativePtr); }

    @Override
    protected void dispose() {
        if (ownsNative() && nativePtr != 0) {
            nativeFree(nativePtr);
            nativePtr = 0;
        }
    }
}
