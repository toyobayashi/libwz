package com.toyobayashi.libwz;

public class WzConvexProperty extends WzImageProperty {
    WzConvexProperty(long ptr) { super(ptr); }

    @Override
    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, false);
    }
}
