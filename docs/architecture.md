# Vessel Architecture

Vessel is composed of several key components that work together to create self-contained Linux application bundles.

## Core Components

### 1. `vsl` CLI
The primary interface for developers. It handles project initialization (`init`), automated building and packaging (`pack`), and managing installed bundles (`update`, `remove`).

### 2. Vessel Stub (`vsl-stub`)
A small binary that is prepended to the final bundle. It contains the logic to:
- Locate the internal payload marker (`[VSL_ARCHIVE]`).
- **Perform Smart Extraction**: Checks if the application is already cached in `/tmp/vessel_cache_...` and only re-extracts if the bundle is newer (comparing `mtime`), significantly reducing startup time for large apps.
- Perform first-run OS integrations (desktop files, registry).
- Launch the application with properly configured environment variables (like `LD_LIBRARY_PATH`).

### 3. Payload
A `.tar.gz` archive containing the entire application filesystem:
- `bin/`: Contains the executable binary specified by `bin_file`.
- `lib/`: All necessary library dependencies.
- `res/`: Bundled resources and the application icon.
- `vessel.json`: The manifest for the runtime environment.

## The Packaging Process (`vsl pack`)

1. **Build**: Orchestrates CMake to build the native binary.
2. **Crawl**: Uses `ldd` to find all library dependencies (excluding base system libraries like libc).
3. **Relocate**: Uses `patchelf` to set `$ORIGIN/../lib` as the RPATH in the binary, making it portable.
4. **Bundle**: Creates the internal directory structure and copies all assets.
5. **Concatenate**: Combines the stub, a special marker, and the payload archive into the final `.vsl` file.

## Integration Philosophy
Vessel believes that "portable" apps should still feel like native desktop citizens. By automatically registering the app in the global registry and creating desktop entries, Vessel bridges the gap between static binaries and full package managers.
