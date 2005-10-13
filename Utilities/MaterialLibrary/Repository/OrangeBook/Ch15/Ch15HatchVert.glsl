//
// Vertex shader for procedurally generated hatching or "woodcut" appearance.
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

uniform vec3  LightPosition;
uniform float Time;

varying vec3  ObjPos;
varying float V;
varying float LightIntensity;
 
void main(void)
{
    ObjPos          = (vec3 (gl_Vertex) + vec3 (0.0, 0.0, Time)) * 0.2;
 
    vec3 pos        = vec3 (gl_ModelViewMatrix * gl_Vertex);
    vec3 tnorm      = normalize(gl_NormalMatrix * gl_Normal);
    vec3 lightVec   = normalize(LightPosition - pos);

    LightIntensity  = max(dot(lightVec, tnorm), 0.0);

    V = gl_MultiTexCoord0.t;  // try .s for vertical stripes

    gl_Position = ftransform();
}