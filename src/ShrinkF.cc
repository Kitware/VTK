/*=========================================================================

  Program:   Visualization Library
  Module:    ShrinkF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Methods for shrink filter
//
#include "ShrinkF.hh"

//
// Shrink cells towards their centroid
//
void vlShrinkFilter::Execute()
{
  vlIdList ptId(25);
  vlFloatPoints pt(25);
  int i, j, k;
  float center[3], *p;
//
// Traverse all cells, obtaining node coordinates.  Compute "center" of cell,
// then create new vertices shrunk towards center.
//
/*
  for (i=0; i<this->Input->GetNumberOfCells(); i++)
    {
    // get the center of the cell
    this->CellPoints(i,ptId);
    if ( ptId.GetNumberOfIds() > 0 )
      {
      this->GetPoints(ptId, pt);
      for (center[0]=center[1]=center[2]=0.0, j=0; j<pt.GetNumberOfPoints(); j++)
        {
        p = pt.GetPoint(j);
        for (k=0; k<3; k++) center[k] += p[k];
        }
      for (k=0; k<3; k++) center[k] /= pt.GetNumberOfPoints();

      // Create new points and cells
      for (j=0; j<pt.GetNumberOfPoints(); j++)
        {
        

        }
      }
    }
*/
}

void vlShrinkFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlShrinkFilter::GetClassName()))
    {
    vlDataSetFilter::PrintSelf(os,indent);

    os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
    }
}
