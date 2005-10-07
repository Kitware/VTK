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

uniform float testFloat;
uniform vec2  testVec2;
uniform vec3  testVec3;
uniform vec4  testVec4;

uniform int   testInt;
uniform ivec2  testIVec2;
uniform ivec3  testIVec3;
uniform ivec4  testIVec4;

uniform mat2 testMat2;
uniform mat3 testMat3;
uniform mat4 testMat4;

struct tStruct {
  float f;
  vec2 f2;
  vec3 f3;
  vec4 f4;
};

uniform tStruct tStruct2;


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
      || testFloat!=2.63749
      || testVec2.x!=76.95621 || testVec2.y!= 50.41138
      || testVec3.x!=76.96096 || testVec3.y!=63.27260 || testVec3.z!= 18.99907
      || testVec4.x!=3.40330 || testVec4.y!=93.71665 || testVec4.z!=3.25358 || testVec4.w!=53.86765

      || testInt!=55
      || testIVec2.x!=33 || testIVec2.y!=14
      || testIVec3.x!=13 || testIVec3.y!=97 || testIVec3.z!=86
      || testIVec4.x!=20 || testIVec4.y!=76 || testIVec4.z!=36 || testIVec4.w!=57

      || testMat2[0][0]!=47.50016 || testMat2[0][1]!=77.17215
      || testMat2[1][0]!=93.53756 || testMat2[1][1]!=26.49386

      || testMat3[0][0]!=20.76451 || testMat3[0][1]!=68.89935 || testMat3[0][2]!= 1.55911
      || testMat3[1][0]!=61.52196 || testMat3[1][1]!=24.00890 || testMat3[1][2]!= 20.14159
      || testMat3[2][0]!=12.79913 || testMat3[2][1]!=41.43690 || testMat3[2][2]!= 43.70222

      || testMat4[0][0]!=78.48394 || testMat4[0][1]!=18.00379 || testMat4[0][2]!=58.96785 || testMat4[0][3]!= 39.45659
      || testMat4[1][0]!=46.36133 || testMat4[1][1]!=20.19386 || testMat4[1][2]!=61.52903 || testMat4[1][3]!= 91.34887
      || testMat4[2][0]!=35.78233 || testMat4[2][1]!=28.60134 || testMat4[2][2]!=23.23688 || testMat4[2][3]!=  4.85008
      || testMat4[3][0]!=94.90575 || testMat4[3][1]!=43.35341 || testMat4[3][2]!=28.75032 || testMat4[3][3]!= 64.18256

      || tStruct2.f!=80.79816
      || tStruct2.f2.x!=55.42347 || tStruct2.f2.y!=84.07679
      || tStruct2.f3.x!=93.45425 || tStruct2.f3.y!=17.00968 || tStruct2.f3.z!=77.06349
      || tStruct2.f4.x!=44.63571 || tStruct2.f4.y!=62.42333 || tStruct2.f4.z!=86.14692 || tStruct2.f4.w!=1.50092

    )
      {
      gl_FragColor = vec4 (1.0, 0.0, 0.0, 1.0);
      }
}
