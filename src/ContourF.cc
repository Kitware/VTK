/*=========================================================================

  Program:   Visualization Library
  Module:    ContourF.cc
  Language:  C++
  Date:      02/07/94
  Version:   1.4

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ContourF.hh"
#include "FScalars.hh"
#include "Cell.hh"

//
// General contouring filter.  Handles arbitrary input.
//
void vlContourFilter::Execute()
{
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  int cellId, i;
  int index;
  vlIdList *cellPts;
  vlScalars *inScalars;
  vlFloatScalars cellScalars(8);
  vlCell *cell;

//
// Check that value is within scalar range
//

//
// Loop over all cells
//
  for (cellId=0; cellId<Input->NumberOfCells(); cellId++)
    {
    cell = Input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    inScalars->GetScalars(*cellPts,cellScalars);

    // Build the case table
    for ( i=0, index = 0; i < cellPts->NumberOfIds(); i++)
	if (cellScalars.GetScalar(i) >= this->Value)
            index |= CASE_MASK[i];

    // Use different primitives to generate


    } // for all cells

}
