# AKASH-DARPAN — Technical Reference

**Adaptive Knowledge of Atmospheric Shack-Hartmann — Dynamic Adaptive Real-time Processing and Reconstruction Node**

*Version 1.0  |  ISRO Adaptive Optics Research Platform*

---

## 1. Overview

AKASH-DARPAN is a high-performance C++17 real-time adaptive optics (AO) mission
control system for Shack-Hartmann wavefront sensor (SH-WFS) data. It reconstructs
atmospheric wavefront distortions, derives turbulence statistics, computes deformable
mirror (DM) actuator commands, and visualises the full AO correction loop in a
professional aerospace-style dashboard — all within a sub-5 ms processing budget.

```
┌─────────────────────────────────────────────────────────────┐
│              AKASH-DARPAN  PIPELINE                         │
│                                                             │
│  BMP Frame → Centroid Engine → Wavefront Reconstructor      │
│       ↓             ↓                    ↓                  │
│  GL Texture   Slope Vectors       Zernike Coefficients      │
│                                          ↓                  │
│                              DM Controller (Fried Geometry) │
│                                          ↓                  │
│                              Actuator Commands → DM         │
│       ↓                                                     │
│  Turbulence Analyser → r₀, τ₀, Seeing                      │
│       ↓                                                     │
│  Mission Control Dashboard (Dear ImGui + ImPlot)            │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Physics Background

### 2.1  Shack-Hartmann Wavefront Sensor

An SH-WFS divides the incoming beam with a Microlens Array (MLA). Each lenslet
forms a spot on the detector whose position encodes the local wavefront tilt.
The slope vector for sub-aperture *(i,j)* is:

```
sx = (cx - cx_ref) / f_eff
sy = (cy - cy_ref) / f_eff
```

where *f_eff* is the effective focal length and *(cx_ref, cy_ref)* is the
flat-field reference centroid.

### 2.2  Zernike Modal Reconstruction

Wavefront slopes are related to Zernike mode amplitudes **a** by the
*interaction matrix* **D**:

```
s = D · a
```

The pseudo-inverse **D†** (computed via normal equations) gives:

```
â = D† · s
```

The reconstructed phase is then:

```
W(r,θ) = Σⱼ aⱼ Zⱼ(r,θ)
```

AKASH-DARPAN uses OSA/ANSI Zernike indexing. Mode 0 = Piston (excluded from
atmospheric estimates), Modes 1–2 = Tip/Tilt, Mode 3 = Defocus, etc.

### 2.3  Fried Parameter (r₀)

Using Noll (1976), the total phase variance from Kolmogorov turbulence is:

```
σ²_φ = 1.0299 (D/r₀)^(5/3)   [rad²]
```

Inverting:

```
r₀ = D · (1.0299 / σ²_φ)^(3/5)
```

Typical values: r₀ = 5–20 cm (visible wavelengths, good sites).

### 2.4  Coherence Time (τ₀)

Taylor's frozen-flow hypothesis gives:

```
τ₀ = 0.314 · r₀ / v_wind
```

where *v_wind* is the effective wind speed. AKASH-DARPAN estimates *v_wind*
from the temporal structure function of the SH-WFS slope variance.

### 2.5  Fried Geometry

In Fried geometry the DM actuator grid is interlaced with the MLA lenslet grid:
actuators sit at the corners of each lenslet square. For a 10×10 MLA this gives
an 11×11 = 121 actuator array (AKASH-DARPAN defaults to a 9×9 interior subset).

### 2.6  Inter-actuator Coupling

The deformable mirror influence function is modelled as a Gaussian:

```
IF(r) = exp(-r² / 2σ²)
```

where σ is derived from the coupling coefficient *c*:

```
c = exp(-d² / 2σ²)  →  σ = d / sqrt(-2 ln c)
```

d = inter-actuator pitch (normalised to 1). Default coupling: c = 0.15.

---

## 3. Software Architecture

```
AKASH-DARPAN/
├── CMakeLists.txt              ← Master build file
├── setup_and_build.sh          ← Linux/macOS one-shot setup
├── build_windows.bat           ← Windows MSVC build
│
├── include/                    ← Public headers
│   ├── akash_types.h           ← Core structs: WFSFrame, DMState, …
│   ├── wavefront_sensor.h      ← High-level WFS coordinator
│   ├── centroid_engine.h       ← CoG / iterative centroiding
│   ├── wavefront_reconstructor.h ← Zernike modal reconstructor
│   ├── dm_controller.h         ← DM command computation
│   ├── turbulence_analyzer.h   ← r₀, τ₀ estimation
│   ├── image_loader.h          ← BMP/PNG loader + synthetic frames
│   ├── renderer.h              ← Mission control dashboard
│   ├── playback_controller.h   ← Time-series playback
│   └── telemetry.h             ← Performance ring-buffers + CSV
│
├── src/                        ← Implementation files
│   ├── main.cpp                ← Mission loop
│   ├── wavefront_sensor.cpp    ← WFS coordinator
│   ├── centroid_engine.cpp     ← Centroiding algorithms
│   ├── wavefront_reconstructor.cpp ← Zernike + pseudo-inverse
│   ├── dm_controller.cpp       ← Influence matrix + commands
│   ├── turbulence_analyzer.cpp ← Atmospheric characterisation
│   ├── image_loader.cpp        ← STB image + synthetic generator
│   ├── renderer.cpp            ← Dear ImGui + ImPlot panels
│   ├── playback_controller.cpp ← File sequence player
│   └── telemetry.cpp           ← Timing + CSV logging
│
├── shaders/
│   ├── wavefront.vert/.frag    ← 3-D phase surface (Phong + jet)
│   ├── spot.vert/.frag         ← Science-camera display
│   └── dm_actuator.vert/.frag  ← Instanced actuator circles
│
├── third_party/                ← External libraries (auto-fetched)
│   ├── imgui/                  ← Dear ImGui v1.90
│   ├── implot/                 ← ImPlot (scrolling graphs)
│   ├── glad/                   ← OpenGL 3.3 Core loader
│   └── stb/                    ← stb_image.h
│
├── tools/
│   └── gen_synthetic_data.cpp  ← Standalone BMP sequence generator
│
└── docs/
    └── TECHNICAL_REFERENCE.md  ← This file
