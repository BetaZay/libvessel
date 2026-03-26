# Vessel Usage Guide

## Getting Started

Build Vessel:
```bash
mkdir -p build
cd build
cmake ..
make
```

Install (optional):
```bash
sudo make install
```

## Workflow By Mode

### C++ / CMake

Initialize a new manifest:
```bash
vsl init --mode=cpp
```

Package your app:
```bash
vsl pack
```

You can test this mode with the sample project in `examples/cmake`.

### Java / Gradle

Initialize a Gradle manifest:
```bash
vsl init --mode=gradle
```

Package your app:
```bash
vsl pack
```

You can test this mode with the sample project in `examples/gradle`.

### Advanced Packaging Option

- `--keep`: Preserves the intermediate folder (`<name>.vsl_struct`) for inspection/debugging.

### Distribute Your Bundle

Share the generated `.vsl` file. End users do not need `vsl` pre-installed to execute it.

## CLI Commands

### `init [--mode=cpp|gradle]`
Creates a starter `vessel.json` in the current directory.

Supported modes:
- `cpp`: Native C++/CMake workflow.
- `gradle`: Java/Gradle workflow.

### `pack [--keep]`
Builds and bundles your project into `<name>.vsl`.

### `list`
Displays all registered applications, their status (Ready/Missing), and their descriptions.

### `update`
Syncs registered apps with their source bundles. If you rebuild a `.vsl` file in your project folder, running `vsl update` will push the new version to your system-wide installation.

### `remove <name> [-y]`
Uninstalls an app and cleans up metadata. By default, it asks for confirmation. Use `-y` to skip.
This command deletes:
- The registry entry.
- The installed `.vsl` bundle in `~/.local/share/vessel/bin`.
- Desktop entries in `~/.local/share/applications` and `~/Vessel Apps`.
- Associated icons in `~/.local/share/icons`.

---

### `import <path_to.AppImage>`
Converts an existing AppImage into a managed Vessel bundle. 
- What it does: Extracts the AppImage, reads metadata (`.desktop`, `Comment`, `Icon`), and re-packages as a Vessel bundle.
- Benefit: Brings third-party AppImages into a consistent managed `vsl` registry flow.

---

## Headless & Store Integration

If you are building an App Store, Launcher, or automated setup script, you can use the following flags when executing any `.vsl` bundle:

### `--install-only`
Silently performs the full installation cycle without launching the application.
- Extracts files to `/tmp/vessel_cache_<name>`.
- Copies the bundle to `~/.local/share/vessel/bin/`.
- Registers the application in the global registry.
- Creates desktop shortcuts in `~/Vessel Apps`.
- **Exits immediately** after setup is complete.

**Usage:**
```bash
./my_app.vsl --install-only
```

### `--vsl-extract`
Dumps the internal filesystem structure of the bundle into a local directory for inspection. This is useful for debugging library dependencies or verifying assets.

**Usage:**
```bash
./my_app.vsl --vsl-extract
```

## Manifest Configuration Reference

The `vessel.json` file allows for extensive customization:

- `name`: (string) Application name used for bundle and desktop integration.
- `mode`: (string) `cpp` or `gradle` (defaults to `cpp` if omitted).
- `bin_file`: (string) C++ binary name/path or Java JAR path.
- `version`: (string) Semantic versioning of your app.
- `description`: (string) Human-readable description used by registry/UI.
- `icon`: (string) Path to the application icon. This will be used for the desktop shortcut.
- `build_dir`: (string) Build directory (default `build`; common Gradle value is `.`).
- `build_cmd`: (string) The custom command used to build your project (defaults to `cmake .. && make`).
- `dist_dir`: (string) The output directory for the final `.vsl` bundle (defaults to `.`).
- `includes`: (array of strings) A list of paths (directories or files) to include in bundle.
- `runtime`: (object, optional) Runtime metadata. Commonly used for Java:
  - `type`: `java`
  - `version`: JRE major version, such as `17` or `21`

### C++ Manifest Example
```json
{
  "name": "Fancy Pong",
  "mode": "cpp",
  "bin_file": "pong",
  "version": "1.5.0",
  "description": "Full-screen multi-word titled Pong clone.",
  "icon": "assets/icon.png",
  "build_dir": "custom_build",
  "build_cmd": "make -j8",
  "dist_dir": "bin",
  "includes": [
    "assets/audio",
    "assets/textures"
  ]
}
```

### Gradle Manifest Example
```json
{
  "name": "JavaHello",
  "mode": "gradle",
  "version": "1.0.0",
  "description": "Vessel-Bundled Java Application (Gradle)",
  "build_cmd": "./gradlew :app:shadowJar",
  "bin_file": "app/build/libs/app-all.jar",
  "build_dir": ".",
  "dist_dir": "dist",
  "runtime": {
    "type": "java",
    "version": "21"
  }
}
```
