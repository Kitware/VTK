/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkProjectedTetrahedraMapper.h"

#include "vtkObjectFactory.h"
#include "vtkVisibilitySort.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkCellCenterDepthSort.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellArray.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkTimerLog.h"
#include "vtkMatrix4x4.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkGarbageCollector.h"

#include "vtkOpenGL.h"

#include <math.h>
#include <vtkstd/algorithm>

//-----------------------------------------------------------------------------

// static int tet_faces[4][3] = { {1,2,3}, {2,0,3}, {0,1,3}, {0,2,1} };
static int tet_edges[6][2] = { {0,1}, {1,2}, {2,0}, 
                               {0,3}, {1,3}, {2,3} };

//-----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkProjectedTetrahedraMapper, "1.1.2.1");
vtkStandardNewMacro(vtkProjectedTetrahedraMapper);

vtkCxxSetObjectMacro(vtkProjectedTetrahedraMapper,
                     VisibilitySort, vtkVisibilitySort);

vtkProjectedTetrahedraMapper::vtkProjectedTetrahedraMapper()
{
  this->TransformedPoints = vtkFloatArray::New();
  this->Colors = vtkUnsignedCharArray::New();
  this->VisibilitySort = vtkCellCenterDepthSort::New();

  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
  this->ArrayName = new char[1];
  this->ArrayName[0] = '\0';
  this->ArrayId = -1;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;

  this->LastVolume = NULL;

  this->OpacityTexture = 0;
  this->MaxCellSize = 0;

  this->GaveError = 0;
}

vtkProjectedTetrahedraMapper::~vtkProjectedTetrahedraMapper()
{
  this->ReleaseGraphicsResources(NULL);
  this->TransformedPoints->Delete();
  this->Colors->Delete();
  if (this->VisibilitySort) this->VisibilitySort->UnRegister(this);

  delete[] this->ArrayName;
}

void vtkProjectedTetrahedraMapper::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VisibilitySort: " << this->VisibilitySort << endl;

  os << indent << "ScalarMode: " << this->GetScalarModeAsString() << endl;
  if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
    os << indent << "ArrayId: " << this->ArrayId << endl;
    }
  else
    {
    os << indent << "ArrayName: " << this->ArrayName << endl;
    }
}

//-----------------------------------------------------------------------------

void vtkProjectedTetrahedraMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->OpacityTexture)
    {
    GLuint texid = this->OpacityTexture;
    glDeleteTextures(1, &texid);
    this->OpacityTexture = 0;
    }
  this->Superclass::ReleaseGraphicsResources(win);
}

//-----------------------------------------------------------------------------

void vtkProjectedTetrahedraMapper::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->VisibilitySort, "VisibilitySort");
}

//-----------------------------------------------------------------------------

void vtkProjectedTetrahedraMapper::SelectScalarArray(int arrayNum)
{
  if (   (this->ArrayId == arrayNum)
      && (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID) )
    {
    return;
    }
  this->Modified();

  this->ArrayId = arrayNum;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
}

void vtkProjectedTetrahedraMapper::SelectScalarArray(const char *arrayName)
{
  if (   !arrayName
      || (   (strcmp(this->ArrayName, arrayName) == 0)
          && (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID) ) )
    {
    return;
    }
  this->Modified();

  delete[] this->ArrayName;
  this->ArrayName = new char[strlen(arrayName) + 1];
  strcpy(this->ArrayName, arrayName);
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_NAME;
}

//-----------------------------------------------------------------------------

