/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataNormals.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPolyDataNormals.h"
#include "vtkMath.h"
#include "vtkNormals.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"
#include "vtkObjectFactory.h"
#include "vtkRemoveGhostCells.h"


//------------------------------------------------------------------------------
vtkPolyDataNormals* vtkPolyDataNormals::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyDataNormals");
  if(ret)
    {
    return (vtkPolyDataNormals*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyDataNormals;
}




// Construct with feature angle=30, splitting and consistency turned on, 
// flipNormals turned off, and non-manifold traversal turned on.
vtkPolyDataNormals::vtkPolyDataNormals()
{
  this->FeatureAngle = 30.0;
  this->Splitting = 1;
  this->Consistency = 1;
  this->FlipNormals = 0;
  this->ComputePointNormals = 1;
  this->ComputeCellNormals = 0;
  this->NonManifoldTraversal = 1;
  this->MaxRecursionDepth = 1000;
  // some internal data
  this->NumFlips = 0;
  this->Mark = 0;
}

#define VTK_CELL_NOT_VISITED     0
#define VTK_CELL_VISITED         1
#define VTK_CELL_NEEDS_VISITING  2


// Generate normals for polygon meshes

void vtkPolyDataNormals::Execute()
{
  int i, j;
  int *pts, npts;
  int numNewPts;
  float *polyNormal, *vertNormal, length;
  float flipDirection=1.0;
  int replacementPoint, numPolys, numStrips;
  int cellId, numPts;
  vtkPoints *inPts;
  vtkCellArray *inPolys, *inStrips, *polys;
  vtkPolygon *poly;
  vtkPoints *newPts = NULL;
  vtkNormals *newNormals;
  vtkPointData *pd, *outPD;
  vtkCellData *outCD;
  float n[3];
  vtkCellArray *newPolys;
  int ptId, oldId;
  vtkIdList *cellIds, *edgeNeighbors;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *ghost;
  int ghostLevel = input->GetUpdateGhostLevel();
  vtkRemoveGhostCells *rmGhostCells;
  int noCellsNeedVisiting;
  int *Visited;
  vtkPolyData *OldMesh, *NewMesh;
  vtkNormals *PolyNormals;
  float	CosAngle;
  vtkIdList *Map;

  vtkDebugMacro(<<"Generating surface normals");

  numPolys=input->GetNumberOfPolys();
  numStrips=input->GetNumberOfStrips();
  if ( (numPts=input->GetNumberOfPoints()) < 1 || (numPolys < 1 && numStrips < 1) )
    {
    vtkErrorMacro(<<"No data to generate normals for!");
    return;
    }

  // If there is nothing to do, pass the data through
  if ( this->ComputePointNormals == 0 && this->ComputeCellNormals == 0) 
    { //don't do anything! pass data through
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return;
    }

//
// Load data into cell structure.  We need two copies: one is a 
// non-writable mesh used to perform topological queries.  The other 
// is used to write into and modify the connectivity of the mesh.
//
  inPts = input->GetPoints();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
  poly = vtkPolygon::New();
  
  edgeNeighbors = vtkIdList::New();

  OldMesh = vtkPolyData::New();
  OldMesh->SetPoints(inPts);
  if ( numStrips > 0 ) //have to decompose strips into triangles
    {
    vtkTriangleStrip *strip = vtkTriangleStrip::New();
    if ( numPolys > 0 )
      {
      polys = vtkCellArray::New();
      polys->DeepCopy(inPolys);
      }
    else 
      {
      polys = vtkCellArray::New();
      polys->Allocate(polys->EstimateSize(numStrips,5));
      }
    for ( inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
      {
      strip->DecomposeStrip(npts, pts, polys);
      }
    OldMesh->SetPolys(polys);
    polys->Delete();
    numPolys = polys->GetNumberOfCells();//added some new triangles
    strip->Delete();
    }
  else
    {
    OldMesh->SetPolys(inPolys);
    polys = inPolys;
    }
  OldMesh->BuildLinks();
  this->UpdateProgress(0.10);
  
  pd = input->GetPointData();
  outPD = output->GetPointData();
    
  outCD = output->GetCellData();
    
  NewMesh = vtkPolyData::New();
  NewMesh->SetPoints(inPts);
  // create a copy because we're modifying it
  newPolys = vtkCellArray::New();
  newPolys->DeepCopy(polys);
  NewMesh->SetPolys(newPolys);
  NewMesh->BuildCells(); //builds connectivity

  cellIds = vtkIdList::New();
  cellIds->Allocate(VTK_CELL_SIZE);

  //
  // The visited array keeps track of which polygons have been visited.
  //
  if ( this->Consistency || this->Splitting ) 
    {
    Visited = new int[numPolys];
    memset(Visited, VTK_CELL_NOT_VISITED, numPolys*sizeof(int));
    this->Mark = 1;
    }
  else
    {
    Visited = NULL;
    }
  //
  //  Traverse all elements insuring proper direction of ordering.  This
  //  is a recursive neighbor search.  Note: have to truncate recursion
  //  and keep track of seeds to start up again.
  //
  this->NumFlips = 0;
  
  if ( this->Consistency ) 
    {    
    for (cellId=0; cellId < numPolys; cellId++)
      {
      noCellsNeedVisiting = 1;
      if ( Visited[cellId] == VTK_CELL_NOT_VISITED) 
        {
        if ( this->FlipNormals ) 
          {
          this->NumFlips++;
          NewMesh->ReverseCell(cellId);
          }
        if (this->TraverseAndOrder(cellId, edgeNeighbors, Visited, OldMesh, NewMesh))
	  {
	  noCellsNeedVisiting = 0;
	  }
        }

      while (!noCellsNeedVisiting)
	{
	noCellsNeedVisiting = 1;
        for (i=0; i < numPolys; i++) 
	  {
	  if ( Visited[i] == VTK_CELL_NEEDS_VISITING )
	    {
	    if (this->TraverseAndOrder(i, edgeNeighbors, Visited, OldMesh, NewMesh))
	      {
	      noCellsNeedVisiting = 0;
	      }
	    }
	  }
        }
      }
    vtkDebugMacro(<<"Reversed ordering of " << this->NumFlips << " polygons");
    }
  this->Mark = VTK_CELL_NEEDS_VISITING + 1;
  this->UpdateProgress(0.333);
  //
  //  Compute polygon normals
  //
  PolyNormals = vtkNormals::New();
  PolyNormals->Allocate(numPolys);
  PolyNormals->SetNumberOfNormals(numPolys);

  for (cellId=0, newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts); 
  cellId++ )
    {
    if ((cellId % 1000) == 0)
      {
      this->UpdateProgress ((float) cellId / (float) numPolys);
      }
    poly->ComputeNormal(inPts, npts, pts, n);
    PolyNormals->SetNormal(cellId,n);
    }
  //
  //  Traverse all nodes; evaluate loops and feature edges.  If feature
  //  edges found, split mesh creating new nodes.  Update element connectivity.
  //
  if ( this->Splitting ) 
    {
    CosAngle = cos ((double) vtkMath::DegreesToRadians() * this->FeatureAngle);
    //
    //  Splitting will create new points.  Have to create index array to map
    //  new points into old points.
    //
    Map = vtkIdList::New();
    Map->SetNumberOfIds(numPts);
    for (i=0; i < numPts; i++)
      {
      Map->SetId(i,i);
      }

    for (ptId=0; ptId < OldMesh->GetNumberOfPoints(); ptId++)
      {
      this->Mark++;
      replacementPoint = ptId;
      OldMesh->GetPointCells(ptId, cellIds);
      for (j=0; j < cellIds->GetNumberOfIds(); j++)
        {
        if ( Visited[cellIds->GetId(j)] != this->Mark )
          {
          this->MarkAndReplace(cellIds->GetId(j), ptId, replacementPoint,
                               PolyNormals, edgeNeighbors,
                               Visited, Map, OldMesh, NewMesh,
                               CosAngle);
          }

        replacementPoint = Map->GetNumberOfIds();
        }
      }

    numNewPts = Map->GetNumberOfIds();

    vtkDebugMacro(<<"Created " << numNewPts-numPts << " new points");
    //
    //  Now need to map values of old points into new points.
    //
    outPD->CopyNormalsOff();
    outPD->CopyAllocate(pd,numNewPts);

    newPts = vtkPoints::New(); newPts->SetNumberOfPoints(numNewPts);
    for (ptId=0; ptId < numNewPts; ptId++)
      {
      oldId = Map->GetId(ptId);
      newPts->SetPoint(ptId,inPts->GetPoint(oldId));
      outPD->CopyData(pd,oldId,ptId);
      }
    Map->Delete();
    } 
  else //no splitting
    {
    numNewPts = numPts;
    outPD->CopyNormalsOff();
    outPD->PassData(pd);
    }
  this->UpdateProgress(0.66);

  //
  //  Finally, traverse all elements, computing polygon normals and
  //  accumulating them at the vertices.
  //
  if ( Visited )
    {
    delete [] Visited;
    }

  if ( this->FlipNormals && ! this->Consistency )
    {
    flipDirection = -1.0;
    }

  newNormals = vtkNormals::New();
  newNormals->SetNumberOfNormals(numNewPts);
  n[0] = n[1] = n[2] = 0.0;
  for (i=0; i < numNewPts; i++)
    {
    newNormals->SetNormal(i,n);
    }

  if (this->ComputePointNormals)
    {
    for (cellId=0, newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts); 
          cellId++ )
      {
      polyNormal = PolyNormals->GetNormal(cellId);

      for (i=0; i < npts; i++) 
        {
        vertNormal = newNormals->GetNormal(pts[i]);
        for (j=0; j < 3; j++)
          {
          n[j] = vertNormal[j] + polyNormal[j];
          }
        newNormals->SetNormal(pts[i],n);
        }
      }

    for (i=0; i < numNewPts; i++) 
      {
      vertNormal = newNormals->GetNormal(i);
      length = vtkMath::Norm(vertNormal);
      if (length != 0.0) 
        {
        for (j=0; j < 3; j++)
          {
          n[j] = vertNormal[j] / length * flipDirection;
          }
        }
      newNormals->SetNormal(i,n);
      }
    }

  //
  //  Update ourselves.  If no new nodes have been created (i.e., no
  //  splitting), can simply pass data through.
  //
  if ( ! this->Splitting ) 
    {
    output->SetPoints(inPts);
    }
  //
  //  If there is splitting, then have to send down the new data.
  //
  else
    {
    output->SetPoints(newPts);
    newPts->Delete();
    }

  if (this->ComputeCellNormals)
    {
    outCD->SetNormals(PolyNormals);
    }
  PolyNormals->Delete();

  if (this->ComputePointNormals)
    {
    outPD->SetNormals(newNormals);
    }
  newNormals->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  cellIds->Delete();
  OldMesh->Delete();
  NewMesh->Delete();
  poly->Delete();
  edgeNeighbors->Delete();
  
  output->GetCellData()->SetGhostLevels(input->GetCellData()->GetGhostLevels());

  // Remove any ghost cells we inserted.
  if (ghostLevel > 0)
    {
    rmGhostCells = vtkRemoveGhostCells::New();
    ghost = vtkPolyData::New();
    ghost->ShallowCopy(output);
    rmGhostCells->SetInput(ghost);
    rmGhostCells->SetGhostLevel(ghostLevel);
    rmGhostCells->Update();
    output->ShallowCopy(rmGhostCells->GetOutput());
    
    ghost->Delete();
    rmGhostCells->Delete();
    }
}

