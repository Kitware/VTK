/*=========================================================================

  Program:   Visualization Library
  Module:    EdgePts.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "EdgePts.hh"

// Description:
// Construct object with contour value of 0.0.
vlEdgePoints::vlEdgePoints()
{
  this->Value = 0.0;
}

vlEdgePoints::~vlEdgePoints()
{
}

//
// General filter: handles arbitrary input.
//
void vlEdgePoints::Execute()
{
  vlScalars *inScalars;
  vlFloatPoints *newPts;
  vlCellArray *newVerts;

  vlDebugMacro(<< "Generating edge points");
//
// Initialize and check input
//
  this->Initialize();

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
    {
    vlErrorMacro(<<"No scalar data to contour");
    return;
    }

  range = inScalars->GetRange();
  if ( value < range[0] || value > range[1] )
    {
    vlErrorMacro(<<"Value lies outside of scalar range");
    return;
    }
//
// Traverse all edges. Since edges are not explicitly represented, use a
// trick: traverse all cells and obtain cell edges and then cell edge
// neighbors. If cell id < all edge neigbors ids, then this edge has not
// yet been visited and is processed.
//
  for (cellId=0; cellId<Input->GetNumberOfCells(); cellId++)
    {
    cell = Input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    inScalars->GetScalars(*cellPts,cellScalars);

    // loop over 8 points of voxel to check if cell straddles value
    for ( above=below=0, ptId=0; ptId < cell->GetNumberOfPoints(); ptId++ )
      {
      if ( cellScalars.GetScalar(ptId) >= this->Value )
        above = 1;
      else if ( cellScalars.GetScalar(ptId) < this->Value )
        below = 1;
      }

    if ( above && below ) //contour passes through cell
      {
      if ( cell->GetCellDimension() < 2 ) //only points can be generated
        {
        cell->Contour(value, &cellScalars, newPts, newVerts, NULL, 
                      NULL, newScalars);
        }

      else //
        {
        numEdges = cell->GetNumberOfEdges();
        for (edgeId=0; edgeId < numEdges; edgeId++)
          {
          edge = cell->GetEdge(edgeId);
          inScalars->GetScalars(edge->PointIds,cellScalars);

          if ( ((s0=cellScalars.GetScalar(0)) < this->Value &&
          (s1=cellScalars.GetScalar(1)) >= this->Value) ||
          (s0 >= this->Value && s1 < this->Value) )
            {
            this->GetCellNeighbors(cellId,edge->PointIds,neighbors);
            for (i=0; i<neighbors.GetNumberOf
            
          }

        }
      }
    }

  vlDebugMacro(<<"Created: " << newPts->GetNumberOfPoints() << " points");
//
// Update ourselves.  Because we don't know up front how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  this->SetPoints(newPts);
  this->SetVerts(newVerts);

  this->Squeeze();
}

void vlEdgePoints::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Contour Values: " << this->Value << "\n";

}


