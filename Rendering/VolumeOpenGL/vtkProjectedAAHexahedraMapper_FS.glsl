/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_HeaderFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// High quality volume renderer for axis-aligned hexahedra
// Implementation by Stephane Marchesin (stephane.marchesin@gmail.com)
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France.
//
// This file implements the paper
// "High-Quality, Semi-Analytical Volume Rendering for AMR Data",
// Stephane Marchesin and Guillaume Colin de Verdiere, IEEE Vis 2009.

// inputs of FS
// texcoord0.xyz : position in the brick space [0,1]^3
// texcoord0.w   : cell size
// texcoord1.xyz : position in object space
// texcoord2.xyzw: node data 0,1,2,3
// texcoord3.xyzw: node data 4,5,6,7

uniform sampler3D preintegration_table;
uniform vec3 observer;
uniform float length_max;

vec4 sample(float sample0, float sample1, float length)
{
  float corrected_length = length * gl_TexCoord[0].w / length_max ;
  return texture3D(preintegration_table,
                   vec3(sample0,sample1,corrected_length));
}

void main()
{
  vec3 pos = gl_TexCoord[0].xyz;
  vec3 progression;
  vec3 dist1,dist2,dist;
  vec3 l=vec3(1.0,0.0,0.0);
  float length;
  float cell_length = gl_TexCoord[0].w;

  progression.xyz = gl_TexCoord[1].xyz - observer.xyz;
  progression = normalize(progression);

  dist1.xyz = abs((1.0-pos.xyz)/progression.xyz);
  dist2.xyz = abs((pos.xyz)/progression.xyz);
  if (progression.x>0.0)
    dist.x=dist1.x;
  else
    dist.x=dist2.x;
  if (progression.y>0.0)
    dist.y=dist1.y;
  else
    dist.y=dist2.y;
  if (progression.z>0.0)
    dist.z=dist1.z;
  else
    dist.z=dist2.z;

  length = min(dist.x,min(dist.y,dist.z));
  vec3 p1 = pos, p2 = pos + vec3(length) * progression;

  float s0 = gl_TexCoord[2].x;
  float s1 = gl_TexCoord[2].y;
  float s2 = gl_TexCoord[2].z;
  float s3 = gl_TexCoord[2].w;

  float s4 = gl_TexCoord[3].x;
  float s5 = gl_TexCoord[3].y;
  float s6 = gl_TexCoord[3].z;
  float s7 = gl_TexCoord[3].w;
  float   x0 = p1.x,
    x1 = p2.x - p1.x,
    y0 = p1.y,
    y1 = p2.y - p1.y,
    z0 = p1.z,
    z1 = p2.z - p1.z;
  float a = (s3 - s0 + s1 + s4 + s6 - s2 - s5 - s7) *x1*y1*z1;
  float b = (-x0*y1*z1 - x1*y0*z1 - x1*y1*z0 + x1*z1)*s7
    + (x0*y1*z1 + x1*y0*z1 + x1*y1*z0)*s6
    + (y1*z1 - x0*y1*z1 - x1*y0*z1 - x1*y1*z0)*s5
    + (-x1*z1 + x1*y1*z0 - y1*z1 + x0*y1*z1 + x1*y0*z1)*s4
    + (-x1*z1 + x1*y0*z1 + x1*y1*z0 - x1*y1 + x0*y1*z1)*s3
    + (-x1*y0*z1 - x0*y1*z1 + x1*y1 - x1*y1*z0)*s2
    + (x1*y1 + y1*z1 - x1*y1*z0 + x1*z1 - x0*y1*z1 - x1*y0*z1)*s0
    + (x1*y1*z0 - y1*z1 - x1*y1 + x0*y1*z1 + x1*y0*z1)*s1;
  float c = (-x0*y0*z1 + x0*z1 + x1*z0 - x1*y0*z0 - x0*y1*z0)*s7
    + (x1*y0*z0 + x0*y1*z0 + x0*y0*z1)*s6
    + (y0*z1 - x0*y1*z0 - x1*y0*z0 + y1*z0 - x0*y0*z1)*s5
    + (x0*y0*z1 + z1 - y0*z1 - y1*z0 - x0*z1 - x1*z0 + x0*y1*z0 + x1*y0*z0)*s4
    + (x1*y0*z0 + x0*y0*z1 + x0*y1*z0 - x1*z0 - x1*y0 - x0*z1 - x0*y1 + x1)*s3
    + (x0*y1 + x1*y0 - x0*y0*z1 - x0*y1*z0 - x1*y0*z0)*s2
    + (-x1*y0 + x0*y1*z0 - y0*z1 - x0*y1 + x0*y0*z1 + y1 + x1*y0*z0 - y1*z0)*s1
    + (-x0*y1*z0 - z1 + x1*y0 - x0*y0*z1 - x1*y0*z0 - y1 + y0*z1 + x1*z0
       + y1*z0 + x0*y1 - x1 + x0*z1)*s0;
  float d = (x0*z0 - x0*y0*z0)*s7 + (y0*z0 - x0*y0*z0)*s5
    + (-x0*z0 - y0*z0 + x0*y0*z0 + z0)*s4 + (-x0*z0 + x0 + x0*y0*z0 - x0*y0)*s3
    + (x0*y0 - x0*y0*z0)*s2 + (-y0*z0 - x0*y0 + y0 + x0*y0*z0)*s1
    + (-y0 - z0 - x0*y0*z0 + x0*z0 + y0*z0 - x0 + x0*y0 + 1.0)*s0
    + s6*x0*y0*z0;
  float r[4];
  r[0] = 0.0;
  r[1] = 0.0;
  r[2] = 0.0;
  r[3] = 0.0;
  int numsteps = 0;

  // at this point P(t) = a.t^3 + b.t^2 + c.t + d

  if ( (abs(a)<=0.00001) && (abs(b)<=0.00001) )
    {
    // P(t) is linear
    numsteps = 0;
    }
  else if (abs(a)<=0.00001)
    {
    // P(t) is quadratic
    r[0] = -c/(2.0*b);

    if ((r[0] <= 0.0) || (r[0] >= 1.0))
      {
      numsteps = 0;
      }
    else
      {
      numsteps = 1;
      }
    }
  else
    {
    // P(t) is cubic
    // small optimization here : we divide delta by 4,
    // and simplify r[0]/r[1] by 2
    float delta = b*b - 3.0*a*c;
    if (delta < 0.0)
      {
      numsteps = 0;
      } else {
    numsteps = 2;
    r[0] = (-b  - sqrt(delta))/(3.0*a);
    r[1] = (-b  + sqrt(delta))/(3.0*a);

    if ((r[1] <= 0.0) || (r[1] >= 1.0))
      {
      numsteps--;
      }

    if ((r[0] <= 0.0) || (r[0] >= 1.0))
      {
      numsteps--;
      r[0] = r[1];
      }
    }
    }

#if 0
  // handle light extrema as well
  if (abs(e)>0.00001)
    {
    // Q(t) is quadratic
    if ((-f/(2.0*e) > 0.0) && (-f/(2.0*e) < 1.0))
      {
      r[numsteps] = -f/(2.0*e);
      numsteps++;
      }
    }
#endif
  vec4 result, val0, val1, val2, val3;
  float sample0,sample1,sample2,sample3,sample4;
  if (numsteps==0)
    {
    // single preintegration over [0,1]

    // evaluate the scalar value at the 2 points :
    // sample0 at t = 0.0;
    // sample1 at t = 1.0;
    sample0 = d;
    sample1 = d + c + b + a;

    // preintegrate over [0,1.0] -> [sample0,sample1]
    val0 = sample(sample0,sample1,length);

    // blend values
    result.rgba = val0.rgba;
    }
  else if (numsteps==1)
    {
    // double preintegration over [0,r[0]] and [r[0],1.0]

    // evaluate the scalar value at the 3 points :
    // sample0 at t = 0.0;
    // sample1 at t = r[0];
    // sample2 at t = 1.0;
    sample0 = d;
    sample1 = d + r[0]* (c + r[0]* (b + r[0]*a));
    sample2 = d + c + b + a;

    // preintegrate over [0,r[0]] -> [sample0,sample1]
    val0 = sample(sample0,sample1,r[0]*length);
    // preintegrate over [r[0],1] -> [sample1,sample2]
    val1 = sample(sample1,sample2,(1.0 - r[0])*length);

    // blend values
    result.rgba = val0.rgba + vec4(1.0 - val0.a) * val1.rgba;
    }
  else if (numsteps==2)
    {
    // numsteps==2
    // triple preintegration over [0,r[0]], [r[0],r[1]] and [r[1],1.0]

    if (r[1]<r[0])
      {
      float tmp = r[0];
      r[0] = r[1];
      r[1] = tmp;
      }

    // evaluate the scalar value at the 4 points :
    // sample0 at t = 0.0;
    // sample1 at t = r[0];
    // sample2 at t = r[1];
    // sample3 at t = 1.0;
    sample0 = d;
    sample1 = d + r[0]* (c + r[0]* (b + r[0]*a));
    sample2 = d + r[1]* (c + r[1]* (b + r[1]*a));
    sample3 = d + c + b + a;

    // preintegrate over [0,r[0]] -> [sample0,sample1]
    val0 = sample(sample0,sample1,r[0]*length);
    // preintegrate over [r[0],r[1]] -> [sample1,sample2]
    val1 = sample(sample1,sample2,(r[1] - r[0])*length);
    // preintegrate over [r[1],1] -> [sample2,sample3]
    val2 = sample(sample2,sample3,(1.0 - r[1])*length);

    // blend values
    result.rgba = val0.rgba + vec4(1.0 - val0.a) *
      (val1.rgba + vec4(1.0 - val1.a) * val2.rgba);
    }
  else
    {
    // numsteps==3
    // triple preintegration over [0,r[0]], [r[0],r[1]], [r[1],r[2]]
    // and [r[2],1.0]

    if (r[0]>r[1])
      {
      float tmp = r[0];
      r[0] = r[1];
      r[1] = tmp;
      }
    if (r[1]>r[2])
      {
      float tmp = r[2];
      r[2] = r[1];
      r[1] = tmp;
      }
    if (r[0]>r[1])
      {
      float tmp = r[0];
      r[0] = r[1];
      r[1] = tmp;
      }

    // evaluate the scalar value at the 4 points :
    // sample0 at t = 0.0;
    // sample1 at t = r[0];
    // sample2 at t = r[1];
    // sample3 at t = 1.0;
    sample0 = d;
    sample1 = d + r[0]* (c + r[0]* (b + r[0]*a));
    sample2 = d + r[1]* (c + r[1]* (b + r[1]*a));
    sample3 = d + r[2]* (c + r[2]* (b + r[2]*a));
    sample4 = d + c + b + a;

    // preintegrate over [0,r[0]] -> [sample0,sample1]
    val0 = sample(sample0,sample1,r[0]*length);
    // preintegrate over [r[0],r[1]] -> [sample1,sample2]
    val1 = sample(sample1,sample2,(r[1] - r[0])*length);
    // preintegrate over [r[1],r[2]] -> [sample2,sample3]
    val2 = sample(sample2,sample3,(r[2] - r[1])*length);
    // preintegrate over [r[2],1] -> [sample3,sample4]
    val3 = sample(sample3,sample4,(1.0 - r[2])*length);

    // blend values
    result.rgba = val0.rgba + vec4(1.0 - val0.a) *
      (val1.rgba + vec4(1.0 - val1.a) *
       (val2.rgba + vec4(1.0 - val2.a) * val3.rgba));
    }
  gl_FragColor.rgba = result.rgba;
}
