# Vessel Testing Guide

Vessel includes a comprehensive test suite designed to ensure the stability of native C++ bundling across multiple Linux distributions.

## 1. Local Automated Testing
You can run the primary test suite on your host machine using:
```bash
./tests/scripts/run_tests.sh
```
**What it tests:**
- Full build of the Vessel SDK.
- Packaging of the `example/` project.
- Extraction and first-run registration of the resulting `.vsl`.
- Registry integrity and cleanup with `vsl update` and `vsl remove`.

---

## 2. Multi-OS Orchestration (Docker)
Vessel is tested against multiple Linux distributions to ensure broad compatibility. You can run these tests locally if you have Docker installed:

### Ubuntu 20.04 (Primary Target)
```bash
docker build -t vessel-test-ubuntu -f tests/docker/ubuntu.Dockerfile .
docker run --rm vessel-test-ubuntu
```

### Fedora (Static Linking & Modern Toolchains)
```bash
docker build -t vessel-test-fedora -f tests/docker/fedora.Dockerfile .
docker run --rm vessel-test-fedora
```

### Arch Linux (Modern Libraries)
```bash
docker build -t vessel-test-arch -f tests/docker/arch.Dockerfile .
docker run --rm vessel-test-arch
```

---

## 3. Integration Tests in CI/CD
Vessel uses **GitHub Actions** (`.github/workflows/ci.yml`) to automatically run the full test suite on every push to `main` and on pull requests.

**The CI Pipeline:**
1. **NATIVE-TEST**: Runs the test suite on a standard Ubuntu runner.
2. **DISTRO-VALIDATION**: Spawns concurrent Docker containers for Ubuntu, Fedora, and Arch to verify binary compatibility.
3. **ARTIFACT-PACKING**: Compiles and provides the `vsl` binary as a downloadable artifact for review.

---

## 4. Manual Testing
When developing new features, it is recommended to test the "Installer" lifecycle:
1. Build Vessel: `cd build && make && sudo make install`
2. Pack an app: `vsl pack` in `example/`
3. Move the `.vsl` to a clean folder (e.g. `~/Downloads`).
4. Run `./example.vsl` and verify:
   - It deletes itself after install.
   - A shortcut appears in `~/Vessel Apps`.
   - The app appears in `vsl list`.
