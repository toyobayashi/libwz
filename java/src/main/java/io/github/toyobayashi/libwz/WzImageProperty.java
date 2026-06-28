package io.github.toyobayashi.libwz;

import java.util.ArrayList;
import java.util.List;

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
        long ptr = property.nativePtr();
        nativeRemoveProperty(nativePtr, property.nativePtr());
        markNativeOwned(ptr);
    }

    public void clearProperties() {
        int count = nativeCountChildren(nativePtr);
        List<Long> ptrs = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            collectPropertyPointers(nativeGetChild(nativePtr, i), ptrs);
        }
        nativeClearProperties(nativePtr);
        invalidateNativePtrs(ptrs);
    }

    @Override
    protected void dispose() {
        if (ownsNative() && nativePtr != 0) {
            List<Long> ptrs = collectNativeSubtreePointers();
            nativeFree(nativePtr);
            invalidateNativePtrs(ptrs);
        }
    }

    @Override
    protected List<Long> collectNativeSubtreePointers() {
        List<Long> ptrs = super.collectNativeSubtreePointers();
        collectPropertyChildrenPointers(nativePtr, ptrs);
        return ptrs;
    }

    static void collectPropertyPointers(long ptr, List<Long> ptrs) {
        addNativePtr(ptrs, ptr);
        collectPropertyChildrenPointers(ptr, ptrs);
    }

    private static void collectPropertyChildrenPointers(long ptr,
                                                        List<Long> ptrs) {
        if (ptr == 0 || !isPropertyContainer(ptr)) return;
        int count = nativeCountChildren(ptr);
        for (int i = 0; i < count; i++) {
            collectPropertyPointers(nativeGetChild(ptr, i), ptrs);
        }
    }

    private static boolean isPropertyContainer(long ptr) {
        return isPropertyContainerType(nativePropertyType(ptr));
    }

    static boolean isPropertyContainerType(int type) {
        return type == WzEnums.PropertyType.SUB.value
            || type == WzEnums.PropertyType.CANVAS.value
            || type == WzEnums.PropertyType.CONVEX.value
            || type == WzEnums.PropertyType.RAW.value;
    }
}
