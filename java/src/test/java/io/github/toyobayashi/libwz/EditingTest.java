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
    void ivConstructorRequiresExactlyFourBytes() {
        assertThrows(IllegalArgumentException.class,
            () -> new WzFile("unused.wz", (byte[])null));
        assertThrows(IllegalArgumentException.class,
            () -> new WzFile("unused.wz", new byte[] {1, 2, 3}));
    }

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
        prop.close();
        assertEquals(0, prop.nativePtr());
    }

    @Test
    void removalInvalidatesJavaWrappers() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzDirectory root = file.getWzDirectory();
            WzDirectory directory = root.addDirectory("String");
            WzImage image = root.addImage("Item.img");
            WzIntProperty property = WzPropertyFactory.createInt("id", 123);
            image.addProperty(property);

            image.removeProperty(property);
            assertEquals(0, property.nativePtr());

            root.removeImage(image);
            assertEquals(0, image.nativePtr());

            root.removeDirectory(directory);
            assertEquals(0, directory.nativePtr());
        }
    }

    @Test
    void objectRemoveInvalidatesTargetWrapper() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzImage image = file.getWzDirectory().addImage("Old.img");
            WzSubProperty sub = WzPropertyFactory.createSub("info");
            WzIntProperty property = WzPropertyFactory.createInt("id", 123);
            sub.addProperty(property);
            image.addProperty(sub);

            property.remove();
            assertEquals(0, property.nativePtr());

            sub.remove();
            assertEquals(0, sub.nativePtr());

            image.remove();
            assertEquals(0, image.nativePtr());
        }
    }

    @Test
    void removalInvalidatesAliasedJavaWrappers() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzDirectory root = file.getWzDirectory();
            WzImage image = root.addImage("Item.img");
            WzImage imageAlias = root.getImageByName("Item.img");
            WzIntProperty property = WzPropertyFactory.createInt("id", 123);
            image.addProperty(property);
            WzImageProperty propertyAlias = image.getFromPath("id");
            assertNotNull(imageAlias);
            assertNotNull(propertyAlias);

            image.removeProperty(property);
            assertEquals(0, property.nativePtr());
            assertEquals(0, propertyAlias.nativePtr());

            root.removeImage(imageAlias);
            assertEquals(0, image.nativePtr());
            assertEquals(0, imageAlias.nativePtr());
        }
    }

    @Test
    void removingWzFileUsesCloseLifecycle() {
        WzFile file = WzFile.create((short)95, MapleVersion.GMS);
        WzDirectory root = file.getWzDirectory();
        WzImage image = root.addImage("Item.img");
        WzIntProperty property = WzPropertyFactory.createInt("id", 123);
        image.addProperty(property);
        WzImage imageAlias = root.getImageByName("Item.img");
        WzImageProperty propertyAlias = image.getFromPath("id");
        assertNotNull(root);
        assertNotNull(imageAlias);
        assertNotNull(propertyAlias);

        file.remove();
        assertEquals(0, file.nativePtr());
        assertEquals(0, root.nativePtr());
        assertEquals(0, image.nativePtr());
        assertEquals(0, imageAlias.nativePtr());
        assertEquals(0, property.nativePtr());
        assertEquals(0, propertyAlias.nativePtr());
        file.close();
        assertEquals(0, file.nativePtr());
    }

    @Test
    void borrowedWzFileRemoveThrowsWithoutInvalidatingOwner() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzFile borrowed = file.getWzDirectory().getWzFileParent();
            assertNotNull(borrowed);

            assertThrows(UnsupportedOperationException.class, borrowed::remove);
            assertEquals(file.nativePtr(), borrowed.nativePtr());
            assertNotNull(file.getWzDirectory());
        }
    }

    @Test
    void clearPropertiesInvalidatesDirectChildAliases() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzImage image = file.getWzDirectory().addImage("Item.img");
            image.addProperty(WzPropertyFactory.createInt("id", 123));
            WzSubProperty info = WzPropertyFactory.createSub("info");
            WzStringProperty title =
                WzPropertyFactory.createString("title", "nested");
            info.addProperty(title);
            image.addProperty(info);
            WzImageProperty imageChildAlias = image.getFromPath("id");
            WzImageProperty infoAlias = image.getFromPath("info");
            WzImageProperty titleAlias = info.getFromPath("title");
            assertNotNull(imageChildAlias);
            assertNotNull(infoAlias);
            assertNotNull(titleAlias);

            image.clearProperties();
            assertEquals(0, imageChildAlias.nativePtr());
            assertEquals(0, info.nativePtr());
            assertEquals(0, infoAlias.nativePtr());
            assertEquals(0, title.nativePtr());
            assertEquals(0, titleAlias.nativePtr());

            WzSubProperty sub = WzPropertyFactory.createSub("info");
            WzStringProperty name = WzPropertyFactory.createString("name", "old");
            sub.addProperty(name);
            image.addProperty(sub);
            WzImageProperty subAlias = image.getFromPath("info");
            WzImageProperty nestedAlias = sub.getFromPath("name");
            assertNotNull(subAlias);
            assertNotNull(nestedAlias);

            sub.clearProperties();
            assertEquals(0, nestedAlias.nativePtr());
            assertEquals(sub.nativePtr(), subAlias.nativePtr());
        }
    }

    @Test
    void removingPropertyInvalidatesDescendantAliases() {
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzImage image = file.getWzDirectory().addImage("Item.img");
            WzSubProperty sub = WzPropertyFactory.createSub("info");
            WzIntProperty id = WzPropertyFactory.createInt("id", 123);
            sub.addProperty(id);
            image.addProperty(sub);
            WzImageProperty subAlias = image.getFromPath("info");
            WzImageProperty idAlias = sub.getFromPath("id");
            assertNotNull(subAlias);
            assertNotNull(idAlias);

            image.removeProperty(subAlias);
            assertEquals(0, sub.nativePtr());
            assertEquals(0, subAlias.nativePtr());
            assertEquals(0, id.nativePtr());
            assertEquals(0, idAlias.nativePtr());
        }
    }

    @Test
    void closeWzFileInvalidatesFetchedTreeWrappers() {
        WzFile file = WzFile.create((short)95, MapleVersion.GMS);
        WzDirectory root = file.getWzDirectory();
        WzImage image = root.addImage("Item.img");
        WzSubProperty sub = WzPropertyFactory.createSub("info");
        WzIntProperty id = WzPropertyFactory.createInt("id", 123);
        sub.addProperty(id);
        image.addProperty(sub);
        WzImage imageAlias = root.getImageByName("Item.img");
        WzImageProperty subAlias = image.getFromPath("info");
        WzImageProperty idAlias = sub.getFromPath("id");

        file.close();
        assertEquals(0, file.nativePtr());
        assertEquals(0, root.nativePtr());
        assertEquals(0, image.nativePtr());
        assertEquals(0, imageAlias.nativePtr());
        assertEquals(0, sub.nativePtr());
        assertEquals(0, subAlias.nativePtr());
        assertEquals(0, id.nativePtr());
        assertEquals(0, idAlias.nativePtr());
    }

    @Test
    void rawPropertiesAreTraversedAsChildContainers() {
        assertEquals(true, WzImageProperty.isPropertyContainerType(
            WzEnums.PropertyType.RAW.value));
    }

    @Test
    void savesInMemoryFileToDisk() throws Exception {
        Path temp = Files.createTempFile("libwz-editing-", ".wz");
        Files.deleteIfExists(temp);
        try (WzFile file = WzFile.create((short)95, MapleVersion.GMS)) {
            WzImage image = file.getWzDirectory().addImage("Item.img");
            image.addProperty(WzPropertyFactory.createInt("id", 123));

            file.saveToDisk(temp.toString());
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
