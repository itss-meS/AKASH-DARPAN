// ============================================================
//  AKASH-DARPAN — shaders/dm_actuator.vert
//  Instanced rendering for DM actuator discs
// ============================================================
#version 330 core

layout(location = 0) in vec2  aUnitCircle;  // unit circle vertices
layout(location = 1) in vec2  aCenter;      // per-instance: actuator centre
layout(location = 2) in float aStroke;      // per-instance: normalised stroke

uniform mat4  uOrtho;
uniform float uRadius;

out float vStroke;

void main() {
    vStroke     = aStroke;
    vec2 pos    = aCenter + aUnitCircle * uRadius;
    gl_Position = uOrtho * vec4(pos, 0.0, 1.0);
}
