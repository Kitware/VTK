/*=========================================================================

  Program:   Visualization Library
  Module:    DCubes.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DCubes.hh"

vlDividingCubes::vlDividingCubes()
{
  this->Value = 0.0;
  this->Distance = 0.1;
}

void vlDividingCubes::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlDividingCubes::GetClassName()))
    {
    vlStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

    os << indent << "Value: " << this->Value << "\n";
    os << indent << "Distance: " << this->Distance << "\n";
    }
}


void vlDividingCubes::Execute()
{
  int cellId;
  vlCell *brick;
  vlPointData *pd;
  vlFloatPoints *newPts;
  vlScalars *inScalars;
  vlIdList *brickPts;
  vlFloatScalars brickScalars(8);
  float len2;
  vlCellArray *newVerts;
  int above, below, vertNum;
  vlStructuredPoints *input=(vlStructuredPoints *)this->Input;

  vlDebugMacro(<< "Executing Dividing Cubes");
//
// Initialize self; check input; create output objects
//
  this->Initialize();

  // make sure we have scalar data
  if ( ! (inScalars = input->GetPointData()->GetScalars()) )
    {
    vlErrorMacro(<<"No scalar data to contour");
    return;
    }

  // just deal with volume data
  if ( input->GetDataDimension() != 3 )
    {
    vlErrorMacro("Bad input: only treats 3D point sets");
    return;
    }

  // creating points
  newPts = new vlFloatPoints(5000,25000);
  newVerts = new vlCellArray(5000,25000);

  // prepare to interpolate data
  pd = input->GetPointData();
  this->PointData.InterpolateAllocate(pd,5000,25000);

//
// Loop over all cells checking to see which straddle the specified value
//
  for ( cellId=0; cellId = input->GetNumberOfCells(); cellId++ )
    {
    brick = input->GetCell(cellId);
    brickPts = brick->GetPointIds();
    inScalars->GetScalars(*brickPts,brickScalars);

    // loop over 8 points of brick to check if cell straddles value
    for ( above=below=0, vertNum=0; vertNum < 8; vertNum++ )
      {
      if ( brickScalars.GetScalar(vertNum) > this->Value )
        above = 1;
      else if ( brickScalars.GetScalar(vertNum) < this->Value )
        below = 1;

      if ( above && below ) // recursively generate points
        {
        len2 = brick->GetLength2();
//        this->SubDivide();
        }
      }
    }    
//
// Update ourselves
//
  newPts->Squeeze();
  this->SetPoints(newPts);

  newVerts->Squeeze();
  this->SetVerts(newVerts);
}

void vlDividingCubes::SubDivide(float pMin[3], float pMax[3], float values[8], 
                                float distance2, vlCell& cell)
{

}

void vlDividingCubes::AddPoint(float pcoords[3], vlCell& cell)
{

}

