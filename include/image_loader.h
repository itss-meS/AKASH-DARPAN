#pragma once
#include "akash_types.h"
#include <string>
#include <vector>

// ── BMP / image loader (uses stb_image) ───────────────────
class ImageLoader {
public:
    // Load single BMP/PNG/JPEG frame, returns greyscale 8-bit
    bool loadFrame(const std::string& path, WFSFrame& out);

    // Scan folder for sorted .bmp files
    std::vector<std::string> scanSequence(const std::string& folder);

    // Generate a synthetic SH-WFS frame (for demo when no data)
    void generateSyntheticFrame(WFSFrame& out,
                                 const SystemConfig& cfg,
                                 double turbStrength,
                                 double timeMs);

    // Upload frame pixels to an existing OpenGL texture (returns texID)
    unsigned int createGLTexture(const WFSFrame& frame);
    void         updateGLTexture(unsigned int texID, const WFSFrame& frame);
};
