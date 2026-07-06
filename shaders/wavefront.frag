// ============================================================
//  AKASH-DARPAN — shaders/wavefront.frag
//  Fragment shader — jet/viridis colormap + Phong lighting
// ============================================================
#version 330 core

in  float vPhase;
in  vec2  vTexCoord;
in  vec3  vNormal;
in  vec3  vFragPos;

out vec4 fragColor;

uniform float uPhaseMin;
uniform float uPhaseMax;
uniform int   uColormap;    // 0=Jet, 1=Viridis, 2=Plasma
uniform float uOpacity;
uniform vec3  uLightPos;
uniform vec3  uViewPos;

// ── Jet colormap ──────────────────────────────────────────
vec3 jetColor(float t) {
    t = clamp(t, 0.0, 1.0);
    float r = clamp(1.5 - abs(4.0*t - 3.0), 0.0, 1.0);
    float g = clamp(1.5 - abs(4.0*t - 2.0), 0.0, 1.0);
    float b = clamp(1.5 - abs(4.0*t - 1.0), 0.0, 1.0);
    return vec3(r, g, b);
}

// ── Viridis colormap (polynomial approximation) ───────────
vec3 viridisColor(float t) {
    t = clamp(t, 0.0, 1.0);
    vec3 c0 = vec3(0.2777, 0.0054, 0.3342);
    vec3 c1 = vec3(0.1050, 0.5748, 0.5813);
    vec3 c2 = vec3(0.9400, 0.9750, 0.1313);
    return c0 + t*(4.0*c1 - 4.0*c0 - c2 + c0) + t*t*(c2 - c1)*3.0;
}

// ── Plasma colormap ───────────────────────────────────────
vec3 plasmaColor(float t) {
    t = clamp(t, 0.0, 1.0);
    float r = 0.5 + 0.5*sin(3.14159*(t + 0.0));
    float g = 0.1 + 0.6*sin(3.14159*(t - 0.3));
    float b = 0.8 - 0.7*t;
    return clamp(vec3(r,g,b), 0.0, 1.0);
}

void main() {
    // Normalise phase to [0,1]
    float range = uPhaseMax - uPhaseMin;
    float t     = (range < 1e-6) ? 0.5 : (vPhase - uPhaseMin) / range;
    t           = clamp(t, 0.0, 1.0);

    // Pick colormap
    vec3 baseColor;
    if      (uColormap == 1) baseColor = viridisColor(t);
    else if (uColormap == 2) baseColor = plasmaColor(t);
    else                     baseColor = jetColor(t);

    // ── Phong shading ─────────────────────────────────────
    vec3  norm     = normalize(vNormal);
    vec3  lightDir = normalize(uLightPos - vFragPos);
    vec3  viewDir  = normalize(uViewPos  - vFragPos);
    vec3  reflect  = reflect(-lightDir, norm);

    float ambient  = 0.30;
    float diffuse  = max(dot(norm, lightDir), 0.0) * 0.55;
    float specular = pow(max(dot(viewDir, reflect), 0.0), 32.0) * 0.15;

    vec3 lit = (ambient + diffuse + specular) * baseColor;

    fragColor = vec4(lit, uOpacity);
}
