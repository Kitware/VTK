/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStripper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStripper.h"

#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkIdTypeArray.h"

vtkStandardNewMacro(vtkStripper);

// Construct object with MaximumLength set to 1000.
vtkStripper::vtkStripper()
{
  this->MaximumLength = 1000;
  this->PassCellDataAsFieldData = 0;
  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;
}

int vtkStripper::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, numCells, i;
  int longestStrip, longestLine, j, numPts;
  vtkIdType numLines, numStrips, nei;
  vtkCellArray *newStrips=NULL, *inStrips, *newLines=NULL, *inLines, *inPolys;
  vtkCellArray *newPolys=0;
  vtkIdType numLinePts = 0;
  vtkIdList *cellIds;
  int foundOne;
  vtkIdType *pts, neighbor=0;
  vtkPolyData *mesh;
  char *visited;
  vtkIdType numStripPts = 0;
  vtkIdType *stripPts = 0;
  vtkIdType *linePts = 0;
  vtkIdType *triPts;
  vtkIdType numTriPts;
  vtkPointData *pd=input->GetPointData();
  vtkCellData* cd = input->GetCellData();

  // The field data, needs to be ordered properly for rendering
  // to work.Hence cell data for each type of cell is collected
  // separately and appended later.
  vtkFieldData* newfd = 0;
  vtkFieldData* newfdPolys = 0;
  vtkFieldData* newfdLines = 0;
  vtkFieldData* newfdStrips = 0;

  vtkDebugMacro(<<"Executing triangle strip / poly-line filter");

  // build cell structure
  inStrips = input->GetStrips();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  vtkIdType inNumVerts = input->GetVerts()->GetNumberOfCells();
  vtkIdType inNumLines = inLines->GetNumberOfCells();
  vtkIdType inNumPolys = inPolys->GetNumberOfCells();

  mesh = vtkPolyData::New();
  mesh->SetPoints(input->GetPoints());
  mesh->SetLines(inLines);
  mesh->SetPolys(inPolys);
  mesh->BuildLinks();

  // check input
  if ( (numCells=mesh->GetNumberOfCells()) < 1  && inStrips->GetNumberOfCells() < 1)
    {
    // pass through verts
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    mesh->Delete();
    vtkDebugMacro(<<"No data to strip!");
    return 1;
    }

  pts = new vtkIdType[this->MaximumLength + 2]; //working array
  cellIds = vtkIdList::New();
  cellIds->Allocate(this->MaximumLength + 2);

  // The new field data object that maintains the transformed cell data.
  if (this->PassCellDataAsFieldData)
    {
    newfd = vtkFieldData::New();
    newfd->CopyStructure(cd);
    newfd->Allocate(3*numCells + 3);

    newfdPolys = vtkFieldData::New();
    newfdPolys->CopyStructure(cd);
    newfdPolys->Allocate(inNumPolys + 1);

    newfdLines = vtkFieldData::New();
    newfdLines->CopyStructure(cd);
    newfdLines->Allocate(inNumLines + 1);

    newfdStrips = vtkFieldData::New();
    newfdStrips->CopyStructure(cd);
    // this is a very gross estimate since we cannot know how long the already
    // present strips are.
    newfdStrips->Allocate(3*inNumPolys + 3);
    }

  vtkIdTypeArray* OriginalCellIds = NULL;
  vtkIdTypeArray* origPolyIds = NULL;
  vtkIdTypeArray* origLineIds = NULL;
  vtkIdTypeArray* origStripIds = NULL;
  if (this->PassThroughCellIds)
    {
    OriginalCellIds = vtkIdTypeArray::New();
    OriginalCellIds->SetName("vtkOriginalCellIds");
    OriginalCellIds->SetNumberOfComponents(1);
    OriginalCellIds->Allocate(3*numCells + 3);

    origPolyIds = vtkIdTypeArray::New();
    origPolyIds->SetNumberOfComponents(1);
    origPolyIds->Allocate(inNumPolys + 1);

    origLineIds = vtkIdTypeArray::New();
    origLineIds->SetNumberOfComponents(1);
    origLineIds->Allocate(inNumLines + 1);

    origStripIds = vtkIdTypeArray::New();
    origStripIds->SetNumberOfComponents(1);
    origStripIds->Allocate(3*inNumPolys + 3);
    }

  // pre-load existing strips
  if ( inStrips->GetNumberOfCells() > 0 || inPolys->GetNumberOfCells() > 0 )
    {
    newStrips = vtkCellArray::New();
    newStrips->Allocate(newStrips->EstimateSize(numCells,6));
    cellId = inNumVerts + inNumLines + inNumPolys;
    for(inStrips->InitTraversal();
        inStrips->GetNextCell(numStripPts,stripPts); )
      {
      newStrips->InsertNextCell(numStripPts,stripPts);
      if (this->PassCellDataAsFieldData)
        {
        for (i=2; i < numStripPts; i++)
          {
          newfdStrips->InsertNextTuple(cellId, cd);
          }
        }
      if (this->PassThroughCellIds)
        {
        origStripIds->InsertNextValue(cellId);
        for (i=2; i < numStripPts; i++)
          {
          origStripIds->InsertNextValue(cellId);
          }
        }
      cellId++;
      }
    // These are for passing through non-triangle polygons
    newPolys = vtkCellArray::New();
    newPolys->Allocate(newStrips->EstimateSize(numCells/2,4));
    }

  // pre-load existing poly-lines
  if ( inLines->GetNumberOfCells() > 0 )
    {
    newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(numCells,6));
    cellId = inNumVerts;
    for (inLines->InitTraversal(); inLines->GetNextCell(numLinePts,linePts); cellId++)
      {
      if ( numLinePts > 2 )
        {
        newLines->InsertNextCell(numLinePts,linePts);
        if (this->PassCellDataAsFieldData)
          {
          newfdLines->InsertNextTuple(cellId, cd);
          }
        if (this->PassThroughCellIds)
          {
          origLineIds->InsertNextValue(cellId);
          }
        }
      }
    }

  // array keeps track of data that's been visited
  visited = new char[numCells];
  for (i=0; i < numCells; i++)
    {
    visited[i] = 0;
    }

  // Loop over all cells and find one that hasn't been visited.
  // Start a triangle strip (or poly-line) and mark as visited, and
  // then find a neighbor that isn't visited.  Add this to the strip
  // (or poly-line) and mark as visited (and so on).
  //
  longestStrip = 0; numStrips = 0;
  longestLine = 0; numLines = 0;

  int cellType;
  int abort=0;
  vtkIdType progressInterval=numCells/20 + 1;
  for ( cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( !(cellId % progressInterval) )
      {
      this->UpdateProgress ((float) cellId/numCells);
      abort = this->GetAbortExecute();
      }
    if ( ! visited[cellId] )
      {
      visited[cellId] = 1;
      if ( (cellType=mesh->GetCellType(cellId)) == VTK_TRIANGLE )
        {
        //  Got a starting point for the strip.  Initialize.  Find a neighbor
        //  to extend strip.
        //
        numStrips++;
        numPts = 3;

        mesh->GetCellPoints(cellId,numTriPts,triPts);

        for (i=0; i<3; i++)
          {
          pts[1] = triPts[i];
          pts[2] = triPts[(i+1)%3];

          mesh->GetCellEdgeNeighbors(cellId, pts[1], pts[2], cellIds);
          if ( cellIds->GetNumberOfIds() > 0 &&
          !visited[neighbor=cellIds->GetId(0)] &&
          mesh->GetCellType(neighbor) == VTK_TRIANGLE )
            {
            pts[0] = triPts[(i+2)%3];
            break;
            }
          }
        //  If no unvisited neighbor, just create the strip of one triangle.
        //
        if ( i >= 3 )
          {
          pts[0] = triPts[0];;
          pts[1] = triPts[1];
          pts[2] = triPts[2];
          newStrips->InsertNextCell(3,pts);
          if (this->PassCellDataAsFieldData)
            {
            newfdStrips->InsertNextTuple(cellId, cd);
            }
          if (this->PassThroughCellIds)
            {
            origStripIds->InsertNextValue(cellId);
            }
          }
        else // continue strip
          {
          //  Have a neighbor.  March along grabbing new points
          //
          if (this->PassCellDataAsFieldData)
            {
            newfdStrips->InsertNextTuple(cellId, cd);
            }
          if (this->PassThroughCellIds)
            {
            origStripIds->InsertNextValue(cellId);
            }
          while ( neighbor >= 0 )
            {
            visited[neighbor] = 1;
            mesh->GetCellPoints(neighbor,numTriPts, triPts);
            if (this->PassCellDataAsFieldData)
              {
              newfdStrips->InsertNextTuple(neighbor, cd);
              }
            if (this->PassThroughCellIds)
              {
              origStripIds->InsertNextValue(neighbor);
              }
            for (i=0; i<3; i++)
              {
              if ( triPts[i] != pts[numPts-2] &&
              triPts[i] != pts[numPts-1] )
                {
                break;
                }
              }

            // only add the triangle to the strip if it isn't degenerate.
            if (i < 3)
              {
              pts[numPts] = triPts[i];
              mesh->GetCellEdgeNeighbors(neighbor, pts[numPts],
                                         pts[numPts-1], cellIds);
              numPts++;
              }

            if ( numPts > longestStrip )
              {
              longestStrip = numPts;
              }

            // note: if updates value of neighbor
            // Note2: for a degenerate triangle this test will
            // correctly fail because the visited[neighbor] will
            // now be visited
            if ( cellIds->GetNumberOfIds() <= 0 ||
                 visited[neighbor=cellIds->GetId(0)] ||
                 mesh->GetCellType(neighbor) != VTK_TRIANGLE ||
                 numPts >= (this->MaximumLength+2) )
              {
              newStrips->InsertNextCell(numPts,pts);
              neighbor = (-1);
              }
            } // while
          } // else continue strip
        } // if triangle

      else if ( cellType == VTK_LINE )
        {
        //
        //  Got a starting point for the line.  Initialize.  Find a neighbor
        //  to extend poly-line.
        //
        numLines++;
        numPts = 2;

        mesh->GetCellPoints(cellId,numLinePts,linePts);

        for ( foundOne=i=0; !foundOne && i<2; i++)
          {
          pts[0] = linePts[i];
          pts[1] = linePts[(i+1)%2];
          mesh->GetPointCells(pts[1], cellIds);
          for (j=0; j < cellIds->GetNumberOfIds(); j++ )
            {
            neighbor = cellIds->GetId(j);
            if ( neighbor != cellId && !visited[neighbor] &&
            mesh->GetCellType(neighbor) == VTK_LINE )
              {
              foundOne = 1;
              break;
              }
            }
          }

        // for each polyline that we construct, we set the cell data to be that
        // for the first cell that formed the polyline. We may build the field data
        // for the mini-cells in the polyline, similar to triangle strips,
        // but that is not required currently.
        if (this->PassCellDataAsFieldData)
          {
          newfdLines->InsertNextTuple(cellId, cd);
          }
        if (this->PassThroughCellIds)
          {
          origLineIds->InsertNextValue(cellId);
          }
        //  If no unvisited neighbor, just create the poly-line from one line.
        //
        if ( !foundOne )
          {
          newLines->InsertNextCell(2,linePts);
          }
        else // continue poly-line
          {
          //  Have a neighbor.  March along grabbing new points
          //
          while ( neighbor >= 0 )
            {
            visited[neighbor] = 1;
            mesh->GetCellPoints(neighbor, numLinePts, linePts);
            for (i=0; i<2; i++)
              {
              if ( linePts[i] != pts[numPts-1] )
                {
                break;
                }
              }
            pts[numPts] = linePts[i];
            mesh->GetPointCells(pts[numPts], cellIds);
            if ( ++numPts > longestLine )
              {
              longestLine = numPts;
              }

            // get new neighbor
            for ( j=0; j < cellIds->GetNumberOfIds(); j++ )
              {
              nei = cellIds->GetId(j);
              if ( nei != neighbor && !visited[nei] &&
              mesh->GetCellType(nei) == VTK_LINE )
                {
                neighbor = nei;
                break;
                }
              }

            if ( j >= cellIds->GetNumberOfIds() ||
            numPts >= (this->MaximumLength+1) )
              {
              newLines->InsertNextCell(numPts,pts);
              neighbor = (-1);
              }
            } // while
          } // else continue line
        } // if line

      //not line, triangle, or strip must be quad or tpolygon which we pass through
      else if ( cellType == VTK_POLYGON || cellType == VTK_QUAD )
        {
        mesh->GetCellPoints(cellId,numTriPts,triPts);
        newPolys->InsertNextCell(numTriPts,triPts);
        if (this->PassCellDataAsFieldData)
          {
          newfdPolys->InsertNextTuple(cellId, cd);
          }
        if (this->PassThroughCellIds)
          {
          origPolyIds->InsertNextValue(cellId);
          }
        }

      } // if not visited
    } // for all elements

  // Update output and release memory
  //
  delete [] pts;
  delete [] visited;
  mesh->Delete();

  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(pd);
  if (this->PassThroughPointIds)
    {
    //make a 1:1 mapping
    vtkIdTypeArray *originalPointIds = vtkIdTypeArray::New();
    originalPointIds->SetName("vtkOriginalPointIds");
    originalPointIds->SetNumberOfComponents(1);
    vtkPointData *outputPD = output->GetPointData();
    outputPD->AddArray(originalPointIds);
    vtkIdType numTup = output->GetNumberOfPoints();
    originalPointIds->SetNumberOfValues(numTup);
    for (vtkIdType cId = 0; cId < numTup; cId++)
      {
      originalPointIds->SetValue(cId, cId);
      }
    originalPointIds->Delete();
    }

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

    if ( newPolys->GetNumberOfCells() > 0 )
      {
      vtkDebugMacro(<<"Passed " << newPolys->GetNumberOfCells()
                    << " polygons");
      newPolys->Squeeze();
      output->SetPolys(newPolys);
      }
    newPolys->Delete();
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
  cellIds->Delete();

  if (this->PassCellDataAsFieldData)
    {
    cellId = 0;
    int max;
    for (i=0; i < inNumVerts; i++, cellId++)
      {
      newfd->InsertNextTuple(cellId, cd);
      }
    if (newfdLines)
      {
      max = newfdLines->GetNumberOfTuples();
      for (i=0; i < max; i++)
        {
        newfd->InsertNextTuple(i, newfdLines);
        }
      newfdLines->Delete();
      }
    if (newfdPolys)
      {
      max = newfdPolys->GetNumberOfTuples();
      for (i=0; i < max; i++)
        {
        newfd->InsertNextTuple(i, newfdPolys);
        }
      newfdPolys->Delete();
      }
    if (newfdStrips)
      {
      max = newfdStrips->GetNumberOfTuples();
      for (i=0; i < max; i++)
        {
        newfd->InsertNextTuple(i, newfdStrips);
        }
      newfdStrips->Delete();
      }
    newfd->Squeeze();
    output->SetFieldData(newfd);
    newfd->Delete();
    }

  if (this->PassThroughCellIds)
    {
    cellId = 0;
    int max;
    for (i=0; i < inNumVerts; i++, cellId++)
      {
      OriginalCellIds->InsertNextValue(cellId);
      }
    if (origLineIds)
      {
      max = origLineIds->GetNumberOfTuples();
      for (i=0; i < max; i++)
        {
        OriginalCellIds->InsertNextTuple(i, origLineIds);
        }
      origLineIds->Delete();
      }
    if (origPolyIds)
      {
      max = origPolyIds->GetNumberOfTuples();
      for (i=0; i < max; i++)
        {
        OriginalCellIds->InsertNextTuple(i, origPolyIds);
        }
      origPolyIds->Delete();
      }
    if (origStripIds)
      {
      max = origStripIds->GetNumberOfTuples();
      for (i=0; i < max; i++)
        {
        OriginalCellIds->InsertNextTuple(i, origStripIds);
        }
      origStripIds->Delete();
      }
    OriginalCellIds->Squeeze();
    output->GetFieldData()->AddArray(OriginalCellIds);
    OriginalCellIds->Delete();
    }


  return 1;
}

void vtkStripper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum Length: " << this->MaximumLength << "\n";
  os << indent << "PassCellDataAsFieldData: " << this->PassCellDataAsFieldData << endl;
  os << indent << "PassThroughCellIds: " << this->PassThroughCellIds << endl;
  os << indent << "PassThroughPointIds: " << this->PassThroughPointIds << endl;
}
