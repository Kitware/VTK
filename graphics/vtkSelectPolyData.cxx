/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectPolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkSelectPolyData.h"
#include "vtkMath.h"
#include "vtkPolygon.h"
#include "vtkTriangleFilter.h"
#include "vtkCharArray.h"
#include "vtkTriangleStrip.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSelectPolyData* vtkSelectPolyData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSelectPolyData");
  if(ret)
    {
    return (vtkSelectPolyData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSelectPolyData;
}




// Description:
// Instantiate object with InsideOut turned off.
vtkSelectPolyData::vtkSelectPolyData()
{
  this->GenerateSelectionScalars = 0;
  this->InsideOut = 0;
  this->Loop = NULL;
  this->SelectionMode = VTK_INSIDE_SMALLEST_REGION;
  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;
  this->GenerateUnselectedOutput = 0;
  this->UnselectedOutput = vtkPolyData::New();
  this->UnselectedOutput->SetSource(this);
  this->SelectionEdges = vtkPolyData::New();
  this->SelectionEdges->SetSource(this);
}

vtkSelectPolyData::~vtkSelectPolyData()
{
  if (this->Loop)
    {
    this->Loop->Delete();
    }
  this->UnselectedOutput->Delete();
  this->SelectionEdges->Delete();
}

void vtkSelectPolyData::Execute()
{
  int numPts, numLoopPts;
  vtkPolyData *input=this->GetInput();
  vtkPolyData *output=this->GetOutput();
  vtkPolyData *triMesh;
  vtkPointData *inPD, *outPD=output->GetPointData();
  vtkCellData *inCD, *outCD=output->GetCellData();
  int i, j, k, closest, numPolys;
  vtkIdList *loopIds, *edgeIds, *neighbors;
  float x[3], xLoop[3], closestDist2, dist2, t;
  float x0[3], x1[3], vec[3], dir[3], neiX[3];
  vtkCellArray *inPolys;
  vtkPoints *inPts;
  int id, pt1, pt2, currentId = 0, nextId, numCells, numNei, neiId;
  int numMeshLoopPts, prevId;
  vtkIdType *cells, *pts, npts;
  unsigned short int ncells;
  int mark, s1, s2, val;

  // Initialize and check data
  vtkDebugMacro(<<"Selecting data...");

  this->UnselectedOutput->Initialize();
  this->SelectionEdges->Initialize();

  if ( (numPts = input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro("Input contains no points");
    return;
    }

  if ( this->Loop == NULL || 
  (numLoopPts=this->Loop->GetNumberOfPoints()) < 3 )
    {
    vtkErrorMacro("Please define a loop with at least three points");
    return;
    }

  // Okay, now we build unstructured representation. Make sure we're
  // working with triangles.
  vtkTriangleFilter *tf=vtkTriangleFilter::New();
  tf->SetInput(input);
  tf->PassLinesOff();
  tf->PassVertsOff();
  tf->Update();
  triMesh = tf->GetOutput();
  triMesh->Register(this);
  tf->Delete();
  inPD = triMesh->GetPointData();
  inCD = triMesh->GetCellData();

  numPts = triMesh->GetNumberOfPoints();
  inPts = triMesh->GetPoints();
  inPolys = triMesh->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  if ( numPolys < 1 )
    {
    vtkErrorMacro("This filter operates on surface primitives");
    tf->Delete();
    return;
    }

  this->Mesh = vtkPolyData::New();
  this->Mesh->SetPoints(inPts); //inPts
  this->Mesh->SetPolys(inPolys);
  this->Mesh->BuildLinks(); //to do neighborhood searching
  numCells = this->Mesh->GetNumberOfCells();

  // First thing to do is find the closest mesh points to the loop
  // points. This creates a list of point ids.
  loopIds = vtkIdList::New();
  loopIds->SetNumberOfIds(numLoopPts);

  for ( i=0; i < numLoopPts; i++)
    {
    this->Loop->GetPoint(i,xLoop);
    closest = -1;
    closestDist2 = VTK_LARGE_FLOAT;

    for ( j=0; j < numPts; j++)
      {
      inPts->GetPoint(j,x);

      dist2 = vtkMath::Distance2BetweenPoints(x, xLoop);
      if ( dist2 < closestDist2 )
        {
        closest = j;
        closestDist2 = dist2;
        }
      } //for all input points

    loopIds->SetId(i,closest);
    } //for all loop points

  // Now that we've got point ids, we build the loop. Start with the
  // first two points in the loop (which define a line), and find the
  // mesh edge that is directed along the line, and whose
  // end point is closest to the line. Continue until loop closes in on
  // itself.
  edgeIds = vtkIdList::New();
  edgeIds->Allocate(numLoopPts*10,1000);
  neighbors = vtkIdList::New();
  neighbors->Allocate(10000);
  edgeIds->InsertNextId(loopIds->GetId(0));

  for ( i=0; i < numLoopPts; i++ )
    {
    currentId = loopIds->GetId(i);
    nextId = loopIds->GetId((i+1)%numLoopPts);
    prevId = (-1);
    inPts->GetPoint(currentId, x);
    inPts->GetPoint(currentId, x0);
    inPts->GetPoint(nextId, x1);
    for (j=0; j<3; j++)
      {
      vec[j] = x1[j] - x0[j];
      }

    // track edge
    for (id=currentId; id != nextId; )
      {
      this->GetPointNeighbors(id,neighbors); //points connected by edge
      numNei = neighbors->GetNumberOfIds();
      closest = -1;
      closestDist2 = VTK_LARGE_FLOAT;
      for (j=0; j<numNei; j++)
        {
        neiId = neighbors->GetId(j);
        inPts->GetPoint(neiId, neiX);
        for (k=0; k<3; k++)
	  {
	  dir[k] = neiX[k] - x[k];
	  }
        if ( neiId != prevId && vtkMath::Dot(dir,vec) > 0.0 ) //candidate
          {
          dist2 = vtkLine::DistanceToLine(neiX, x0, x1);
          if ( dist2 < closestDist2 )
            {
            closest = neiId;
            closestDist2 = dist2;
            }
          }//in direction of line
        }//for all neighbors

      if ( closest < 0 )
        {
        vtkErrorMacro(<<"Can't follow edge");
        return;
        }
      else
        {
        edgeIds->InsertNextId(closest);
        prevId = id;
        id = closest;
        inPts->GetPoint(id, x);
        }
      }//for tracking edge
    }//for all edges of loop

  // mainly for debugging
  numMeshLoopPts = edgeIds->GetNumberOfIds();
  vtkCellArray *selectionEdges=vtkCellArray::New();
  selectionEdges->Allocate(selectionEdges->EstimateSize(1,numMeshLoopPts),100);
  selectionEdges->InsertNextCell(numMeshLoopPts);
  for (i=0; i<numMeshLoopPts; i++)
    {
    selectionEdges->InsertCellPoint(edgeIds->GetId(i));
    }
  this->SelectionEdges->SetPoints(inPts);
  this->SelectionEdges->SetLines(selectionEdges);
  selectionEdges->Delete();

  // Phew...we've defined loop, now want to do a fill so we can extract the
  // inside from the outside. Depending on GenerateSelectionScalars flag,
  // we either set the distance of the points to the selection loop as
  // zero (GenerateSelectionScalarsOff) or we evaluate the distance of these
  // points to the lines. (Later we'll use a connected traversal to compute
  // the distances to the remaining points.)

  // Next, prepare to mark off inside/outside and on boundary of loop.
  // Mark the boundary of the loop using point marks. Also initialize
  // the advancing front (used to mark traversal/compute scalars).
  // Prepare to compute the advancing front
  vtkIntArray *cellMarks = vtkIntArray::New();
  cellMarks->SetNumberOfValues(numCells);
  for (i=0; i<numCells; i++)  //mark unvisited
    {
    cellMarks->SetValue(i,VTK_LARGE_INTEGER);
    }
  vtkIntArray *pointMarks = vtkIntArray::New();
  pointMarks->SetNumberOfValues(numPts);
  for (i=0; i<numPts; i++)  //mark unvisited
    {
    pointMarks->SetValue(i,VTK_LARGE_INTEGER);
    }

  vtkIdList *currentFront = vtkIdList::New(), *tmpFront;
  vtkIdList *nextFront = vtkIdList::New();
  for (i=0; i<numMeshLoopPts; i++)
    {
    id = edgeIds->GetId(i);
    pointMarks->SetValue(id, 0); //marks the start of the front
    currentFront->InsertNextId(id);
    }

  // Traverse the front as long as we can. We're basically computing a 
  // topological distance.
  int maxFront=0, maxFrontCell=(-1);
  int currentFrontNumber=1, numPtsInFront;
  while ( (numPtsInFront = currentFront->GetNumberOfIds()) )
    {
    for (i=0; i < numPtsInFront; i++)
      {
      id = currentFront->GetId(i);
      this->Mesh->GetPointCells(id, ncells, cells);
      for (j=0; j<ncells; j++)
        {
        id = cells[j];
        if ( cellMarks->GetValue(id) == VTK_LARGE_INTEGER )
          {
          if ( currentFrontNumber > maxFront )
	    {
	    maxFrontCell = id;
	    }
          cellMarks->SetValue(id, currentFrontNumber);
          this->Mesh->GetCellPoints(id,npts,pts);
          for (k=0; k<npts; k++)
            {
            if ( pointMarks->GetValue(pts[k]) == VTK_LARGE_INTEGER )
              {
              pointMarks->SetValue(pts[k], 1);
              nextFront->InsertNextId(pts[k]);
              }
            }
          }
        } //for cells surrounding point
      } //all points in front

    currentFrontNumber++;
    tmpFront = currentFront;
    currentFront = nextFront;
    nextFront = tmpFront;
    nextFront->Reset();
    } //while still advancing

  // Okay, now one of the regions is filled with negative values. This fill
  // operation assumes that everthing is connected.
  if ( this->SelectionMode == VTK_INSIDE_CLOSEST_POINT_REGION )
    {// find closest point and use as a seed
    for (closestDist2=VTK_LARGE_FLOAT, j=0; j < numPts; j++)
      {
      inPts->GetPoint(j,x);

      dist2 = vtkMath::Distance2BetweenPoints(x, this->ClosestPoint);
      // get closest point not on the boundary
      if ( dist2 < closestDist2 && pointMarks->GetValue(j) != 0 )
        {
        closest = j;
        closestDist2 = dist2;
        }
      } //for all input points
    this->Mesh->GetPointCells(closest, ncells, cells);
    }

  // We do the fill as a moving front. This is an alternative to recursion. The
  // fill negates one region of the mesh on one side of the loop.
  currentFront->Reset(); nextFront->Reset();
  currentFront->InsertNextId(maxFrontCell);
  int numCellsInFront;
  
  cellMarks->SetValue(maxFrontCell,-1);

  while ( (numCellsInFront = currentFront->GetNumberOfIds()) > 0 )
    {
    for (i=0; i < numCellsInFront; i++)
      {
      id = currentFront->GetId(i);

      this->Mesh->GetCellPoints(id, npts, pts);
      for (j=0; j<3; j++)
        {
        pt1 = pts[j];
        pt2 = pts[(j+1)%3];
        s1 = pointMarks->GetValue(pt1);
        s2 = pointMarks->GetValue(pt2);

        if ( s1 != 0 )
          {
          pointMarks->SetValue(pt1, -1);
          }

        if ( ! (s1 == 0 && s2 == 0) )
          {
          this->Mesh->GetCellEdgeNeighbors(id, pt1, pt2, neighbors);
          numNei = neighbors->GetNumberOfIds();
          for (k=0; k<numNei; k++)
            {
            neiId = neighbors->GetId(k);
            val = cellMarks->GetValue(neiId);
            if ( val != -1 ) //-1 is what we're filling with
              {
              cellMarks->SetValue(neiId,-1);
              nextFront->InsertNextId(neiId);
              }
            }
          }//if can cross boundary
        }//for all edges of cell
      } //all cells in front

    tmpFront = currentFront;
    currentFront = nextFront;
    nextFront = tmpFront;
    nextFront->Reset();
    } //while still advancing

  // Now may have to invert fill value depending on what we wan to extract
  if ( this->SelectionMode == VTK_INSIDE_SMALLEST_REGION )
    {
    for (i=0; i < numCells; i++)
      {
      mark = cellMarks->GetValue(i);
      cellMarks->SetValue(i, -mark);
      }
    for (i=0; i < numPts; i++)
      {
      mark = pointMarks->GetValue(i);
      pointMarks->SetValue(i, -mark);
      }
    }

  // If generating selection scalars, we now have to modify the scalars to
  // approximate a distance function. Otherwise, we can create the output.
  if ( ! this->GenerateSelectionScalars )
    {//spit out all the negative cells
    vtkCellArray *newPolys=vtkCellArray::New();
    newPolys->Allocate(numCells/2, numCells/2);
    for (i=0; i< numCells; i++)
      {
      if ( (cellMarks->GetValue(i) < 0) || 
      (cellMarks->GetValue(i) > 0 && this->InsideOut) )
        {
        this->Mesh->GetCellPoints(i, npts, pts);
        newPolys->InsertNextCell(npts,pts);
        }
      }
    output->SetPoints(inPts);
    output->SetPolys(newPolys);
    outPD->PassData(inPD);
    newPolys->Delete();

    if ( this->GenerateUnselectedOutput )
      {
      vtkCellArray *unPolys=vtkCellArray::New();
      unPolys->Allocate(numCells/2, numCells/2);
      for (i=0; i< numCells; i++)
        {
        if ( (cellMarks->GetValue(i) >= 0) || 
        (cellMarks->GetValue(i) < 0 && this->InsideOut) )
          {
          this->Mesh->GetCellPoints(i, npts, pts);
          unPolys->InsertNextCell(npts,pts);
          }
        }
      this->UnselectedOutput->SetPoints(inPts);
      this->UnselectedOutput->SetPolys(unPolys);
      this->UnselectedOutput->GetPointData()->PassData(inPD);
      unPolys->Delete();
      }

    }
  else //modify scalars to generate selection scalars
    {
    vtkScalars *selectionScalars=vtkScalars::New();
    selectionScalars->SetNumberOfScalars(numPts);
    
    // compute distance to lines. Really this should be computed based on
    // the connected fill distance.
    for (j=0; j < numPts; j++) //compute minimum distance to loop
      {
      if ( pointMarks->GetValue(j) != 0 )
        {
        inPts->GetPoint(j,x);
        for ( closestDist2=VTK_LARGE_FLOAT, i=0; i < numLoopPts; i++ )
          {
          this->Loop->GetPoint(i, x0);
          this->Loop->GetPoint((i+1)%numLoopPts, x1);
          dist2 = vtkLine::DistanceToLine(x, x0, x1, t, xLoop);
          if ( dist2 < closestDist2 )
            {
            closestDist2 = dist2;
            }
          }//for all loop edges
          closestDist2 = sqrt((double)closestDist2);
          selectionScalars->SetScalar(j,closestDist2*pointMarks->GetValue(j));
        }
      }

    // Now, determine the sign of those points "on the boundary" to give a 
    // better approximation to the scalar field.
    for (j=0; j < numMeshLoopPts; j++)
      {
      id = edgeIds->GetId(j);
      inPts->GetPoint(id, x);
      for ( closestDist2=VTK_LARGE_FLOAT, i=0; i < numLoopPts; i++ )
        {
        this->Loop->GetPoint(i, x0);
        this->Loop->GetPoint((i+1)%numLoopPts, x1);
        dist2 = vtkLine::DistanceToLine(x, x0, x1, t, xLoop);
        if ( dist2 < closestDist2 )
          {
          closestDist2 = dist2;
          neiX[0] = xLoop[0]; neiX[1] = xLoop[1]; neiX[2] = xLoop[2];
          }
        }//for all loop edges
      closestDist2 = sqrt((double)closestDist2);

      // find neighbor not on boundary and compare negative/positive values
      // to see what makes the most sense
      this->GetPointNeighbors(id, neighbors);
      numNei = neighbors->GetNumberOfIds();
      for (dist2=0.0, i=0; i<numNei; i++)
        {
        neiId = neighbors->GetId(i);
        if ( pointMarks->GetValue(neiId) != 0 ) //find the furthest away
          {
          if ( fabs(selectionScalars->GetScalar(neiId)) > dist2 )
            {
            currentId = neiId;
            dist2 = fabs(selectionScalars->GetScalar(neiId));
            }
          }
        }

      inPts->GetPoint(currentId, x0);
      if ( vtkMath::Distance2BetweenPoints(x0,x) < 
           vtkMath::Distance2BetweenPoints(x0,neiX) )
        {
        closestDist2 = closestDist2 * pointMarks->GetValue(currentId);
        }
      else
        {
        closestDist2 = -closestDist2 * pointMarks->GetValue(currentId);
        }

      selectionScalars->SetScalar(id, closestDist2);
      }//for all boundary points

    output->CopyStructure(this->Mesh); //pass geometry/topology unchanged
    outPD->SetScalars(selectionScalars);
    outPD->CopyScalarsOff();
    outPD->PassData(inPD);
    outCD->PassData(inCD);
    selectionScalars->Delete();
    }
    
  // Clean up and update output
  triMesh->UnRegister(this);
  this->Mesh->Delete();
  neighbors->Delete();
  edgeIds->Delete();
  loopIds->Delete();
  cellMarks->Delete();
  pointMarks->Delete();
  currentFront->Delete();
  nextFront->Delete();
}

void vtkSelectPolyData::GetPointNeighbors (int ptId, vtkIdList *nei)
{
  unsigned short ncells;
  int i, j;
  vtkIdType *cells, *pts, npts;
  
  nei->Reset();
  this->Mesh->GetPointCells(ptId, ncells, cells);
  for (i=0; i<ncells; i++)
    {
    this->Mesh->GetCellPoints(cells[i], npts, pts);
    for (j=0; j<3; j++)
      {
      if ( pts[j] != ptId )
        {
        nei->InsertUniqueId(pts[j]);
        }
      }
    }
}

unsigned long int vtkSelectPolyData::GetMTime()
{
  unsigned long mTime=this->vtkPolyDataToPolyDataFilter::GetMTime();
  unsigned long time;

  if ( this->Loop != NULL )
    {
    time = this->Loop->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

void vtkSelectPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Generate Unselected Output: " 
     << (this->GenerateUnselectedOutput ? "On\n" : "Off\n");

  os << indent << "Inside Mode: ";
  os << this->GetSelectionModeAsString() << "\n";

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", " 
     << this->ClosestPoint[1] << ", " << this->ClosestPoint[2] << ")\n";

  os << indent << "Generate Selection Scalars: " 
     << (this->GenerateSelectionScalars ? "On\n" : "Off\n");

  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");

  if ( this->Loop )
    {
    os << indent << "Loop of " << this->Loop->GetNumberOfPoints()
       << "points defined\n";
    }
  else
    {
    os << indent << "Loop not defined\n";
    }
}


void vtkSelectPolyData::UnRegister(vtkObject *o)
{
  // detect the circular loop source <-> data
  // If we have two references and one of them is my data
  // and I am not being unregistered by my data, break the loop.
  if (this->ReferenceCount == 4 &&
      this->GetOutput() != o && this->UnselectedOutput != o &&
      this->SelectionEdges != o &&
      this->GetOutput()->GetNetReferenceCount() == 1 &&
      this->UnselectedOutput->GetNetReferenceCount() == 1 &&
      this->SelectionEdges->GetNetReferenceCount() == 1)
    {
    this->GetOutput()->SetSource(NULL);
    this->UnselectedOutput->SetSource(NULL);
    this->SelectionEdges->SetSource(NULL);
    }
  if (this->ReferenceCount == 3 &&
      (this->GetOutput() == o || this->UnselectedOutput == o ||
      this->SelectionEdges == o) &&
      (this->GetOutput()->GetNetReferenceCount() +
      this->UnselectedOutput->GetNetReferenceCount() +
      this->SelectionEdges->GetNetReferenceCount()) == 4)
    {
    this->GetOutput()->SetSource(NULL);
    this->UnselectedOutput->SetSource(NULL);
    this->SelectionEdges->SetSource(NULL);
    }
  
  this->vtkObject::UnRegister(o);
}

int vtkSelectPolyData::InRegisterLoop(vtkObject *o)
{
  int num = 0;
  int cnum = 0;
  
  if (this->GetOutput()->GetSource() == this)
    {
    num++;
    cnum += this->GetOutput()->GetNetReferenceCount();
    }
  if (this->UnselectedOutput->GetSource() == this)
    {
    num++;
    cnum += this->UnselectedOutput->GetNetReferenceCount();
    }
  if (this->SelectionEdges->GetSource() == this)
    {
    num++;
    cnum += this->SelectionEdges->GetNetReferenceCount();
    }
  
  // if no one outside is using us
  // and our data objects are down to one net reference
  // and we are being asked by one of our data objects
  if (this->ReferenceCount == num &&
      cnum == (num + 1) &&
      (this->GetOutput() == o ||
       this->UnselectedOutput == o ||
       this->SelectionEdges == o))
    {
    return 1;
    }
  return 0;
}
