//
// Begin "3Dlabs-License.txt"
//
// Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//     Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//     Redistributions in binary form must reproduce the above
//     copyright notice, this list of conditions and the following
//     disclaimer in the documentation and/or other materials provided
//     with the distribution.
//
//     Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//     contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//
// End "3Dlabs-License.txt"



uniform vec3 Color;
uniform vec3 AmbientColor;
uniform vec3 DiffuseColor;
uniform vec3 SpecularColor;
uniform vec3 EdgeColor;

uniform float Ambient;
uniform float Diffuse;
uniform float Specular;
uniform float SpecularPower;
uniform float Opacity;

uniform float PointSize;
uniform float LineWidth;

uniform int LineStipplePattern;
uniform int LineStippleRepeatFactor;
uniform int Interpolation;
uniform int Representation;
uniform int EdgeVisibility;
uniform int BackfaceCulling;
uniform int FrontfaceCulling;


uniform vec3  SurfaceColor; // (0.75, 0.75, 0.75)
uniform vec3  WarmColor;    // (0.6, 0.6, 0.0)
uniform vec3  CoolColor;    // (0.0, 0.0, 0.6)
uniform float DiffuseWarm;  // 0.45
uniform float DiffuseCool;  // 0.45

varying float NdotL;
varying vec3  ReflectVec;
varying vec3  ViewVec;

void main (void)
{
    vec3 kcool    = min(CoolColor + DiffuseCool * SurfaceColor, 1.0);
    vec3 kwarm    = min(WarmColor + DiffuseWarm * SurfaceColor, 1.0);
    vec3 kfinal   = mix(kcool, kwarm, NdotL);

    vec3 nreflect = normalize(ReflectVec);
    vec3 nview    = normalize(ViewVec);

    float spec    = max(dot(nreflect, nview), 0.0);
    spec          = pow(spec, 32.0);

    gl_FragColor = vec4 (min(kfinal + spec, 1.0), 1.0);


    if( 0
      || AmbientColor.x!=0.75 || AmbientColor.y!=0.751 || AmbientColor.z!=0.752
      || DiffuseColor.x!=0.61 || DiffuseColor.y!=0.62 || DiffuseColor.z!=0.006
      || SpecularColor.x!=0.001 || SpecularColor.y!=0.002 || SpecularColor.z!=0.61
      || EdgeColor.x!=0.1 || EdgeColor.y!=0.2 || EdgeColor.z!=0.3
      || Ambient!=0.45
      || Diffuse!=0.451
      || Specular!=0.4
      || SpecularPower!=1.0
      || Opacity!=1.0
      || PointSize!=1.0
      || LineWidth!=1.0
      || LineStipplePattern!=0
      || LineStippleRepeatFactor!=1
      || Interpolation!=1
      || Representation!=2
      || EdgeVisibility!=0
      || BackfaceCulling!=0
      || FrontfaceCulling!=0
    )
      {
      gl_FragColor = vec4 (1.0, 0.0, 0.0, 1.0);
      }
}
