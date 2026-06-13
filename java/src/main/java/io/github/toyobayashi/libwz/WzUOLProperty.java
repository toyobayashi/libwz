package io.github.toyobayashi.libwz;

public class WzUOLProperty extends WzImageProperty {
    WzUOLProperty(long ptr) { super(ptr); }

    private static native String nativeGetValue(long ptr);
    private static native long nativeGetLinkValue(long ptr);

    public String getValue() { return nativeGetValue(nativePtr); }

    public WzObject getLinkValue() {
        long p = nativeGetLinkValue(nativePtr);
        if (p == 0) return null;
        int type = WzObject.nativeObjectType(p);
        return WzObjectFactory.wrap(type, p);
    }
}
