//VTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkStructuredGridLIC2D_fs.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

// Filename: vtkStructuredGridLIC2D_fs.glsl
// Filename is useful when using gldb-gui

// the output of this shader
//VTK::Output::Dec

varying vec2 tcoordVC;

/*
For an input structure grid, this computes the inverse jacobian for each point.
*/

uniform sampler2D texPoints;  // point coordinates
uniform sampler2D texVectorField; // vector field.
uniform vec3 uDimensions;     // structured dimensions; initially == (width, height, 1)

uniform int uSlice; // 0,1,2

ivec3 getIJK(vec3 ninjnk, vec3 dims)
{
  return ivec3(floor(ninjnk * (dims - 1.0) + vec3(0.5, 0.5, 0.5)));
}

vec3 getVector(ivec3 ijk, vec3 dims, sampler2D field)
{
  // ignoring k component for now since dims =  (width, height, 1)
  // not any more.
  vec3 rcoord = vec3(ijk) / max(vec3(1.0), dims - 1.0);
  vec2 tcoord;

  if(uSlice == 0)
   {
    tcoord.xy = rcoord.yz;
   }
  else
  {
   if(uSlice == 1)
    {
     tcoord.xy = rcoord.xz;
    }
    else
    {
     tcoord.xy = rcoord.xy;
    }
  }

  return texture2D(field, tcoord).xyz;
}

float vtkDeterminant(mat3 m)
{
  // develop determinant along first row.
  return m[0][0]*(m[2][2]*m[1][1] - m[2][1]*m[1][2])
       - m[1][0]*(m[2][2]*m[0][1] - m[2][1]*m[0][2])
       + m[2][0]*(m[1][2]*m[0][1] - m[1][1]*m[0][2]);
}

mat3 vtkInverse(mat3 mm, float det)
{
  mat3 m=transpose(mm);

  mat3 adjM = mat3(
    m[2][2]*m[1][1]-m[2][1]*m[1][2], -(m[2][2]*m[0][1]-m[2][1]*m[0][2]),  m[1][2]*m[0][1]-m[1][1]*m[0][2],
  -(m[2][2]*m[1][0]-m[2][0]*m[1][2]),  m[2][2]*m[0][0]-m[2][0]*m[0][2], -(m[1][2]*m[0][0]-m[1][0]*m[0][2]),
    m[2][1]*m[1][0]-m[2][0]*m[1][1], -(m[2][1]*m[0][0]-m[2][0]*m[0][1]),  m[1][1]*m[0][0]-m[1][0]*m[0][1]
  );

  return adjM / det;
}

mat3 vtkJacobian(ivec3 ijk, vec3 dims, sampler2D tex)
{
  // Jacobian is estimated with a central finite difference technique.
  /*
  | dx/di, dx/dj, dx/dk |
  | dy/di, dy/dj, dy/dk |
  | dz/di, dz/dj, dz/dk |
  */

  //(i-1, j, k), (i+1, j, k)
  vec3 pts_IM1_J_K = getVector(ivec3(ijk.x-1, ijk.yz), dims, tex);
  vec3 pts_I1_J_K = getVector(ivec3(ijk.x+1, ijk.yz), dims, tex);

  //   (i, j-1, k), (i, j+1, k)
  vec3 pts_I_JM1_K = getVector(ivec3(ijk.x, ijk.y-1, ijk.z), dims, tex);
  vec3 pts_I_J1_K = getVector(ivec3(ijk.x, ijk.y+1, ijk.z), dims, tex);

  // (i, j, k-1), (i, j, k+1).
  vec3 pts_I_J_KM1 = getVector(ivec3(ijk.xy, ijk.z-1), dims, tex);
  vec3 pts_I_J_K1 = getVector(ivec3(ijk.xy, ijk.z+1), dims, tex);

  vec3 col1 = 0.5 * (pts_I1_J_K - pts_IM1_J_K);
  vec3 col2 = 0.5 * (pts_I_J1_K - pts_I_JM1_K);
  vec3 col3 = 0.5 * (pts_I_J_K1 - pts_I_J_KM1);

 if(uSlice == 0)
  {
    col1[0] = 1.0;
  }
 else
  {
     if(uSlice == 1)
      {
      col2[1] = 1.0;
      }
     else
      {
      col3[2] = 1.0;
      }
  }

  mat3 J = mat3(col1, col2, col3);
  return J;
}

void main(void)
{
  // determine the structured coordinate for the current location.
  vec3 tcoord;
  if (uSlice == 0)
  {
   tcoord = vec3(0, tcoordVC.st);
  }
  else
  {
   if (uSlice == 1)
    {
     tcoord = vec3(tcoordVC.s, 0, tcoordVC.t);
    }
   else
    {
     tcoord = vec3(tcoordVC.st, 0);
    }
  }

  ivec3 ijk = getIJK(tcoord, uDimensions);

  // compute partial derivative for X.
  mat3 J = vtkJacobian(ijk, uDimensions, texPoints);

  // compute inverse of J.
  vec3 vector = getVector(ijk, uDimensions, texVectorField);
  float detJ = vtkDeterminant(J);
  mat3 invJ = vtkInverse(J, detJ);
  gl_FragData[0] = vec4(invJ * vector, 1.0);
}
