package io.github.toyobayashi.libwz;

public class WzCanvasProperty extends WzImageProperty {
    WzCanvasProperty(long ptr) { super(ptr); }

    private static native long nativePngProperty(long ptr);
    private static native boolean nativeContainsInlink(long ptr);
    private static native boolean nativeContainsOutlink(long ptr);
    private static native boolean nativeSaveToFile(long ptr, String path);

    public WzPngProperty getPngProperty() {
        long p = nativePngProperty(nativePtr);
        return p == 0 ? null : new WzPngProperty(p);
    }

    private static native long nativeGetLinkedWzImageProperty(long ptr);

    @Override
    public WzImageProperty getLinkedWzImageProperty() {
        long p = nativeGetLinkedWzImageProperty(nativePtr);
        return p == 0 ? null : WzPropertyFactory.wrap(p);
    }

    public boolean containsInlinkProperty() { return nativeContainsInlink(nativePtr); }
    public boolean containsOutlinkProperty() { return nativeContainsOutlink(nativePtr); }
    public boolean saveToFile(String path) { return nativeSaveToFile(nativePtr, path); }

    @Override
    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, false);
    }
}