// Return the method for obtaining scalar data.
const char *vtkProjectedTetrahedraMapper::GetScalarModeAsString(void)
{
  if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
    {
    return "UseCellData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_DATA ) 
    {
    return "UsePointData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA )
    {
    return "UsePointFieldData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    return "UseCellFieldData";
    }
  else 
    {
    return "Default";
    }
}

//-----------------------------------------------------------------------------

// Given a unit opacity, converts it to an opacity for the given depth.
// Note that opacity is not the same as the attenuation.  Specifically,
//      opacity = 1 - exp(-attenuation)
//      attenuation = -ln(1 - opacity)
// The correction for opacity is simply depth * attenuation.  Therefore,
// the correction for opacity is:
//      1 - exp(-depth * (-ln(1 - opacity)))
// which resolves to
//      1 - (1 - opacity)^depth
// static inline float CorrectOpacityForDepth(float opacity, float depth)
// {
//   return (1 - (float)pow(1 - opacity, depth));
// }

void vtkProjectedTetrahedraMapper::Render(vtkRenderer *renderer,
                                          vtkVolume *volume)
{
  vtkUnstructuredGrid *input = this->GetInput();

  float last_max_cell_size = this->MaxCellSize;

  // Check to see if input changed.
  if (   (this->InputAnalyzedTime < this->MTime)
      || (this->InputAnalyzedTime < input->GetMTime()) )
    {
    this->GaveError = 0;
    float max_cell_size2 = 0;

    vtkCellArray *cells = input->GetCells();
    if (!cells)
      {
      // Apparently, the input has no cells.  Just do nothing.
      return;
      }

    vtkIdType npts, *pts;
    cells->InitTraversal();
    for (vtkIdType i = 0; cells->GetNextCell(npts, pts); i++)
      {
      int j;
      if (npts != 4)
        {
        if (!this->GaveError)
          {
          vtkErrorMacro("Encountered non-tetrahedra cell!");
          this->GaveError = 1;
          }
        continue;
        }
      for (j = 0; j < 6; j++)
        {
        double p1[3], p2[3];
        input->GetPoint(pts[tet_edges[j][0]], p1);
        input->GetPoint(pts[tet_edges[j][1]], p2);
        float size2 = (float)vtkMath::Distance2BetweenPoints(p1, p2);
        if (size2 > max_cell_size2) max_cell_size2 = size2;
        }
      }

    this->MaxCellSize = (float)sqrt(max_cell_size2);

    this->InputAnalyzedTime.Modified();
    }

  if (renderer->GetRenderWindow()->CheckAbortStatus() || this->GaveError)
    {
    return;
    }

  // Check to see if we need to rebuild opacity texture.
  if (   !this->OpacityTexture
      || (last_max_cell_size != this->MaxCellSize)
      || (this->LastVolume != volume)
      || (this->OpacityTextureTime < volume->GetMTime())
      || (this->OpacityTextureTime < volume->GetProperty()->GetMTime()) )
    {
    if (!this->OpacityTexture)
      {
      GLuint texid;
      glGenTextures(1, &texid);
      this->OpacityTexture = texid;
      }
    glBindTexture(GL_TEXTURE_2D, this->OpacityTexture);

    float unit_distance = volume->GetProperty()->GetScalarOpacityUnitDistance();

#define TEXRES  258
    float *texture = new float[TEXRES*TEXRES];
    for (int depthi = 0; depthi < TEXRES; depthi++)
      {
      if (renderer->GetRenderWindow()->CheckAbortStatus())
        {
        delete[] texture;
        return;
        }
      float depth = depthi*this->MaxCellSize/(TEXRES);
//       for (int opacityi = 0; opacityi < TEXRES; opacityi++)
//         {
//         float opacity = (float)opacityi/(TEXRES-1);
//         float alpha = CorrectOpacityForDepth(opacity, depth/unit_distance);
//         texture[(depthi*TEXRES + opacityi)] = alpha;
//         }
      for (int attenuationi = 0; attenuationi < TEXRES; attenuationi++)
        {
        float attenuation = (float)attenuationi/(TEXRES);
        float alpha = 1 - (float)exp(-attenuation*depth/unit_distance);
        texture[(depthi*TEXRES + attenuationi)] = alpha;
        }
      }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY, TEXRES, TEXRES, 1, GL_RED,
                 GL_FLOAT, texture);
    delete[] texture;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glBindTexture(GL_TEXTURE_2D, 0);

    this->OpacityTextureTime.Modified();
    }
  if (renderer->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  // Check to see if we need to remap colors.
  if (   (this->ColorsMappedTime < this->MTime)
      || (this->ColorsMappedTime < input->GetMTime())
      || (this->LastVolume != volume)
      || (this->ColorsMappedTime < volume->GetMTime())
      || (this->ColorsMappedTime < volume->GetProperty()->GetMTime()) )
    {
    vtkDataArray *scalars = this->GetScalars(input, this->ScalarMode,
                                             this->ArrayAccessMode,
                                             this->ArrayId, this->ArrayName,
                                             this->UsingCellColors);
    if (!scalars)
      {
      vtkErrorMacro(<< "Can't use projected tetrahedra without scalars!");
      return;
      }

    vtkProjectedTetrahedraMapper::MapScalarsToColors(this->Colors, volume,
                                                     scalars);

    this->ColorsMappedTime.Modified();
    this->LastVolume = volume;
    }
  if (renderer->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  this->Timer->StartTimer();

  this->ProjectTetrahedra(renderer, volume);

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
}

//-----------------------------------------------------------------------------

template<class point_type>
void vtkProjectedTetrahedraMapperTransformPoints(const point_type *in_points,
                                                 vtkIdType num_points,
                                                 const float projection_mat[16],
                                                 const float modelview_mat[16],
                                                 float *out_points);

static inline float GetCorrectedDepth(float x, float y, float z1, float z2,
                                      const float inverse_projection_mat[16],
                                      int use_linear_depth_correction,
                                      float linear_depth_correction)
{
  if (use_linear_depth_correction)
    {
    float depth = linear_depth_correction*(z1 - z2);
    if (depth < 0) depth = -depth;
    return depth;
    }
  else
    {
    float eye1[3], eye2[3], invw;

    invw = 1/(  inverse_projection_mat[ 3]*x
              + inverse_projection_mat[ 7]*y
              + inverse_projection_mat[11]*z1
              + inverse_projection_mat[15] );
    eye1[0] = invw*(  inverse_projection_mat[ 0]*x
                    + inverse_projection_mat[ 4]*y
                    + inverse_projection_mat[ 8]*z1
                    + inverse_projection_mat[12] );
    eye1[1] = invw*(  inverse_projection_mat[ 1]*x
                    + inverse_projection_mat[ 5]*y
                    + inverse_projection_mat[ 9]*z1
                    + inverse_projection_mat[13] );
    eye1[2] = invw*(  inverse_projection_mat[ 2]*x
                    + inverse_projection_mat[ 6]*y
                    + inverse_projection_mat[10]*z1
                    + inverse_projection_mat[14] );

    invw = 1/(  inverse_projection_mat[ 3]*x
              + inverse_projection_mat[ 7]*y
              + inverse_projection_mat[11]*z2
              + inverse_projection_mat[15] );
    eye2[0] = invw*(  inverse_projection_mat[ 0]*x
                    + inverse_projection_mat[ 4]*y
                    + inverse_projection_mat[ 8]*z2
                    + inverse_projection_mat[12] );
    eye2[1] = invw*(  inverse_projection_mat[ 1]*x
                    + inverse_projection_mat[ 5]*y
                    + inverse_projection_mat[ 9]*z2
                    + inverse_projection_mat[13] );
    eye2[2] = invw*(  inverse_projection_mat[ 2]*x
                    + inverse_projection_mat[ 6]*y
                    + inverse_projection_mat[10]*z2
                    + inverse_projection_mat[14] );

    return (float)sqrt(vtkMath::Distance2BetweenPoints(eye1, eye2));
    }
}

void vtkProjectedTetrahedraMapper::ProjectTetrahedra(vtkRenderer *renderer,
                                                     vtkVolume *volume)
{
  vtkUnstructuredGrid *input = this->GetInput();

  this->VisibilitySort->SetInput(input);
  this->VisibilitySort->SetDirectionToBackToFront();
  this->VisibilitySort->SetModelTransform(volume->GetMatrix());
  this->VisibilitySort->SetCamera(renderer->GetActiveCamera());
  this->VisibilitySort->SetMaxCellsReturned(1000);

  this->VisibilitySort->InitTraversal();

  if (renderer->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  float projection_mat[16];
  float modelview_mat[16];
  glGetFloatv(GL_PROJECTION_MATRIX, projection_mat);
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview_mat);

  // Get the inverse projection matrix so that we can convert distances in
  // clipping space to distances in world or eye space.
  float inverse_projection_mat[16];
  float linear_depth_correction = 1;
  int use_linear_depth_correction;
  double tmp_mat[16];

  // VTK's matrix functions use doubles.
  vtkstd::copy(projection_mat, projection_mat+16, tmp_mat);
  // VTK and OpenGL store their matrices differently.  Correct.
  vtkMatrix4x4::Transpose(tmp_mat, tmp_mat);
  // Take the inverse.
  vtkMatrix4x4::Invert(tmp_mat, tmp_mat);
  // Restore back to OpenGL form.
  vtkMatrix4x4::Transpose(tmp_mat, tmp_mat);
  // Copy back to float for faster computation.
  vtkstd::copy(tmp_mat, tmp_mat+16, inverse_projection_mat);

  // Check to see if we can just do a linear depth correction from clipping
  // space to eye space.
  use_linear_depth_correction = (   (projection_mat[ 3] == 0.0)
                                 && (projection_mat[ 7] == 0.0)
                                 && (projection_mat[11] == 0.0)
                                 && (projection_mat[15] == 1.0) );
  if (use_linear_depth_correction)
    {
    float pos1[3], *pos2;

    pos1[0] = inverse_projection_mat[8] + inverse_projection_mat[12];
    pos1[1] = inverse_projection_mat[9] + inverse_projection_mat[13];
    pos1[2] = inverse_projection_mat[10] + inverse_projection_mat[14];

    pos2 = inverse_projection_mat + 12;

    linear_depth_correction = sqrt(vtkMath::Distance2BetweenPoints(pos1, pos2));
    }

  // Transform all the points.
  vtkIdType num_points = input->GetNumberOfPoints();
  this->TransformedPoints->SetNumberOfComponents(3);
  this->TransformedPoints->SetNumberOfTuples(num_points);
  float *points = this->TransformedPoints->GetPointer(0);
  switch (input->GetPoints()->GetDataType())
    {
    vtkTemplateMacro
      (vtkProjectedTetrahedraMapperTransformPoints(
                          (const VTK_TT *)input->GetPoints()->GetVoidPointer(0),
                           num_points,
                           projection_mat, modelview_mat,
                           points));
    }

  if (renderer->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  glDisable(GL_LIGHTING);
  glDepthMask(GL_FALSE);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, this->OpacityTexture);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Establish vertex arrays.
  float tet_points[5*3];
  glVertexPointer(3, GL_FLOAT, 0, tet_points);
  glEnableClientState(GL_VERTEX_ARRAY);

  unsigned char tet_colors[5*3];
  glColorPointer(3, GL_UNSIGNED_BYTE, 0, tet_colors);
  glEnableClientState(GL_COLOR_ARRAY);

  float tet_texcoords[5*2];
  glTexCoordPointer(2, GL_FLOAT, 0, tet_texcoords);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // Since we had to transform the points on the CPU, replace the OpenGL
  // transforms with the identity matrix.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  unsigned char *colors = this->Colors->GetPointer(0);
  vtkIdType *cells = input->GetCells()->GetPointer();
  vtkIdType totalnumcells = input->GetNumberOfCells();
  vtkIdType numcellsrendered = 0;

  // Let's do it!
  for (vtkIdTypeArray *sorted_cell_ids = this->VisibilitySort->GetNextCells();
       sorted_cell_ids != NULL;
       sorted_cell_ids = this->VisibilitySort->GetNextCells())
    {
    this->UpdateProgress((double)numcellsrendered/totalnumcells);
    if (renderer->GetRenderWindow()->CheckAbortStatus())
      {
      break;
      }
    vtkIdType *cell_ids = sorted_cell_ids->GetPointer(0);
    vtkIdType num_cell_ids = sorted_cell_ids->GetNumberOfTuples();
    for (vtkIdType i = 0; i < num_cell_ids; i++)
      {
      vtkIdType cell = cell_ids[i];
      int j;

      // Get the data for the tetrahedra.
      for (j = 0; j < 4; j++)
        {
        // Assuming we only have tetrahedra, each entry in cells has 5
        // components.
        const float *p = points + 3*cells[5*cell + j + 1];
        tet_points[j*3 + 0] = p[0];
        tet_points[j*3 + 1] = p[1];
        tet_points[j*3 + 2] = p[2];

        const unsigned char *c;
        if (this->UsingCellColors)
          {
          c = colors + 4*cell;
          }
        else
          {
          c = colors + 4*cells[5*cell + j + 1];
          }
        tet_colors[j*3 + 0] = c[0];
        tet_colors[j*3 + 1] = c[1];
        tet_colors[j*3 + 2] = c[2];

        tet_texcoords[j*2 + 0] = (float)c[3]/255;
        tet_texcoords[j*2 + 1] = 0;
        }

      // The classic PT algorithm uses face normals to determine the
      // projection class and then do calculations individually.  However,
      // Wylie 2002 shows how to use the intersection of two segments to
      // calculate the depth of the thick part for any case.  Here, we use
      // face normals to determine which segments to use.  One segment
      // should be between two faces that are either both front facing or
      // back facing.  Obviously, we only need to test three faces to find
      // two such faces.  We test the three faces connected to point 0.
      vtkIdType segment1[2];
      vtkIdType segment2[2];

      float v1[2], v2[2], v3[3];
      v1[0] = tet_points[1*3 + 0] - tet_points[0*3 + 0];
      v1[1] = tet_points[1*3 + 1] - tet_points[0*3 + 1];
      v2[0] = tet_points[2*3 + 0] - tet_points[0*3 + 0];
      v2[1] = tet_points[2*3 + 1] - tet_points[0*3 + 1];
      v3[0] = tet_points[3*3 + 0] - tet_points[0*3 + 0];
      v3[1] = tet_points[3*3 + 1] - tet_points[0*3 + 1];

      float face_dir1 = v3[0]*v2[1] - v3[1]*v2[0];
      float face_dir2 = v1[0]*v3[1] - v1[1]*v3[0];
      float face_dir3 = v2[0]*v1[1] - v2[1]*v1[0];

      if (   (face_dir1 * face_dir2 >= 0)
          && (   (face_dir1 != 0)       // Handle a special case where 2 faces
              || (face_dir2 != 0) ) )   // are perpendicular to the view plane.
        {
        segment1[0] = 0;  segment1[1] = 3;
        segment2[0] = 1;  segment2[1] = 2;
        }
      else if (face_dir1 * face_dir3 >= 0)
        {
        segment1[0] = 0;  segment1[1] = 2;
        segment2[0] = 1;  segment2[1] = 3;
        }
      else      // Unless the tet is degenerate, face_dir2*face_dir3 >= 0
        {
        segment1[0] = 0;  segment1[1] = 1;
        segment2[0] = 2;  segment2[1] = 3;
        }

#define VEC3SUB(Z,X,Y)          \
  (Z)[0] = (X)[0] - (Y)[0];     \
  (Z)[1] = (X)[1] - (Y)[1];     \
  (Z)[2] = (X)[2] - (Y)[2];
#define P1 (tet_points + 3*segment1[0])
#define P2 (tet_points + 3*segment1[1])
#define P3 (tet_points + 3*segment2[0])
#define P4 (tet_points + 3*segment2[1])
#define C1 (tet_colors + 3*segment1[0])
#define C2 (tet_colors + 3*segment1[1])
#define C3 (tet_colors + 3*segment2[0])
#define C4 (tet_colors + 3*segment2[1])
#define T1 (tet_texcoords + 2*segment1[0])
#define T2 (tet_texcoords + 2*segment1[1])
#define T3 (tet_texcoords + 2*segment2[0])
#define T4 (tet_texcoords + 2*segment2[1])
      // Find the intersection of the projection of the two segments in the
      // XY plane.  This algorithm is based on that given in Graphics Gems
      // III, pg. 199-202.
      float A[3], B[3], C[3];
      // We can define the two lines parametrically as:
      //        P1 + alpha(A)
      //        P3 + beta(B)
      // where A = P2 - P1
      // and   B = P4 - P3.
      // alpha and beta are in the range [0,1] within the line segment.
      VEC3SUB(A, P2, P1);
      VEC3SUB(B, P4, P3);
      // The lines intersect when the values of the two parameteric equations
      // are equal.  Setting them equal and moving everything to one side:
      //        0 = C + beta(B) - alpha(A)
      // where C = P3 - P1.
      VEC3SUB(C, P3, P1);
      // When we project the lines to the xy plane (which we do by throwing
      // away the z value), we have two equations and two unkowns.  The
      // following are the solutions for alpha and beta.
      float alpha = (B[1]*C[0]-B[0]*C[1])/(A[0]*B[1]-A[1]*B[0]);
      float beta = (A[1]*C[0]-A[0]*C[1])/(A[0]*B[1]-A[1]*B[0]);

      if ((alpha >= 0) && (alpha <= 1))
        {
        // The two segments intersect.  This corresponds to class 2 in
        // Shirley and Tuchman (or one of the degenerate cases).

        // Make new point at intersection.
        tet_points[3*4 + 0] = P1[0] + alpha*A[0];
        tet_points[3*4 + 1] = P1[1] + alpha*A[1];
        tet_points[3*4 + 2] = P1[2] + alpha*A[2];

        // Find depth at intersection.
        float depth = GetCorrectedDepth(tet_points[3*4 + 0],
                                        tet_points[3*4 + 1],
                                        tet_points[3*4 + 2],
                                        P3[2] + beta*B[2],
                                        inverse_projection_mat,
                                        use_linear_depth_correction,
                                        linear_depth_correction);

        // Find color at intersection.
        tet_colors[3*4 + 0] =
          (unsigned char)(0.5f*(  C1[0] + alpha*(C2[0]-C1[0])
                                + C3[0] +  beta*(C4[0]-C3[0]) ));
        tet_colors[3*4 + 1] =
          (unsigned char)(0.5f*(  C1[1] + alpha*(C2[1]-C1[1])
                                + C3[1] +  beta*(C4[1]-C3[1]) ));
        tet_colors[3*4 + 2] =
          (unsigned char)(0.5f*(  C1[2] + alpha*(C2[2]-C1[2])
                                + C3[2] +  beta*(C4[2]-C3[2]) ));

//         tet_colors[3*0 + 0] = 255;
//         tet_colors[3*0 + 1] = 0;
//         tet_colors[3*0 + 2] = 0;
//         tet_colors[3*1 + 0] = 255;
//         tet_colors[3*1 + 1] = 0;
//         tet_colors[3*1 + 2] = 0;
//         tet_colors[3*2 + 0] = 255;
//         tet_colors[3*2 + 1] = 0;
//         tet_colors[3*2 + 2] = 0;
//         tet_colors[3*3 + 0] = 255;
//         tet_colors[3*3 + 1] = 0;
//         tet_colors[3*3 + 2] = 0;
//         tet_colors[3*4 + 0] = 255;
//         tet_colors[3*4 + 1] = 0;
//         tet_colors[3*4 + 2] = 0;

        // Find the opacity at intersection.
        tet_texcoords[2*4 + 0] = 0.5f*(  T1[0] + alpha*(T2[0]-T1[0])
                                       + T3[0] + alpha*(T4[0]-T3[0]));

        // Record the depth at the intersection.
        tet_texcoords[2*4 + 1] = depth/this->MaxCellSize;

        // Establish the order in which the points should be rendered.
        unsigned char gl_indices[6];
        gl_indices[0] = 4;
        gl_indices[1] = segment1[0];
        gl_indices[2] = segment2[0];
        gl_indices[3] = segment1[1];
        gl_indices[4] = segment2[1];
        gl_indices[5] = segment1[0];

        // Render
        glDrawElements(GL_TRIANGLE_FAN, 6, GL_UNSIGNED_BYTE, gl_indices);
        }
      else
        {
        // The two segments do not intersect.  This corresponds to class 1
        // in Shirley and Tuchman.
        if (alpha <= 0)
          {
          // Flip segment1 so that alpha is >= 1.  P1 and P2 are also
          // flipped as are C1-C2 and T1-T2.  Note that this will
          // invalidate A.  B and beta are unaffected.
          vtkstd::swap(segment1[0], segment1[1]);
          alpha = 1 - alpha;
          }
        // From here on, we can assume P2 is the "thick" point.

        // Find the depth under the thick point.  Use the alpha and beta
        // from intersection to determine location of face under thick
        // point.
        float edgez = P3[2] + beta*B[2];
        float pointz = P1[2];
        float facez = (edgez + (alpha-1)*pointz)/alpha;
        float depth = GetCorrectedDepth(P2[0], P2[1], P2[2], facez,
                                        inverse_projection_mat,
                                        use_linear_depth_correction,
                                        linear_depth_correction);

        // Fix color at thick point.  Average color with color of opposite
        // face.
        for (j = 0; j < 3; j++)
          {
          float edgec = C3[j] + beta*(C4[j]-C3[j]);
          float pointc = C1[j];
          float facec = (edgec + (alpha-1)*pointc)/alpha;
          C2[j] = (unsigned char)(0.5f*(facec + C2[j]));
          }

//         tet_colors[3*segment1[0] + 0] = 0;
//         tet_colors[3*segment1[0] + 1] = 255;
//         tet_colors[3*segment1[0] + 2] = 0;
//         tet_colors[3*segment1[1] + 0] = 0;
//         tet_colors[3*segment1[1] + 1] = 255;
//         tet_colors[3*segment1[1] + 2] = 0;
//         tet_colors[3*segment2[0] + 0] = 0;
//         tet_colors[3*segment2[0] + 1] = 255;
//         tet_colors[3*segment2[0] + 2] = 0;
//         tet_colors[3*segment2[1] + 0] = 0;
//         tet_colors[3*segment2[1] + 1] = 255;
//         tet_colors[3*segment2[1] + 2] = 0;

        // Fix opacity at thick point.  Average opacity with opacity of
        // opposite face.
        float edgea = T3[0] + beta*(T4[0]-T3[0]);
        float pointa = T1[0];
        float facea = (edgea + (alpha-1)*pointa)/alpha;
        T2[0] = 0.5f*(facea + T2[0]);

        // Record thickness at thick point.
        T2[1] = depth/this->MaxCellSize;

        // Establish the order in which the points should be rendered.
        unsigned char gl_indices[5];
        gl_indices[0] = segment1[1];
        gl_indices[1] = segment1[0];
        gl_indices[2] = segment2[0];
        gl_indices[3] = segment2[1];
        gl_indices[4] = segment1[0];

        // Render
        glDrawElements(GL_TRIANGLE_FAN, 5, GL_UNSIGNED_BYTE, gl_indices);
        }
      }
    numcellsrendered += num_cell_ids;
    }

  // Restore OpenGL state.
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(projection_mat);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(modelview_mat);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  glDepthMask(GL_TRUE);
  glEnable(GL_LIGHTING);

  this->UpdateProgress(1.0);
}

//-----------------------------------------------------------------------------

template<class point_type>
void vtkProjectedTetrahedraMapperTransformPoints(const point_type *in_points,
                                                 vtkIdType num_points,
                                                 const float projection_mat[16],
                                                 const float modelview_mat[16],
                                                 float *out_points)
{
  float mat[16];
  int row, col;
  vtkIdType i;
  const point_type *in_p;
  float *out_p;

  // Combine two transforms into one transform.
  for (col = 0; col < 4; col++)
    {
    for (row = 0; row < 4; row++)
      {
      mat[col*4+row] = (  projection_mat[0*4+row]*modelview_mat[col*4+0]
                        + projection_mat[1*4+row]*modelview_mat[col*4+1]
                        + projection_mat[2*4+row]*modelview_mat[col*4+2]
                        + projection_mat[3*4+row]*modelview_mat[col*4+3]);
      }
    }

  // Transform all points.
  for (i = 0, in_p = in_points, out_p = out_points; i < num_points;
       i++, in_p += 3, out_p += 3)
    {
    for (row = 0; row < 3; row++)
      {
      out_p[row] = (  mat[0*4+row]*in_p[0] + mat[1*4+row]*in_p[1]
                    + mat[2*4+row]*in_p[2] + mat[3*4+row]);
      }
    }

  // Check to see if we need to divide by w.
  if (   (mat[0*4+3] != 0) || (mat[1*4+3] != 0)
      || (mat[0*4+3] != 0) || (mat[1*4+3] != 1) )
    {
    for (i = 0, in_p = in_points, out_p = out_points; i < num_points;
         i++, in_p += 3, out_p += 3)
      {
      float w = (  mat[0*4+3]*in_p[0] + mat[1*4+3]*in_p[1]
                 + mat[2*4+3]*in_p[2] + mat[3*4+3]);
      out_p[0] /= w;
      out_p[1] /= w;
      out_p[2] /= w;
      }
    }
}

//-----------------------------------------------------------------------------

namespace vtkProjectedTetrahedraMapperNamespace
{
  template<class ColorType>
  void MapScalarsToColors1(ColorType *colors, vtkVolumeProperty *property,
                           vtkDataArray *scalars);
  template<class ColorType, class ScalarType>
  void MapScalarsToColors2(ColorType *colors, vtkVolumeProperty *property,
                           ScalarType *scalars,
                           int num_scalar_components,
                           vtkIdType num_scalars);
  template<class ColorType, class ScalarType>
  void MapIndependentComponents(ColorType *colors,
                                vtkVolumeProperty *property,
                                ScalarType *scalars,
                                int num_scalar_components,
                                vtkIdType num_scalars);
  template<class ColorType, class ScalarType>
  void Map2DependentComponents(ColorType *colors, ScalarType *scalars,
                               vtkIdType num_scalars);
  template<class ColorType, class ScalarType>
  void Map4DependentComponents(ColorType *colors, ScalarType *scalars,
                               vtkIdType num_scalars);
}

void vtkProjectedTetrahedraMapper::MapScalarsToColors(vtkDataArray *colors,
                                                      vtkVolume *volume,
                                                      vtkDataArray *scalars)
{
  using namespace vtkProjectedTetrahedraMapperNamespace;

  vtkDataArray *tmpColors;
  int castColors;

  if (   (colors->GetDataType() == VTK_UNSIGNED_CHAR)
      && (   (scalars->GetDataType() != VTK_UNSIGNED_CHAR)
          || (volume->GetProperty()->GetIndependentComponents()) ) )
    {
    // Special case.  Need to convert from range [0,1] to [0,255].
    tmpColors = vtkDoubleArray::New();
    castColors = 1;
    }
  else
    {
    tmpColors = colors;
    castColors = 0;
    }

  vtkIdType numscalars = scalars->GetNumberOfTuples();

  tmpColors->Initialize();
  tmpColors->SetNumberOfComponents(4);
  tmpColors->SetNumberOfTuples(numscalars);

  void *colorpointer = tmpColors->GetVoidPointer(0);
  switch (tmpColors->GetDataType())
    {
    vtkTemplateMacro(MapScalarsToColors1(static_cast<VTK_TT *>(colorpointer),
                                         volume->GetProperty(), scalars));
    }

  if (castColors)
    {
    // Special case.  Need to convert from range [0,1] to [0,255].
    colors->Initialize();
    colors->SetNumberOfComponents(4);
    colors->SetNumberOfTuples(scalars->GetNumberOfTuples());

    unsigned char *c
      = static_cast<vtkUnsignedCharArray *>(colors)->GetPointer(0);

    for (vtkIdType i = 0; i < numscalars; i++, c+= 4)
      {
      double *dc = tmpColors->GetTuple(i);
      c[0] = static_cast<unsigned char>(dc[0]*255.9999);
      c[1] = static_cast<unsigned char>(dc[1]*255.9999);
      c[2] = static_cast<unsigned char>(dc[2]*255.9999);
      c[3] = static_cast<unsigned char>(dc[3]*255.9999);
      }

    tmpColors->Delete();
    }
}

namespace vtkProjectedTetrahedraMapperNamespace
{

  template<class ColorType>
  void MapScalarsToColors1(ColorType *colors, vtkVolumeProperty *property,
                           vtkDataArray *scalars)
  {
    void *scalarpointer = scalars->GetVoidPointer(0);
    switch(scalars->GetDataType())
      {
      vtkTemplateMacro(MapScalarsToColors2(colors, property,
                                           static_cast<VTK_TT *>(scalarpointer),
                                           scalars->GetNumberOfComponents(),
                                           scalars->GetNumberOfTuples()));
      }
  }

  template<class ColorType, class ScalarType>
  void MapScalarsToColors2(ColorType *colors, vtkVolumeProperty *property,
                           ScalarType *scalars,
                           int num_scalar_components, vtkIdType num_scalars)
  {
    if (property->GetIndependentComponents())
      {
      MapIndependentComponents(colors, property,
                               scalars, num_scalar_components, num_scalars);
      }
    else
      {
      switch (num_scalar_components)
        {
        case 2:
          Map2DependentComponents(colors, scalars, num_scalars);
          break;
        case 4:
          Map4DependentComponents(colors, scalars, num_scalars);
          break;
        default:
          vtkGenericWarningMacro("Attempted to map scalar with "
                                 << num_scalar_components
                                 << " with dependent components");
          break;
        }
      }
  }

  template<class ColorType, class ScalarType>
  void MapIndependentComponents(ColorType *colors,
                                vtkVolumeProperty *property,
                                ScalarType *scalars,
                                int num_scalar_components,
                                vtkIdType num_scalars)
  {
    // I don't really know what to do if there is more than one component.
    // How am I supposed to mix the resulting colors?  Since I don't know
    // what to do, and the whole thing seems kinda pointless anyway, I'm just
    // going to punt and copy over the first scalar.
    ColorType *c = colors;
    ScalarType *s = scalars;
    vtkIdType i;

    if (property->GetColorChannels() == 1)
      {
      vtkPiecewiseFunction *gray = property->GetGrayTransferFunction();
      vtkPiecewiseFunction *alpha = property->GetScalarOpacity();

      for (i = 0; i < num_scalars; i++, c += 4, s += num_scalar_components)
        {
        c[0] = c[1] = c[2] = static_cast<ColorType>(gray->GetValue(s[0]));
        c[3] = static_cast<ColorType>(alpha->GetValue(s[0]));
        }
      }
    else
      {
      vtkColorTransferFunction *rgb = property->GetRGBTransferFunction();
      vtkPiecewiseFunction *alpha = property->GetScalarOpacity();

      for (i = 0; i < num_scalars; i++, c += 4, s += num_scalar_components)
        {
        double trgb[3];
        rgb->GetColor(s[0], trgb);
        c[0] = static_cast<ColorType>(trgb[0]);
        c[1] = static_cast<ColorType>(trgb[1]);
        c[2] = static_cast<ColorType>(trgb[2]);
        c[3] = static_cast<ColorType>(alpha->GetValue(s[0]));
        }
      }
  }

  template<class ColorType, class ScalarType>
  void Map2DependentComponents(ColorType *colors, ScalarType *scalars,
                               vtkIdType num_scalars)
  {
    for (vtkIdType i = 0; i < num_scalars; i++)
      {
      colors[0] = colors[1] = colors[2] = static_cast<ColorType>(scalars[0]);
      colors[3] = static_cast<ColorType>(scalars[3]);

      colors += 4;
      scalars += 2;
      }
  }

  template<class ColorType, class ScalarType>
  void Map4DependentComponents(ColorType *colors, ScalarType *scalars,
                               vtkIdType num_scalars)
  {
    for (vtkIdType i = 0; i < num_scalars; i++)
      {
      colors[0] = static_cast<ColorType>(scalars[0]);
      colors[1] = static_cast<ColorType>(scalars[1]);
      colors[2] = static_cast<ColorType>(scalars[2]);
      colors[3] = static_cast<ColorType>(scalars[3]);

      colors += 4;
      scalars += 4;
      }
  }

}
