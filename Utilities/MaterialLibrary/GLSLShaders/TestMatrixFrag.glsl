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

      || testMat2[0][0]!=3.14159 || testMat2[0][1]!=6.28319
      || testMat2[1][0]!=9.42478 || testMat2[1][1]!=12.5664

      || testMat3[0][0]!=9.4248  || testMat3[0][1]!=18.8496 || testMat3[0][2]!=28.2743
      || testMat3[1][0]!=37.6991 || testMat3[1][1]!=47.1239 || testMat3[1][2]!=56.5487
      || testMat3[2][0]!=65.9734 || testMat3[2][1]!=75.3982 || testMat3[2][2]!=84.8230

      || testMat4[0][0]!=12.5664  || testMat4[0][1]!=25.1327  || testMat4[0][2]!=37.6991  || testMat4[0][3]!=50.2655 
      || testMat4[1][0]!=62.8319  || testMat4[1][1]!=75.3982  || testMat4[1][2]!=87.9646  || testMat4[1][3]!=100.5310
      || testMat4[2][0]!=113.0973 || testMat4[2][1]!=125.6637 || testMat4[2][2]!=138.2301 || testMat4[2][3]!=150.7964
      || testMat4[3][0]!=163.3628 || testMat4[3][1]!=175.9292 || testMat4[3][2]!=188.4956 || testMat4[3][3]!=201.0619

    )
      {
      gl_FragColor = vec4 (1.0, 0.0, 0.0, 1.0);
      }
}
