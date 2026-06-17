{
  "targets": [
    {
      "target_name": "tiny_aes",
      "type": "static_library",
      "sources": [
        "deps/tiny-aes/aes.c"
      ],
      "include_dirs": [
        "deps/tiny-aes"
      ],
      "defines": [
        "AES256=1"
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "AdditionalOptions": [
            "/utf-8"
          ]
        }
      }
    },
    {
      "target_name": "zlibstatic",
      "type": "static_library",
      "sources": [
        "deps/zlib/adler32.c",
        "deps/zlib/compress.c",
        "deps/zlib/crc32.c",
        "deps/zlib/deflate.c",
        "deps/zlib/gzclose.c",
        "deps/zlib/gzlib.c",
        "deps/zlib/gzread.c",
        "deps/zlib/gzwrite.c",
        "deps/zlib/infback.c",
        "deps/zlib/inffast.c",
        "deps/zlib/inflate.c",
        "deps/zlib/inftrees.c",
        "deps/zlib/trees.c",
        "deps/zlib/uncompr.c",
        "deps/zlib/zutil.c"
      ],
      "include_dirs": [
        "deps/zlib"
      ],
      "conditions": [
        [
          "OS=='win'",
          {
            "defines": [
              "_CRT_SECURE_NO_DEPRECATE",
              "_CRT_NONSTDC_NO_DEPRECATE"
            ]
          }
        ],
        [
          "OS!='win'",
          {
            "defines": [
              "HAVE_UNISTD_H"
            ]
          }
        ]
      ]
    },
    {
      "target_name": "libwz",
      "sources": [
        "src/node/binding.cpp",
        "src/capi/wz_api.cpp",
        "src/WzEnums.cpp",
        "src/WzAESConstant.cpp",
        "src/Util/WzMutableKey.cpp",
        "src/Util/WzKeyGenerator.cpp",
        "src/Util/WzBinaryReader.cpp",
        "src/Util/WzTool.cpp",
        "src/Util/ListFileParser.cpp",
        "src/PngUtility.cpp",
        "src/Properties/WzNullProperty.cpp",
        "src/Properties/WzShortProperty.cpp",
        "src/Properties/WzIntProperty.cpp",
        "src/Properties/WzLongProperty.cpp",
        "src/Properties/WzFloatProperty.cpp",
        "src/Properties/WzDoubleProperty.cpp",
        "src/Properties/WzStringProperty.cpp",
        "src/Properties/WzSubProperty.cpp",
        "src/Properties/WzCanvasProperty.cpp",
        "src/Properties/WzPngProperty.cpp",
        "src/Properties/WzVectorProperty.cpp",
        "src/Properties/WzConvexProperty.cpp",
        "src/Properties/WzBinaryProperty.cpp",
        "src/Properties/WzUOLProperty.cpp",
        "src/Properties/WzLuaProperty.cpp",
        "src/Properties/WzRawDataProperty.cpp",
        "src/Properties/WzVideoProperty.cpp",
        "src/WzFile.cpp",
        "src/WzDirectory.cpp",
        "src/WzImage.cpp",
        "src/WzImageProperty.cpp",
        "src/WzObject.cpp",
        "src/WzPropertyCollection.cpp",
        "src/WzFileManager.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "include",
        "include/wz/capi",
        "deps/tiny-aes",
        "deps/zlib"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")",
        "tiny_aes",
        "zlibstatic"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "AES256=1",
        "_HAS_EXCEPTIONS=0"
      ],
      "cflags_cc!": [
        "-std=c++20"
      ],
      "cflags_cc": [
        "-std=c++23",
        "-fno-exceptions",
        "-fno-rtti"
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 0,
          "RuntimeTypeInfo": "false",
          "AdditionalOptions!": [
            "-std:c++20",
            "/std:c++20"
          ],
          "AdditionalOptions": [
            "/std:c++latest",
            "/utf-8",
            "/EHs-c-",
            "/GR-"
          ]
        }
      }
    }
  ]
}
