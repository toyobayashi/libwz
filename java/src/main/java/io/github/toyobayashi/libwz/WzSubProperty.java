package io.github.toyobayashi.libwz;

public class WzSubProperty extends WzImageProperty {
    WzSubProperty(long ptr) { super(ptr); }
    WzSubProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    @Override
    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, false);
    }
}
