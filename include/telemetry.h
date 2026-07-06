#pragma once
// ============================================================
//  AKASH-DARPAN  —  telemetry.h
//  Performance ring-buffers, statistics, CSV export
// ============================================================
#include "akash_types.h"
#include <deque>
#include <string>
#include <cstdio>
#include <vector>

struct TelemetryStats {
    float mean   = 0.f;
    float stddev = 0.f;
    float min    = 0.f;
    float max    = 0.f;
    float median = 0.f;
};

class Telemetry {
public:
    using Ring = std::deque<float>;
    static constexpr int RING_SIZE = 512;

    Telemetry();
    ~Telemetry() { closeCSV(); }

    // Feed one frame's performance data
    void record(const PerfTelemetry& p);

    // Rolling history for ImPlot
    const Ring& ioTimes()    const;
    const Ring& centTimes()  const;
    const Ring& reconTimes() const;
    const Ring& dmTimes()    const;
    const Ring& totalTimes() const;
    const Ring& fpsValues()  const;
    const Ring& timeAxis()   const;

    // Aggregate statistics
    TelemetryStats totalStats() const;
    TelemetryStats fpsStats()   const;

    // CSV export
    bool openCSV (const std::string& path);
    void closeCSV();

    // Diagnostics
    void printSummary() const;
    void reset();

    int framesProcessed() const { return framesSinceReset_; }

private:
    void   push(Ring& r, float v);
    static double wallMs();
    TelemetryStats stats(const Ring& r) const;

    Ring    ioRing_, centRing_, reconRing_, dmRing_;
    Ring    totalRing_, fpsRing_, timeRing_;
    FILE*   csvFile_         = nullptr;
    int     framesSinceReset_= 0;
    double  sessionStartMs_  = 0.0;
};
