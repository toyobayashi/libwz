package com.toyobayashi.libwz;

public final class WzEnums {
    public enum MapleVersion {
        GMS(0), EMS(1), BMS(2), CLASSIC(3), GENERATE(4), GETFROMZLZ(5), CUSTOM(6), UNKNOWN(99);
        final int value;
        MapleVersion(int v) { this.value = v; }
    }

    public enum ParseStatus {
        PATH_IS_NULL(-1), ERROR_GAME_VER_HASH(-2), FAILED_UNKNOWN(0), SUCCESS(1);
        final int value;
        ParseStatus(int v) { this.value = v; }
    }

    public enum PropertyType {
        NULL(0), SHORT(1), INT(2), LONG(3), FLOAT(4), DOUBLE(5),
        STRING(6), SUB(7), CANVAS(8), VECTOR(9), CONVEX(10),
        SOUND(11), RAW(12), UOL(13), LUA(14), PNG(15);
        final int value;
        PropertyType(int v) { this.value = v; }
    }

    public enum ObjectType {
        FILE(0), IMAGE(1), DIRECTORY(2), PROPERTY(3), LIST(4);
        final int value;
        ObjectType(int v) { this.value = v; }
    }

    public enum BinaryType {
        RAW(0), MP3(1), WAV(2);
        final int value;
        BinaryType(int v) { this.value = v; }
    }

    public enum ErrorCode {
        NONE(0), NOT_IMPLEMENTED(1), INVALID_ARGUMENT(2), PARSE_ERROR(3),
        IO_ERROR(4), DATA_ERROR(5), NOT_FOUND(6), NULL_HANDLE(7),
        WRONG_TYPE(8), INDEX_OUT_OF_RANGE(9);
        final int value;
        ErrorCode(int v) { this.value = v; }
    }
}
