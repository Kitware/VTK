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
  vlStructuredPoints *input=(vlStructuredPoints *)this->Input;
  vlPointData *pd=input->GetPointData();
  vlScalars *inScalars=pd->GetScalars();
  short *scalars;
  int dims[3];

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
//
// Traverse all voxel cells, generating triangles and point normals
// using marching cubes algorithm.
//  



  vlDebugMacro(<<"Created: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newPolys->GetNumberOfCells() << " triangles");
//
// Update ourselves.  Because we don't know up front how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  this->SetPoints(newPts);
  this->SetPolys(newPolys);

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


