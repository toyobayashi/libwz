package io.github.toyobayashi.libwz;

public class WzUOLProperty extends WzImageProperty {
    WzUOLProperty(long ptr) { super(ptr); }
    WzUOLProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native String nativeGetValue(long ptr);
    private static native void nativeSetValue(long ptr, String value);
    private static native long nativeGetLinkValue(long ptr);

    public String getValue() { return nativeGetValue(nativePtr); }
    public void setValue(String value) { nativeSetValue(nativePtr, value); }

    public WzObject getLinkValue() {
        long p = nativeGetLinkValue(nativePtr);
        if (p == 0) return null;
        int type = WzObject.nativeObjectType(p);
        return WzObjectFactory.wrap(type, p);
    }
}
