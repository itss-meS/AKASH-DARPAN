# AKASH-DARPAN
### आकाश दर्पण — Sky Mirror

**Adaptive Knowledge of Atmospheric Shack-Hartmann —
Dynamic Adaptive Real-time Processing and Reconstruction Node**

> Real-time Wavefront Reconstruction & Turbulence Characterisation System  
> ISRO Adaptive Optics Research Platform

---

## What It Does

AKASH-DARPAN processes time-series frames from a **Shack-Hartmann Wavefront
Sensor (SH-WFS)** to:

| Task | Algorithm |
|------|-----------|
| Locate spot centroids | Iterative windowed Centre-of-Gravity |
| Reconstruct wavefront W(x,y) | Zernike modal reconstruction (D† = pseudo-inverse) |
| Characterise turbulence | Fried r₀ (Noll 1976), coherence time τ₀ |
| Compute DM commands | Gaussian influence matrix, Fried geometry, coupling correction |
| Visualise everything | Dear ImGui + ImPlot mission-control dashboard |

All within a **< 5 ms processing budget** per frame.

---

## Screenshots (Dashboard Layout)

```
┌──────────────────────────── MISSION CONSOLE ───────────────────────────────┐
│ ▶ PLAY  ◄ ►  ↺   Speed 1.0x   Thresh 30   Frame 042/200   [Load Sequence] │
├─────────────────┬───────────────────────┬──────────────────────────────────┤
│  RAW SPOT FEED  │   WAVEFRONT  W(x,y)   │   DM ACTUATOR GRID  9×9          │
│  + centroids    │   Jet colour map       │   Fried geometry                 │
│  + slopes       │   Phase colour bar     │   Coupling slider + glow         │
│  + reference    │   Pupil overlay        ├──────────────────────────────────┤
│                 │                        │   ZERNIKE MODES  (15 bars)       │
├─────────────────┴───────────────────────┼──────────────────────────────────┤
│  ATMOSPHERIC TELEMETRY                  │   SYSTEM PERFORMANCE [HPC]       │
│  r₀ / τ₀ / RMS scrolling plots         │   Loop budget  |  Timing chart    │
│  r₀  τ₀  σ_φ  Seeing summary box       │   I/O CoG Recon DM  |  FPS       │
└─────────────────────────────────────────┴──────────────────────────────────┘
```

---

## Quick Start

### Linux / macOS

```bash
git clone <repo> AKASH-DARPAN && cd AKASH-DARPAN
chmod +x setup_and_build.sh
./setup_and_build.sh          # installs deps, downloads libs, builds
./build/bin/akash_darpan      # runs with synthetic data
```

### With real SH-WFS frames

```bash
./build/bin/akash_darpan /path/to/bmp/frames/
```

### Generate test data

```bash
./build/bin/gen_synthetic_data ./data 200 0.12
# 200 frames, r₀ = 0.12 m (moderate turbulence)
./build/bin/akash_darpan ./data
```

### Windows

```bat
build_windows.bat
build\bin\Release\akash_darpan.exe
```

---

## Dependencies

| Library | Purpose | Auto-fetched? |
|---------|---------|--------------|
| GLFW3 | Window + input | apt/brew/vcpkg |
| OpenGL 3.3 | GPU rendering | System |
| GLAD | OpenGL loader | `glad2` pip |
| Dear ImGui v1.90 | Dashboard UI | ✅ setup script |
| ImPlot | Scrolling graphs | ✅ setup script |
| stb_image.h | BMP/PNG loader | ✅ setup script |
| OpenMP | Parallel centroiding | System |

---

## Project Structure

```
AKASH-DARPAN/
├── CMakeLists.txt              ← Master build (see §4 docs)
├── setup_and_build.sh          ← One-shot Linux/macOS setup
├── build_windows.bat           ← MSVC + vcpkg build
├── include/                    ← All headers
├── src/                        ← All implementation files
├── shaders/                    ← GLSL 3.3 shaders
├── tools/gen_synthetic_data.cpp← Standalone BMP generator
└── docs/TECHNICAL_REFERENCE.md ← Full physics & API docs
```

---

## Performance

| Stage | Budget | Typical |
|-------|--------|---------|
| Image I/O (GL upload) | 0.5 ms | 0.1 ms |
| Centroiding (10×10 MLA) | 1.0 ms | 0.3 ms |
| Zernike Reconstruction | 2.0 ms | 0.8 ms |
| DM Actuator Mapping | 1.0 ms | 0.4 ms |
| **Total loop** | **< 5 ms** | **~1.6 ms** |
| Render | — | ~90 FPS |

---

## References

- Noll 1976 — Zernike polynomials and atmospheric turbulence
- Fried 1965 — Fried parameter r₀
- Roddier 1999 — *Adaptive Optics in Astronomy*
- Hardy 1998 — *Adaptive Optics for Astronomical Telescopes*

---

*AKASH-DARPAN © 2024 — Developed for ISRO AO Research*
