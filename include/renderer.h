#pragma once
#include "imgui.h"
#include "akash_types.h"
#include "turbulence_analyzer.h"

struct GLFWwindow;

// ── AKASH-DARPAN Mission Control Renderer ────────────────
class AkashRenderer {
public:
    AkashRenderer()  = default;
    ~AkashRenderer() = default;

    // ── Life-cycle ────────────────────────────────────────
    bool init(int width, int height, const char* title);
    void shutdown();
    bool shouldClose() const;
    void beginFrame();
    void endFrame();

    // ── Panel drawing ─────────────────────────────────────
    // Left: Raw spot-field + centroid overlay
    void drawSpotMonitor(const WFSFrame& frame,
                          unsigned int texID,
                          const SystemConfig& cfg);

    // Centre: 3-D wavefront phase surface
    void drawWavefrontSurface(const WFSFrame& frame,
                               const SystemConfig& cfg);

    // Right-top: DM actuator grid
    void drawDMActuatorGrid(const DMState& dm,
                             const SystemConfig& cfg,
                             float& couplingSlider);

    // Right-bottom: Zernike mode bar chart
    void drawZernikeBarChart(const WFSFrame& frame);

    // Bottom-left: Atmospheric telemetry
    void drawAtmosphericTelemetry(const TurbulenceAnalyzer& atm);

    // Bottom-right: System performance dashboard
    void drawPerformanceDashboard(const PerfTelemetry& perf);

    // Top: Playback console
    void drawPlaybackConsole(SystemConfig& cfg,
                              bool& stepForward,
                              bool& stepBack,
                              bool& resetRequested,
                              bool& loadRequested,
                              std::string& loadPath);

    // ── Theme ─────────────────────────────────────────────
    void applyMissionControlTheme();

    GLFWwindow* window() const { return window_; }

private:
    GLFWwindow* window_ = nullptr;
    int         winW_   = 1920;
    int         winH_   = 1080;

    // ── 3-D surface mesh OpenGL state ────────────────────
    unsigned int meshVAO_ = 0, meshVBO_ = 0, meshEBO_ = 0;
    unsigned int meshShader_ = 0;
    unsigned int colormapTex_ = 0;
    int          meshIndicesCount_ = 0;

    void buildMeshBuffers(const WFSFrame& frame);
    void updateMeshVertices(const WFSFrame& frame);
    unsigned int compileShader(const char* vert, const char* frag);
    void         buildColormapTexture();

    // ── Helpers ───────────────────────────────────────────
    static ImVec4 strokeColor(double normStroke);
    static void   drawGlowCircle(ImDrawList* dl,
                                  ImVec2 center, float r,
                                  ImU32 col, int segments = 32);

    // Zoom state for spot-monitor
    int  zoomedSubAp_ = -1;
    bool showReference_ = true;
    bool showSlope_     = true;

    // Rotation for 3-D view
    float rotX_ = 30.f, rotY_ = 20.f;
    bool  dragging3D_ = false;
    float lastMouseX_ = 0, lastMouseY_ = 0;
};

