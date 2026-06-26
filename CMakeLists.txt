cmake_minimum_required(VERSION 3.16)
project(libwz VERSION 1.0.0 LANGUAGES CXX C)

if(MSVC)
    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "/GR-" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    add_compile_options("/utf-8")
else()
    add_compile_options("-Wall" "-Wextra")
endif()

# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
#     set(CMAKE_CXX_CLANG_TIDY
#         clang-tidy
#         -checks=clang-analyzer-*           # 开启所有内存/指针/泄漏检查
#         -warnings-as-errors=*               # 有问题直接报错
#     )
# endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_CAPI "Build C API layer" ON)
option(BUILD_JNI "Build JNI bindings" OFF)
option(BUILD_WASM "Build WebAssembly-specific targets" OFF)

if(BUILD_WASM)
    if(NOT EMSCRIPTEN)
        message(FATAL_ERROR "BUILD_WASM requires Emscripten")
    endif()
    set(BUILD_CAPI ON)
    set(BUILD_JNI OFF)
    set(EMNAPI_FIND_NODE_ADDON_API ON CACHE BOOL "" FORCE)
endif()

include(FetchContent)

if(BUILD_WASM)
    add_subdirectory(node_modules/emnapi EXCLUDE_FROM_ALL)
endif()

# ---- tiny-AES-c (AES-256-ECB) ----
add_subdirectory(deps/tiny-aes)
target_compile_definitions(tiny-aes PUBLIC AES256=1)

# ---- zlib (static) ----
add_library(zlibstatic STATIC
    deps/zlib/adler32.c
    deps/zlib/compress.c
    deps/zlib/crc32.c
    deps/zlib/deflate.c
    deps/zlib/gzclose.c
    deps/zlib/gzlib.c
    deps/zlib/gzread.c
    deps/zlib/gzwrite.c
    deps/zlib/infback.c
    deps/zlib/inffast.c
    deps/zlib/inflate.c
    deps/zlib/inftrees.c
    deps/zlib/trees.c
    deps/zlib/uncompr.c
    deps/zlib/zutil.c
)
target_include_directories(zlibstatic PUBLIC deps/zlib)
if(MSVC)
    target_compile_definitions(zlibstatic PRIVATE
        _CRT_SECURE_NO_DEPRECATE
        _CRT_NONSTDC_NO_DEPRECATE
    )
elseif(UNIX)
    target_compile_definitions(zlibstatic PRIVATE HAVE_UNISTD_H)
endif()

set(WZ_SOURCES
    src/WzEnums.cpp
    src/WzAESConstant.cpp
    src/Util/WzMutableKey.cpp
    src/Util/WzKeyGenerator.cpp
    src/Util/WzBinaryReader.cpp
    src/Util/WzDataSource.cpp
    src/Util/WzBinaryWriter.cpp
    src/Util/WzTool.cpp
    src/Util/ListFileParser.cpp
    src/PngUtility.cpp
    src/Properties/WzNullProperty.cpp
    src/Properties/WzShortProperty.cpp
    src/Properties/WzIntProperty.cpp
    src/Properties/WzLongProperty.cpp
    src/Properties/WzFloatProperty.cpp
    src/Properties/WzDoubleProperty.cpp
    src/Properties/WzStringProperty.cpp
    src/Properties/WzSubProperty.cpp
    src/Properties/WzCanvasProperty.cpp
    src/Properties/WzPngProperty.cpp
    src/Properties/WzVectorProperty.cpp
    src/Properties/WzConvexProperty.cpp
    src/Properties/WzBinaryProperty.cpp
    src/Properties/WzUOLProperty.cpp
    src/Properties/WzLuaProperty.cpp
    src/Properties/WzRawDataProperty.cpp
    src/Properties/WzVideoProperty.cpp
    src/WzFile.cpp
    src/WzDirectory.cpp
    src/WzImage.cpp
    src/WzImageProperty.cpp
    src/WzObject.cpp
    src/WzPropertyCollection.cpp
    src/WzFileManager.cpp
)

