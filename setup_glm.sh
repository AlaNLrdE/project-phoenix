#!/bin/bash
# Download GLM header-only library if not present

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GLM_DIR="${PROJECT_ROOT}/extern/glm"

if [ -d "$GLM_DIR/glm" ]; then
    echo "✓ GLM already downloaded at $GLM_DIR"
    exit 0
fi

echo "📥 Downloading GLM..."
mkdir -p "${PROJECT_ROOT}/extern"

# Download the latest GLM release from GitHub
cd "${PROJECT_ROOT}/extern"
curl -L https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip -o glm.zip 2>/dev/null || {
    # Fallback to wget if curl fails
    wget -q https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip -O glm.zip || {
        echo "✗ Failed to download GLM"
        exit 1
    }
}

unzip -q glm.zip
# The zip extracts to glm-X.X.X/ directory, so we need to move the glm subdirectory up
if [ -d "glm-"* ]; then
    mv glm-*/glm ./
    rm -rf glm-*
fi
rm glm.zip

echo "✓ GLM downloaded to $GLM_DIR"
ls -la "$GLM_DIR/glm/glm.hpp"
