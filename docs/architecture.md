# Vessel Architecture

Vessel is composed of several components that work together to create self-contained Linux application bundles for both native C++ and Java/Gradle projects.

## Core Components

### 1. `vsl` CLI
The primary interface for developers. It handles project initialization (`init`), automated packaging (`pack`), and installed-bundle lifecycle operations (`list`, `update`, `remove`, `import`).

### 2. Packer Layer (`Packer`, `CMakePacker`, `GradlePacker`)
The CLI delegates build and payload assembly to mode-specific packers:
- `mode: cpp` -> `CMakePacker`
- `mode: gradle` -> `GradlePacker`

This keeps build logic isolated by ecosystem while sharing a common bundle output format.

### 3. Vessel Stub (`vsl-stub`)
A small binary that is prepended to the final bundle. It contains the logic to:
- Locate the internal payload marker (`[VSL_ARCHIVE]`).
- **Perform Smart Extraction**: Checks if the application is already cached in `/tmp/vessel_cache_...` and only re-extracts if the bundle is newer (comparing `mtime`), significantly reducing startup time for large apps.
- Perform first-run OS integrations (desktop files, registry).
- Launch the application with mode-appropriate environment setup:
  - C++: sets `LD_LIBRARY_PATH` for bundled libraries.
  - Java: can configure `JAVA_HOME` and run the bundled JAR via `VesselRun`.

### 4. Payload
A `.tar.gz` archive containing the entire application filesystem:
- `bin/`: C++ executable or Java JAR + generated `VesselRun` launcher.
- `lib/`: Bundled native dependencies (primarily for C++).
- `res/`: Bundled resources (and optional icon).
- `vessel.json`: The manifest for the runtime environment.

## The Packaging Process (`vsl pack`)

1. **Parse Manifest**: Reads `vessel.json` and selects the packer by `mode`.
2. **Build**:
   - C++ mode: runs the CMake-oriented build command.
   - Gradle mode: runs the configured Gradle command.
3. **Assemble Payload**:
   - Copies main artifact (`bin_file`) into `bin/`.
   - Includes custom files/resources.
   - C++ mode also crawls `ldd` dependencies and copies `.so` libraries into `lib/`.
4. **Relocate (C++ mode)**: Uses `patchelf` to set `$ORIGIN/../lib` RPATH on bundled binaries.
5. **Generate Launcher**: Writes `bin/VesselRun` to normalize runtime execution.
6. **Concatenate**: Combines `vsl-stub` + `[VSL_ARCHIVE]` marker + payload archive into the final `.vsl`.

## Integration Philosophy
Vessel believes that "portable" apps should still feel like native desktop citizens. By automatically registering the app in the global registry and creating desktop entries, Vessel bridges the gap between static binaries and full package managers.
