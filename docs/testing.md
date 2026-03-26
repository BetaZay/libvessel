# Vessel Testing Guide

Vessel includes automated and manual checks for both C++ (CMake) and Java (Gradle) packaging flows.

## 1. Local Automated Testing
You can run the primary test suite on your host machine using:
```bash
./tests/run.sh -t=cmake
```
**What it tests:**
- Full build of the Vessel SDK.
- Packaging and execution flow for the repository example app.
- Extraction and first-run registration of the resulting `.vsl`.
- Registry integrity and cleanup with `vsl update` and `vsl remove`.

---

## 2. Multi-OS Orchestration (Docker)
Vessel is tested against multiple Linux distributions to validate compatibility. You can run these tests locally if Docker is installed:

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

## 3. Example Project Smoke Tests

### C++ Example (`examples/cmake`)
```bash
cd examples/cmake
../../build/vsl pack
./dist/pong.vsl --install-only
```

### Java/Gradle Example (`examples/gradle`)
```bash
cd examples/gradle
../../build/vsl pack
./dist/JavaHello.vsl --install-only
```

---

## 4. Unified Test Runner
Use the root test runner to choose suite(s):

```bash
./tests/run.sh -t=cmake
./tests/run.sh -t=gradle
./tests/run.sh -t=all
```

Run inside distro Docker matrix:

```bash
./tests/run.sh --docker -t=cmake
./tests/run.sh --docker -t=gradle
./tests/run.sh --docker -t=all
```

---

## 5. Integration Tests in CI/CD
Vessel uses GitHub Actions (`.github/workflows/ci.yml`) on pushes and pull requests to `main`:

1. Native build job.
2. Docker-based multi-distro integration job.
3. Draft release job for `v*` tags.

---

## 6. Manual Testing
When developing new features, it is recommended to test the "Installer" lifecycle:
1. Build Vessel: `cd build && make && sudo make install`
2. Pack an app from `examples/cmake` or `examples/gradle`.
3. Move the `.vsl` to a clean folder (e.g. `~/Downloads`).
4. Run `./<app>.vsl` and verify:
   - It deletes itself after install.
   - A shortcut appears in `~/Vessel Apps`.
   - The app appears in `vsl list`.
