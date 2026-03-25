# 🚢 Vessel
### The Native Linux Application Bundler

Vessel is a high-performance C++ toolchain designed to bundle native Linux applications into single-file, self-extracting executables (`.vsl`). It automates dependency crawling, library relocation, and OS integration, providing a "compile once, run anywhere" experience for the Linux desktop.

---

## Key Features

- **Single-File Bundles**: Pack your entire app, libraries, and resources into one executable.
- **Dependency Crawling**: Automatically detects and packs required `.so` files using `ldd`.
- **Relocation Magic**: Patches ELF RPATHs natively to ensure libraries are found in the bundle.
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

### Initialize a Project
```bash
vsl init
```

### Pack your App
```bash
vsl pack
```

---

## Command Reference

| Command | Description |
| :--- | :--- |
| `init` | Creates a `vessel.json` manifest in the current directory. |
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
  "name": "my_app",
  "version": "1.0.0",
  "entry": "bin/my_app",
  "icon": "res/icon.png",
  "build_dir": "build",
  "dist_dir": "dist",
  "includes": [
    "res",
    "lib/custom_lib.so"
  ]
}
```

---

## License
MIT License. Built with ❤️ for the Linux community.
