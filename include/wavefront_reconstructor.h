#pragma once
#include "akash_types.h"
#include <vector>

// ── Wavefront reconstruction (modal Zernike + zonal least-squares) ──
class WavefrontReconstructor {
public:
    WavefrontReconstructor() = default;

    // Build the interaction / reconstruction matrices
    void buildMatrices(const SystemConfig& cfg,
                       const std::vector<SubAperture>& apertures);

    // Reconstruct phase from slopes → fills frame.zernikeCoeffs & phaseMesh
    void reconstruct(WFSFrame& frame, const SystemConfig& cfg);

    // Evaluate W(x,y) on a fine mesh from Zernike coefficients
    void evaluatePhaseMesh(WFSFrame& frame,
                            int meshCols, int meshRows,
                            double pupilRadius) const;

    int nModes = 15;  // number of Zernike modes to use

private:
    // ── Zernike basis ─────────────────────────────────────
    // Returns Zernike polynomial value at (r, theta) for OSA/ANSI mode j
    static double zernikeValue(int j, double r, double theta);
    static void   zernikeNM(int j, int& n, int& m);
    static double radialPoly(int n, int m, double r);

    // ── Pseudo-inverse via SVD (simple Jacobi or in-house) ──
    void pseudoInverse(const std::vector<double>& A,
                        int rows, int cols,
                        std::vector<double>& Ainv,
                        double rcondThresh = 1e-6) const;

    // ── Interaction matrix (slopes → Zernike coefficients) ──
    std::vector<double> D_;       // [nSlopes x nModes]
    std::vector<double> Dpinv_;   // [nModes  x nSlopes]  pseudo-inverse

    // ── Zernike basis evaluated at sub-aperture centres ──
    std::vector<double> Z_;       // [nAp x nModes]

    int nApertures_ = 0;
    int nSlopes_    = 0;
    bool ready_     = false;
};
