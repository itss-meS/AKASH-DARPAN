// ============================================================
//  AKASH-DARPAN — shaders/dm_actuator.frag
//  Colours each actuator by stroke + glow for coupling
// ============================================================
#version 330 core

in  float vStroke;
out vec4  fragColor;

uniform float uCouplingGlow;   // 0..1

vec3 strokeToJet(float s) {
    // s in [-1,+1] → map to [0,1]
    float t = clamp(s * 0.5 + 0.5, 0.0, 1.0);
    float r = clamp(1.5 - abs(4.0*t - 3.0), 0.0, 1.0);
    float g = clamp(1.5 - abs(4.0*t - 2.0), 0.0, 1.0);
    float b = clamp(1.5 - abs(4.0*t - 1.0), 0.0, 1.0);
    return vec3(r, g, b);
}

void main() {
    vec3  col   = strokeToJet(vStroke);
    float glow  = uCouplingGlow * abs(vStroke) * 0.4;
    col        += vec3(glow * 0.3, glow * 0.6, glow);
    fragColor   = vec4(clamp(col, 0.0, 1.0), 0.90);
}
