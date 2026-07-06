#pragma once
// ============================================================
//  AKASH-DARPAN  —  wavefront_sensor.h
//  High-level WFS coordinator: owns centroider, reconstructor,
//  DM controller and turbulence analyser
// ============================================================
#include "akash_types.h"
#include "centroid_engine.h"
#include "wavefront_reconstructor.h"
#include "dm_controller.h"
#include "turbulence_analyzer.h"

// Per-frame processing timing (microseconds)
struct WFSTiming {
    double centroidUs   = 0.0;
    double reconstructUs= 0.0;
    double dmMapUs      = 0.0;
    double totalUs() const { return centroidUs + reconstructUs + dmMapUs; }
};

class WavefrontSensor {
public:
    WavefrontSensor() = default;

    // Initialise all sub-systems from config
    void initialise(const SystemConfig& cfg);

    // Run flat-field calibration on a reference (no-turbulence) frame
    void calibrate(WFSFrame& refFrame);

    // Full pipeline: centroid → reconstruct → DM map → atmosphere
    // Returns false on first (calibration) frame
    bool processFrame(WFSFrame& frame);

    // Live configuration updates (e.g. from GUI sliders)
    void setCoupling (double c);
    void setThreshold(float  t);
    void setNModes   (int    n) { reconstructor_.nModes = n; }

    // Accessors
    const DMState&            getDMState() const;
    DMState&                  getDMState();
    const TurbulenceAnalyzer& getAtmos()   const;
    const WFSTiming&          getTiming()  const;
    bool                      isReady()    const { return ready_; }
    bool                      isCalibrated() const { return calibrated_; }
    int                       frameCount() const { return frameCount_; }

private:
    void initDMState(const SystemConfig& cfg);

    SystemConfig          cfg_;
    CentroidEngine        centroider_;
    WavefrontReconstructor reconstructor_;
    DMController          dmCtrl_;
    DMState               dm_;
    TurbulenceAnalyzer    atmosAnalyzer_;
    WFSTiming             lastTiming_;
    HiResClock            clock_;
    bool                  ready_      = false;
    bool                  calibrated_ = false;
    int                   frameCount_ = 0;
};
