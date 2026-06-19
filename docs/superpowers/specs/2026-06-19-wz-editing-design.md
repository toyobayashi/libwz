# WZ Editing Support Design

## Context

`libwz` can currently parse and read MapleStory `.wz` archives through C++,
C API, JNI, and NodeAPI bindings. The C++ model already has partial edit
surface area, such as `AddProperty`, `RemoveProperty`, directory image lists,
and `WzImage::Changed`, but it does not yet have a complete MapleLib-compatible
write/repack pipeline.

The reference behavior is MapleLib in
`Harepacker-resurrected/MapleLib/MapleLib/WzLib/`. The implementation should
study the C# algorithms and reimplement the same behavior in C++ from scratch.
The most important reference files are:

- `WzFile.cs`: `SaveToDisk`, version hash/header behavior, IV selection.
- `WzDirectory.cs`: `GenerateDataFile`, `SaveDirectory`, `SaveImages`,
  `GetOffsets`, `GetImgOffsets`, directory/image add/remove behavior.
- `WzImage.cs`: `AddProperty`, `RemoveProperty`, `ClearProperties`,
  `SaveImage`, changed/parsed behavior.
- `WzImageProperty.cs` and `WzProperties/*.cs`: property list writing and each
  property type's `WriteValue`.
- `Util/WzBinaryWriter.cs`: string encryption, string cache, compressed
  integer encoding, and offset encoding.

## Goals

Add editing support for `.wz` files and nodes:

- Rename objects.
- Add/remove directories and images.
- Add/remove/clear image properties and nested property children.
- Change scalar property values.
- Save/repack a modified `.wz` file to disk.
- Expose the edit/save surface through C API, JNI, and NodeAPI.

The implementation must preserve the existing no-exceptions C++ style and use
`Result<T, Error>` for fallible core operations.

## Non-Goals

- Do not add unrelated serializer formats such as XML, JSON, NX, or img export.
- Do not redesign ownership across the entire library.
- Do not attempt to fix MapleLib behavior that appears odd; match it unless the
  behavior makes C++ memory safety impossible.
- Do not implement GUI/editor workflows.

## Core Architecture

### Writer Layer

Add a C++ `WzBinaryWriter` counterpart under `include/wz/Util/` and
`src/Util/`. It should mirror MapleLib's binary writer behavior:

- Generate and hold a `WzMutableKey` from the selected IV.
- Maintain `Hash`, `Header`, and string cache.
- Write encrypted ASCII and UTF-16 style WZ strings.
- Write string values with cached-offset forms.
- Write directory object names with cached-offset forms.
- Write compressed `int` and `long`.
- Write encoded offsets using the WZ offset constant and version hash.
- Write null-terminated header copyright text.

This writer is the foundation for image and directory save behavior.

### Object Editing

Keep the current `unique_ptr` ownership model:

- `WzFile` owns the root `WzDirectory`.
- `WzDirectory` owns child `WzDirectory` and `WzImage` objects.
- `WzImage` and property containers own `WzImageProperty` children.

Add or tighten editing methods to match MapleLib semantics where possible:

- Adding sets the child's parent.
- Removing clears the child's parent before ownership is released or destroyed.
- Adding a duplicate name should report an error instead of silently doing
  nothing.
- Edits under an image mark the owning `WzImage` as changed.
- Directory/image structural edits mark affected new images as changed and
  make the file save path recompute directory metadata.

Because C++ currently destroys removed children, C API, JNI, and NodeAPI handles
to removed objects become invalid immediately. Bindings must document and follow
that rule instead of caching removed node handles.

### Property Writing

Add virtual property writing support, implemented by each concrete property
class according to the C# `WriteValue` methods:

- Scalar properties: null, short, int, long, float, double, string.
- Containers: sub, canvas, vector, convex.
- Links/data: UOL, Lua, raw data, binary/sound, video.
- PNG/canvas data: write the existing compressed image data first, with broader
  bitmap re-encoding only if the current C++ class already has enough data to do
  so safely.

