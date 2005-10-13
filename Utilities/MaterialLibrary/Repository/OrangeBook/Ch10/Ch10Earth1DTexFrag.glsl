//
// Fragment shader for drawing the earth with one texture
//
// Author: Randi Rost
//
// Copyright (c) 2002-2004 3Dlabs Inc. Ltd. 
//
// See 3Dlabs-License.txt for license information
//

varying float LightIntensity;
uniform sampler2D EarthTexture;

void main (void)
{
    vec3 lightColor = vec3(texture2D(EarthTexture, gl_TexCoord[0].st));
    gl_FragColor    = vec4(lightColor * LightIntensity, 1.0);
}
