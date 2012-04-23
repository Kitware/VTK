//
// Fragment shader for producing clouds (mostly cloudy)
//
// Author: Randi Rost
//
// Copyright (c) 2002-2004 3Dlabs Inc. Ltd.
//
// See 3Dlabs-License.txt for license information
//

varying float LightIntensity;
varying vec3  MCposition;

uniform sampler3D Noise;
uniform vec3 SkyColor;     // (0.0, 0.0, 0.8)
uniform vec3 CloudColor;   // (0.8, 0.8, 0.8)

void main (void)
{
    vec4  noisevec  = texture3D(Noise, MCposition);

    float intensity = (noisevec[0] + noisevec[1] +
                       noisevec[2] + noisevec[3] + 0.03125) * 1.5;

    vec3 color   = mix(SkyColor, CloudColor, intensity) * LightIntensity;

    gl_FragColor = vec4 (color, 1.0);
}