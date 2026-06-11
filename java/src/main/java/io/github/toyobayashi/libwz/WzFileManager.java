package io.github.toyobayashi.libwz;

import io.github.toyobayashi.libwz.WzEnums.*;

import java.util.LinkedHashMap;
import java.util.Map;

public class WzFileManager implements AutoCloseable {

    static { NativeLibraryLoader.load(); }

    private long nativePtr;

    public WzFileManager(String directory, boolean isStandalone) {
        this.nativePtr = nativeCreate(directory, isStandalone);
    }

    private static native long nativeCreate(String directory, boolean isStandalone);
    private static native void nativeBuildWzFileList(long ptr);
    private static native String[] nativeGetWzFileListKeys(long ptr);
    private static native String[] nativeGetWzFileListValues(long ptr, String baseName);
    private static native long nativeLoadDataWzHotfixFile(long ptr, String basePath, int mapVersion);
    private static native long nativeLoadWzFile(long ptr, String baseName, int mapVersion);
    private static native void nativeUnloadWzFile(long ptr, long wzFilePtr);
    private static native void nativeDispose(long ptr);

    public void buildWzFileList() {
        nativeBuildWzFileList(nativePtr);
    }

    public Map<String, String[]> getWzFilesList() {
        Map<String, String[]> map = new LinkedHashMap<>();
        String[] keys = nativeGetWzFileListKeys(nativePtr);
        if (keys == null) return map;
        for (String key : keys) {
            map.put(key, nativeGetWzFileListValues(nativePtr, key));
        }
        return map;
    }

    public WzImage loadDataWzHotfixFile(String basePath, MapleVersion version) {
        long p = nativeLoadDataWzHotfixFile(nativePtr, basePath, version.value);
        return p == 0 ? null : new WzImage(p);
    }

    public WzFile loadWzFile(String baseName, MapleVersion version) {
        long p = nativeLoadWzFile(nativePtr, baseName, version.value);
        return p == 0 ? null : new WzFile(p);
    }

    public void unloadWzFile(WzFile wzFile) {
        if (wzFile == null) return;
        long ptr = wzFile.nativePtr();
        nativeUnloadWzFile(nativePtr, ptr);
        wzFile.updateNativePtr(0);
    }

    @Override
    public void close() {
        if (nativePtr != 0) {
            nativeDispose(nativePtr);
            nativePtr = 0;
        }
    }
}
