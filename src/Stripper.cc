/*=========================================================================

  Program:   Visualization Library
  Module:    Stripper.cc
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
#include "Stripper.hh"

vlStripper::vlStripper()
{
  this->MaximumStripLength = MAX_CELL_SIZE;

  this->PassVerts = 1;
  this->PassLines = 1;
}

void vlStripper::Execute()
{
  int longest, cellId, i, numCells, numPts, numStrips;
  vlCell *cell;
  vlCellArray *newStrips, *inStrips;
  vlPointData *pd=this->Input->GetPointData();
  vlIdList *triPts;
  vlIdList edge(2);
  vlIdList cellIds(MAX_CELL_SIZE);
  int neighbor;
  vlPolyData Mesh;
  char *visited;
  int pts[MAX_CELL_SIZE];
  int numStripPts, *stripPts;

  vlDebugMacro(<<"Executing triangle strip filter");

  this->Initialize();

  // build cell structure.  Only operate with polygons and triangle strips.
  Mesh.SetPoints(this->Input->GetPoints());
  Mesh.SetPolys(this->Input->GetPolys());
  Mesh.SetStrips(this->Input->GetStrips());

  // check input
  if ( (numCells=Mesh.GetNumberOfCells()) < 1 )
    {
    vlErrorMacro(<<"No data to strip!");
    return;
    }

  newStrips = new vlCellArray;
  newStrips->Allocate(newStrips->EstimateSize(numCells,6));

  // pre-load existing strips
  inStrips = this->Input->GetStrips();
  for (inStrips->InitTraversal(); inStrips->GetNextCell(numStripPts,stripPts); )
    {
    newStrips->InsertNextCell(numStripPts,stripPts);
    }

  // array keeps track of data that's been visited
  visited = new char[numCells];
  for (i=0; i < numCells; i++) visited[i] = 0;

//
//  Loop over all elements and find one that hasn't been visited.
//  Start a triangle strip and mark as visited, and then find a
//  neighbor that isn't visited.  Add this to the strip and mark as
//  visited (and so on).
//
  for (longest=3, numStrips=0, cellId=0; cellId < numCells; cellId++)
    {
    if ( ! visited[cellId] )
      {
      visited[cellId] = 1;
      cell = Mesh.GetCell(cellId);
      if ( cell->GetCellType() == vlTRIANGLE )
        {
//
//  Got a starting point for the strip.  Initialize.  Find a neighbor
//  to extend strip.
//
        numStrips++;
        numPts = 3;

        triPts = cell->GetPointIds();

        for (i=0; i<3; i++) 
          {
          pts[1] = triPts->GetId(i);
          pts[2] = triPts->GetId((i+1)%3);

          edge.SetId(0,pts[1]);
          edge.SetId(1,pts[2]);

          Mesh.GetCellNeighbors(cellId, edge, cellIds);
          if ( cellIds.GetNumberOfIds() > 0 && 
          !visited[neighbor=cellIds.GetId(0)] &&
          Mesh.GetCellType(neighbor) == vlTRIANGLE )
            {
            pts[0] = triPts->GetId((i+2)%3);
            break;
            }
          }
//
//  If no unvisited neighbor, just create the strip of one triangle.
//
        if ( i >= 3 ) 
          {
          newStrips->InsertNextCell(3,pts);
          } 
        else // continue strip 
          { 
//
//  Have a neighbor.  March along grabbing new points
//
          while ( neighbor >= 0 && numPts < (this->MaximumStripLength-1) )
            {
            visited[neighbor] = 1;
            cell = Mesh.GetCell(neighbor);
            triPts = cell->GetPointIds();

            for (i=0; i<3; i++)
              if ( triPts->GetId(i) != pts[numPts-2] && 
              triPts->GetId(i) != pts[numPts-1] )
                break;

            pts[numPts] = triPts->GetId(i);
            edge.SetId(0,pts[numPts]);
            edge.SetId(1,pts[numPts-1]);
            if ( ++numPts > longest ) longest = numPts;

            Mesh.GetCellNeighbors(neighbor, edge, cellIds);
            // note: if updates value of neighbor
            if ( cellIds.GetNumberOfIds() <= 0 || 
            visited[neighbor=cellIds.GetId(0)] ||
            Mesh.GetCellType(neighbor) != vlTRIANGLE )
              {
              newStrips->InsertNextCell(numPts,pts);
              neighbor = (-1);
              }
            } // while
          } // else continue strip
        } // if triangle
      } // if not visited
    } // for all elements
//
// Update ourselves
//
  delete [] visited;

  this->SetPoints(this->Input->GetPoints());
  this->PointData = *pd; // pass data through as is

  newStrips->Squeeze();
  this->SetStrips(newStrips);

  // pass through verts and lines
  this->SetVerts(this->Input->GetVerts());
  this->SetLines(this->Input->GetLines());

  vlDebugMacro (<<"Reduced " << numCells << " cells to " << numStrips << " triangle strips \n\t(Average " << (float)numCells/numStrips << " triangles per strip, longest strip = "<<  (longest-2) << " triangles)");

}

void vlStripper::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStripper::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Maximum Strip Length: " << this->MaximumStripLength << "\n";
    }

}

