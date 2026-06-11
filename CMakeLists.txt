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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_CAPI "Build C API layer" ON)
option(BUILD_JNI "Build JNI bindings" OFF)

include(FetchContent)

# ---- tiny-AES-c (AES-256-ECB) ----
FetchContent_Declare(
    tiny-aes
    GIT_REPOSITORY https://github.com/kokke/tiny-AES-c.git
    GIT_TAG 23856752fbd139da0b8ca6e471a13d5bcc99a08d
)
FetchContent_MakeAvailable(tiny-aes)
target_compile_definitions(tiny-aes PUBLIC AES256=1)

# ---- zlib (static) ----
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3.1
)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(zlib)

set(WZ_SOURCES
    src/WzEnums.cpp
    src/WzAESConstant.cpp
    src/Util/WzMutableKey.cpp
    src/Util/WzKeyGenerator.cpp
    src/Util/WzBinaryReader.cpp
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

add_library(wz STATIC ${WZ_SOURCES} ${WZ_HEADERS})
target_include_directories(wz PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(wz PUBLIC tiny-aes zlibstatic)
set_target_properties(wz PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(wz PUBLIC "/EHs-c-" "/GR-")
    target_compile_definitions(wz PUBLIC "_HAS_EXCEPTIONS=0")
    target_compile_options(wz PRIVATE /analyze)
else()
    target_compile_options(wz PUBLIC "-fno-exceptions" "-fno-rtti")
endif()


add_executable(playground tests/main.cpp)
target_link_libraries(playground PRIVATE wz)

if(BUILD_CAPI)
    add_library(wz_capi STATIC src/capi/wz_api.cpp)
    target_include_directories(wz_capi PUBLIC include/wz/capi)
    target_link_libraries(wz_capi PUBLIC wz)
    set_target_properties(wz_capi PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(wz_test
        tests/TestUnitTest1.cpp
        tests/TestMapleLib.cpp
        tests/TestPortalType.cpp
        tests/TestPngUtility.cpp
    )
    target_include_directories(wz_test PRIVATE include)
    target_link_libraries(wz_test PRIVATE wz GTest::gtest GTest::gtest_main)
    add_test(NAME wz_test COMMAND wz_test)
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
