/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiscreteMarchingCubes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.


     THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
     "System and Method for the Display of Surface Structures Contained
     Within the Interior Region of a Solid Body".
     Application of this software for commercial purposes requires 
     a license grant from GE. Contact:

         Carl B. Horton
         Sr. Counsel, Intellectual Property
         3000 N. Grandview Blvd., W-710
         Waukesha, WI  53188
         Phone:  (262) 513-4022
         E-Mail: Carl.Horton@med.ge.com

     for more information.

=========================================================================*/

#include "vtkDiscreteMarchingCubes.h"

#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMarchingCubesCases.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkCellData.h"
#include "vtkShortArray.h"
#include "vtkStructuredPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#ifndef vtkFloatingPointType
#define vtkFloatingPointType float
#endif

vtkCxxRevisionMacro(vtkDiscreteMarchingCubes, "1.1");
vtkStandardNewMacro(vtkDiscreteMarchingCubes);

// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0. ComputeNormals is off, ComputeGradients is off and ComputeScalars is on.
vtkDiscreteMarchingCubes::vtkDiscreteMarchingCubes()
{
  this->ComputeNormals = 0;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;
}

vtkDiscreteMarchingCubes::~vtkDiscreteMarchingCubes()
{
}


//
// Contouring filter specialized for volumes and "short int" data values.  
//
template <class T>
void vtkDiscreteMarchingCubesComputeGradient(
  vtkDiscreteMarchingCubes *self,T *scalars, int dims[3], 
  vtkFloatingPointType origin[3], vtkFloatingPointType Spacing[3],
  vtkPointLocator *locator, 
  vtkDataArray *newCellScalars, 
  vtkCellArray *newPolys, vtkFloatingPointType *values, 
  int numValues)
{
  vtkFloatingPointType s[8], value;
  int i, j, k, sliceSize;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  vtkMarchingCubesTriangleCases *triCase, *triCases;
  EDGE_LIST  *edge;
  int contNum, jOffset, kOffset, idx, ii, index, *vert;
  vtkIdType ptIds[3];
  vtkIdType cellId;
  int ComputeScalars = newCellScalars != NULL;
  vtkFloatingPointType t, *x1, *x2, x[3], min, max;
  vtkFloatingPointType pts[8][3], xp, yp, zp;
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  triCases =  vtkMarchingCubesTriangleCases::GetCases();

//
// Get min/max contour values
//
  if ( numValues < 1 )
    {
    return;
    }
  for ( min=max=values[0], i=1; i < numValues; i++)
    {
    if ( values[i] < min )
      {
      min = values[i];
      }
    if ( values[i] > max )
      {
      max = values[i];
      }
    }
//
// Traverse all voxel cells, generating triangles
// using marching cubes algorithm.
//  
  sliceSize = dims[0] * dims[1];
  for ( k=0; k < (dims[2]-1); k++)
    {
    self->UpdateProgress ((vtkFloatingPointType) k / ((vtkFloatingPointType) dims[2] - 1));
    if (self->GetAbortExecute())
      {
      break;
      }
    kOffset = k*sliceSize;
    pts[0][2] = origin[2] + k*Spacing[2];
    zp = origin[2] + (k+1)*Spacing[2];
    for ( j=0; j < (dims[1]-1); j++)
      {
      jOffset = j*dims[0];
      pts[0][1] = origin[1] + j*Spacing[1];
      yp = origin[1] + (j+1)*Spacing[1];
      for ( i=0; i < (dims[0]-1); i++)
        {
        //get scalar values
        idx = i + jOffset + kOffset;
        s[0] = scalars[idx];
        s[1] = scalars[idx+1];
        s[2] = scalars[idx+1 + dims[0]];
        s[3] = scalars[idx + dims[0]];
        s[4] = scalars[idx + sliceSize];
        s[5] = scalars[idx+1 + sliceSize];
        s[6] = scalars[idx+1 + dims[0] + sliceSize];
        s[7] = scalars[idx + dims[0] + sliceSize];

        if ( (s[0] < min && s[1] < min && s[2] < min && s[3] < min &&
              s[4] < min && s[5] < min && s[6] < min && s[7] < min) ||
             (s[0] > max && s[1] > max && s[2] > max && s[3] > max &&
              s[4] > max && s[5] > max && s[6] > max && s[7] > max) )
          {
          continue; // no contours possible
          }

        //create voxel points
        pts[0][0] = origin[0] + i*Spacing[0];
        xp = origin[0] + (i+1)*Spacing[0];

        pts[1][0] = xp;
        pts[1][1] = pts[0][1];
        pts[1][2] = pts[0][2];

        pts[2][0] = xp;
        pts[2][1] = yp;
        pts[2][2] = pts[0][2];

        pts[3][0] = pts[0][0];
        pts[3][1] = yp;
        pts[3][2] = pts[0][2];

        pts[4][0] = pts[0][0];
        pts[4][1] = pts[0][1];
        pts[4][2] = zp;

        pts[5][0] = xp;
        pts[5][1] = pts[0][1];
        pts[5][2] = zp;

        pts[6][0] = xp;
        pts[6][1] = yp;
        pts[6][2] = zp;

        pts[7][0] = pts[0][0];
        pts[7][1] = yp;
        pts[7][2] = zp;

        for (contNum=0; contNum < numValues; contNum++)
          {
          value = values[contNum];
          // Build the case table
          for ( ii=0, index = 0; ii < 8; ii++)
            {
            // for discrete marching cubes, we are looking for an
            // exact match of a scalar at a vertex to a value
            if ( s[ii] == value )
              {
              index |= CASE_MASK[ii];
              }
            }
          if ( index == 0 || index == 255 ) //no surface
            {
            continue;
            }

          triCase = triCases+ index;
          edge = triCase->edges;

          for ( ; edge[0] > -1; edge += 3 )
            {
            for (ii=0; ii<3; ii++) //insert triangle
              {
              vert = edges[edge[ii]];
              // for discrete marching cubes, the interpolation point
              // is always 0.5.
              t = 0.5;
              x1 = pts[vert[0]];
              x2 = pts[vert[1]];
              x[0] = x1[0] + t * (x2[0] - x1[0]);
              x[1] = x1[1] + t * (x2[1] - x1[1]);
              x[2] = x1[2] + t * (x2[2] - x1[2]);

              // add point
              locator->InsertUniquePoint(x, ptIds[ii]);
              }
            // check for degenerate triangle
            if ( ptIds[0] != ptIds[1] &&
                 ptIds[0] != ptIds[2] &&
                 ptIds[1] != ptIds[2] )
                {
                cellId = newPolys->InsertNextCell(3,ptIds);
                // Note that DiscreteMarchingCubes stores the scalar
                // data in the cells. It does not use the point data
                // since cells from different labeled segments may use
                // the same point.
                if (ComputeScalars)
                  {
                  newCellScalars->InsertNextTuple(&value);
                  }
                }
            }//for each triangle
          }//for all contours
        }//for i
      }//for j
    }//for k
}