//
//  Mark current polygon as visited, make sure that all neighboring
//  polygons are ordered consistent with this one.
//
int vtkPolyDataNormals::TraverseAndOrder (int cellId, vtkIdList *cellIds,
					  int *Visited, vtkPolyData *OldMesh, vtkPolyData *NewMesh)
{
  int p1, p2;
  int j, k, l;
  int npts, *pts;
  int numNeiPts, *neiPts, neighbor;
  int queuedCells = 0;

  Visited[cellId] = VTK_CELL_VISITED; //means that it's been ordered properly

  NewMesh->GetCellPoints(cellId, npts, pts);

  for (j=0; j < npts; j++) 
    {
    p1 = pts[j];
    p2 = pts[(j+1)%npts];

    OldMesh->GetCellEdgeNeighbors(cellId, p1, p2, cellIds);
    //
    //  Check the direction of the neighbor ordering.  Should be
    //  consistent with us (i.e., if we are n1->n2, neighbor should be n2->n1).
    //
    if ( cellIds->GetNumberOfIds() == 1 ||
    this->NonManifoldTraversal )
      {
      for (k=0; k < cellIds->GetNumberOfIds(); k++) 
        {
        if ( Visited[cellIds->GetId(k)] == VTK_CELL_NOT_VISITED) 
          {
          neighbor = cellIds->GetId(k);
          NewMesh->GetCellPoints(neighbor,numNeiPts,neiPts);
          for (l=0; l < numNeiPts; l++)
	    {
            if (neiPts[l] == p2)
	      {
               break;
	      }
	    }
	  //
	  //  Have to reverse ordering if neighbor not consistent
	  //
           if ( neiPts[(l+1)%numNeiPts] != p1 ) 
             {
             this->NumFlips++;
             NewMesh->ReverseCell(neighbor);
             }
	   Visited[neighbor] = VTK_CELL_NEEDS_VISITING;
	   queuedCells = 1;
          }
        } // for each edge neighbor
      } //for manifold or non-manifold traversal allowed
    } // for all edges of this polygon

  return queuedCells;
}

