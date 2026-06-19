package io.github.toyobayashi.libwz;

public class WzPropertyFactory {
    private static native int nativePropertyType(long ptr);
    private static native boolean nativeIsVideoProperty(long ptr);
    private static native long nativeCreateNull(String name);
    private static native long nativeCreateShort(String name, short value);
    private static native long nativeCreateInt(String name, int value);
    private static native long nativeCreateLong(String name, long value);
    private static native long nativeCreateFloat(String name, float value);
    private static native long nativeCreateDouble(String name, double value);
    private static native long nativeCreateString(String name, String value);
    private static native long nativeCreateSub(String name);
    private static native long nativeCreateVector(String name, int x, int y);
    private static native long nativeCreateUOL(String name, String value);

    static WzImageProperty wrap(long ptr) {
        int type = nativePropertyType(ptr);
        return switch (type) {
            case 0 -> new WzNullProperty(ptr); case 1 -> new WzShortProperty(ptr);
            case 2 -> new WzIntProperty(ptr); case 3 -> new WzLongProperty(ptr);
            case 4 -> new WzFloatProperty(ptr); case 5 -> new WzDoubleProperty(ptr);
            case 6 -> new WzStringProperty(ptr); case 7 -> new WzSubProperty(ptr);
            case 8 -> new WzCanvasProperty(ptr); case 9 -> new WzVectorProperty(ptr);
            case 10 -> new WzConvexProperty(ptr); case 11 -> new WzBinaryProperty(ptr);
            case 12 -> nativeIsVideoProperty(ptr) ? new WzVideoProperty(ptr) : new WzRawDataProperty(ptr);
            case 13 -> new WzUOLProperty(ptr);
            case 14 -> new WzLuaProperty(ptr); case 15 -> new WzPngProperty(ptr);
            default -> null;
        };
    }

    public static WzNullProperty createNull(String name) {
        return new WzNullProperty(nativeCreateNull(name), true);
    }

    public static WzShortProperty createShort(String name, short value) {
        return new WzShortProperty(nativeCreateShort(name, value), true);
    }

    public static WzIntProperty createInt(String name, int value) {
        return new WzIntProperty(nativeCreateInt(name, value), true);
    }

    public static WzLongProperty createLong(String name, long value) {
        return new WzLongProperty(nativeCreateLong(name, value), true);
    }

    public static WzFloatProperty createFloat(String name, float value) {
        return new WzFloatProperty(nativeCreateFloat(name, value), true);
    }

    public static WzDoubleProperty createDouble(String name, double value) {
        return new WzDoubleProperty(nativeCreateDouble(name, value), true);
    }

    public static WzStringProperty createString(String name, String value) {
        return new WzStringProperty(nativeCreateString(name, value), true);
    }

    public static WzSubProperty createSub(String name) {
        return new WzSubProperty(nativeCreateSub(name), true);
    }

    public static WzVectorProperty createVector(String name, int x, int y) {
        return new WzVectorProperty(nativeCreateVector(name, x, y), true);
    }

    public static WzUOLProperty createUOL(String name, String value) {
        return new WzUOLProperty(nativeCreateUOL(name, value), true);
    }
}
