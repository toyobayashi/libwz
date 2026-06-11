package io.github.toyobayashi.libwz;

public class WzSubProperty extends WzImageProperty {
    WzSubProperty(long ptr) { super(ptr); }

    @Override
    public WzPropertyCollection wzProperties() {
        return new WzPropertyCollection(nativePtr, false);
    }
}