//
// Contouring filter specialized for volumes and "short int" data values.  
//
void vtkDiscreteMarchingCubes::Execute()
{
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  vtkFloatArray *newCellScalars;
  vtkImageData *input = this->GetInput();
  vtkPointData *pd;
  vtkDataArray *inScalars;
  int dims[3];
  int estimatedSize;
  vtkFloatingPointType Spacing[3], origin[3];
  vtkFloatingPointType bounds[6];
  vtkPolyData *output = this->GetOutput();
  int numContours=this->ContourValues->GetNumberOfContours();
  vtkFloatingPointType *values=this->ContourValues->GetValues();
  
  vtkDebugMacro(<< "Executing marching cubes");

//
// Initialize and check input
//
  if (input == NULL)
    {
    vtkErrorMacro(<<"Input is NULL");
    return;
    }
  pd=input->GetPointData();
  if (pd ==NULL)
    {
    vtkErrorMacro(<<"PointData is NULL");
    return;
    }
  inScalars=pd->GetScalars();
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return;
    }

  if ( input->GetDataDimension() != 3 )
    {
    vtkErrorMacro(<<"Cannot contour data of dimension != 3");
    return;
    }
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(Spacing);

  // estimate the number of points from the volume dimensions
  estimatedSize = (int) pow ((vtkFloatingPointType) (dims[0] * dims[1] * dims[2]), .75);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }
  vtkDebugMacro(<< "Estimated allocation size is " << estimatedSize);
  newPts = vtkPoints::New(); newPts->Allocate(estimatedSize,estimatedSize/2);
  // compute bounds for merging points
  for ( int i=0; i<3; i++)
    {
    bounds[2*i] = origin[i];
    bounds[2*i+1] = origin[i] + (dims[i]-1) * Spacing[i];
    }
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, bounds, estimatedSize);

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(estimatedSize,3));


  if (this->ComputeScalars)
    {
    newCellScalars = vtkFloatArray::New();
    newCellScalars->Allocate(estimatedSize,3);
    }
  else
    {
    newCellScalars = NULL;
    }

  if (inScalars->GetNumberOfComponents() == 1 )
    {
    switch (inScalars->GetDataType())
      {
      case VTK_CHAR:
      {
      char *scalars = static_cast<vtkCharArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_UNSIGNED_CHAR:
      {
      unsigned char *scalars = static_cast<vtkUnsignedCharArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_SHORT:
      {
      short *scalars = static_cast<vtkShortArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_UNSIGNED_SHORT:
      {
      unsigned short *scalars = static_cast<vtkUnsignedShortArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_INT:
      {
      int *scalars = static_cast<vtkIntArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_UNSIGNED_INT:
      {
      unsigned int *scalars = static_cast<vtkUnsignedIntArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_LONG:
      {
      long *scalars = static_cast<vtkLongArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_UNSIGNED_LONG:
      {
      unsigned long *scalars = static_cast<vtkUnsignedLongArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_FLOAT:
      {
      float *scalars = 
        static_cast<vtkFloatArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      case VTK_DOUBLE:
      {
      double *scalars = 
        static_cast<vtkDoubleArray *>(inScalars)->GetPointer(0);
      vtkDiscreteMarchingCubesComputeGradient(this,
                                              scalars,
                                              dims,
                                              origin,
                                              Spacing,
                                              this->Locator,
                                              newCellScalars,
                                              newPolys,
                                              values,
                                              numContours);
      }
      break;
      } //switch
    }

  else //multiple components - have to convert
    {
    int dataSize = dims[0] * dims[1] * dims[2];
    vtkDoubleArray *image=vtkDoubleArray::New(); 
    image->SetNumberOfComponents(inScalars->GetNumberOfComponents());
    image->SetNumberOfTuples(image->GetNumberOfComponents()*dataSize);
    inScalars->GetTuples(0,dataSize,image);

    double *scalars = image->GetPointer(0);
    vtkDiscreteMarchingCubesComputeGradient(this,
                                            scalars,
                                            dims,
                                            origin,
                                            Spacing,
                                            this->Locator,
                                            newCellScalars,
                                            newPolys,
                                            values,
                                            numContours);
    image->Delete();
    }
  
  vtkDebugMacro(<<"Created: " 
                << newPts->GetNumberOfPoints() << " points, " 
                << newPolys->GetNumberOfCells() << " triangles");
  //
  // Update ourselves.  Because we don't know up front how many triangles
  // we've created, take care to reclaim memory. 
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  if (newCellScalars)
    {
    output->GetCellData()->SetScalars(newCellScalars);
    newCellScalars->Delete();
    }
  output->Squeeze();
  if (this->Locator)
    {
    this->Locator->Initialize(); //free storage
    }
}
