package io.github.toyobayashi.libwz;

public class WzFloatProperty extends WzImageProperty {
    WzFloatProperty(long ptr) { super(ptr); }
    WzFloatProperty(long ptr, boolean ownsNative) { super(ptr, ownsNative); }

    private static native float nativeGetValue(long ptr);
    private static native void nativeSetValue(long ptr, float value);

    public float getValue() { return nativeGetValue(nativePtr); }
    public void setValue(float value) { nativeSetValue(nativePtr, value); }
}
