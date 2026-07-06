// ============================================================
//  AKASH-DARPAN — shaders/wavefront.vert
//  Vertex shader for 3-D reconstructed wavefront surface
// ============================================================
#version 330 core

layout(location = 0) in vec3  aPosition;   // (x, y, 0) grid position
layout(location = 1) in float aPhase;      // W(x,y) in radians
layout(location = 2) in vec2  aTexCoord;   // (u,v) for colormap

uniform mat4  uModel;
uniform mat4  uView;
uniform mat4  uProjection;
uniform float uZScale;      // vertical exaggeration
uniform float uPhaseMin;
uniform float uPhaseMax;

out float vPhase;
out vec2  vTexCoord;
out vec3  vNormal;
out vec3  vFragPos;

void main() {
    // Lift Z by phase value
    vec3 pos    = aPosition;
    pos.z       = aPhase * uZScale;

    vPhase      = aPhase;
    vTexCoord   = aTexCoord;
    vFragPos    = vec3(uModel * vec4(pos, 1.0));

    // Approximate normal via finite-difference handled CPU-side;
    // pass as flat for now — renderer can refine with geometry shader
    vNormal     = normalize(vec3(0.0, 0.0, 1.0));

    gl_Position = uProjection * uView * uModel * vec4(pos, 1.0);
}
