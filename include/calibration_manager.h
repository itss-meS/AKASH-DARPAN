#pragma once
// ============================================================
//  AKASH-DARPAN  —  calibration_manager.h / .cpp  (header-only)
//  Manages:
//   1. Dark frame subtraction
//   2. Flat-field (reference) calibration
//   3. Pupil mask generation
//   4. Bad-pixel map
//   5. Saving / loading calibration to disk
// ============================================================
#include "akash_types.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <numeric>

class CalibrationManager {
public:
    // ── State ─────────────────────────────────────────────
    enum class State {
        UNCALIBRATED,
        DARK_ONLY,
        FLAT_ONLY,
        FULL          // dark + flat + pupil mask ready
    };

    // ── Acquire dark frame (average N frames with shutter closed) ──
    void addDarkFrame(const WFSFrame& frame) {
        if (darkAccum_.empty()) {
            darkAccum_.assign(frame.width * frame.height, 0.0);
            darkW_ = frame.width;
            darkH_ = frame.height;
        }
        for (int i = 0; i < (int)frame.pixels.size(); ++i)
            darkAccum_[i] += frame.pixels[i];
        ++darkCount_;
    }

    void finaliseDark() {
        if (darkCount_ == 0) return;
        dark_.resize(darkAccum_.size());
        for (size_t i = 0; i < darkAccum_.size(); ++i)
            dark_[i] = (uint8_t)std::min(255.0, darkAccum_[i] / darkCount_);
        darkCount_ = 0;
        darkAccum_.clear();
        printf("[Cal] Dark frame finalised (%dx%d)\n", darkW_, darkH_);
        if (state_ == State::UNCALIBRATED) state_ = State::DARK_ONLY;
        else if (state_ == State::FLAT_ONLY) state_ = State::FULL;
    }

    // ── Acquire flat (reference) frame ────────────────────
    void addFlatFrame(const WFSFrame& frame) {
        if (flatAccum_.empty()) {
            flatAccum_.assign(frame.width * frame.height, 0.0);
            flatW_ = frame.width;
            flatH_ = frame.height;
        }
        for (int i = 0; i < (int)frame.pixels.size(); ++i)
            flatAccum_[i] += frame.pixels[i];
        ++flatCount_;
    }

    void finaliseFlat() {
        if (flatCount_ == 0) return;
        flat_.resize(flatAccum_.size());
        for (size_t i = 0; i < flatAccum_.size(); ++i)
            flat_[i] = (uint8_t)std::min(255.0, flatAccum_[i] / flatCount_);
        flatCount_ = 0;
        flatAccum_.clear();

        // Build pupil mask: pixels above 20% of max flat value are illuminated
        buildPupilMask();

        printf("[Cal] Flat frame finalised (%dx%d)  |  pupil pixels: %d\n",
               flatW_, flatH_, (int)std::count(pupilMask_.begin(), pupilMask_.end(), true));

        if (state_ == State::UNCALIBRATED) state_ = State::FLAT_ONLY;
        else if (state_ == State::DARK_ONLY) state_ = State::FULL;
    }

    // ── Apply calibration to incoming frame ───────────────
    void applyTo(WFSFrame& frame) const {
        if (state_ == State::UNCALIBRATED) return;
        for (int i = 0; i < (int)frame.pixels.size(); ++i) {
            int v = frame.pixels[i];
            // Dark subtraction
            if (!dark_.empty() && i < (int)dark_.size())
                v -= dark_[i];
            // Flat normalisation: scale so illuminated pixels are on common baseline
            if (!flat_.empty() && i < (int)flat_.size() && flat_[i] > 10)
                v = (int)(v * 128.0 / flat_[i]);
            frame.pixels[i] = (uint8_t)std::max(0, std::min(255, v));
            // Zero out pixels outside pupil mask
            if (!pupilMask_.empty() && !pupilMask_[i])
                frame.pixels[i] = 0;
        }
    }

