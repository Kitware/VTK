/*=========================================================================

  Program:   Visualization Library
  Module:    OutlineF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "OutlineF.hh"

void vlOutlineFilter::Execute()
{
  float *bounds;
  float x[3];
  int pts[2];
  vlFloatPoints *newPts;
  vlCellArray *newLines;

  vlDebugMacro(<< "Creating dataset outline");
//
// Initialize
//
  this->Initialize();
  bounds = this->Input->GetBounds();
//
// Allocate storage and create outline
//
  newPts = new vlFloatPoints(8);
  newLines = new vlCellArray;
  newLines->Initialize(newLines->EstimateSize(12,2));

  x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
  newPts->InsertPoint(0,x);
  x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
  newPts->InsertPoint(1,x);
  x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
  newPts->InsertPoint(2,x);
  x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
  newPts->InsertPoint(3,x);
  x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
  newPts->InsertPoint(4,x);
  x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
  newPts->InsertPoint(5,x);
  x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
  newPts->InsertPoint(6,x);
  x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
  newPts->InsertPoint(7,x);

  pts[0] = 0; pts[1] = 1;
  newLines->InsertNextCell(2,pts);
  pts[0] = 2; pts[1] = 3;
  newLines->InsertNextCell(2,pts);
  pts[0] = 4; pts[1] = 5;
  newLines->InsertNextCell(2,pts);
  pts[0] = 6; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  pts[0] = 0; pts[1] = 2;
  newLines->InsertNextCell(2,pts);
  pts[0] = 1; pts[1] = 3;
  newLines->InsertNextCell(2,pts);
  pts[0] = 4; pts[1] = 6;
  newLines->InsertNextCell(2,pts);
  pts[0] = 5; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  pts[0] = 0; pts[1] = 4;
  newLines->InsertNextCell(2,pts);
  pts[0] = 1; pts[1] = 5;
  newLines->InsertNextCell(2,pts);
  pts[0] = 2; pts[1] = 6;
  newLines->InsertNextCell(2,pts);
  pts[0] = 3; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
//
// Update selves
//
  this->SetPoints(newPts);
  this->SetLines(newLines);

}
