package io.github.toyobayashi.libwz;

class WzPropertyFactory {
    private static native int nativePropertyType(long ptr);

    static WzImageProperty wrap(long ptr) {
        int type = nativePropertyType(ptr);
        return switch (type) {
            case 0 -> new WzNullProperty(ptr); case 1 -> new WzShortProperty(ptr);
            case 2 -> new WzIntProperty(ptr); case 3 -> new WzLongProperty(ptr);
            case 4 -> new WzFloatProperty(ptr); case 5 -> new WzDoubleProperty(ptr);
            case 6 -> new WzStringProperty(ptr); case 7 -> new WzSubProperty(ptr);
            case 8 -> new WzCanvasProperty(ptr); case 9 -> new WzVectorProperty(ptr);
            case 10 -> new WzConvexProperty(ptr); case 11 -> new WzBinaryProperty(ptr);
            case 12 -> new WzRawDataProperty(ptr); case 13 -> new WzUOLProperty(ptr);
            case 14 -> new WzLuaProperty(ptr); case 15 -> new WzPngProperty(ptr);
            default -> null;
        };
    }
}