```

---

## 4. Build Instructions

### 4.1  Linux / macOS (Recommended)

```bash
git clone <your-repo> AKASH-DARPAN
cd AKASH-DARPAN
chmod +x setup_and_build.sh
./setup_and_build.sh
```

The script installs GLFW3, downloads ImGui/ImPlot/stb_image, generates GLAD,
configures CMake in Release mode, and compiles with `-O3 -march=native`.

### 4.2  Manual CMake Build

```bash
# 1. Install system dependencies
sudo apt install cmake build-essential libglfw3-dev libgl-dev libomp-dev

# 2. Generate GLAD (OpenGL 3.3 Core)
pip install glad2
glad --profile core --api gl:3.3 --generator c --out-path third_party/glad

# 3. Download ImGui v1.90.5
wget https://github.com/ocornut/imgui/archive/refs/tags/v1.90.5.tar.gz
tar -xzf v1.90.5.tar.gz --strip-components=1 -C third_party/imgui

# 4. Download ImPlot
wget https://github.com/epezent/implot/archive/refs/heads/master.tar.gz
tar -xzf master.tar.gz --strip-components=1 -C third_party/implot

# 5. Download stb_image
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h \
     -O third_party/stb/stb_image.h

# 6. Build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
```

### 4.3  Windows (MSVC + vcpkg)

```bat
vcpkg install glfw3:x64-windows
build_windows.bat
```

---

## 5. Running the Application

### 5.1  With real SH-WFS data

```bash
./build/akash_darpan /path/to/bmp/sequence/
```

The folder should contain sorted `.bmp` (or `.png`) files captured at ~10 ms
cadence. The first frame is used as the flat-field reference.

### 5.2  Synthetic demo (no hardware required)

```bash
./build/akash_darpan        # auto-generates 500 synthetic frames
```

### 5.3  Generate your own test sequence

```bash
g++ -O2 -std=c++17 tools/gen_synthetic_data.cpp -o gen_data
./gen_data ./data 200 0.12   # 200 frames, r₀=0.12 m
./build/akash_darpan ./data
```

---

## 6. Dashboard Layout

```
┌─────────────────── MISSION CONTROL CONSOLE ──────────────────────────────┐
│ ▶PLAY  ◄ ►  ↺  Speed: 1.0x  Thresh: 30  Frame: 042/200  [Load Sequence] │
├───────────────────┬───────────────────────┬──────────────────────────────┤
│  SPOT MONITOR     │  WAVEFRONT PHASE MAP  │  DM ACTUATOR GRID            │
│  Raw SH-WFS       │  W(x,y)  Jet colormap │  Fried Geometry  9×9         │
│  + Centroid       │  + Phase colour bar   │  Coupling slider             │
│    crosshairs     │  + Pupil overlay      │  Max stroke bar              │
│  + Slope vectors  │                       ├──────────────────────────────┤
│  + Reference pts  │                       │  ZERNIKE DECOMPOSITION       │
│                   │                       │  Bar chart  15 modes         │
├───────────────────┴───────────────────────┼──────────────────────────────┤
│  ATMOSPHERIC TELEMETRY                    │  SYSTEM PERFORMANCE [HPC]    │
│  r₀ scrolling plot (green)                │  Loop budget indicator       │
│  τ₀ scrolling plot (yellow)               │  Stacked timing bar chart    │
│  RMS WFE scrolling plot (red)             │  I/O / CoG / Recon / DM      │
│  r₀ | τ₀ | σ_φ | Seeing summary          │  Processing FPS counter      │
└───────────────────────────────────────────┴──────────────────────────────┘
```

---

## 7. Key Classes and APIs

### `WavefrontSensor`

The top-level AO pipeline controller. Call order:

```cpp
WavefrontSensor wfs;
wfs.initialise(cfg);          // set up grid & matrices
wfs.calibrate(refFrame);      // flat-field calibration
wfs.processFrame(liveFrame);  // centroid → reconstruct → DM → atmos
```

### `CentroidEngine`

Supports four algorithms:
- `COG` — standard centre-of-gravity
- `WCOG` — windowed/thresholded CoG (default)
- `ITERATIVE_COG` — trimmed iterative CoG (most accurate)
- `QUAD_CELL` — fastest, least accurate

### `WavefrontReconstructor`

Modal Zernike reconstruction using normal equations:

```
D† = (DᵀD)⁻¹ Dᵀ     [pseudo-inverse, Tikhonov regularised]
â  = D† · s
W  = Σ aⱼ Zⱼ(r,θ)
```

### `DMController`

Computes DM commands from the conjugate wavefront:

```
target = -W(x,y)
C · a  = target      [Gaussian influence matrix, least-squares]
```

Inter-actuator coupling correction applied via Fried-geometry nearest-neighbour
smoothing.

### `TurbulenceAnalyzer`

Maintains a rolling history of r₀, τ₀, RMS WFE and seeing. Updates on every
processed frame. Call `getParams()` for the current estimate.

### `Telemetry`

Captures per-frame I/O, centroiding, reconstruction and DM-mapping times.
Writes CSV to `akash_darpan_telemetry.csv`. Call `printSummary()` on exit.

---

## 8. Performance Targets

| Stage          | Target  | Typical |
|----------------|---------|---------|
| I/O (GL upload)| < 0.5 ms| ~0.1 ms |
| Centroiding    | < 1.0 ms| ~0.3 ms |
| Reconstruction | < 2.0 ms| ~0.8 ms |
| DM mapping     | < 1.0 ms| ~0.4 ms |
| **Total loop** | **< 5 ms** | **~1.6 ms** |
| Render FPS     | ≥ 60 Hz | ~90 Hz  |

OpenMP parallelism across sub-apertures is enabled with `-fopenmp`.
For production, link Intel MKL or OpenBLAS for the matrix pseudo-inverse.

---

## 9. Configuration Parameters

All parameters live in `SystemConfig` (`include/akash_types.h`):

| Parameter           | Default | Description                        |
|---------------------|---------|------------------------------------|
| `frameWidth/Height` | 512     | Camera sensor resolution (pixels)  |
| `pixelSizeUm`       | 5.5     | Detector pixel pitch (µm)          |
| `mlaGridX/Y`        | 10×10   | MLA lenslet count per axis         |
| `lensletSizeUm`     | 300     | Physical lenslet pitch (µm)        |
| `focalLengthMm`     | 5.0     | MLA focal length (mm)              |
| `pupilDiamMm`       | 8.0     | Beam diameter at MLA (mm)          |
| `dmGridX/Y`         | 9×9     | DM actuator count per axis         |
| `dmMaxStroke`       | 5.0     | Maximum actuator stroke (µm)       |
| `dmCoupling`        | 0.15    | Inter-actuator coupling fraction   |
| `wavelengthNm`      | 532     | Science wavelength (nm)            |
| `centroidThreshold` | 30      | ADU floor for centroiding          |

---

## 10. References

1. Noll, R.J. (1976). *Zernike polynomials and atmospheric turbulence.*
   J. Opt. Soc. Am. 66(3), 207–211.

2. Fried, D.L. (1965). *Statistics of a geometric representation of wavefront
   distortion.* J. Opt. Soc. Am. 55(11), 1427–1435.

3. Roddier, F. (1999). *Adaptive Optics in Astronomy.* Cambridge University Press.

4. Hardy, J.W. (1998). *Adaptive Optics for Astronomical Telescopes.*
   Oxford University Press.

5. Southwell, W.H. (1980). *Wavefront estimation from wavefront slope measurements.*
   J. Opt. Soc. Am. 70(8), 998–1006.

---

*AKASH-DARPAN is developed for ISRO research purposes.*
*Project name meaning: Sky Mirror (Sanskrit: आकाश दर्पण)*
