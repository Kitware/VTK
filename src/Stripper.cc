/*=========================================================================

  Program:   Visualization Library
  Module:    Stripper.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Stripper.hh"

// Description:
// Construct object with vertex and line passing turned on.
vlStripper::vlStripper()
{
  this->MaximumStripLength = MAX_CELL_SIZE - 2;

  this->PassVerts = 1;
  this->PassLines = 1;
}

void vlStripper::Execute()
{
  int longest, cellId, i, numCells, numPts, numStrips;
  vlCellArray *newStrips, *inStrips;
  vlPointData *pd=this->Input->GetPointData();
  int numTriPts, *triPts;
  vlIdList cellIds(MAX_CELL_SIZE);
  int neighbor;
  vlPolyData Mesh;
  char *visited;
  int pts[MAX_CELL_SIZE];
  int numStripPts, *stripPts;
  vlPolyData *input=(vlPolyData *)this->Input;

  vlDebugMacro(<<"Executing triangle strip filter");

  this->Initialize();

  // build cell structure.  Only operate with polygons and triangle strips.
  Mesh.SetPoints(input->GetPoints());
  Mesh.SetPolys(input->GetPolys());
  Mesh.SetStrips(input->GetStrips());
  Mesh.BuildLinks();
  // check input
  if ( (numCells=Mesh.GetNumberOfCells()) < 1 )
    {
    vlErrorMacro(<<"No data to strip!");
    return;
    }

  newStrips = new vlCellArray;
  newStrips->Allocate(newStrips->EstimateSize(numCells,6));

  // pre-load existing strips
  inStrips = input->GetStrips();
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
  for (longest=0, numStrips=0, cellId=0; cellId < numCells; cellId++)
    {
    if ( ! visited[cellId] )
      {
      visited[cellId] = 1;
      if ( Mesh.GetCellType(cellId) == vlTRIANGLE )
        {
//
//  Got a starting point for the strip.  Initialize.  Find a neighbor
//  to extend strip.
//
        numStrips++;
        numPts = 3;

        Mesh.GetCellPoints(cellId,numTriPts,triPts);

        for (i=0; i<3; i++) 
          {
          pts[1] = triPts[i];
          pts[2] = triPts[(i+1)%3];

          Mesh.GetCellEdgeNeighbors(cellId, pts[1], pts[2], cellIds);
          if ( cellIds.GetNumberOfIds() > 0 && 
          !visited[neighbor=cellIds.GetId(0)] &&
          Mesh.GetCellType(neighbor) == vlTRIANGLE )
            {
            pts[0] = triPts[(i+2)%3];
            break;
            }
          }
//
//  If no unvisited neighbor, just create the strip of one triangle.
//
        if ( i >= 3 ) 
          {
          pts[0] = triPts[0];;
          pts[1] = triPts[1];
          pts[2] = triPts[2];
          newStrips->InsertNextCell(3,pts);
          } 
        else // continue strip 
          { 
//
//  Have a neighbor.  March along grabbing new points
//
          while ( neighbor >= 0 )
            {
            visited[neighbor] = 1;
            Mesh.GetCellPoints(neighbor,numTriPts, triPts);

            for (i=0; i<3; i++)
              if ( triPts[i] != pts[numPts-2] && 
              triPts[i] != pts[numPts-1] )
                break;

            pts[numPts] = triPts[i];
            Mesh.GetCellEdgeNeighbors(neighbor, pts[numPts], pts[numPts-1], cellIds);
            if ( ++numPts > longest ) longest = numPts;

            // note: if updates value of neighbor
            if ( cellIds.GetNumberOfIds() <= 0 || 
            visited[neighbor=cellIds.GetId(0)] ||
            Mesh.GetCellType(neighbor) != vlTRIANGLE ||
            numPts >= (this->MaximumStripLength+2) )
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

  this->SetPoints(input->GetPoints());
  this->PointData = *pd; // pass data through as is

  newStrips->Squeeze();
  this->SetStrips(newStrips);

  // pass through verts and lines
  this->SetVerts(input->GetVerts());
  this->SetLines(input->GetLines());

  vlDebugMacro (<<"Reduced " << numCells << " cells to " << numStrips << " triangle strips \n\t(Average " << (float)numCells/numStrips << " triangles per strip, longest strip = "<<  ((longest-2)>0?(longest-2):0) << " triangles)");

}

void vlStripper::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStripper::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Maximum Strip Length: " << this->MaximumStripLength << "\n";
    }

}

