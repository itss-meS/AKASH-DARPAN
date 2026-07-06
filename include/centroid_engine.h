#pragma once
#include "akash_types.h"

// ── Sub-aperture centroiding algorithms ───────────────────
enum class CentroidMethod {
    COG,           // Centre of Gravity (weighted)
    WCOG,          // Windowed / thresholded CoG
    QUAD_CELL,     // Quad-cell approximation
    ITERATIVE_COG, // Iterative trimmed CoG
};

class CentroidEngine {
public:
    CentroidEngine() = default;

    // Initialise sub-aperture grid from config
    void initialise(const SystemConfig& cfg);

    // Detect centroids in a frame; fills frame.subApertures
    void process(WFSFrame& frame, const SystemConfig& cfg);

    // Set reference positions (flat-field calibration)
    void setReference(const WFSFrame& refFrame);

    // Compute slopes from centroid deltas
    void computeSlopes(WFSFrame& frame, const SystemConfig& cfg) const;

    CentroidMethod method = CentroidMethod::WCOG;

private:
    Vec2 computeCOG(const uint8_t* pixels, int stride,
                    int ox, int oy, int size, float threshold) const;

    Vec2 computeIterativeCOG(const uint8_t* pixels, int stride,
                              int ox, int oy, int size, float threshold,
                              int maxIter = 3) const;

    std::vector<SubAperture> referenceApertures_;
    bool hasReference_ = false;
};
