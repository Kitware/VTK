//
// Vertex shader for drawing the earth with one texture
//
// Author: Randi Rost
//
// Copyright (c) 2002-2004 3Dlabs Inc. Ltd.
//
// See 3Dlabs-License.txt for license information
//

varying float LightIntensity;
uniform vec3 LightPosition;

const float specularContribution = 0.1;
const float diffuseContribution  = 1.0 - specularContribution;

void main(void)
{
    vec3 ecPosition = vec3(gl_ModelViewMatrix * gl_Vertex);
    vec3 ecLightPosition = vec3(gl_ModelViewMatrix * vec4(LightPosition,1.0));

    vec3 tnorm      = normalize(gl_NormalMatrix * gl_Normal);
    vec3 lightVec   = normalize(ecLightPosition - ecPosition);

    vec3 reflectVec = reflect(-lightVec, tnorm);
    vec3 viewVec    = normalize(-ecPosition);

    float spec      = clamp(dot(reflectVec, viewVec), 0.0, 1.0);
    spec            = pow(spec, 16.0);

    LightIntensity  = diffuseContribution * max(dot(lightVec, tnorm), 0.0)
                      + specularContribution * spec;

    gl_TexCoord[0]  = gl_MultiTexCoord0;
    gl_Position     = ftransform();
}
