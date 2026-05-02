# PROJECT PHOENIX — GUÍA DE INSTALACIÓN

## Plataformas soportadas

- **macOS** 12+ (Apple Silicon y Intel) — plataforma principal de desarrollo
- **Linux** (Ubuntu 20.04+, Fedora 35+, Arch)

---

## Requisitos

| Dependencia | Versión mínima | Descripción                    |
|-------------|----------------|--------------------------------|
| C++ compiler | Clang 12+ / GCC 10+ | Soporte C++20 requerido   |
| CMake       | 3.20+          | Build system                   |
| GLM         | 0.9.9+         | Header-only, librería de matemáticas |

---

## macOS

### Con Homebrew (recomendado)

```bash
# Instalar Homebrew si no está disponible
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Instalar dependencias
brew install cmake glm

# Verificar
cmake --version    # debe ser 3.20+
ls /opt/homebrew/include/glm/glm.hpp   # Apple Silicon
ls /usr/local/include/glm/glm.hpp       # Intel Mac
```

### Compilar y ejecutar

```bash
cd project-phoenix

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(sysctl -n hw.ncpu)

./build/phoenix
```

---

## Linux — Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libglm-dev

# Verificar
gcc --version
cmake --version
dpkg -l libglm-dev
```

### Compilar y ejecutar

```bash
cd project-phoenix

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

./build/phoenix
```

---

## Linux — Fedora / Red Hat

```bash
sudo dnf install gcc-c++ cmake glm-devel

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
./build/phoenix
```

---

## Linux — Arch

```bash
sudo pacman -S base-devel cmake glm

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
./build/phoenix
```

---

## Instalación manual de GLM

Si el gestor de paquetes no tiene GLM o la versión es muy antigua:

```bash
# Opción 1: Homebrew (macOS)
brew install glm

# Opción 2: clonar desde GitHub
cd project-phoenix/extern
git clone --depth=1 https://github.com/g-truc/glm.git

# Opción 3: descargar release
cd project-phoenix/extern
curl -L https://github.com/g-truc/glm/releases/download/1.0.1/glm-1.0.1.zip -o glm.zip
unzip glm.zip && rm glm.zip
```

Con GLM en `extern/glm`, CMake lo detectará automáticamente (ya configurado en `CMakeLists.txt`).

---

## Tipos de build

```bash
# Release: optimizado para producción (-O3)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(sysctl -n hw.ncpu 2>/dev/null || nproc)

# Debug: con símbolos de depuración (-g -O0 -Wall -Wextra)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Limpiar artefactos
rm -rf build
```

### Usando build.sh

```bash
chmod +x build.sh

./build.sh release   # build optimizado
./build.sh debug     # build con símbolos
./build.sh run       # build Release + ejecutar
./build.sh clean     # eliminar directorio build/
```

---

## Verificación

```bash
# Compilador
clang++ --version    # macOS (preferido)
g++ --version        # Linux

# CMake
cmake --version      # debe ser >= 3.20

# GLM (macOS Apple Silicon)
ls /opt/homebrew/include/glm/glm.hpp

# GLM (macOS Intel)
ls /usr/local/include/glm/glm.hpp

# GLM (Linux)
dpkg -l libglm-dev 2>/dev/null || rpm -q glm-devel 2>/dev/null

# Binario compilado
file build/phoenix                  # debe decir "Mach-O" (macOS) o "ELF" (Linux)
./build/phoenix                     # ejecutar demo completa
```

---

## GLM: rutas de búsqueda automática

`CMakeLists.txt` busca GLM en el siguiente orden:

1. Sistema (`find_package(glm)`)
2. `/usr/include`
3. `/usr/local/include`
4. `/opt/local/include`
5. `/opt/homebrew/include` (Apple Silicon)
6. `extern/` dentro del proyecto

Para especificar una ruta personalizada:

```bash
cmake -S . -B build -DGLM_INCLUDE_DIR=/ruta/a/glm/include
```

---

## Resolución de problemas

| Error | Causa probable | Solución |
|-------|---------------|----------|
| `GLM headers not found` | GLM no instalado | `brew install glm` o `sudo apt install libglm-dev` |
| `C++20 not supported` | Compilador antiguo | Actualizar a Clang 12+ / GCC 10+ |
| `cmake: command not found` | CMake no instalado | `brew install cmake` |
| `No such file: build/phoenix` | Build no ejecutado | Ejecutar los pasos de compilación |
| `Kepler solver: NaN` | Órbita degenerada | Verificar que `a > 0` y `0 ≤ e < 1` |
