#pragma once
#include "akash_types.h"

// ── Deformable Mirror control ─────────────────────────────
class DMController {
public:
    DMController() = default;

    // Initialise actuator grid and influence matrix
    void initialise(const SystemConfig& cfg);

    // Compute actuator commands from reconstructed wavefront
    // incorporates inter-actuator coupling (Fried geometry)
    void computeCommands(const WFSFrame& frame,
                          DMState& dm,
                          const SystemConfig& cfg);

    // Apply coupling correction (Fried-geometry nearest-neighbour)
    void applyCoupling(DMState& dm) const;

    // Load custom influence matrix from file
    bool loadInfluenceMatrix(const std::string& path, DMState& dm);

    // Build synthetic Gaussian influence matrix
    void buildGaussianInfluence(DMState& dm,
                                 double couplingCoeff);

private:
    // Least-squares solve: A * x = b  (overdetermined, minimal stroke)
    void leastSquaresSolve(const std::vector<double>& A,
                            const std::vector<double>& b,
                            std::vector<double>& x,
                            int rows, int cols) const;

    std::vector<double> Cpinv_;   // pseudo-inverse of influence matrix
    bool ready_ = false;
};
