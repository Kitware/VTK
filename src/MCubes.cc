/*=========================================================================

  Program:   Visualization Library
  Module:    MCubes.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "MCubes.hh"
#include "MC_Cases.h"
#include "StrPts.hh"
#include "SScalars.hh"

// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0.
vlMarchingCubes::vlMarchingCubes()
{
  for (int i=0; i<MAX_CONTOURS; i++) this->Values[i] = 0.0;
  this->NumberOfContours = 1;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
}

vlMarchingCubes::~vlMarchingCubes()
{
}

// Description:
// Set a particular contour value at contour number i.
void vlMarchingCubes::SetValue(int i, float value)
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
void vlMarchingCubes::GenerateValues(int numContours, float range[2])
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

//
// Contouring filter specialized for volumes and "short int" data values.  
//
void vlMarchingCubes::Execute()
{
  vlFloatPoints *newPts;
  vlCellArray *newPolys;
  vlShortScalars *newScalars;
  vlStructuredPoints *input=(vlStructuredPoints *)this->Input;
  vlPointData *pd=input->GetPointData();
  vlScalars *inScalars=pd->GetScalars();
  short *scalars, s[8], value;
  int dims[3];
  float ar[3], origin[3];
  int i, j, k, sliceSize;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int contNum, jOffset, kOffset, idx, ii, jj, index, *vert;
  int ptIds[3];
  float t, *x1, *x2, x[3];
  float pts[8][3];
  static int edges[12][2] = { {0,1}, {1,2}, {2,3}, {3,0},
                              {4,5}, {5,6}, {6,7}, {7,4},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  vlDebugMacro(<< "Executing marching cubes");
  this->Initialize();
//
// Initialize and check input
//
  if ( inScalars == NULL )
    {
    vlErrorMacro(<<"Scalars must be defined for contouring");
    return;
    }

  if ( input->GetDataDimension() != 3 )
    {
    vlErrorMacro(<<"Cannot contour data of dimension != 3");
    return;
    }
  input->GetDimensions(dims);

  if ( strcmp("short",inScalars->GetDataType()) )
    {
    vlErrorMacro(<<"Scalars must be short ints...");
    return;
    }
  scalars = ((vlShortScalars *)inScalars)->GetPtr(0);

  newPts = new vlFloatPoints(10000,50000);
  newScalars = new vlShortScalars(10000,50000);
  newPolys = new vlCellArray();
  newPolys->Allocate(newPolys->EstimateSize(25000,3));
//
// Traverse all voxel cells, generating triangles and point normals
// using marching cubes algorithm.
//  
  sliceSize = dims[0] * dims[1];
  for (contNum=0; contNum < this->NumberOfContours; contNum++)
    {
    value = (short) this->Values[contNum];
    for ( k=0; k < (dims[2]-1); k++)
      {
      kOffset = k*sliceSize;
      pts[0][2] = origin[2] + k*ar[2];
      for ( j=0; j < (dims[1]-1); j++)
        {
        jOffset = j*dims[0];
        pts[0][1] = origin[1] + j*ar[1];
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
                  index |= CASE_MASK[i];

          if ( index == 0 || index == 255 ) continue; //no surface

          //create voxel points
          pts[0][0] = origin[0] + i*ar[0];

          pts[1][0] = pts[0][0] + ar[0];  
          pts[1][1] = pts[0][1];
          pts[1][2] = pts[0][2];

          pts[2][0] = pts[0][0] + ar[0];  
          pts[2][1] = pts[0][1] + ar[1];
          pts[2][2] = pts[0][2];

          pts[3][0] = pts[0][0];
          pts[3][1] = pts[0][1] + ar[1];
          pts[3][2] = pts[0][2];

          pts[4][0] = pts[0][0];
          pts[4][1] = pts[0][1];
          pts[4][2] = pts[0][2] + ar[2];

          pts[5][0] = pts[0][0] + ar[0];  
          pts[5][1] = pts[0][1];
          pts[5][2] = pts[0][2] + ar[2];

          pts[6][0] = pts[0][0] + ar[0];  
          pts[6][1] = pts[0][1] + ar[1];
          pts[6][2] = pts[0][2] + ar[2];

          pts[7][0] = pts[0][0];
          pts[7][1] = pts[0][1] + ar[1];
          pts[7][2] = pts[0][2] + ar[2];

          triCase = triCases + index;
          edge = triCase->edges;

          for ( ; edge[0] > -1; edge += 3 )
            {
            for (ii=0; ii<3; ii++) //insert triangle
              {
              vert = edges[edge[ii]];
              t = (value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
              x1 = pts[vert[0]];
              x2 = pts[vert[1]];
              for (jj=0; jj<3; jj++) x[jj] = x1[jj] + t * (x2[jj] - x1[jj]);
              ptIds[ii] = newPts->InsertNextPoint(x);
              newScalars->InsertNextScalar(value);
              }
            newPolys->InsertNextCell(3,ptIds);
            }//for each triangle
          }//for i
        }//for j
      }//for k
    }//for all contours

  vlDebugMacro(<<"Created: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newPolys->GetNumberOfCells() << " triangles");
//
// Update ourselves.  Because we don't know up front how many triangles
// we've created, take care to reclaim memory. 
//
  this->SetPoints(newPts);
  this->SetPolys(newPolys);
  this->PointData.SetScalars(newScalars);
  this->Squeeze();
}

void vlMarchingCubes::PrintSelf(ostream& os, vlIndent indent)
{
  int i;

  vlStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Number Of Contours : " << this->NumberOfContours << "\n";
  os << indent << "Contour Values: \n";
  for ( i=0; i<this->NumberOfContours; i++)
    {
    os << indent << "  Value " << i << ": " << this->Values[i] << "\n";
    }
}