//
//  Mark polygons around vertex.  Create new vertex (if necessary) and
//  replace (i.e., split mesh).
//
void vtkPolyDataNormals::MarkAndReplace (int cellId, int n, 
					 int replacementPoint, vtkNormals *PolyNormals, vtkIdList *cellIds,
					 int *Visited, vtkIdList *Map,
					 vtkPolyData *OldMesh, vtkPolyData *NewMesh,
					 float CosAngle)
{
  int i, spot;
  int neiNode[2];
  float *thisNormal, *neiNormal;
  int numOldPts, *oldPts;
  int numNewPts, *newPts;

  Visited[cellId] = this->Mark;
  OldMesh->GetCellPoints(cellId,numOldPts,oldPts);
  //
  //  Replace the node if necessary
  //
  if ( n != replacementPoint ) 
    {
    Map->InsertId(replacementPoint, n);

    NewMesh->GetCellPoints(cellId,numNewPts,newPts);
    for (i=0; i < numNewPts; i++) 
      {
      if ( newPts[i] == n ) 
        {
        newPts[i] = replacementPoint; // this is very nasty! direct write!
        break;
        }
      }
    }
//
//  Look at neighbors who share central point and see whether a
//  feature edge separates us.  If not, can recursively call this
//  routine. 
//
  for (spot=0; spot < numOldPts; spot++)
    {
    if ( oldPts[spot] == n )
      {
      break;
      }
    }

  if ( spot == 0 ) 
    {
    neiNode[0] = oldPts[spot+1];
    neiNode[1] = oldPts[numOldPts-1];
    } 
  else if ( spot == (numOldPts-1) ) 
    {
    neiNode[0] = oldPts[spot-1];
    neiNode[1] = oldPts[0];
    } 
  else 
    {
    neiNode[0] = oldPts[spot+1];
    neiNode[1] = oldPts[spot-1];
    }

  for (i=0; i<2; i++) 
    {
    OldMesh->GetCellEdgeNeighbors(cellId, n, neiNode[i], cellIds);
    if ( cellIds->GetNumberOfIds() == 1 && Visited[cellIds->GetId(0)] != this->Mark)
      {
      thisNormal = PolyNormals->GetNormal(cellId);
      neiNormal =  PolyNormals->GetNormal(cellIds->GetId(0));

      if ( vtkMath::Dot(thisNormal,neiNormal) > CosAngle )
	{
	// NOTE: cellIds is reused recursively without harm because
	// after the recusion call, it is no longer used
        this->MarkAndReplace (cellIds->GetId(0), n, replacementPoint,
			      PolyNormals, cellIds,
			      Visited, Map, OldMesh, NewMesh,
			      CosAngle);
	}
      }
    }

  return;
}

void vtkPolyDataNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Splitting: " << (this->Splitting ? "On\n" : "Off\n");
  os << indent << "Consistency: " << (this->Consistency ? "On\n" : "Off\n"); 
  os << indent << "Flip Normals: " << (this->FlipNormals ? "On\n" : "Off\n");
  os << indent << "Compute Point Normals: " << (this->ComputePointNormals ? "On\n" : "Off\n");
  os << indent << "Compute Cell Normals: " << (this->ComputeCellNormals ? "On\n" : "Off\n");
  os << indent << "Maximum Recursion Depth: " << this->MaxRecursionDepth << "\n";
  os << indent << "Non-manifold Traversal: " << 
    (this->NonManifoldTraversal ? "On\n" : "Off\n");
}

void vtkPolyDataNormals::ComputeInputUpdateExtents(vtkDataObject *output)
{
  int numPieces, ghostLevel;
  
  this->vtkPolyDataSource::ComputeInputUpdateExtents(output);

  numPieces = output->GetUpdateNumberOfPieces();
  ghostLevel = output->GetUpdateGhostLevel();
  if (numPieces > 1)
    {
    this->GetInput()->SetUpdateGhostLevel(ghostLevel + 1);
    }
}
