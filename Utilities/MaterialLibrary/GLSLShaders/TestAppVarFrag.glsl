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

uniform vec4 appVara;
uniform vec4 appVarb;
uniform vec4 appVarc;
uniform vec4 appVard;
uniform vec4 appVare;
uniform vec4 appVarf;
uniform vec4 appVarg;

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
        || appVara.x != 0.37714 || appVara.y != 0.61465 || appVara.z != 0.48399 || appVara.w != 0.68252
        || appVarb.x != 0.03900 || appVarb.y != 0.15857 || appVarb.z != 0.57913 || appVarb.w != 0.54458
        || appVarc.x != 0.97061 || appVarc.y != 0.86053 || appVarc.z != 0.63583 || appVarc.w != 0.51058
        || appVard.x != 0.12885 || appVard.y != 0.91490 || appVard.z != 0.86394 || appVard.w != 0.58951
        || appVare.x != 0.23403 || appVare.y != 0.35340 || appVare.z != 0.52559 || appVare.w != 0.77830
        || appVarf.x != 0.19550 || appVarf.y != 0.17429 || appVarf.z != 0.89958 || appVarf.w != 0.15063
        || appVarg.x != 0.75796 || appVarg.y != 0.48072 || appVarg.z != 0.07728 || appVarg.w != 0.16434
      )
      {
      gl_FragColor = vec4 (1.0, 0.0, 0.0, 1.0);
      }
}
