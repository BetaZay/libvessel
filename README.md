# 🚢 Vessel
### The Native Linux Application Bundler

Vessel is a high-performance Linux bundler that packages applications into single-file, self-extracting executables (`.vsl`). It supports both native C++ projects (CMake) and Java projects (Gradle), with dependency bundling, runtime setup, and desktop integration.

---

## Key Features

- **Dual Build Modes**: Package C++ apps (`mode: cpp`) and Java apps (`mode: gradle`).
- **Single-File Bundles**: Pack app binaries/JARs, libraries, and resources into one executable.
- **Dependency Crawling (C++)**: Automatically detects and packs required `.so` files using `ldd`.
- **Runtime-Aware Launching (Java)**: Supports runtime metadata and shared Java runtime setup.
- **Relocation**: Patches ELF RPATHs so bundled native libraries can be discovered at runtime.
- **OS Integration**: First-run auto-installation (Desktop entries, Icons, App menu).
- **Global Registry**: Manage and update all your Vessel apps from a central CLI.

---

## Quick Start

### Build Vessel
```bash
mkdir build && cd build
cmake ..
make
```

### Initialize a C++ Project
```bash
vsl init --mode=cpp
```

### Initialize a Java/Gradle Project
```bash
vsl init --mode=gradle
```

### Pack Your App
```bash
vsl pack
```

You can also try the included sample apps in:
- `examples/cmake`
- `examples/gradle`

---

## Command Reference

| Command | Description |
| :--- | :--- |
| `init [--mode=cpp|gradle]` | Creates a `vessel.json` manifest in the current directory. |
| `pack [--keep]` | Builds and packages your project into a `.vsl` bundle. |
| `import <file.AppImage>` | Converts a third-party AppImage into a managed Vessel bundle. |
| `list` | Lists all installed applications with their status and descriptions. |
| `update` | Syncs installed apps with their source bundles and cleans the registry. |
| `remove <name> [-y]` | Uninstalls an app and cleans up Vessel Apps shortcuts. |
| `help` | Shows the help menu with detailed usage. |

---

## Manifest (`vessel.json`)
Customize your bundle by editing the manifest:
```json
{
  "name": "my_cpp_app",
  "mode": "cpp",
  "bin_file": "my_cpp_app",
  "version": "1.0.0",
  "description": "A native C++ app bundled with Vessel",
  "icon": "res/icon.png",
  "build_dir": "build",
  "build_cmd": "cmake .. && make",
  "dist_dir": "dist",
  "includes": [
    "res",
    "lib/custom_lib.so"
  ]
}
```

Gradle example:
```json
{
  "name": "my_java_app",
  "mode": "gradle",
  "version": "1.0.0",
  "description": "A Java app bundled with Vessel",
  "build_cmd": "./gradlew :app:shadowJar",
  "bin_file": "app/build/libs/app-all.jar",
  "build_dir": ".",
  "dist_dir": "dist",
  "runtime": {
    "type": "java",
    "version": "21"
  },
  "includes": [
    "assets"
  ]
}
```

For full usage details, see:
- `docs/usage.md`
- `docs/architecture.md`
- `docs/testing.md`

---

## License
MIT License.
