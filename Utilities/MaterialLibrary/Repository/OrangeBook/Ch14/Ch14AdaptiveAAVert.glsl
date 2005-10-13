//
// Vertex shader for adaptively antialiasing a procedural stripe pattern
//
// Author: Randi Rost
//         based on a shader by Bert Freudenberg
//
// Copyright (c) 2002-2004 3Dlabs Inc. Ltd. 
//
// See 3Dlabs-License.txt for license information
//

uniform vec3  LightPosition;

varying float V;
varying float LightIntensity;
 
void main(void)
{
    vec3 pos        = vec3(gl_ModelViewMatrix * gl_Vertex);
    vec3 tnorm      = normalize(gl_NormalMatrix * gl_Normal);
    vec3 lightVec   = normalize(LightPosition - pos);

    LightIntensity = max(dot(lightVec, tnorm), 0.0);

    V = gl_MultiTexCoord0.s;  // try .s for vertical stripes

    gl_Position = ftransform();
}
