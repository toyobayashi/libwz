package com.toyobayashi.libwz;

import com.toyobayashi.libwz.WzEnums.*;

public class WzFile extends WzObject {
    public WzFile(String path, short gameVersion, MapleVersion version) {
        super(0);
        this.nativePtr = nativeOpen(path, gameVersion, version.ordinal());
    }

    public WzFile(String path, MapleVersion version) {
        this(path, (short)-1, version);
    }

    public WzFile(String path, byte[] iv) {
        super(0);
        this.nativePtr = nativeOpenWithIv(path, iv);
    }

    WzFile(long ptr) { super(ptr); }

    private static native long nativeOpen(String path, short gameVersion, int version);
    private static native long nativeOpenWithIv(String path, byte[] iv);
    private static native int nativeParseWzFile(long ptr);
    private static native void nativeDispose(long ptr);
    private static native String nativeName(long ptr);
    private static native String nativeFilePath(long ptr);
    private static native short nativeVersion(long ptr);
    private static native int nativeMapleVersion(long ptr);
    private static native long nativeGetWzDirectory(long ptr);
    private static native boolean nativeIs64BitWzFile(long ptr);
    private static native boolean nativeIsUnloaded(long ptr);
    private static native int nativeVersionHash(long ptr);
    private static native long nativeGetObjectFromPath(long ptr, String path,
                                                        boolean checkFirstDirectoryName);

    public ParseStatus parseWzFile() {
        int v = nativeParseWzFile(nativePtr);
        for (ParseStatus s : ParseStatus.values()) if (s.value == v) return s;
        return ParseStatus.FAILED_UNKNOWN;
    }

    @Override public String getName() { return nativeName(nativePtr); }
    public String getFilePath() { return nativeFilePath(nativePtr); }
    public short getVersion() { return nativeVersion(nativePtr); }

    public MapleVersion getMapleVersion() {
        int v = nativeMapleVersion(nativePtr);
        for (var m : MapleVersion.values()) if (m.value == v) return m;
        return MapleVersion.UNKNOWN;
    }

    public WzDirectory getWzDirectory() {
        long p = nativeGetWzDirectory(nativePtr);
        return p == 0 ? null : new WzDirectory(p);
    }

    public boolean is64BitWzFile() { return nativeIs64BitWzFile(nativePtr); }
    public boolean isUnloaded() { return nativeIsUnloaded(nativePtr); }
    public int getVersionHash() { return nativeVersionHash(nativePtr); }

    public WzObject getObjectFromPath(String path) {
        return getObjectFromPath(path, true);
    }

    public WzObject getObjectFromPath(String path,
                                      boolean checkFirstDirectoryName) {
        long p = nativeGetObjectFromPath(nativePtr, path,
                                          checkFirstDirectoryName);
        if (p == 0) return null;
        int type = WzObject.nativeObjectType(p);
        return WzObjectFactory.wrap(type, p);
    }

    @Override
    protected void dispose() { nativeDispose(nativePtr); nativePtr = 0; }
}
