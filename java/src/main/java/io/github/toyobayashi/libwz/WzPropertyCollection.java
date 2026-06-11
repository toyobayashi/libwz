package io.github.toyobayashi.libwz;

import java.util.Iterator;
import java.util.NoSuchElementException;

public class WzPropertyCollection implements Iterable<WzImageProperty> {
    private final long nativePtr;
    private final boolean isImage;

    WzPropertyCollection(long nativePtr, boolean isImage) {
        this.nativePtr = nativePtr;
        this.isImage = isImage;
    }

    public int size() {
        return isImage ? WzImage.nativeCountProperties(nativePtr)
                       : WzImageProperty.nativeCountChildren(nativePtr);
    }

    public WzImageProperty get(int index) {
        long p = isImage ? WzImage.nativeGetProperty(nativePtr, index)
                         : WzImageProperty.nativeGetChild(nativePtr, index);
        return p == 0 ? null : WzPropertyFactory.wrap(p);
    }

    @Override
    public Iterator<WzImageProperty> iterator() {
        return new Iterator<WzImageProperty>() {
            private int idx = 0;
            private final int sz = size();
            @Override public boolean hasNext() { return idx < sz; }
            @Override public WzImageProperty next() {
                if (idx >= sz) throw new NoSuchElementException();
                return get(idx++);
            }
        };
    }
}
