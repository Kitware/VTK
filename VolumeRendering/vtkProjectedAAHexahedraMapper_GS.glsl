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

#version 120
#extension GL_EXT_geometry_shader4 : enable

// inputs of GS
// vertexpos.xyz : cell position (xmin,ymin,zmin)
// texcoord0.xyz : cell position (xmax,ymax,zmax)
// texcoord1.xyzw: node data 0,1,2,3
// texcoord2.xyzw: ode data 4,5,6,7

void draw_cell(vec4 scalars0, vec4 scalars1, vec3 m, vec3 M);

// ----------------------------------------------------------------------------
void main()
{
  draw_cell(gl_TexCoordIn[0][1],gl_TexCoordIn[0][2],gl_PositionIn[0].xyz,
            gl_TexCoordIn[0][0].xyz);
}

// ----------------------------------------------------------------------------
void draw_cell(vec4 scalars0, vec4 scalars1, vec3 m, vec3 M)
{
  // common node data
  gl_TexCoord[2].xyzw = scalars0;
  gl_TexCoord[3].xyzw = scalars1;
  float cs = M.x - m.x;
  vec4 p0 = vec4(m.x,m.y,m.z,1.0);
  vec4 p1 = vec4(m.x,m.y,M.z,1.0);
  vec4 p2 = vec4(m.x,M.y,m.z,1.0);
  vec4 p3 = vec4(m.x,M.y,M.z,1.0);
  vec4 p4 = vec4(M.x,m.y,m.z,1.0);
  vec4 p5 = vec4(M.x,m.y,M.z,1.0);
  vec4 p6 = vec4(M.x,M.y,m.z,1.0);
  vec4 p7 = vec4(M.x,M.y,M.z,1.0);
  vec4 t0 = gl_ModelViewProjectionMatrix * p0;
  vec4 t1 = gl_ModelViewProjectionMatrix * p1;
  vec4 t2 = gl_ModelViewProjectionMatrix * p2;
  vec4 t3 = gl_ModelViewProjectionMatrix * p3;
  vec4 t4 = gl_ModelViewProjectionMatrix * p4;
  vec4 t5 = gl_ModelViewProjectionMatrix * p5;
  vec4 t6 = gl_ModelViewProjectionMatrix * p6;
  vec4 t7 = gl_ModelViewProjectionMatrix * p7;

  // face 0
  gl_TexCoord[0] = vec4(1.0,0.0,0.0,cs);
  gl_TexCoord[1] = p4;
  gl_Position  = t4;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,0.0,1.0,cs);
  gl_TexCoord[1] = p5;
  gl_Position  = t5;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,1.0,0.0,cs);
  gl_TexCoord[1] = p6;
  gl_Position  = t6;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,1.0,1.0,cs);
  gl_TexCoord[1] = p7;
  gl_Position  = t7;
  EmitVertex();
  EndPrimitive();
  // face 1
  gl_TexCoord[0] = vec4(0.0,1.0,0.0,cs);
  gl_TexCoord[1] = p2;
  gl_Position  = t2;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,1.0,0.0,cs);
  gl_TexCoord[1] = p6;
  gl_Position  = t6;
  EmitVertex();
  gl_TexCoord[0] = vec4(0.0,1.0,1.0,cs);
  gl_TexCoord[1] = p3;
  gl_Position  = t3;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,1.0,1.0,cs);
  gl_TexCoord[1] = p7;
  gl_Position  = t7;
  EmitVertex();
  EndPrimitive();
  // face 2
  gl_TexCoord[0] = vec4(0.0,1.0,1.0,cs);
  gl_TexCoord[1] = p3;
  gl_Position  = t3;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,1.0,1.0,cs);
  gl_TexCoord[1] = p7;
  gl_Position  = t7;
  EmitVertex();
  gl_TexCoord[0] = vec4(0.0,0.0,1.0,cs);
  gl_TexCoord[1] = p1;
  gl_Position  = t1;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,0.0,1.0,cs);
  gl_TexCoord[1] = p5;
  gl_Position  = t5;
  EmitVertex();
  EndPrimitive();
  // face 3
  gl_TexCoord[0] = vec4(0.0,1.0,0.0,cs);
  gl_TexCoord[1] = p2;
  gl_Position  = t2;
  EmitVertex();
  gl_TexCoord[0] = vec4(0.0,1.0,1.0,cs);
  gl_TexCoord[1] = p3;
  gl_Position  = t3;
  EmitVertex();
  gl_TexCoord[0] = vec4(0.0,0.0,0.0,cs);
  gl_TexCoord[1] = p0;
  gl_Position  = t0;
  EmitVertex();
  gl_TexCoord[0] = vec4(0.0,0.0,1.0,cs);
  gl_TexCoord[1] = p1;
  gl_Position  = t1;
  EmitVertex();
  EndPrimitive();
  // face 4
  gl_TexCoord[0] = vec4(0.0,0.0,0.0,cs);
  gl_TexCoord[1] = p0;
  gl_Position  = t0;
  EmitVertex();
  gl_TexCoord[0] = vec4(0.0,0.0,1.0,cs);
  gl_TexCoord[1] = p1;
  gl_Position  = t1;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,0.0,0.0,cs);
  gl_TexCoord[1] = p4;
  gl_Position  = t4;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,0.0,1.0,cs);
  gl_TexCoord[1] = p5;
  gl_Position  = t5;
  EmitVertex();
  EndPrimitive();
  // face 5
  gl_TexCoord[0] = vec4(0.0,1.0,0.0,cs);
  gl_TexCoord[1] = p2;
  gl_Position  = t2;
  EmitVertex();
  gl_TexCoord[0] = vec4(0.0,0.0,0.0,cs);
  gl_TexCoord[1] = p0;
  gl_Position  = t0;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,1.0,0.0,cs);
  gl_TexCoord[1] = p6;
  gl_Position  = t6;
  EmitVertex();
  gl_TexCoord[0] = vec4(1.0,0.0,0.0,cs);
  gl_TexCoord[1] = p4;
  gl_Position  = t4;
  EmitVertex();
  EndPrimitive();
}