set(WZ_HEADERS
    include/wz/WzObject.h
    include/wz/WzFile.h
    include/wz/WzDirectory.h
    include/wz/WzImage.h
    include/wz/WzImageProperty.h
    include/wz/WzPropertyCollection.h
    include/wz/WzHeader.h
    include/wz/WzEnums.h
    include/wz/WzAESConstant.h
    include/wz/PngUtility.h
    include/wz/PortalType.h
    include/wz/CRC32.h
    include/wz/WzFileManager.h
    include/wz/Util/WzBinaryReader.h
    include/wz/Util/WzDataSource.h
    include/wz/Util/WzBinaryWriter.h
    include/wz/Util/WzMutableKey.h
    include/wz/Util/WzKeyGenerator.h
    include/wz/Util/WzTool.h
    include/wz/Util/ListFileParser.h
    include/wz/Properties/WzNullProperty.h
    include/wz/Properties/WzShortProperty.h
    include/wz/Properties/WzIntProperty.h
    include/wz/Properties/WzLongProperty.h
    include/wz/Properties/WzFloatProperty.h
    include/wz/Properties/WzDoubleProperty.h
    include/wz/Properties/WzStringProperty.h
    include/wz/Properties/WzSubProperty.h
    include/wz/Properties/WzCanvasProperty.h
    include/wz/Properties/WzPngProperty.h
    include/wz/Properties/WzVectorProperty.h
    include/wz/Properties/WzConvexProperty.h
    include/wz/Properties/WzBinaryProperty.h
    include/wz/Properties/WzUOLProperty.h
    include/wz/Properties/WzLuaProperty.h
    include/wz/Properties/WzRawDataProperty.h
    include/wz/Properties/WzVideoProperty.h
)

if(EMSCRIPTEN)
    list(APPEND WZ_SOURCES src/Util/WzBlobDataSource.cpp)
    list(APPEND WZ_HEADERS include/wz/Util/WzBlobDataSource.h)
endif()

