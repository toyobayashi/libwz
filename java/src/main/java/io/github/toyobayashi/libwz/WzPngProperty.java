package io.github.toyobayashi.libwz;

public class WzPngProperty extends WzImageProperty {
    WzPngProperty(long ptr) { super(ptr); }

    private static native int nativeWidth(long ptr);
    private static native int nativeHeight(long ptr);
    private static native int nativeFormat(long ptr);
    private static native boolean nativeListWzUsed(long ptr);
    private static native byte[] nativeGetImage(long ptr);
    private static native byte[] nativeGetCompressedBytes(long ptr);
    private static native boolean nativeSaveToFile(long ptr, String path);

    public int getWidth() { return nativeWidth(nativePtr); }
    public int getHeight() { return nativeHeight(nativePtr); }
    public int getFormat() { return nativeFormat(nativePtr); }
    public boolean isListWzUsed() { return nativeListWzUsed(nativePtr); }
    public byte[] getImage() { return nativeGetImage(nativePtr); }
    public byte[] getCompressedBytes() { return nativeGetCompressedBytes(nativePtr); }
    public boolean saveToFile(String path) { return nativeSaveToFile(nativePtr, path); }
}
