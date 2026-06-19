package io.github.toyobayashi.libwz;

public class WzDoubleProperty extends WzImageProperty {
    WzDoubleProperty(long ptr) { super(ptr); }
    WzDoubleProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native double nativeGetValue(long ptr);
    private static native void nativeSetValue(long ptr, double value);

    public double getValue() { return nativeGetValue(nativePtr); }
    public void setValue(double value) { nativeSetValue(nativePtr, value); }
}
