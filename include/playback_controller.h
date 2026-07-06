#pragma once
#include "akash_types.h"
#include <vector>
#include <string>

// ── BMP time-series playback controller ───────────────────
class PlaybackController {
public:
    PlaybackController() = default;

    bool loadSequence(const std::string& folder, SystemConfig& cfg);
    bool generateSyntheticSequence(int nFrames, SystemConfig& cfg);

    // Returns true when a new frame is ready
    bool tick(SystemConfig& cfg, double wallTimeMs, WFSFrame& outFrame);

    void play()  { playing_ = true; }
    void pause() { playing_ = false; }
    void reset() { cfg_frame_ = 0; lastTickMs_ = 0.0; }
    void stepForward(SystemConfig& cfg);
    void stepBack   (SystemConfig& cfg);
    bool isPlaying() const { return playing_; }

private:
    std::vector<std::string> filePaths_;
    bool   playing_    = false;
    bool   synthetic_  = false;
    double lastTickMs_ = 0.0;
    int    cfg_frame_  = 0;

    double frameDtMs()  const { return 10.0; }  // 10 ms cadence
};
