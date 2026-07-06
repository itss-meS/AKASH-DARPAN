// ============================================================
//  AKASH-DARPAN — shaders/spot.frag
//  Renders greyscale WFS frame as blue-tinted science image
// ============================================================
#version 330 core

in  vec2 vUV;
out vec4 fragColor;

uniform sampler2D uFrame;   // R8 greyscale texture
uniform float     uBrightness;
uniform float     uContrast;

void main() {
    float grey = texture(uFrame, vUV).r;

    // Brightness / contrast adjustment
    grey = clamp((grey - 0.5) * uContrast + 0.5 + uBrightness, 0.0, 1.0);

    // Science-camera blue tint (like a CCD display)
    vec3 col = vec3(grey * 0.55, grey * 0.75, grey * 1.00);

    // Slight vignette
    vec2 d   = vUV - 0.5;
    float vig = 1.0 - dot(d, d) * 1.0;
    col *= clamp(vig, 0.0, 1.0);

    fragColor = vec4(col, 1.0);
}
