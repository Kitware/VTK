/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiscreteMarchingCubes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDiscreteMarchingCubes.h"

#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMarchingCubesTriangleCases.h"
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
#include "vtkStreamingDemandDrivenPipeline.h"

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
  double origin[3], double spacing[3],
  vtkIncrementalPointLocator *locator,
  vtkDataArray *newCellScalars,
  vtkCellArray *newPolys, double *values,
  int numValues)
{
  double s[8], value;
  int i, j, k;
  vtkIdType sliceSize, rowSize;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  vtkMarchingCubesTriangleCases *triCase, *triCases;
  EDGE_LIST  *edge;
  int contNum, ii, index, *vert;
  vtkIdType jOffset, kOffset, idx;
  vtkIdType ptIds[3];
  int extent[6];
  int ComputeScalars = newCellScalars != NULL;
  double t, *x1, *x2, x[3], min, max;
  double pts[8][3], xp, yp, zp;
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  vtkInformation *inInfo = self->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent);

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
  rowSize = dims[0];
  sliceSize = rowSize*dims[1];
  for ( k=0; k < (dims[2]-1); k++)
    {
    self->UpdateProgress(static_cast<double>(k)/(dims[2] - 1));
    if (self->GetAbortExecute())
      {
      break;
      }
    kOffset = k*sliceSize;
    pts[0][2] = origin[2] + (k+extent[4])*spacing[2];
    zp = pts[0][2] + spacing[2];
    for ( j=0; j < (dims[1]-1); j++)
      {
      jOffset = j*rowSize;
      pts[0][1] = origin[1] + (j+extent[2])*spacing[1];
      yp = pts[0][1] + spacing[1];
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
        pts[0][0] = origin[0] + (i+extent[0])*spacing[0];
        xp = pts[0][0] + spacing[0];

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
                newPolys->InsertNextCell(3,ptIds);
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
int vtkDiscreteMarchingCubes::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkPoints *newPts;
  vtkCellArray *newPolys;
  vtkFloatArray *newCellScalars;
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData *pd;
  vtkDataArray *inScalars;
  int dims[3], extent[6];
  vtkIdType estimatedSize;
  double spacing[3], origin[3];
  double bounds[6];
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  int numContours=this->ContourValues->GetNumberOfContours();
  double *values=this->ContourValues->GetValues();

  vtkDebugMacro(<< "Executing marching cubes");

  // initialize and check input
  pd=input->GetPointData();
  if (pd ==NULL)
    {
    vtkErrorMacro(<<"PointData is NULL");
    return 1;
    }
  inScalars=pd->GetScalars();
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return 1;
    }

  if ( input->GetDataDimension() != 3 )
    {
    vtkErrorMacro(<<"Cannot contour data of dimension != 3");
    return 1;
    }
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  // estimate the number of points from the volume dimensions
  estimatedSize = dims[0];
  estimatedSize *= dims[1]; // The "*=" ensures coercion to vtkIdType,
  estimatedSize *= dims[2]; // which might be wider than "int"
  estimatedSize = static_cast<vtkIdType>(
    pow(static_cast<double>(estimatedSize), .75));
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }
  vtkDebugMacro(<< "Estimated allocation size is " << estimatedSize);
  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize/2);

  // compute bounds for merging points
  for ( int i=0; i<3; i++)
    {
    bounds[2*i] = origin[i] + extent[2*i] * spacing[i];
    bounds[2*i+1] = origin[i] + extent[2*i+1] * spacing[i];
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
    void* scalars = inScalars->GetVoidPointer(0);
    switch (inScalars->GetDataType())
      {
      vtkTemplateMacro(
        vtkDiscreteMarchingCubesComputeGradient(this,
                                                static_cast<VTK_TT*>(scalars),
                                                dims, origin, spacing,
                                                this->Locator, newCellScalars,
                                                newPolys, values, numContours)
        );
      } //switch
    }

  else //multiple components - have to convert
    {
    vtkIdType dataSize = dims[0];
    dataSize *= dims[1]; // The "*=" ensures coercion to vtkIdType,
    dataSize *= dims[2]; // which might be wider than "int".
    vtkDoubleArray *image=vtkDoubleArray::New();
    image->SetNumberOfComponents(inScalars->GetNumberOfComponents());
    image->SetNumberOfTuples(image->GetNumberOfComponents()*dataSize);
    inScalars->GetTuples(0,dataSize,image);

    double *scalars = image->GetPointer(0);
    vtkDiscreteMarchingCubesComputeGradient(this,
                                            scalars,
                                            dims,
                                            origin,
                                            spacing,
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

  return 1;
}
