#pragma once
// ============================================================
//  AKASH-DARPAN  —  math_utils.h
//  Shared mathematics: matrix ops, statistics, coordinate
//  transforms, and Zernike index utilities.
//  Header-only — no separate .cpp needed.
// ============================================================
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <cstring>

namespace AkashMath {

constexpr double PI     = 3.14159265358979323846;
constexpr double TWO_PI = 2.0 * PI;

// ═══════════════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════════════

inline double mean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

inline double variance(const std::vector<double>& v) {
    if (v.size() < 2) return 0.0;
    double m = mean(v);
    double s = 0;
    for (double x : v) s += (x-m)*(x-m);
    return s / (v.size() - 1);
}

inline double stddev(const std::vector<double>& v) {
    return std::sqrt(variance(v));
}

inline double median(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return (n % 2) ? v[n/2] : 0.5*(v[n/2-1]+v[n/2]);
}

// Root-mean-square
inline double rms(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    double s = 0;
    for (double x : v) s += x*x;
    return std::sqrt(s / v.size());
}

// Peak-to-valley
inline double ptv(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    auto mm = std::minmax_element(v.begin(), v.end());
    return *mm.second - *mm.first;
}

// ═══════════════════════════════════════════════════════════
//  COORDINATE TRANSFORMS
// ═══════════════════════════════════════════════════════════

// Pixel → normalised pupil coords  [-1, 1]
inline void pixelToPupil(int px, int py, int W, int H,
                          double& xn, double& yn) {
    xn = (px + 0.5) / W * 2.0 - 1.0;
    yn = (py + 0.5) / H * 2.0 - 1.0;
}

// Normalised → polar
inline void cartToPolar(double x, double y, double& r, double& theta) {
    r     = std::sqrt(x*x + y*y);
    theta = std::atan2(y, x);
}

// ═══════════════════════════════════════════════════════════
//  ZERNIKE INDEX UTILITIES  (OSA/ANSI)
// ═══════════════════════════════════════════════════════════

// j → (n, m)
inline void zernikeNM(int j, int& n, int& m) {
    n = 0;
    int idx = j;
    while (idx > n) { ++n; idx -= n; }
    m = -n + 2*idx;
}

// (n, m) → j
inline int zernikeJ(int n, int m) {
    return n*(n+2)/2 + (m+n)/2;  // simplified for even (n+m)
}

// Human-readable name for first 22 modes
inline const char* zernikeName(int j) {
    static const char* names[] = {
        "Piston",
        "Tilt X",       "Tilt Y",
        "Defocus",
        "Astig 0°",     "Astig 45°",
        "Coma X",       "Coma Y",
        "Trefoil X",    "Trefoil Y",
        "Spherical",
        "2° Astig X",   "2° Astig Y",
        "Tetrafoil X",  "Tetrafoil Y",
        "2° Coma X",    "2° Coma Y",
        "2° Trefoil X", "2° Trefoil Y",
        "Pentafoil X",  "Pentafoil Y",
        "2° Spherical"
    };
    if (j >= 0 && j < 22) return names[j];
    return "Higher order";
}

// ═══════════════════════════════════════════════════════════
//  MATRIX UTILITIES  (row-major, size = rows*cols)
// ═══════════════════════════════════════════════════════════

// C = A * B    A:[r×k]  B:[k×c]  C:[r×c]
inline void matmul(const std::vector<double>& A, int rA, int kA,
                   const std::vector<double>& B, int /*rB*/, int cB,
                   std::vector<double>& C) {
    C.assign(rA * cB, 0.0);
    for (int i = 0; i < rA; ++i)
        for (int j = 0; j < cB; ++j)
            for (int k = 0; k < kA; ++k)
                C[i*cB+j] += A[i*kA+k] * B[k*cB+j];
}

// AT = transpose of A  [r×c] → [c×r]
inline void transpose(const std::vector<double>& A, int r, int c,
                       std::vector<double>& AT) {
    AT.resize(c * r);
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            AT[j*r+i] = A[i*c+j];
}

// Frobenius norm
inline double frobeniusNorm(const std::vector<double>& A) {
    double s = 0;
    for (double v : A) s += v*v;
    return std::sqrt(s);
}

// Scale vector in-place
inline void scale(std::vector<double>& v, double s) {
    for (double& x : v) x *= s;
}

// ═══════════════════════════════════════════════════════════
//  SIGNAL PROCESSING
// ═══════════════════════════════════════════════════════════

// Simple moving average (causal)
inline void movingAverage(const std::vector<float>& in,
                            std::vector<float>& out, int window) {
    out.resize(in.size(), 0.f);
    for (int i = 0; i < (int)in.size(); ++i) {
        int lo = std::max(0, i - window/2);
        int hi = std::min((int)in.size()-1, i + window/2);
        float s = 0;
        for (int k = lo; k <= hi; ++k) s += in[k];
        out[i] = s / (hi - lo + 1);
    }
}

// Hann window for spectral analysis
inline std::vector<double> hannWindow(int N) {
    std::vector<double> w(N);
    for (int i = 0; i < N; ++i)
        w[i] = 0.5 * (1.0 - std::cos(TWO_PI * i / (N - 1)));
    return w;
}

// ═══════════════════════════════════════════════════════════
//  ATMOSPHERE UTILITIES
// ═══════════════════════════════════════════════════════════

// Convert r0 to seeing angle (arcseconds) at wavelength lambda (m)
inline double r0ToSeeing(double r0, double lambda) {
    constexpr double RAD_TO_ARCSEC = 180.0 * 3600.0 / PI;
    return 0.976 * lambda / r0 * RAD_TO_ARCSEC;
}

// Noll residual variance after J modes corrected (Noll 1976 Table 1)
// Returns coefficient c_J such that sigma^2_J = c_J * (D/r0)^(5/3)
inline double nollResidual(int J) {
    // Approximation valid for J >> 1: c_J ≈ 0.2944 J^(-sqrt(3)/2)
    if (J <= 0) return 1.0299;
    return 0.2944 * std::pow((double)J, -std::sqrt(3.0)/2.0);
}

// Greenwood frequency (temporal bandwidth requirement)
// f_G = 0.427 * v_wind / r0
inline double greenwoodFrequency(double r0, double vWind) {
    return 0.427 * vWind / r0;
}

// Strehl ratio from Maréchal approximation
// S ≈ exp(-sigma_phi^2)   (valid for sigma < 1 rad)
inline double marechalStrehl(double phaseVarianceRad2) {
    return std::exp(-phaseVarianceRad2);
}

// Extended Maréchal (Born & Wolf)
inline double extendedStrehl(double phaseVarianceRad2) {
    double S = 1.0 / (1.0 + phaseVarianceRad2 +
                      phaseVarianceRad2*phaseVarianceRad2/2.0);
    return std::max(0.0, S);
}

} // namespace AkashMath
