/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStripper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkStripper.h"

// Description:
// Construct object with MaximumLength set to 1000.
vtkStripper::vtkStripper()
{
  this->MaximumLength = 1000;
}

void vtkStripper::Execute()
{
  int longestStrip, longestLine, cellId, i, j, numCells, numPts;
  int numLines, numStrips, nei;
  vtkCellArray *newStrips=NULL, *inStrips, *newLines=NULL, *inLines, *inPolys;
  vtkPointData *pd=this->Input->GetPointData();
  int numTriPts, *triPts, numLinePts, *linePts;
  vtkIdList cellIds(this->MaximumLength + 2);
  int *pts, neighbor=0, foundOne;
  vtkPolyData Mesh;
  char *visited;
  int numStripPts, *stripPts;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;

  vtkDebugMacro(<<"Executing triangle strip / poly-line filter");

  // build cell structure
  inStrips = input->GetStrips();
  inLines = input->GetLines();
  inPolys = input->GetPolys();

  Mesh.SetPoints(input->GetPoints());
  Mesh.SetLines(inLines);
  Mesh.SetPolys(inPolys);
  Mesh.SetStrips(inStrips);
  Mesh.BuildLinks();

  // check input
  if ( (numCells=Mesh.GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro(<<"No data to strip!");
    return;
    }

  pts = new int[this->MaximumLength + 2]; //working array

  // pre-load existing strips
  if ( inStrips->GetNumberOfCells() > 0 || inPolys->GetNumberOfCells() > 0 )
    {
    newStrips = vtkCellArray::New();
    newStrips->Allocate(newStrips->EstimateSize(numCells,6));
    for(inStrips->InitTraversal(); inStrips->GetNextCell(numStripPts,stripPts);)
      {
      newStrips->InsertNextCell(numStripPts,stripPts);
      }
    }

  // pre-load existing poly-lines
  if ( inLines->GetNumberOfCells() > 0 )
    {
    newLines = vtkCellArray::New();
    newLines->Allocate(newStrips->EstimateSize(numCells,6));
    for (inLines->InitTraversal(); inLines->GetNextCell(numLinePts,linePts); )
      {
      if ( numLinePts > 2 )
        {
        newLines->InsertNextCell(numLinePts,linePts);
        }
      }
    }

  // array keeps track of data that's been visited
  visited = new char[numCells];
  for (i=0; i < numCells; i++) visited[i] = 0;
//
// Loop over all cells and find one that hasn't been visited.
// Start a triangle strip (or poly-line) and mark as visited, and 
// then find a neighbor that isn't visited.  Add this to the strip 
// (or poly-line) and mark as visited (and so on).
//
  longestStrip = 0; numStrips = 0;
  longestLine = 0; numLines = 0;

  for ( cellId=0; cellId < numCells; cellId++)
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
            if ( ++numPts > longestStrip ) longestStrip = numPts;

            // note: if updates value of neighbor
            if ( cellIds.GetNumberOfIds() <= 0 || 
            visited[neighbor=cellIds.GetId(0)] ||
            Mesh.GetCellType(neighbor) != VTK_TRIANGLE ||
            numPts >= (this->MaximumLength+2) )
              {
              newStrips->InsertNextCell(numPts,pts);
              neighbor = (-1);
              }
            } // while
          } // else continue strip
        } // if triangle

      else if ( Mesh.GetCellType(cellId) == VTK_LINE )
        {
//
//  Got a starting point for the line.  Initialize.  Find a neighbor
//  to extend poly-line.
//
        numLines++;
        numPts = 2;

        Mesh.GetCellPoints(cellId,numLinePts,linePts);

        for ( foundOne=i=0; !foundOne && i<2; i++) 
          {
          pts[0] = linePts[i];
          pts[1] = linePts[(i+1)%2];
          Mesh.GetPointCells(pts[1], cellIds);
          for (j=0; j < cellIds.GetNumberOfIds(); j++ )
            {
            neighbor = cellIds.GetId(j);
            if ( neighbor != cellId && !visited[neighbor] &&
            Mesh.GetCellType(neighbor) == VTK_LINE )
              {
              foundOne = 1;
              break;
              }
            }
          }
//
//  If no unvisited neighbor, just create the poly-line from one line.
//
        if ( !foundOne ) 
          {
          newLines->InsertNextCell(2,linePts);
          } 
        else // continue poly-line
          { 
//
//  Have a neighbor.  March along grabbing new points
//
          while ( neighbor >= 0 )
            {
            visited[neighbor] = 1;
            Mesh.GetCellPoints(neighbor, numLinePts, linePts);

            for (i=0; i<2; i++)
              if ( linePts[i] != pts[numPts-1] )
                break;

            pts[numPts] = linePts[i];
            Mesh.GetPointCells(pts[numPts], cellIds);
            if ( ++numPts > longestLine ) longestLine = numPts;

            // get new neighbor
            for ( j=0; j < cellIds.GetNumberOfIds(); j++ )
              {
              nei = cellIds.GetId(j);
              if ( nei != neighbor && !visited[nei] &&
              Mesh.GetCellType(nei) == VTK_LINE )
                {
                neighbor = nei;
                break;
                }
              }

            if ( j >= cellIds.GetNumberOfIds() ||
            numPts >= (this->MaximumLength+1) )
              {
              newLines->InsertNextCell(numPts,pts);
              neighbor = (-1);
              }
            } // while
          } // else continue strip
        } // if line

      } // if not visited
    } // for all elements
//
// Update output and release memory
//
  delete [] pts;
  delete [] visited;

  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(pd);

  // output strips
  if ( newStrips )
    {
    newStrips->Squeeze();
    output->SetStrips(newStrips);
    newStrips->Delete();
    vtkDebugMacro (<<"Reduced " << numCells << " cells to " << numStrips 
                   << " triangle strips \n\t(Average " 
                   << (float)numCells/numStrips 
                   << " triangles per strip, longest strip = "
                   << ((longestStrip-2)>0?(longestStrip-2):0) << " triangles)");

    }

  // output poly-lines
  if ( newLines )
    {
    newLines->Squeeze();
    output->SetLines(newLines);
    newLines->Delete();
    vtkDebugMacro (<<"Reduced " << numCells << " cells to " << numLines 
                   << " poly-lines \n\t(Average " << (float)numCells/numLines 
                   << " lines per poly-line, longest poly-line = "
                   << ((longestLine-1)>0?(longestLine-1):0) << " lines)");

    }

  // pass through verts
  output->SetVerts(input->GetVerts());

}

void vtkStripper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Maximum Length: " << this->MaximumLength << "\n";

}

