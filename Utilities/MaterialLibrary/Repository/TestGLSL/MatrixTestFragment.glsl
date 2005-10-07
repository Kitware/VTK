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

uniform mat2 testMat2;
uniform mat3 testMat3;
uniform mat4 testMat4;




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
      || testMat2[0][0]!=44.38361 || testMat2[0][1]!=58.62439
      || testMat2[1][0]!=22.02428 || testMat2[1][1]!=97.35272

      || testMat3[0][0]!= 5.91774 || testMat3[0][1]!=25.49759 || testMat3[0][2]!=50.20272
      || testMat3[1][0]!= 6.45461 || testMat3[1][1]!=42.84395 || testMat3[1][2]!=11.17144
      || testMat3[2][0]!= 8.02892 || testMat3[2][1]!=29.76296 || testMat3[2][2]!= 1.92514

      || testMat4[0][0]!=46.22906 || testMat4[0][1]!=11.80764 || testMat4[0][2]!= 5.07503 || testMat4[0][3]!=46.32990
      || testMat4[1][0]!=39.79442 || testMat4[1][1]!=81.58471 || testMat4[1][2]!=52.86966 || testMat4[1][3]!=95.58122
      || testMat4[2][0]!=35.94935 || testMat4[2][1]!=56.07540 || testMat4[2][2]!=81.56149 || testMat4[2][3]!=63.69266
      || testMat4[3][0]!=28.87369 || testMat4[3][1]!=52.99193 || testMat4[3][2]!=69.44439 || testMat4[3][3]!=94.62996
    )
      {
      gl_FragColor = vec4 (1.0, 0.0, 0.0, 1.0);
      }
}
