package io.github.toyobayashi.libwz;

class WzObjectFactory {
    static WzObject wrap(int objectType, long ptr) {
        if (ptr == 0) return null;
        return switch (objectType) {
            case 0 -> new WzFile(ptr);  case 1 -> new WzImage(ptr);
            case 2 -> new WzDirectory(ptr); case 3 -> WzPropertyFactory.wrap(ptr);
            default -> null;
        };
    }
}
