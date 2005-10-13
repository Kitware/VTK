//
// Fragment shader for procedurally generated hatching or "woodcut" appearance.
//
// This is an OpenGL 2.0 implementation of Scott F. Johnston's "Mock Media"
// (from "Advanced RenderMan: Beyond the Companion" SIGGRAPH 98 Course Notes)
//
// Author: Bert Freudenberg <bert@isg.cs.uni-magdeburg.de>
//
// Copyright (c) 2002-2003 3Dlabs, Inc. 
//
// See 3Dlabs-License.txt for license information
//

const float frequency = 1.0;

varying vec3  ObjPos;               // object space coord (noisy)
varying float V;                    // generic varying
varying float LightIntensity;

uniform sampler3D Noise;            // value of Noise = 3;
uniform float Swidth;               // relative width of stripes = 16.0

void main (void)
{
    float dp       = length(vec2 (dFdx(V * Swidth), dFdy(V * Swidth)));
    float logdp    = -log2(dp);
    float ilogdp   = floor(logdp);
    float stripes  = exp2(ilogdp);

    float noise    = texture3D(Noise, ObjPos).x;

    float sawtooth = fract((V + noise * 0.1) * frequency * stripes);
    float triangle = abs(2.0 * sawtooth - 1.0);

    // adjust line width
    float transition = logdp - ilogdp;

    // taper ends
    triangle = abs((1.0 + transition) * triangle - transition);

    const float edgew = 0.2;            // width of smooth step

    float edge0  = clamp(LightIntensity - edgew, 0.0, 1.0);
    float edge1  = clamp(LightIntensity, 0.0, 1.0);
    float square = 1.0 - smoothstep(edge0, edge1, triangle);

    gl_FragColor = vec4 (vec3 (square), 1.0);
}