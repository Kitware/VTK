/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MCubes.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "MCubes.hh"
#include "MC_Cases.h"
#include "StrPts.hh"
#include "SScalars.hh"
#include "vtkMath.hh"

// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkMarchingCubes::vtkMarchingCubes()
{
  for (int i=0; i<MAX_CONTOURS; i++) this->Values[i] = 0.0;
  this->NumberOfContours = 1;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
}

vtkMarchingCubes::~vtkMarchingCubes()
{
}

// Description:
// Set a particular contour value at contour number i.
void vtkMarchingCubes::SetValue(int i, float value)
{
  i = (i >= MAX_CONTOURS ? MAX_CONTOURS-1 : (i < 0 ? 0 : i) );
  if ( this->Values[i] != value )
    {
    this->Modified();
    this->Values[i] = value;
    if ( i >= this->NumberOfContours ) this->NumberOfContours = i + 1;
    if ( value < this->Range[0] ) this->Range[0] = value;
    if ( value > this->Range[1] ) this->Range[1] = value;
    }
}

// Description:
// Generate numContours equally spaced contour values between specified
// range.
void vtkMarchingCubes::GenerateValues(int numContours, float range[2])
{
  float val, incr;
  int i;

  numContours = (numContours > MAX_CONTOURS ? MAX_CONTOURS : 
                 (numContours > 1 ? numContours : 2) );

  incr = (range[1] - range[0]) / (numContours-1);
  for (i=0, val=range[0]; i < numContours; i++, val+=incr)
    {
    this->SetValue(i,val);
    }
}

// Description:
// Generate numContours equally spaced contour values between specified
// range.
void vtkMarchingCubes::GenerateValues(int numContours, float r1, float r2)
{
  float rng[2];

  rng[0] = r1;
  rng[1] = r2;
  this->GenerateValues(numContours,rng);
}

