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
  int cellId, above, below, ptId, i, numEdges, edgeId;
  vlCell *cell, *edge;
  float range[2];
  float s0, s1, x0[3], x1[3], x[3], r;
  vlFloatScalars *newScalars, cellScalars(MAX_CELL_SIZE);
  vlIdList neighbors(MAX_CELL_SIZE);
  int visitedNei, nei, pts[1];

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

  inScalars->GetRange(range);
  if ( this->Value < range[0] || this->Value > range[1] )
    {
    vlWarningMacro(<<"Value lies outside of scalar range");
    return;
    }

  newPts = new vlFloatPoints(5000,10000);
  newScalars = new vlFloatScalars(5000,10000);
  newVerts = new vlCellArray(5000,10000);
//
// Traverse all edges. Since edges are not explicitly represented, use a
// trick: traverse all cells and obtain cell edges and then cell edge
// neighbors. If cell id < all edge neigbors ids, then this edge has not
// yet been visited and is processed.
//
  for (cellId=0; cellId<Input->GetNumberOfCells(); cellId++)
    {
    cell = this->Input->GetCell(cellId);
    inScalars->GetScalars(cell->PointIds,cellScalars);

    // loop over cell points to check if cell straddles iso-surface value
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
        cell->Contour(this->Value, &cellScalars, newPts, newVerts, NULL, 
                      NULL, newScalars);
        }

      else //
        {
        numEdges = cell->GetNumberOfEdges();
        for (edgeId=0; edgeId < numEdges; edgeId++)
          {
          edge = cell->GetEdge(edgeId);
          inScalars->GetScalars(edge->PointIds,cellScalars);

          s0 = cellScalars.GetScalar(0);
          s1 = cellScalars.GetScalar(1);
          if ( (s0 < this->Value && s1 >= this->Value) ||
          (s0 >= this->Value && s1 < this->Value) )
            {
            this->Input->GetCellNeighbors(cellId,edge->PointIds,neighbors);
            for (visitedNei=0, i=0; i<neighbors.GetNumberOfIds(); i++)
              {
              if ( neighbors.GetId(i) < cellId )
                {
                visitedNei = 1;
                break;
                }
              }
            if ( ! visitedNei ) //interpolate edge for point
              {
              edge->Points.GetPoint(0,x0);
              edge->Points.GetPoint(1,x1);
              r = (this->Value - s0) / (s1 - s0);
              for (i=0; i<3; i++) x[i] = x0[i] + r * (x1[i] - x0[i]);
              pts[0] = newPts->InsertNextPoint(x);
              newScalars->InsertScalar(pts[0],this->Value);
              newVerts->InsertNextCell(1,pts);
              }
            }
          } //for each edge
        } //dimension 2 and higher
      } //above and below
    } //for all cells

  vlDebugMacro(<<"Created: " << newPts->GetNumberOfPoints() << " points");
//
// Update ourselves.  Because we don't know up front how many verts we've 
// created, take care to reclaim memory. 
//
  this->SetPoints(newPts);
  this->SetVerts(newVerts);
  this->PointData.SetScalars(newScalars);

  this->Squeeze();
}

void vlEdgePoints::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Contour Value: " << this->Value << "\n";

}


