/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStripper.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkStripper.hh"

// Description:
// Construct object with vertex and line passing turned on.
vtkStripper::vtkStripper()
{
  this->MaximumStripLength = VTK_CELL_SIZE - 2;

  this->PassVerts = 1;
  this->PassLines = 1;
}

void vtkStripper::Execute()
{
  int longest, cellId, i, numCells, numPts, numStrips;
  vtkCellArray *newStrips, *inStrips;
  vtkPointData *pd=this->Input->GetPointData();
  int numTriPts, *triPts;
  vtkIdList cellIds(VTK_CELL_SIZE);
  int pts[VTK_CELL_SIZE];
  int neighbor = 0;
  vtkPolyData Mesh;
  char *visited;
  int numStripPts, *stripPts;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;

  vtkDebugMacro(<<"Executing triangle strip filter");

  // build cell structure.  Only operate with polygons and triangle strips.
  Mesh.SetPoints(input->GetPoints());
  Mesh.SetPolys(input->GetPolys());
  Mesh.SetStrips(input->GetStrips());
  Mesh.BuildLinks();
  // check input
  if ( (numCells=Mesh.GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro(<<"No data to strip!");
    return;
    }

  newStrips = new vtkCellArray;
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
      if ( Mesh.GetCellType(cellId) == VTK_TRIANGLE )
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
          Mesh.GetCellType(neighbor) == VTK_TRIANGLE )
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
            Mesh.GetCellType(neighbor) != VTK_TRIANGLE ||
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
// Update output and release memory
//
  delete [] visited;

  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(pd);

  newStrips->Squeeze();
  output->SetStrips(newStrips);
  newStrips->Delete();

  // pass through verts and lines
  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());

  vtkDebugMacro (<<"Reduced " << numCells << " cells to " << numStrips << " triangle strips \n\t(Average " << (float)numCells/numStrips << " triangles per strip, longest strip = "<<  ((longest-2)>0?(longest-2):0) << " triangles)");

}

void vtkStripper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Maximum Strip Length: " << this->MaximumStripLength << "\n";

}

