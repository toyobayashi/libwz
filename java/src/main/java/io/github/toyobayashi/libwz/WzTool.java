package io.github.toyobayashi.libwz;

import io.github.toyobayashi.libwz.WzEnums.*;

public final class WzTool {
    static { NativeLibraryLoader.load(); }
    private WzTool() {}

    private static native int nativeDetectMapleVersion(String path, short[] outVersion);
    private static native byte[] nativeGetIvForVersion(int version);

    public static MapleVersion detectMapleVersion(String path, short[] outVersion) {
        int v = nativeDetectMapleVersion(path, outVersion);
        for (MapleVersion m : MapleVersion.values()) if (m.value == v) return m;
        return MapleVersion.UNKNOWN;
    }

    public static byte[] getIvForVersion(MapleVersion ver) {
        return nativeGetIvForVersion(ver.value);
    }
}
