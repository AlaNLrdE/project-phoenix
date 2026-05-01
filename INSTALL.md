# PROJECT PHOENIX - INSTALLATION GUIDE

## System Requirements

- C++20 compliant compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20 or higher
- GLM (header-only mathematics library)
- Make or Ninja build system

---

## Platform-Specific Installation

### Ubuntu / Debian

```bash
# Install compiler toolchain
sudo apt-get update
sudo apt-get install -y build-essential cmake

# Install GLM
sudo apt-get install -y libglm-dev

# Verify installation
dpkg -l | grep glm
```

### Fedora / Red Hat

```bash
sudo dnf install gcc-c++ cmake glm-devel
```

### macOS (with Homebrew)

```bash
brew install cmake glm

# Verify
brew list glm
```

### Windows (MSVC)

1. **Install Visual Studio 2019 or later** with C++ development tools
2. **Install CMake** from https://cmake.org/download/
3. **Install GLM** from https://github.com/g-truc/glm/releases
   - Extract to `C:\Program Files\glm\`
   - Or set `GLM_INCLUDE_DIR` environment variable

---

## Manual GLM Installation

If your package manager doesn't have GLM, or for manual control:

### Option 1: Clone from GitHub

```bash
cd /path/to/TTSP/extern
git clone https://github.com/g-truc/glm.git
cd ..
```

### Option 2: Download Release

```bash
cd /path/to/TTSP/extern
wget https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip
unzip glm-0.9.9.8.zip
# Extract folder should be: extern/glm/glm/glm.hpp
```

### Option 3: Copy to System Path

```bash
# After downloading GLM
sudo cp -r glm/glm /usr/include/

# Verify
ls /usr/include/glm/glm.hpp
```

---

## Build Configuration

### Step 1: Verify GLM Location

```bash
# Check if GLM is in standard locations
ls /usr/include/glm/glm.hpp              # Linux
ls /usr/local/include/glm/glm.hpp        # macOS
ls /opt/homebrew/include/glm/glm.hpp     # macOS (M1/M2)
ls C:\Program\ Files\glm\glm\glm.hpp     # Windows
```

### Step 2: Configure CMake

#### Automatic (Recommended)

```bash
cd /path/to/TTSP
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### Manual (If automatic fails)

```bash
# Find GLM location
find /usr -name glm.hpp 2>/dev/null

# Use it in CMake
cmake -DGLM_INCLUDE_DIR=/path/to/glm/include -DCMAKE_BUILD_TYPE=Release ..
```

---

## Build & Run

### Using build.sh script (Linux/macOS)

```bash
cd /path/to/TTSP
chmod +x build.sh

# Release build (optimized)
./build.sh release

# Debug build (with symbols)
./build.sh debug

# Run directly
./build.sh run

# Clean
./build.sh clean
```

### Manual CMake build

```bash
cd /path/to/TTSP
mkdir -p build
cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --parallel $(nproc)
# or: make -j$(nproc)

# Run
./phoenix
```

### Windows (MSVC)

```bash
cd C:\path\to\TTSP
mkdir build
cd build

# Configure for MSVC
cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --config Release

# Run
Release\phoenix.exe
```

---

## Verify Installation

### Check Compiler

```bash
g++ --version          # Linux/macOS
clang++ --version      # Alternative
cl.exe /?              # Windows MSVC
```

### Check CMake

```bash
cmake --version        # Should be 3.20+
```

### Check GLM Headers

```bash
find /usr -name "glm.hpp" -type f 2>/dev/null
# or
python3 -c "from pathlib import Path; import glob; print(glob.glob('/usr/**/glm/glm.hpp', recursive=True))"
```

### Test Compilation

```bash
cd /path/to/TTSP
python3 check_structure.py
./build.sh release

# If successful, binary should be at:
./build/phoenix
```

---

## Troubleshooting

### "GLM headers not found"

1. **Verify installation:**

   ```bash
   ls -la /usr/include/glm/glm.hpp
   # or
   ls -la /usr/local/include/glm/glm.hpp
   ```

2. **Set environment variable:**

   ```bash
   export GLM_INCLUDE_DIR=/path/to/glm/include
   cd build && cmake -DGLM_INCLUDE_DIR=$GLM_INCLUDE_DIR ..
   ```

3. **Copy to standard location:**
   ```bash
   sudo cp -r /custom/path/glm /usr/include/
   ```

### CMake configuration fails

```bash
# Clear CMake cache and reconfigure
rm -rf build/
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DGLM_INCLUDE_DIR=/explicit/path/to/glm \
      ..
```

### Compilation errors after installing GLM

```bash
# Clean build
rm -rf build/
./build.sh clean

# Rebuild from scratch
./build.sh rebuild
```

### Permission denied on build.sh

```bash
chmod +x build.sh
./build.sh release
```

---

## Supported Compilers

| Compiler   | Version | Platform    | Status       |
| ---------- | ------- | ----------- | ------------ |
| GCC        | 10.0+   | Linux       | ✅ Tested    |
| Clang      | 12.0+   | Linux/macOS | ✅ Tested    |
| MSVC       | 2019+   | Windows     | ✅ Supported |
| AppleClang | 13.0+   | macOS       | ✅ Supported |

---

## Next Steps

1. **Verify build:** `./build/phoenix`
2. **Read documentation:** See [README.md](README.md)
3. **Explore examples:** See Phase 1 examples in `src/main.cpp`
4. **Plan Phase 2:** See [ROADMAP.md](ROADMAP.md)

---

## Support

If you encounter issues:

1. Check this guide first
2. Review CMake output carefully for specific errors
3. Verify all dependencies are installed
4. Check GitHub Issues: https://github.com/g-truc/glm/issues
5. CMake documentation: https://cmake.org/cmake/help/

---

**Last Updated:** Phase 1 Complete  
**Maintainer:** Astrodynamics Engineering Team