For changed images, `WzImage::SaveImage` should build the same root
`WzSubProperty` wrapper used by MapleLib and write its children as an image
property list.

### Repack Pipeline

Add `WzFile::SaveToDisk` with the MapleLib flow:

1. Select the output IV from the current or requested Maple version.
2. Determine whether to save as 32-bit-header or 64-bit-header WZ.
3. Recompute the WZ version hash and version header.
4. Generate a temporary image data stream:
   - Changed images are serialized through `WzImage::SaveImage`.
   - Unchanged images reuse original bytes from the parsed file.
   - Image checksum and block size are recalculated for rewritten images.
5. Recompute directory sizes, image offsets, and directory offsets.
6. Write the WZ header.
7. Write the directory table.
8. Append image blocks, reusing unchanged original blocks and changed temporary
   blocks.

The output path should be explicit; saving in-place can be supported by writing
to a temporary sibling and replacing only after success.

## C API

Extend `include/wz/wz_api.h` and `src/capi/wz_api.cpp` with functions grouped
around stable concepts:

- File save:
  - `wz_file_save_to_disk(file, path)`
  - Optional overload with save-as-64-bit and target Maple version.
- Object edits:
  - `wz_object_set_name(object, name)`
  - `wz_object_remove(object)`
- Directory edits:
  - `wz_dir_create_directory(dir, name, out_dir)`
  - `wz_dir_create_image(dir, name, out_image)`
  - `wz_dir_remove_directory(dir, child)`
  - `wz_dir_remove_image(dir, child)`
- Image/property container edits:
  - create scalar/container properties.
  - add/remove/clear properties.
  - set scalar values for all scalar types, not only int.

All C API functions return `wz_error_code` and set the last error on failure.
No STL types appear in public signatures.

## JNI

Update Java classes and `src/jni/wz_jni.cpp` to expose the same operations with
the existing lower-camel Java style:

- `saveToDisk(...)`
- `setName(...)`
- `remove()`
- `addDirectory(...)`, `addImage(...)`
- `addProperty(...)`, `removeProperty(...)`, `clearProperties()`
- scalar setters such as `setValue(...)`

JNI should call the C API where practical, preserving one native behavior
surface.

## NodeAPI

Update `src/node/binding.cpp`, TypeScript declarations, and package entrypoints
to wrap the new C API:

- save file.
- rename/remove nodes.
- create directories/images/properties.
- add/remove/clear children.
- set scalar values.

Node should throw JavaScript exceptions using existing `ThrowIfError`.

## Testing

Add focused Google Test coverage:

- Writer primitive tests for compressed integers, encrypted string round-trips
  where reader support exists, and offset encoding against known reference
  values.
- In-memory edit tests for add/remove/rename/parent/changed behavior.
- Round-trip repack tests using sample `.wz` files from
  `Harepacker-resurrected/UnitTest_WzFile/WzFiles/`.
- Reopen saved files and compare selected directory trees and property values.
- Binding smoke tests for C API and NodeAPI; JNI tests in the Java test tree
  when native library loading is available.

After C++ changes, run format and lint:

```bash
./format.sh
cpplint --recursive --config .cpplint src include
```

Then build and test:

```bash
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=ON
cmake --build build
ctest --test-dir build
```

## Risks

- Repack correctness depends on exact writer behavior, especially string cache
  offsets and encoded offsets.
- Unchanged image reuse must preserve original byte ranges exactly.
- Existing raw pointer handles in bindings can become invalid after removal.
- Full PNG re-encoding may need additional work if the current C++ PNG property
  cannot reconstruct MapleLib's compressed WZ PNG payload from edited pixels.
- Very large WZ files may expose 64-bit offset and memory pressure issues.

## Implementation Order

1. Add `WzBinaryWriter` and writer primitive tests.
2. Add property `WriteValue` support and image save tests.
3. Add directory/file repack support and round-trip tests.
4. Tighten editing methods and changed propagation.
5. Extend C API.
6. Extend JNI and Java wrappers.
7. Extend NodeAPI and TypeScript surface.
8. Run format, lint, build, and tests.
