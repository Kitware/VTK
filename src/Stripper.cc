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
}

void vlStripper::Execute()
{
  int longest, cellId, i, numCells, numPts, numStrips;
  vlCell *cell;
  vlCellArray *newStrips;
  vlPointData *pd=this->Input->GetPointData();
  vlIdList *triPts;
  vlIdList edge(2);
  vlIdList cellIds(MAX_CELL_SIZE);
  int neighbor;
  vlPolyData Mesh(*this->Input);
  char *visited;
  int pts[MAX_CELL_SIZE];

  vlDebugMacro(<<"Executing triangle strip filter");

  this->Initialize();

  // build data structure and convert everything to triangles
  Mesh.TriangleMeshOn();
  Mesh.BuildLinks();  

  // check input
  if ( (numCells=Mesh.GetNumberOfCells()) < 1 )
    {
    vlErrorMacro(<<"No data to strip!");
    return;
    }

  newStrips = new vlCellArray;
  newStrips->Allocate(newStrips->EstimateSize(numCells,6));

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
      cell = Mesh.GetCell(cellId);
//
//  Got a starting point for the strip.  Initialize.  Find a neighbor
//  to extend strip.
//
      numStrips++;
      numPts = 3;

      triPts = cell->GetPointIds();

      for (neighbor=(-1), i=0; i<3; i++) 
        {
        pts[1] = triPts->GetId(i);
        pts[2] = triPts->GetId((i+1)%3);

        edge.SetId(0,pts[1]);
        edge.SetId(1,pts[2]);

        Mesh.GetCellNeighbors(cellId, &edge, &cellIds);
        if ( cellIds.GetNumberOfIds() > 0 && !visited[cellIds.GetId(0)] )
          {
          neighbor = cellIds.GetId(0);
          pts[0] = triPts->GetId((i+2)%3);
          break;
          }
        }
//
//  If no unvisited neighbor, just create the strip of one triangle.
//
      if ( neighbor == -1 ) 
        {
        newStrips->InsertNextCell(3,pts);
        visited[cellId] = 1;
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

          Mesh.GetCellNeighbors(cellId, &edge, &cellIds);
          if ( cellIds.GetNumberOfIds() <= 0 || visited[cellIds.GetId(0)] )
            {
            newStrips->InsertNextCell(numPts,pts);
            neighbor = (-1);
            }
          else
            {
            neighbor = cellIds.GetId(0);
            }

          } // while
        } // else continue strip
      } // if not visited
    } // for all elements
//
// Update ourselves
//
  delete [] visited;
  newStrips->Squeeze();

  this->SetStrips(newStrips);
  this->PointData = *pd; // pass data through as is

  vlDebugMacro (<<"Reduced " << numCells << " cells to " << numStrips << "triangle strips \n\t(Average " << (float)longest/numStrips << "triangles per strip, longest strip = "<<  longest << " triangles)");

}

void vlStripper::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStripper::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Maximum Strip Length: " << this->MaximumStripLength << "\n";
    }

}

