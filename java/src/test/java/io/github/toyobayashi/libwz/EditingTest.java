package io.github.toyobayashi.libwz;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;

import io.github.toyobayashi.libwz.WzEnums.MapleVersion;
import io.github.toyobayashi.libwz.WzEnums.ParseStatus;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.jupiter.api.Test;

class EditingTest {
    @Test
    void createsFileAddsPropertyAndUpdatesValue() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzDirectory root = file.getWzDirectory();
            assertNotNull(root);

            WzImage image = root.addImage("Character.img");
            WzIntProperty prop = WzPropertyFactory.createInt("level", 1);
            image.addProperty(prop);

            prop.setValue(255);
            assertEquals(255, prop.getValue());
            assertEquals(1, image.wzProperties().size());
            assertEquals(255, ((WzIntProperty)image.getFromPath("level")).getValue());
        }
    }

    @Test
    void supportsDirectoryImageAndPropertyRemoval() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzDirectory root = file.getWzDirectory();
            WzDirectory childDir = root.addDirectory("String");
            WzImage childImage = root.addImage("Item.img");
            assertEquals(1, root.wzDirectories().size());
            assertEquals(1, root.wzImages().size());

            WzSubProperty sub = WzPropertyFactory.createSub("info");
            WzStringProperty name = WzPropertyFactory.createString("name", "old");
            sub.addProperty(name);
            childImage.addProperty(sub);
            name.setValue("new");
            assertEquals("new", name.getValue());
            assertEquals(1, sub.wzProperties().size());

            sub.removeProperty(name);
            assertEquals(0, sub.wzProperties().size());

            WzIntProperty id = WzPropertyFactory.createInt("id", 100);
            childImage.addProperty(id);
            childImage.removeProperty(id);
            assertEquals(1, childImage.wzProperties().size());

            childImage.clearProperties();
            assertEquals(0, childImage.wzProperties().size());

            root.removeDirectory(childDir);
            root.removeImage(childImage);
            assertEquals(0, root.wzDirectories().size());
            assertEquals(0, root.wzImages().size());
        }
    }

    @Test
    void renamesAndRemovesObjects() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzImage image = file.getWzDirectory().addImage("Old.img");
            image.setName("New.img");
            assertEquals("New.img", image.getName());

            image.remove();
            assertEquals(0, file.getWzDirectory().wzImages().size());
        }
    }

    @Test
    void duplicateNamesThrowRuntimeException() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzDirectory root = file.getWzDirectory();
            root.addImage("Same.img");
            assertThrows(RuntimeException.class, () -> root.addImage("Same.img"));
        }
    }

    @Test
    void detachedPropertiesCanBeClosed() {
        WzIntProperty prop = WzPropertyFactory.createInt("detached", 7);
        assertEquals(7, prop.getValue());
        prop.close();
        assertEquals(0, prop.nativePtr());
    }

    @Test
    void savesInMemoryFileToDisk() throws Exception {
        Path temp = Files.createTempFile("libwz-editing-", ".wz");
        Files.deleteIfExists(temp);
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzImage image = file.getWzDirectory().addImage("Item.img");
            image.addProperty(WzPropertyFactory.createInt("id", 123));

            file.saveToDiskEx(temp.toString(), false, MapleVersion.GMS);
            assertEquals(true, Files.exists(temp));
            try (WzFile reopened =
                    new WzFile(temp.toString(), (short)95, MapleVersion.GMS)) {
                assertEquals(ParseStatus.SUCCESS, reopened.parseWzFile());
                WzImage reopenedImage =
                    reopened.getWzDirectory().getImageByName("Item.img");
                assertNotNull(reopenedImage);
                reopenedImage.parseImage();
                assertEquals(1, reopenedImage.wzProperties().size());
                WzImageProperty reopenedProperty = reopenedImage.getFromPath("id");
                assertNotNull(reopenedProperty);
                assertEquals(123, ((WzIntProperty)reopenedProperty).getValue());
            }
        } finally {
            Files.deleteIfExists(temp);
        }
    }
}
