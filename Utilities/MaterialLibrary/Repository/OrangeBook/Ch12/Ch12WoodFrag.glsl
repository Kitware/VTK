//
// Fragment shader for producing a wood effect
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

uniform vec3  LightWoodColor;
uniform vec3  DarkWoodColor;
uniform float RingFreq;
uniform float LightGrains;
uniform float DarkGrains;
uniform float GrainThreshold;
uniform vec3  NoiseScale;
uniform float Noisiness;
uniform float GrainScale;

void main(void)
{
    vec3 noisevec = vec3 (texture3D(Noise, MCposition * NoiseScale) *
                                                 Noisiness);
    vec3 location = MCposition + noisevec;

    float dist = sqrt(location.x * location.x + location.z * location.z);
    dist *= RingFreq;

    float r = fract(dist + noisevec[0] + noisevec[1] + noisevec[2]) * 2.0;

    if (r > 1.0)
        r = 2.0 - r;
 
    vec3 color = mix(LightWoodColor, DarkWoodColor, r);

    /*r = fract((MCposition.x + MCposition.z) * GrainScale + 0.5);
    noisevec[2] *= r;
    if (r < GrainThreshold)
        color += LightWoodColor * LightGrains * noisevec[2];
    else
        color -= LightWoodColor * DarkGrains * noisevec[2];*/

    color *= LightIntensity;
    
    gl_FragColor = vec4 (color, 1.0);
}