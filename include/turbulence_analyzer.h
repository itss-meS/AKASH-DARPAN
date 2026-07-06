#pragma once
#include "akash_types.h"
#include <deque>

// ── Atmospheric turbulence characterisation ───────────────
class TurbulenceAnalyzer {
public:
    TurbulenceAnalyzer();

    // Update with latest reconstructed frame
    void update(const WFSFrame& frame,
                const SystemConfig& cfg,
                double dtMs);

    // Current estimated parameters
    AtmosParams getParams() const { return params_; }

    // Scrolling history for graphs
    const std::deque<float>& r0History()   const { return r0History_; }
    const std::deque<float>& tau0History() const { return tau0History_; }
    const std::deque<float>& rmsHistory()  const { return rmsHistory_; }
    const std::deque<float>& timeAxis()    const { return timeAxis_; }

    void reset();

private:
    // Estimate r0 from phase variance (Noll 1976)
    double estimateR0(double phaseVarianceRad2,
                       double pupilDiamM,
                       double wavelengthM) const;

    // Estimate τ0 from temporal structure function (cross-frame slope diff)
    double estimateTau0(const std::deque<double>& slopeVarHistory,
                         double dtMs) const;

    AtmosParams params_;
    std::deque<float>  r0History_, tau0History_, rmsHistory_, timeAxis_;
    std::deque<double> slopeVarHistory_;
    double elapsedMs_ = 0.0;
    static constexpr int HISTORY_LEN = 512;
};
