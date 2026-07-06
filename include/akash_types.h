#pragma once
// ============================================================
//  AKASH-DARPAN  —  Adaptive Optics Mission Control System
//  Developed for ISRO Wavefront Sensing & Correction Research
//  Version 1.0.0
// ============================================================

#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <cstdint>
#include <memory>
#include <chrono>
#include <functional>

// ── Physical / Optical Constants ──────────────────────────
namespace AkashConstants {
    constexpr double WAVELENGTH_SODIUM  = 589e-9;  // metres
    constexpr double WAVELENGTH_DEFAULT = 532e-9;
    constexpr double PI                 = 3.14159265358979323846;
    constexpr double TWO_PI             = 2.0 * PI;
    constexpr double RAD_TO_ARCSEC      = (180.0 * 3600.0) / PI;
    constexpr int    MAX_LENSLETS       = 256;
    constexpr int    MAX_ZERNIKE_MODES  = 36;
    constexpr int    MAX_DM_ACTUATORS   = 256;
    constexpr int    TELEMETRY_BUFFER   = 512;
}

// ── 2-D Point / Vector ────────────────────────────────────
struct Vec2 {
    double x = 0.0, y = 0.0;
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    double norm() const { return std::sqrt(x*x + y*y); }
};

// ── Sub-aperture / Lenslet cell ───────────────────────────
struct SubAperture {
    int   col = 0, row = 0;           // grid indices
    int   px = 0, py = 0;             // pixel origin in frame
    int   size = 0;                   // pixel size of cell
    bool  valid = true;               // inside pupil?
    Vec2  refCentroid{};              // reference (flat-field) centroid
    Vec2  centroid{};                 // current centroid
    Vec2  slope{};                    // normalised slope (radians / focal_len)
    float intensity = 0.f;           // total flux in cell
};

// ── Full WFS frame result ─────────────────────────────────
struct WFSFrame {
    int            frameIndex  = 0;
    double         timestampMs = 0.0;
    int            width = 0, height = 0;
    std::vector<uint8_t>    pixels;              // raw 8-bit grey
    std::vector<SubAperture> subApertures;
    std::vector<double>      zernikeCoeffs;      // size = MAX_ZERNIKE_MODES
    std::vector<double>      phaseMesh;          // reconstructed W(x,y)
    int                      meshCols = 0, meshRows = 0;
    double                   rmsWFE   = 0.0;    // radians
};

// ── DM Actuator state ─────────────────────────────────────
struct Actuator {
    int    col = 0, row = 0;
    double strokeNorm  = 0.0;   // normalised  [-1, 1]
    double strokeMicron= 0.0;   // physical µm
    float  px = 0, py = 0;      // screen position for rendering
};

struct DMState {
    int                   nActuatorsX = 0, nActuatorsY = 0;
    double                maxStrokeMicron = 5.0;
    double                couplingCoeff   = 0.15;
    std::vector<Actuator> actuators;
    std::vector<double>   influenceMatrix; // flattened [nAct x nSub*2]
    std::vector<double>   commandVector;
};

// ── Atmospheric parameters ────────────────────────────────
struct AtmosParams {
    double r0       = 0.15;  // Fried parameter (metres)
    double tau0     = 5e-3;  // Coherence time (seconds)
    double seeingAS = 0.0;   // Seeing in arcseconds
    double phaseVar = 0.0;   // Total phase variance (rad^2)
    double windSpeed= 10.0;  // Effective wind speed (m/s)
};

// ── Performance telemetry ─────────────────────────────────
struct PerfTelemetry {
    double ioTimeUs          = 0.0;
    double centroidTimeUs    = 0.0;
    double reconTimeUs       = 0.0;
    double actuatorTimeUs    = 0.0;
    double totalLoopTimeUs   = 0.0;
    double renderFps         = 0.0;
    double processingFps     = 0.0;
    bool   withinBudget      = true;   // < 5ms budget
};

// ── System configuration ──────────────────────────────────
struct SystemConfig {
    // Camera / frame
    int    frameWidth   = 512;
    int    frameHeight  = 512;
    double pixelSizeUm  = 5.5;

    // MLA
    int    mlaGridX     = 10;
    int    mlaGridY     = 10;
    double lensletSizeUm= 300.0;
    double focalLengthMm= 5.0;

    // Pupil
    double pupilDiamMm  = 8.0;

    // DM
    int    dmGridX      = 9;
    int    dmGridY      = 9;
    double dmMaxStroke  = 5.0;
    double dmCoupling   = 0.15;

    // Wavelength
    double wavelengthNm = 532.0;

    // Centroiding
    float  centroidThreshold = 30.0f;
    int    centroidWindowPx  = 5;

    // Playback
    float  playbackSpeed     = 1.0f;
    bool   autoPlay          = false;
    int    currentFrame      = 0;
    int    totalFrames       = 0;
    std::string dataPath;
};

// ── Timing helper ─────────────────────────────────────────
class HiResClock {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TP    = Clock::time_point;
    void   start() { t0_ = Clock::now(); }
    double elapsedUs() const {
        auto dt = Clock::now() - t0_;
        return std::chrono::duration<double, std::micro>(dt).count();
    }
private:
    TP t0_;
};
