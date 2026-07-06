#!/usr/bin/env bash
# ============================================================
#  AKASH-DARPAN  —  setup_and_build.sh
#  Downloads dependencies and builds the project
# ============================================================
set -e
REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_ROOT"

echo "╔══════════════════════════════════════════════════════╗"
echo "║  AKASH-DARPAN  —  Build & Setup Script              ║"
echo "╚══════════════════════════════════════════════════════╝"

# ── Detect OS ─────────────────────────────────────────────
OS="$(uname -s)"
if [ "$OS" = "Linux" ]; then
    echo "[Setup] Installing system dependencies (requires sudo)..."
    sudo apt-get update -qq
    sudo apt-get install -y \
        cmake build-essential libgl-dev libglfw3-dev \
        libomp-dev git wget 2>/dev/null || true
fi

# ── Create third_party dirs ───────────────────────────────
mkdir -p third_party/{imgui/backends,implot,glad/{src,include/glad,include/KHR},stb}

# ── Download Dear ImGui (v1.90) ───────────────────────────
IMGUI_DIR="third_party/imgui"
if [ ! -f "$IMGUI_DIR/imgui.h" ]; then
    echo "[Setup] Downloading Dear ImGui..."
    IMGUI_VER="v1.90.5"
    IMGUI_URL="https://github.com/ocornut/imgui/archive/refs/tags/${IMGUI_VER}.tar.gz"
    wget -q "$IMGUI_URL" -O /tmp/imgui.tar.gz
    tar -xzf /tmp/imgui.tar.gz --strip-components=1 -C "$IMGUI_DIR"
    echo "[Setup] ImGui downloaded."
fi

# ── Download ImPlot ────────────────────────────────────────
IMPLOT_DIR="third_party/implot"
if [ ! -f "$IMPLOT_DIR/implot.h" ]; then
    echo "[Setup] Downloading ImPlot..."
    IMPLOT_URL="https://github.com/epezent/implot/archive/refs/heads/master.tar.gz"
    wget -q "$IMPLOT_URL" -O /tmp/implot.tar.gz
    tar -xzf /tmp/implot.tar.gz --strip-components=1 -C "$IMPLOT_DIR"
    echo "[Setup] ImPlot downloaded."
fi

# ── Write GLAD loader (OpenGL 3.3) ────────────────────────
if [ ! -f "third_party/glad/include/glad/glad.h" ]; then
    echo "[Setup] Writing GLAD OpenGL 3.3 loader..."
    # Minimal glad for OpenGL 3.3 core — full version generated at build
    # For a real build, run: python -m glad --profile core --api gl:3.3 --generator c
    # Here we create a placeholder that instructs the user
    cat > third_party/glad/README.md << 'EOF'
# GLAD OpenGL Loader

Generate with:
  pip install glad2
  glad --profile core --api gl:3.3 --generator c --out-path third_party/glad

Or download pre-generated from https://glad.dav1d.de/
Select: OpenGL 3.3 Core, C/C++, Generate loader checked.
Extract into third_party/glad/.
EOF
    echo "[Setup] IMPORTANT: Please generate GLAD manually (see third_party/glad/README.md)"
    echo "        Run: pip install glad2 && glad --profile core --api gl:3.3 --generator c --out-path third_party/glad"
fi

# ── Download stb_image ─────────────────────────────────────
if [ ! -f "third_party/stb/stb_image.h" ]; then
    echo "[Setup] Downloading stb_image.h..."
    wget -q "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" \
         -O "third_party/stb/stb_image.h"
fi

# ── Build ─────────────────────────────────────────────────
echo "[Build] Configuring CMake..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -5
echo "[Build] Compiling (Release / -O3)..."
cmake --build . --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║  Build complete!  Run:                               ║"
echo "║    ./build/akash_darpan [path/to/bmp/sequence]       ║"
echo "╚══════════════════════════════════════════════════════╝"
