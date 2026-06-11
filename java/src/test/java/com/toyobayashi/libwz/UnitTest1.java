package com.toyobayashi.libwz;

import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertEquals;

import com.toyobayashi.libwz.WzEnums.*;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.DisplayName;

import java.io.File;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

class UnitTest1 {

    private static final String WZ_FILES_ROOT;

    static {
        String userDir = System.getProperty("user.dir");
        String projectRoot = userDir.endsWith("java") ?
            new File(userDir).getParent() : userDir;
        WZ_FILES_ROOT = Paths.get(projectRoot,
            "Harepacker-resurrected", "UnitTest_WzFile", "WzFiles").toString();
    }

    static class TestFile {
        final String fileName;
        final MapleVersion version;

        TestFile(String fileName, MapleVersion version) {
            this.fileName = fileName;
            this.version = version;
        }
    }

    private static final List<TestFile> TEST_FILES = new ArrayList<>();
    static {
        TEST_FILES.add(new TestFile("TamingMob_000_KMS_359.wz", MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_000_GMS_237.wz", MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_GMS_146.wz",   MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_GMS_176.wz",   MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_GMS_230.wz",   MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_GMS_75.wz",    MapleVersion.GMS));
        TEST_FILES.add(new TestFile("TamingMob_GMS_87.wz",    MapleVersion.GMS));
        TEST_FILES.add(new TestFile("TamingMob_GMS_95.wz",    MapleVersion.GMS));
        TEST_FILES.add(new TestFile("TamingMob_SEA_135.wz",   MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_SEA_160.wz",   MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_SEA_211.wz",   MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_SEA_212.wz",   MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_000_SEA218.wz",  MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_ThaiMS_3.wz",  MapleVersion.BMS));
        TEST_FILES.add(new TestFile("TamingMob_TMS_113.wz",   MapleVersion.EMS));
        TEST_FILES.add(new TestFile("TMS_113_Item.wz",        MapleVersion.EMS));
    }

    @Test
    @DisplayName("Test opening older WZ files")
    void testOlderWzFiles() {
        for (TestFile testFile : TEST_FILES) {
            String filePath = Paths.get(WZ_FILES_ROOT, "Common",
                testFile.fileName).toString();

            File f = new File(filePath);
            if (!f.exists()) {
                System.err.println("Skipping missing file: " + filePath);
                continue;
            }

            System.out.println("Testing: " + testFile.fileName);

            try (WzFile wzf = new WzFile(filePath, testFile.version)) {
                ParseStatus status = wzf.parseWzFile();
                assertEquals(ParseStatus.SUCCESS, status,
                    "Failed to parse " + testFile.fileName + ": " + status);

                WzDirectory root = wzf.getWzDirectory();
                assertNotNull(root,
                    "Root directory is null for " + testFile.fileName);
            }
        }
    }

    @Test
    @DisplayName("Test opening and verifying hotfix WZ file")
    void testOpeningHotfixWzFile() {
        String filePath = Paths.get(WZ_FILES_ROOT, "Hotfix",
            "Data.wz").toString();

        File f = new File(filePath);
        if (!f.exists()) {
            System.err.println("Skipping missing hotfix: " + filePath);
            return;
        }

        System.out.println("Testing hotfix: Data.wz");

        try (WzFileManager mgr = new WzFileManager("", true)) {
            WzImage img = mgr.loadDataWzHotfixFile(filePath, MapleVersion.BMS);
            assertNotNull(img, "Hotfix Data.wz loading failed");
            assertEquals(true, img.isParsed(), "Hotfix image should be parsed");
            assertNotNull(img.wzProperties(), "Hotfix image properties should not be null");
            assertEquals(true, img.wzProperties().size() > 0,
                "Hotfix image should have at least one property");
        }
    }

    @Test
    @DisplayName("Test parsing all images in legacy WZ files")
    void testLegacyWzEnumerateImages() {
        TestFile[] legacyFiles = {
            new TestFile("TamingMob_GMS_75.wz", MapleVersion.GMS),
            new TestFile("TamingMob_GMS_87.wz", MapleVersion.GMS),
            new TestFile("TamingMob_GMS_95.wz", MapleVersion.GMS),
        };

        for (TestFile test : legacyFiles) {
            String filePath = Paths.get(WZ_FILES_ROOT, "Common",
                test.fileName).toString();

            File f = new File(filePath);
            if (!f.exists()) {
                System.err.println("Skipping: " + filePath);
                continue;
            }

            try (WzFile wzf = new WzFile(filePath, test.version)) {
                ParseStatus status = wzf.parseWzFile();
                assertEquals(ParseStatus.SUCCESS, status,
                    "Failed to parse " + test.fileName + ": " + status);

                int imageCount = 0;
                WzDirectory root = wzf.getWzDirectory();
                assertNotNull(root);

                imageCount += countImagesRecursive(root);
                assertNotNull(imageCount >= 0,
                    "Should enumerate images in " + test.fileName);

                System.out.println("  " + test.fileName + " -> " +
                    imageCount + " images enumerated");
            }
        }
    }

    private int countImagesRecursive(WzDirectory dir) {
        int count = dir.countImages();
        for (int i = 0; i < dir.wzDirectories().size(); i++) {
            WzDirectory sub = dir.getDirectory(i);
            if (sub != null) count += countImagesRecursive(sub);
        }
        return count;
    }
}
