package io.github.toyobayashi.libwz;

import io.github.toyobayashi.libwz.WzEnums.*;

import java.io.File;

public class Playground {
    public static void main(String[] args) {
        MapleVersion version = MapleVersion.GMS;
        String HOME = System.getProperty("user.home");
        String baseDir = HOME + File.separator + "kinoko" + File.separator + "MapleStory";
        new File("tmp").mkdirs();

        String uiPath = baseDir + File.separator + "UI.wz";
        try (WzFile wzFile = new WzFile(uiPath, version)) {
            ParseStatus status = wzFile.parseWzFile();
            if (status != ParseStatus.SUCCESS) {
                System.err.println("Failed to parse .wz file: " + uiPath);
                return;
            }
            WzDirectory root = wzFile.getWzDirectory();
            if (root == null) {
                System.err.println("Root directory is null for file: UI.wz");
                return;
            }

            WzImage img = root.getImageByName("StatusBar.img");
            if (img == null) {
                System.err.println("Failed to find image 'StatusBar' in file: UI.wz");
                return;
            }

            WzImageProperty p = img.getFromPath("gauge/hpFlash/0");
            if (p instanceof WzCanvasProperty c) {
                c.saveToFile("tmp/hpFlash0.png");
            }

            for (WzImageProperty prop : img.wzProperties()) {
                System.out.println("Property: " + prop.getName()
                        + ", Type: " + prop.getPropertyType().value);
                if ("gauge".equals(prop.getName())) {
                    if (prop instanceof WzSubProperty sub) {
                        for (WzImageProperty subProp : sub.wzProperties()) {
                            System.out.println("subProp: " + subProp.getName()
                                    + ", Type: " + subProp.getPropertyType().value);
                            if ("graduation".equals(subProp.getName())) {
                                if (subProp instanceof WzCanvasProperty canvasSub
                                        && canvasSub.getPngProperty() != null) {
                                    canvasSub.getPngProperty().saveToFile("tmp/graduation.png");
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }

        String filePath = baseDir + File.separator + "Sound.wz";
        try (WzFile wzFile = new WzFile(filePath, version)) {
            ParseStatus status = wzFile.parseWzFile();
            if (status != ParseStatus.SUCCESS) {
                System.err.println("Failed to parse .wz file: " + filePath);
                return;
            }
            String p = "Bgm00.img/FloralLife";
            System.err.println("Getting object from path: " + p);
            WzObject obj = wzFile.getObjectFromPath(p, false);
            System.err.println("Object type: " + obj.getObjectType());
            if (obj instanceof WzBinaryProperty sound) {
                sound.saveToFile("tmp/FloralLife333.mp3");
            } else {
                System.err.println("Failed to find sound property at path: Bgm00.img/FloralLife");
            }

            WzImage img = wzFile.getWzDirectory().getImageByName("Bgm00.img");
            if (img == null) {
                System.err.println("Failed to find image 'Bgm00' in file: " + filePath);
                return;
            }

            WzImageProperty soundP = img.getFromPath("FloralLife");
            if (soundP instanceof WzBinaryProperty sound) {
                sound.saveToFile("tmp/FloralLife2.mp3");
            }

            for (WzImageProperty prop : img.wzProperties()) {
                System.out.println("Property: " + prop.getName()
                        + ", Type: " + prop.getPropertyType().value);
                if (prop.getPropertyType() == PropertyType.SOUND) {
                    WzBinaryProperty s = (WzBinaryProperty) prop;
                    s.saveToFile("tmp/" + prop.getName() + ".mp3");
                }
            }
        }
    }
}
