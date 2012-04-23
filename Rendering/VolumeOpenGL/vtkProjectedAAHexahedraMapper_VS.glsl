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

// inputs of VS
// vertexpos.xyz : cell position (xmin,ymin,zmin)
// texcoord0.xyz : cell position (xmax,ymax,zmax)
// texcoord1.xyzw: node data 0,1,2,3
// texcoord2.xyzw: node data 4,5,6,7

#version 110

void main()
{
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_TexCoord[1] = gl_MultiTexCoord1;
  gl_TexCoord[2] = gl_MultiTexCoord2;
  gl_Position = gl_Vertex;
}
