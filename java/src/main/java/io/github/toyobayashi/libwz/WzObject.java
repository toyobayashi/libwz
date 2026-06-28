package io.github.toyobayashi.libwz;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public abstract class WzObject implements AutoCloseable {
    static { NativeLibraryLoader.load(); }

    private static final Map<Long, List<WeakReference<WzObject>>> HANDLE_REGISTRY =
        new HashMap<>();

    protected long nativePtr;
    private boolean ownsNative;

    protected WzObject(long ptr) {
        this(ptr, false);
    }

    protected WzObject(long ptr, boolean ownsNative) {
        this.ownsNative = ownsNative;
        updateNativePtr(ptr);
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

    public void remove() {
        long ptr = nativePtr;
        nativeRemove(nativePtr);
        markNativeOwned(ptr);
    }

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

    void updateNativePtr(long ptr) {
        synchronized (HANDLE_REGISTRY) {
            unregisterLocked(nativePtr, this);
            nativePtr = ptr;
            if (ptr != 0) {
                cleanupLocked(ptr);
                HANDLE_REGISTRY
                    .computeIfAbsent(ptr, ignored -> new ArrayList<>())
                    .add(new WeakReference<>(this));
            }
        }
    }

    void releaseNativeOwnership() { this.ownsNative = false; }
    void takeNativeOwnership() { this.ownsNative = true; }
    protected boolean ownsNative() { return ownsNative; }

    static void markNativeOwned(long ptr) {
        if (ptr == 0) return;
        synchronized (HANDLE_REGISTRY) {
            List<WeakReference<WzObject>> refs = HANDLE_REGISTRY.get(ptr);
            if (refs == null) return;
            for (WeakReference<WzObject> ref : refs) {
                WzObject obj = ref.get();
                if (obj != null && obj.nativePtr == ptr) obj.takeNativeOwnership();
            }
        }
    }

    static void invalidateNativePtr(long ptr) {
        if (ptr == 0) return;
        synchronized (HANDLE_REGISTRY) {
            List<WeakReference<WzObject>> refs = HANDLE_REGISTRY.remove(ptr);
            if (refs == null) return;
            for (WeakReference<WzObject> ref : refs) {
                WzObject obj = ref.get();
                if (obj != null && obj.nativePtr == ptr) {
                    obj.nativePtr = 0;
                    obj.ownsNative = false;
                }
            }
        }
    }

    static void invalidateNativePtrs(List<Long> ptrs) {
        for (long ptr : ptrs) invalidateNativePtr(ptr);
    }

    protected List<Long> collectNativeSubtreePointers() {
        List<Long> ptrs = new ArrayList<>();
        addNativePtr(ptrs, nativePtr);
        return ptrs;
    }

    static void addNativePtr(List<Long> ptrs, long ptr) {
        if (ptr == 0 || ptrs.contains(ptr)) return;
        ptrs.add(ptr);
    }

    private static void unregisterLocked(long ptr, WzObject object) {
        if (ptr == 0) return;
        List<WeakReference<WzObject>> refs = HANDLE_REGISTRY.get(ptr);
        if (refs == null) return;
        Iterator<WeakReference<WzObject>> it = refs.iterator();
        while (it.hasNext()) {
            WzObject obj = it.next().get();
            if (obj == null || obj == object) it.remove();
        }
        if (refs.isEmpty()) HANDLE_REGISTRY.remove(ptr);
    }

    private static void cleanupLocked(long ptr) {
        List<WeakReference<WzObject>> refs = HANDLE_REGISTRY.get(ptr);
        if (refs == null) return;
        refs.removeIf(ref -> ref.get() == null);
        if (refs.isEmpty()) HANDLE_REGISTRY.remove(ptr);
    }

    @Override
    public void close() { dispose(); }
    protected void dispose() {}
}
