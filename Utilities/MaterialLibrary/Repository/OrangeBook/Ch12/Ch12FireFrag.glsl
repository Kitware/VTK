//
// Fragment shader for producing a turbulent fire effect
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
uniform vec3 Color1;       // (0.8, 0.7, 0.0)
uniform vec3 Color2;       // (0.6, 0.1, 0.0)
uniform float NoiseScale;  // 1.2

void main (void)
{
    vec4 noisevec = texture3D(Noise, MCposition * NoiseScale);


    float intensity = abs(noisevec[0] - 0.25) +
                      abs(noisevec[1] - 0.125) +
                      abs(noisevec[2] - 0.0625) +
                      abs(noisevec[3] - 0.03125);

    intensity    = clamp(intensity * 6.0, 0.0, 1.0);
    vec3 color   = mix(Color1, Color2, intensity) * LightIntensity;
    
    gl_FragColor = vec4 (color, 1.0);
}