void ComputePointGradient(int i, int j, int k, short *s, int dims[3], 
                      int sliceSize, float origin[3], float aspectRatio[3], float n[3])
{
  float sp, sm;

  // x-direction
  if ( i == 0 )
    {
    sp = s[i+1 + j*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[0] = (sp - sm) / aspectRatio[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i-1 + j*dims[0] + k*sliceSize];
    n[0] = (sp - sm) / aspectRatio[0];
    }
  else
    {
    sp = s[i+1 + j*dims[0] + k*sliceSize];
    sm = s[i-1 + j*dims[0] + k*sliceSize];
    n[0] = 0.5 * (sp - sm) / aspectRatio[0];
    }

  // y-direction
  if ( j == 0 )
    {
    sp = s[i + (j+1)*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[1] = (sp - sm) / aspectRatio[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i + (j-1)*dims[0] + k*sliceSize];
    n[1] = (sp - sm) / aspectRatio[1];
    }
  else
    {
    sp = s[i + (j+1)*dims[0] + k*sliceSize];
    sm = s[i + (j-1)*dims[0] + k*sliceSize];
    n[1] = 0.5 * (sp - sm) / aspectRatio[1];
    }

  // z-direction
  if ( k == 0 )
    {
    sp = s[i + j*dims[0] + (k+1)*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[2] = (sp - sm) / aspectRatio[2];
    }
  else if ( k == (dims[2]-1) )
    {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + (k-1)*sliceSize];
    n[2] = (sp - sm) / aspectRatio[2];
    }
  else
    {
    sp = s[i + j*dims[0] + (k+1)*sliceSize];
    sm = s[i + j*dims[0] + (k-1)*sliceSize];
    n[2] = 0.5 * (sp - sm) / aspectRatio[2];
    }

}

//
// Contouring filter specialized for volumes and "short int" data values.  
//
void vtkMarchingCubes::Execute()
{
  static vtkMath math;
  vtkFloatPoints *newPts;
  vtkCellArray *newPolys;
  vtkShortScalars *newScalars;
  vtkFloatNormals *newNormals;
  vtkFloatVectors *newGradients;
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;
  vtkPointData *pd=input->GetPointData();
  vtkScalars *inScalars=pd->GetScalars();
  short *scalars, s[8], value;
  int dims[3];
  float aspectRatio[3], origin[3];
  int i, j, k, sliceSize;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int contNum, jOffset, kOffset, idx, ii, jj, index, *vert;
  int ptIds[3];
  float t, *x1, *x2, x[3], *n1, *n2, n[3];
  float pts[8][3], gradients[8][3];
  static int edges[12][2] = { {0,1}, {1,2}, {2,3}, {3,0},
                              {4,5}, {5,6}, {6,7}, {7,4},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  vtkDebugMacro(<< "Executing marching cubes");
  this->Initialize();
//
// Initialize and check input
//
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
  input->GetAspectRatio(aspectRatio);

  if ( strcmp("short",inScalars->GetDataType()) )
    {
    vtkErrorMacro(<<"Scalars must be short ints...");
    return;
    }
  scalars = ((vtkShortScalars *)inScalars)->GetPtr(0);

  newPts = new vtkFloatPoints(10000,50000);
  newScalars = new vtkShortScalars(10000,50000);
  newNormals = new vtkFloatNormals(10000,50000);
  newGradients = new vtkFloatVectors(10000,50000);
  newPolys = new vtkCellArray();
  newPolys->Allocate(newPolys->EstimateSize(25000,3));
//
// Traverse all voxel cells, generating triangles and point gradients
// using marching cubes algorithm.
//  
  sliceSize = dims[0] * dims[1];
  for (contNum=0; contNum < this->NumberOfContours; contNum++)
    {
    value = (short) this->Values[contNum];
    for ( k=0; k < (dims[2]-1); k++)
      {
      kOffset = k*sliceSize;
      pts[0][2] = origin[2] + k*aspectRatio[2];
      for ( j=0; j < (dims[1]-1); j++)
        {
        jOffset = j*dims[0];
        pts[0][1] = origin[1] + j*aspectRatio[1];
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

          // Build the case table
          for ( ii=0, index = 0; ii < 8; ii++)
              if ( s[ii] >= value )
                  index |= CASE_MASK[ii];

          if ( index == 0 || index == 255 ) continue; //no surface

          //create voxel points
          pts[0][0] = origin[0] + i*aspectRatio[0];

          pts[1][0] = pts[0][0] + aspectRatio[0];  
          pts[1][1] = pts[0][1];
          pts[1][2] = pts[0][2];

          pts[2][0] = pts[0][0] + aspectRatio[0];  
          pts[2][1] = pts[0][1] + aspectRatio[1];
          pts[2][2] = pts[0][2];

          pts[3][0] = pts[0][0];
          pts[3][1] = pts[0][1] + aspectRatio[1];
          pts[3][2] = pts[0][2];

          pts[4][0] = pts[0][0];
          pts[4][1] = pts[0][1];
          pts[4][2] = pts[0][2] + aspectRatio[2];

          pts[5][0] = pts[0][0] + aspectRatio[0];  
          pts[5][1] = pts[0][1];
          pts[5][2] = pts[0][2] + aspectRatio[2];

          pts[6][0] = pts[0][0] + aspectRatio[0];  
          pts[6][1] = pts[0][1] + aspectRatio[1];
          pts[6][2] = pts[0][2] + aspectRatio[2];

          pts[7][0] = pts[0][0];
          pts[7][1] = pts[0][1] + aspectRatio[1];
          pts[7][2] = pts[0][2] + aspectRatio[2];

          //create gradients
          ComputePointGradient(i,j,k, scalars, dims, sliceSize, origin, aspectRatio, gradients[0]);
          ComputePointGradient(i+1,j,k, scalars, dims, sliceSize, origin, aspectRatio, gradients[1]);
          ComputePointGradient(i+1,j+1,k, scalars, dims, sliceSize, origin, aspectRatio, gradients[2]);
          ComputePointGradient(i,j+1,k, scalars, dims, sliceSize, origin, aspectRatio, gradients[3]);
          ComputePointGradient(i,j,k+1, scalars, dims, sliceSize, origin, aspectRatio, gradients[4]);
          ComputePointGradient(i+1,j,k+1, scalars, dims, sliceSize, origin, aspectRatio, gradients[5]);
          ComputePointGradient(i+1,j+1,k+1, scalars, dims, sliceSize, origin, aspectRatio, gradients[6]);
          ComputePointGradient(i,j+1,k+1, scalars, dims, sliceSize, origin, aspectRatio, gradients[7]);

          triCase = triCases + index;
          edge = triCase->edges;

          for ( ; edge[0] > -1; edge += 3 )
            {
            for (ii=0; ii<3; ii++) //insert triangle
              {
              vert = edges[edge[ii]];
              t = (float)(value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
              x1 = pts[vert[0]];
              x2 = pts[vert[1]];
              n1 = gradients[vert[0]];
              n2 = gradients[vert[1]];
              for (jj=0; jj<3; jj++)
                {
                x[jj] = x1[jj] + t * (x2[jj] - x1[jj]);
                n[jj] = n1[jj] + t * (n2[jj] - n1[jj]);
                }
              ptIds[ii] = newPts->InsertNextPoint(x);
              newScalars->InsertScalar(ptIds[ii],value);
              newGradients->InsertVector(ptIds[ii],n);
	      math.Normalize(n);
              newNormals->InsertNormal(ptIds[ii],n);
              }
            newPolys->InsertNextCell(3,ptIds);
            }//for each triangle
          }//for i
        }//for j
      }//for k
    }//for all contours

  vtkDebugMacro(<<"Created: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newPolys->GetNumberOfCells() << " triangles");
//
// Update ourselves.  Because we don't know up front how many triangles
// we've created, take care to reclaim memory. 
//
  this->SetPoints(newPts);
  this->SetPolys(newPolys);
  this->PointData.SetScalars(newScalars);
  this->PointData.SetVectors(newGradients);
  this->PointData.SetNormals(newNormals);
  this->Squeeze();
}

void vtkMarchingCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Number Of Contours : " << this->NumberOfContours << "\n";
  os << indent << "Contour Values: \n";
  for ( i=0; i<this->NumberOfContours; i++)
    {
    os << indent << "  Value " << i << ": " << this->Values[i] << "\n";
    }
}