    // ── Bad pixel map (sigma-clip on dark) ────────────────
    void buildBadPixelMap(double sigmaThresh = 5.0) {
        if (dark_.empty()) return;
        double mean = 0;
        for (uint8_t v : dark_) mean += v;
        mean /= dark_.size();
        double var = 0;
        for (uint8_t v : dark_) var += (v - mean) * (v - mean);
        double sigma = std::sqrt(var / dark_.size());
        double thresh = mean + sigmaThresh * sigma;
        badPixels_.resize(dark_.size(), false);
        int nBad = 0;
        for (size_t i = 0; i < dark_.size(); ++i)
            if (dark_[i] > thresh) { badPixels_[i] = true; ++nBad; }
        printf("[Cal] Bad pixels flagged: %d  (%.2f%%)\n",
               nBad, 100.0*nBad/dark_.size());
    }

    // Interpolate bad pixels from 4-neighbours
    void fixBadPixels(WFSFrame& frame) const {
        if (badPixels_.empty()) return;
        int W = frame.width, H = frame.height;
        for (int y = 1; y < H-1; ++y) {
            for (int x = 1; x < W-1; ++x) {
                int idx = y*W+x;
                if (!badPixels_[idx]) continue;
                int sum = frame.pixels[(y-1)*W+x] + frame.pixels[(y+1)*W+x]
                        + frame.pixels[y*W+x-1]   + frame.pixels[y*W+x+1];
                frame.pixels[idx] = (uint8_t)(sum / 4);
            }
        }
    }

    // ── Persistence ───────────────────────────────────────
    bool saveToFile(const std::string& path) const {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        auto writeVec = [&](const std::vector<uint8_t>& v) {
            uint32_t sz = (uint32_t)v.size();
            f.write((char*)&sz, 4);
            if (sz) f.write((char*)v.data(), sz);
        };
        writeVec(dark_);
        writeVec(flat_);
        uint32_t ms = (uint32_t)pupilMask_.size();
        f.write((char*)&ms, 4);
        for (bool b : pupilMask_) { uint8_t u = b?1:0; f.write((char*)&u,1); }
        printf("[Cal] Saved calibration to %s\n", path.c_str());
        return true;
    }

    bool loadFromFile(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        auto readVec = [&](std::vector<uint8_t>& v) {
            uint32_t sz; f.read((char*)&sz, 4);
            v.resize(sz);
            if (sz) f.read((char*)v.data(), sz);
        };
        readVec(dark_);
        readVec(flat_);
        uint32_t ms; f.read((char*)&ms, 4);
        pupilMask_.resize(ms);
        for (uint32_t i = 0; i < ms; ++i) {
            uint8_t u; f.read((char*)&u, 1);
            pupilMask_[i] = (u != 0);
        }
        state_ = State::FULL;
        printf("[Cal] Loaded calibration from %s\n", path.c_str());
        return true;
    }

    // ── Accessors ─────────────────────────────────────────
    State state()                          const { return state_; }
    const std::vector<bool>& pupilMask()  const { return pupilMask_; }
    const char* stateString() const {
        switch(state_) {
        case State::UNCALIBRATED: return "Uncalibrated";
        case State::DARK_ONLY:    return "Dark only";
        case State::FLAT_ONLY:    return "Flat only";
        case State::FULL:         return "FULL";
        default:                  return "Unknown";
        }
    }

    int darkFramesAcquired() const { return darkCount_; }
    int flatFramesAcquired() const { return flatCount_; }

    // Reset everything
    void reset() {
        dark_.clear(); flat_.clear();
        pupilMask_.clear(); badPixels_.clear();
        darkAccum_.clear(); flatAccum_.clear();
        darkCount_ = flatCount_ = 0;
        darkW_ = darkH_ = flatW_ = flatH_ = 0;
        state_ = State::UNCALIBRATED;
    }

private:
    void buildPupilMask() {
        pupilMask_.resize(flat_.size(), false);
        if (flat_.empty()) return;
        uint8_t maxVal = *std::max_element(flat_.begin(), flat_.end());
        uint8_t thresh = (uint8_t)(maxVal * 0.20f);
        for (size_t i = 0; i < flat_.size(); ++i)
            pupilMask_[i] = (flat_[i] > thresh);
    }

    State state_ = State::UNCALIBRATED;

    // Calibration frames
    std::vector<uint8_t> dark_, flat_;
    std::vector<bool>    pupilMask_, badPixels_;

    // Accumulation buffers
    std::vector<double> darkAccum_, flatAccum_;
    int darkCount_ = 0, flatCount_ = 0;
    int darkW_ = 0, darkH_ = 0;
    int flatW_ = 0, flatH_ = 0;
};