add_library(wz STATIC ${WZ_SOURCES} ${WZ_HEADERS})
target_include_directories(wz PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(wz PUBLIC tiny-aes zlibstatic)
if(MSVC)
    target_compile_options(wz PUBLIC "/EHs-c-" "/GR-")
    target_compile_definitions(wz PUBLIC "_HAS_EXCEPTIONS=0")
    # target_compile_options(wz PRIVATE /analyze)
else()
    target_compile_options(wz PUBLIC "-fno-exceptions" "-fno-rtti")
endif()


add_executable(playground tests/main.cpp)
target_link_libraries(playground PRIVATE wz)

if(BUILD_CAPI)
    add_library(wz_capi STATIC src/capi/wz_api.cpp)
    target_include_directories(wz_capi PUBLIC include/wz/capi)
    target_link_libraries(wz_capi PUBLIC wz)
endif()

if(BUILD_WASM)
    # set(WZ_WASM_SIZE_OPT_FLAGS
    #     "$<$<CONFIG:Release>:-Oz>"
    #     "$<$<CONFIG:Release>:-flto>"
    # )
    # foreach(wz_wasm_opt_target wz wz_capi zlibstatic tiny-aes emnapi)
    #     if(TARGET ${wz_wasm_opt_target})
    #         target_compile_options(${wz_wasm_opt_target} PRIVATE ${WZ_WASM_SIZE_OPT_FLAGS})
    #     endif()
    # endforeach()

    set(WZ_WASM_DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist")
    set(WZ_WASM_EXPORTED_FUNCTIONS "['_malloc','_free','_napi_register_wasm_v1','_node_api_module_get_api_version_v1']")
    set(WZ_WASM_EXPORTED_RUNTIME_METHODS "['emnapiInit','HEAPU8','FS','NODEFS']")

    function(configure_wz_wasm_target target_name output_name environment exported_runtime_methods enable_noderawfs)
        target_include_directories(${target_name} PRIVATE include)
        target_compile_definitions(${target_name} PRIVATE
            BUILDING_NODE_EXTENSION
            NAPI_DISABLE_CPP_EXCEPTIONS
        )
        target_link_libraries(${target_name} PRIVATE wz_capi emnapi)
        set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME ${output_name}
            RUNTIME_OUTPUT_DIRECTORY "${WZ_WASM_DIST_DIR}"
        )
        target_link_options(${target_name} PRIVATE
            "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/node_modules/emnapi/dist/library_napi.js"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sENVIRONMENT=${environment}"
            "-sEXPORT_ES6=1"
            "-sEXPORTED_FUNCTIONS=${WZ_WASM_EXPORTED_FUNCTIONS}"
            "-sEXPORTED_RUNTIME_METHODS=${exported_runtime_methods}"
            "-sFORCE_FILESYSTEM=1"
            "-sMALLOC=emmalloc"
            "-sMODULARIZE=1"
            "-lnodefs.js"
        )
        target_link_options(${target_name} PRIVATE
            # "$<$<CONFIG:Release>:-Oz>"
            # "$<$<CONFIG:Release>:-flto>"
            "$<$<CONFIG:Release>:-sASSERTIONS=0>"
            "$<$<CONFIG:Release>:--minify=0>"
        )
        if(enable_noderawfs)
            target_link_options(${target_name} PRIVATE
                "-sNODERAWFS=1"
            )
        endif()
    endfunction()

    add_executable(wz_wasm src/node/binding.cpp)
    configure_wz_wasm_target(wz_wasm libwz "web,worker,node" "${WZ_WASM_EXPORTED_RUNTIME_METHODS}" FALSE)

    set(WZ_PATCH_WASM_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/patch-libwz.cmake")
    file(WRITE "${WZ_PATCH_WASM_SCRIPT}" [==[
file(READ "${JS_FILE}" js_contents)
string(REPLACE "\r\n" "\n" js_contents "${js_contents}")

function(wz_replace_optional needle replacement)
  string(FIND "${js_contents}" "${needle}" match_index)
  if(NOT match_index EQUAL -1)
    string(REPLACE "${needle}" "${replacement}" js_contents "${js_contents}")
    set(js_contents "${js_contents}" PARENT_SCOPE)
    set(WZ_PATCHED_NODE_MODULE_IMPORT TRUE PARENT_SCOPE)
  endif()
endfunction()

set(WZ_PATCHED_NODE_MODULE_IMPORT FALSE)
wz_replace_optional("await import('module')" "process.getBuiltinModule('module')")
wz_replace_optional("await import(\"module\")" "process.getBuiltinModule('module')")
if(NOT WZ_PATCHED_NODE_MODULE_IMPORT)
  string(FIND "${js_contents}" "process.getBuiltinModule('module')" already_patched_index)
  if(already_patched_index EQUAL -1)
    message(FATAL_ERROR "Failed to patch libwz.js: missing Node module dynamic import")
  endif()
endif()

file(WRITE "${JS_FILE}" "${js_contents}")
]==])
    add_custom_command(TARGET wz_wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DJS_FILE="${WZ_WASM_DIST_DIR}/libwz.js"
            -P "${WZ_PATCH_WASM_SCRIPT}"
    )
endif()

if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    set(WZ_TEST_SOURCES
        tests/TestUnitTest1.cpp
        tests/TestMapleLib.cpp
        tests/TestResult.cpp
        tests/TestPortalType.cpp
        tests/TestPngUtility.cpp
        tests/TestWzBinaryWriter.cpp
        tests/TestPropertyWriteValue.cpp
        tests/TestEditing.cpp
        tests/TestRepack.cpp
        tests/TestWzDataSource.cpp
    )
    if(BUILD_CAPI)
        list(APPEND WZ_TEST_SOURCES tests/TestCapiError.cpp)
    endif()
    add_executable(wz_test
        ${WZ_TEST_SOURCES}
    )
    target_include_directories(wz_test PRIVATE include)
    target_link_libraries(wz_test PRIVATE GTest::gtest_main)
    if(BUILD_CAPI)
        target_link_libraries(wz_test PRIVATE wz_capi)
    else()
        target_link_libraries(wz_test PRIVATE wz)
    endif()
    add_test(NAME wz_test COMMAND wz_test)

    if(BUILD_WASM)
        add_executable(wz_wasm_test
            tests/TestWzBlobDataSource.cpp
        )
        target_include_directories(wz_wasm_test PRIVATE include)
        target_link_libraries(wz_wasm_test PRIVATE wz GTest::gtest_main)
    endif()
endif()

if(BUILD_JNI)
    find_package(JNI REQUIRED)
    add_library(wz_jni SHARED src/jni/wz_jni.cpp)
    target_include_directories(wz_jni PRIVATE include ${JNI_INCLUDE_DIRS})
    target_link_libraries(wz_jni PRIVATE wz wz_capi ${JNI_LIBRARIES})

    message(STATUS "Host System Processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^arm" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(ARCH_DIR "aarch64")
    else()
        set(ARCH_DIR "x86_64")
    endif()

    if(APPLE)
        set(PLATFORM_DIR "darwin")
    elseif(MSVC)
        set(PLATFORM_DIR "win32")
    elseif(UNIX)
        set(PLATFORM_DIR "linux")
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()

    add_custom_command(TARGET wz_jni POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/java/src/main/resources/native/${PLATFORM_DIR}/${ARCH_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:wz_jni> "${CMAKE_CURRENT_SOURCE_DIR}/java/src/main/resources/native/${PLATFORM_DIR}/${ARCH_DIR}/$<TARGET_FILE_NAME:wz_jni>"
    )
endif()
