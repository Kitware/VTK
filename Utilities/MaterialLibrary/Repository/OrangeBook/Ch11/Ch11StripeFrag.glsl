//
// Fragment shader for drawing procedural stripes
//
// Author: OGLSL implementation by Ian Nurse
//
// Copyright (C) 2002-2004  LightWork Design Ltd.
//          www.lightworkdesign.com
//
// See LightworkDesign-License.txt for license information
//

uniform vec3  StripeColor;
uniform vec3  BackColor;
uniform float Width;
uniform float Fuzz;
uniform float Scale;

varying vec3  DiffuseColor;
varying vec3  SpecularColor;

void main(void)
{
    float scaled_t = fract(gl_TexCoord[0].t * Scale);

    float frac1 = clamp(scaled_t / Fuzz, 0.0, 1.0);
    float frac2 = clamp((scaled_t - Width) / Fuzz, 0.0, 1.0);

    frac1 = frac1 * (1.0 - frac2);
    frac1 = frac1 * frac1 * (3.0 - (2.0 * frac1));
   
    vec3 finalColor = mix(BackColor, StripeColor, frac1);
    finalColor = finalColor * DiffuseColor + SpecularColor;

    gl_FragColor = vec4 (finalColor, 1.0);
}