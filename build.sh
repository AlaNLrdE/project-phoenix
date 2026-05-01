#!/bin/bash

# ============================================================================
# BUILD SCRIPT FOR PROJECT PHOENIX
# ============================================================================
# Usage: ./build.sh [debug|release|clean|rebuild]
# ============================================================================

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
SETUP_SCRIPT="${PROJECT_ROOT}/setup_glm.sh"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC}  $1"
    echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

# Ensure GLM is available
if [ ! -d "${PROJECT_ROOT}/extern/glm/glm" ]; then
    print_info "GLM not found. Setting up..."
    if [ -f "$SETUP_SCRIPT" ]; then
        bash "$SETUP_SCRIPT" || {
            print_error "Failed to setup GLM. Please install libglm-dev manually:"
            echo "  Ubuntu/Debian: sudo apt-get install libglm-dev"
            echo "  macOS: brew install glm"
            echo "  Or download from: https://github.com/g-truc/glm"
            exit 1
        }
    fi
fi

# Default build type
BUILD_TYPE="${1:-release}"

case "$BUILD_TYPE" in
    debug)
        print_header "Building Phoenix (DEBUG)"
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Debug ..
        cmake --build . --parallel $(nproc)
        print_success "Build complete: $BUILD_DIR/phoenix"
        ;;
    
    release)
        print_header "Building Phoenix (RELEASE)"
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release ..
        cmake --build . --parallel $(nproc)
        print_success "Build complete: $BUILD_DIR/phoenix"
        ;;
    
    clean)
        print_header "Cleaning Build Directory"
        rm -rf "$BUILD_DIR"
        print_success "Clean complete"
        ;;
    
    rebuild)
        print_header "Rebuilding Project (RELEASE)"
        rm -rf "$BUILD_DIR"
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release ..
        cmake --build . --parallel $(nproc)
        print_success "Rebuild complete: $BUILD_DIR/phoenix"
        ;;
    
    run)
        print_header "Running Phoenix"
        if [ ! -f "$BUILD_DIR/phoenix" ]; then
            print_error "Phoenix not built. Running 'build.sh release' first..."
            bash "$PROJECT_ROOT/build.sh" release
        fi
        "$BUILD_DIR/phoenix"
        ;;
    
    *)
        echo "Usage: $0 [debug|release|clean|rebuild|run]"
        echo ""
        echo "  debug     - Build with debugging symbols"
        echo "  release   - Build optimized release (default)"
        echo "  clean     - Remove build directory"
        echo "  rebuild   - Clean and build release"
        echo "  run       - Build and run (if needed)"
        exit 1
        ;;
esac

exit 0
