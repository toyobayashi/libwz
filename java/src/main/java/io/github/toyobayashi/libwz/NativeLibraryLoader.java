package io.github.toyobayashi.libwz;

import java.io.*;
import java.nio.file.*;

final class NativeLibraryLoader {
    private static volatile boolean loaded = false;

    private NativeLibraryLoader() {}

    static synchronized void load() {
        if (loaded) return;
        try {
            System.loadLibrary("wz_jni");
            loaded = true;
            return;
        } catch (UnsatisfiedLinkError e) {
            // not on java.library.path, try extracting from resources
        }

        String os = System.getProperty("os.name").toLowerCase();
        String arch = System.getProperty("os.arch").toLowerCase();

        String subPath;
        String libName;
        if (os.contains("win")) {
            libName = "wz_jni.dll";
            subPath = "win32/" + (arch.contains("64") ? "x86_64" : "x86");
        } else if (os.contains("mac")) {
            libName = "libwz_jni.dylib";
            subPath = "darwin/" + (arch.contains("aarch64") || arch.contains("arm") ? "aarch64" : "x86_64");
        } else {
            libName = "libwz_jni.so";
            subPath = "linux/" + (arch.contains("aarch64") || arch.contains("arm") ? "aarch64" : "x86_64");
        }

        String resourcePath = "/native/" + subPath + "/" + libName;

        try (InputStream in = NativeLibraryLoader.class.getResourceAsStream(resourcePath)) {
            if (in == null) {
                throw new UnsatisfiedLinkError(
                    "Native library not found in JAR: " + resourcePath +
                    "\nBuild the native library with CMake (-DBUILD_JNI=ON) " +
                    "and place it in src/main/resources" + resourcePath);
            }

            Path tmpDir = Files.createTempDirectory("libwz_native");
            Path tmpFile = tmpDir.resolve(libName);
            tmpDir.toFile().deleteOnExit();
            tmpFile.toFile().deleteOnExit();

            Files.copy(in, tmpFile, StandardCopyOption.REPLACE_EXISTING);
            System.load(tmpFile.toAbsolutePath().toString());
            loaded = true;
        } catch (IOException e) {
            throw new UnsatisfiedLinkError("Failed to extract native library: " + e.getMessage());
        }
    }
}
