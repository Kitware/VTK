/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataNormals.cxx
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
#include "vtkPolyDataNormals.h"
#include "vtkMath.h"
#include "vtkNormals.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
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
  // some internal data
  this->NumFlips = 0;
}

#define VTK_CELL_NOT_VISITED     0
#define VTK_CELL_VISITED         1

// Generate normals for polygon meshes
void vtkPolyDataNormals::Execute()
{
  int j;
  vtkIdType npts, i;
  vtkIdType *pts;
  vtkIdType numNewPts;
  float *polyNormal, *vertNormal, length;
  float flipDirection=1.0;
  vtkIdType numPolys, numStrips;
  vtkIdType cellId;
  vtkIdType numPts;
  vtkPoints *inPts;
  vtkCellArray *inPolys, *inStrips, *polys;
  vtkPoints *newPts = NULL;
  vtkNormals *newNormals;
  vtkPointData *pd, *outPD;
  vtkCellData *outCD;
  float n[3];
  vtkCellArray *newPolys;
  vtkIdType ptId, oldId;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  vtkDebugMacro(<<"Generating surface normals");

  numPolys=input->GetNumberOfPolys();
  numStrips=input->GetNumberOfStrips();
  if ( (numPts=input->GetNumberOfPoints()) < 1 || 
       (numPolys < 1 && numStrips < 1) )
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
	output->SetFieldData(input->GetFieldData());
    return;
    }
  output->GetCellData()->PassData(input->GetCellData());
  output->SetFieldData(input->GetFieldData());

  // Load data into cell structure.  We need two copies: one is a 
  // non-writable mesh used to perform topological queries.  The other 
  // is used to write into and modify the connectivity of the mesh.
  //
  inPts = input->GetPoints();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
  
  this->OldMesh = vtkPolyData::New();
  this->OldMesh->SetPoints(inPts);
  if ( numStrips > 0 ) //have to decompose strips into triangles
    {
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
      vtkTriangleStrip::DecomposeStrip(npts, pts, polys);
      }
    this->OldMesh->SetPolys(polys);
    polys->Delete();
    numPolys = polys->GetNumberOfCells();//added some new triangles
    }
  else
    {
    this->OldMesh->SetPolys(inPolys);
    polys = inPolys;
    }
  this->OldMesh->BuildLinks();
  this->UpdateProgress(0.10);
  
  pd = input->GetPointData();
  outPD = output->GetPointData();
    
  outCD = output->GetCellData();
    
  this->NewMesh = vtkPolyData::New();
  this->NewMesh->SetPoints(inPts);
  // create a copy because we're modifying it
  newPolys = vtkCellArray::New();
  newPolys->DeepCopy(polys);
  this->NewMesh->SetPolys(newPolys);
  this->NewMesh->BuildCells(); //builds connectivity

  // The visited array keeps track of which polygons have been visited.
  //
  if ( this->Consistency || this->Splitting ) 
    {
    this->Visited = new int[numPolys];
    memset(this->Visited, VTK_CELL_NOT_VISITED, numPolys*sizeof(int));
    this->CellIds = vtkIdList::New();
    this->CellIds->Allocate(VTK_CELL_SIZE);
    }
  else
    {
    this->Visited = NULL;
    }

  //  Traverse all polygons insuring proper direction of ordering.  This
  //  works by propagating a wave from a seed polygon to the polygon's
  //  edge neighbors. Each neighbor may be reordered to maintain consistency
  //  with its (already checked) neighbors.
  //
  this->NumFlips = 0;
  if ( this->Consistency ) 
    {    
    this->Wave = vtkIdList::New();
    this->Wave->Allocate(numPolys/4+1,numPolys);
    this->Wave2 = vtkIdList::New();
    this->Wave2->Allocate(numPolys/4+1,numPolys);
    for (cellId=0; cellId < numPolys; cellId++)
      {
      if ( this->Visited[cellId] == VTK_CELL_NOT_VISITED) 
        {
        if ( this->FlipNormals ) 
          {
          this->NumFlips++;
          this->NewMesh->ReverseCell(cellId);
          }
        this->Wave->InsertNextId(cellId);
        this->Visited[cellId] = VTK_CELL_VISITED; 
        this->TraverseAndOrder();
        }

      this->Wave->Reset();
      this->Wave2->Reset(); 
      }

    this->Wave->Delete();
    this->Wave2->Delete();
    vtkDebugMacro(<<"Reversed ordering of " << this->NumFlips << " polygons");
    }//Consistent ordering
  
  this->UpdateProgress(0.333);

  //  Initial pass to compute polygon normals without effects of neighbors
  //
  this->PolyNormals = vtkNormals::New();
  this->PolyNormals->Allocate(numPolys);
  this->PolyNormals->GetData()->SetName("Normals");
  this->PolyNormals->SetNumberOfNormals(numPolys);

  for (cellId=0, newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts); 
       cellId++ )
    {
    if ((cellId % 1000) == 0)
      {
      this->UpdateProgress (0.333 + 0.333 * (float) cellId / (float) numPolys);
      if (this->GetAbortExecute())
        {
        break; 
        }
      }
    vtkPolygon::ComputeNormal(inPts, npts, pts, n);
    this->PolyNormals->SetNormal(cellId,n);
    }

  // Split mesh if sharp features
  if ( this->Splitting ) 
    {
    //  Traverse all nodes; evaluate loops and feature edges.  If feature
    //  edges found, split mesh creating new nodes.  Update polygon 
    // connectivity.
    //
    this->CosAngle = cos ((double) 
                          vtkMath::DegreesToRadians() * this->FeatureAngle);
    //  Splitting will create new points.  We have to create index array 
    // to map new points into old points.
    //
    this->Map = vtkIdList::New();
    this->Map->SetNumberOfIds(numPts);
    for (i=0; i < numPts; i++)
      {
      this->Map->SetId(i,i);
      }

    for (ptId=0; ptId < numPts; ptId++)
      {
      this->MarkAndSplit(ptId);
      }//for all input points

    numNewPts = this->Map->GetNumberOfIds();

    vtkDebugMacro(<<"Created " << numNewPts-numPts << " new points");

    //  Now need to map attributes of old points into new points.
    //
    outPD->CopyNormalsOff();
    outPD->CopyAllocate(pd,numNewPts);

    newPts = vtkPoints::New(); newPts->SetNumberOfPoints(numNewPts);
    for (ptId=0; ptId < numNewPts; ptId++)
      {
      oldId = this->Map->GetId(ptId);
      newPts->SetPoint(ptId,inPts->GetPoint(oldId));
      outPD->CopyData(pd,oldId,ptId);
      }
    this->Map->Delete();
    } //splitting

  else //no splitting, so no new points
    {
    numNewPts = numPts;
    outPD->CopyNormalsOff();
    outPD->PassData(pd);
    }

  if ( this->Consistency || this->Splitting )
    {
    delete [] this->Visited;
    this->CellIds->Delete();
    }

  this->UpdateProgress(0.80);

  //  Finally, traverse all elements, computing polygon normals and
  //  accumulating them at the vertices.
  //
  if ( this->FlipNormals && ! this->Consistency )
    {
    flipDirection = -1.0;
    }

  newNormals = vtkNormals::New();
  newNormals->SetNumberOfNormals(numNewPts);
  newNormals->GetData()->SetName("Normals");
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
      polyNormal = this->PolyNormals->GetNormal(cellId);

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

  //  Update ourselves.  If no new nodes have been created (i.e., no
  //  splitting), we can simply pass data through.
  //
  if ( ! this->Splitting ) 
    {
    output->SetPoints(inPts);
    }

  //  If there is splitting, then have to send down the new data.
  //
  else
    {
    output->SetPoints(newPts);
    newPts->Delete();
    }

  if (this->ComputeCellNormals)
    {
    outCD->SetNormals(this->PolyNormals);
    }
  this->PolyNormals->Delete();

  if (this->ComputePointNormals)
    {
    outPD->SetNormals(newNormals);
    }
  newNormals->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  this->OldMesh->Delete();
  this->NewMesh->Delete();
}

//  Propagate wave of consistently ordered polygons.
//
void vtkPolyDataNormals::TraverseAndOrder (void)
{
  vtkIdType p1, p2, i, k;
  int j, l;
  vtkIdType numIds, cellId;
  vtkIdType *pts, *neiPts, npts, numNeiPts;
  vtkIdType neighbor;
  vtkIdList *tmpWave;

  // propagate wave until nothing left in wave
  while ( (numIds=this->Wave->GetNumberOfIds()) > 0 )
    {
    for ( i=0; i < numIds; i++ )
      {
      cellId = this->Wave->GetId(i);

      this->NewMesh->GetCellPoints(cellId, npts, pts);

      for (j=0; j < npts; j++) //for each edge neighbor
        {
        p1 = pts[j];
        p2 = pts[(j+1)%npts];

        this->OldMesh->GetCellEdgeNeighbors(cellId, p1, p2, this->CellIds);

        //  Check the direction of the neighbor ordering.  Should be
        //  consistent with us (i.e., if we are n1->n2, 
        // neighbor should be n2->n1).
        if ( this->CellIds->GetNumberOfIds() == 1 ||
             this->NonManifoldTraversal )
          {
          for (k=0; k < this->CellIds->GetNumberOfIds(); k++) 
            {
            if (this->Visited[this->CellIds->GetId(k)]==VTK_CELL_NOT_VISITED) 
              {
              neighbor = this->CellIds->GetId(k);
              this->NewMesh->GetCellPoints(neighbor,numNeiPts,neiPts);
              for (l=0; l < numNeiPts; l++)
                {
                if (neiPts[l] == p2)
                  {
                  break;
                  }
                }

              //  Have to reverse ordering if neighbor not consistent
              //
              if ( neiPts[(l+1)%numNeiPts] != p1 ) 
                {
                this->NumFlips++;
                this->NewMesh->ReverseCell(neighbor);
                }
              this->Visited[neighbor] = VTK_CELL_VISITED; 
              this->Wave2->InsertNextId(neighbor);
              }// if cell not visited
            } // for each edge neighbor
          } //for manifold or non-manifold traversal allowed
        } // for all edges of this polygon
      } //for all cells in wave

    //swap wave and proceed with propagation
    tmpWave = this->Wave;
    this->Wave = this->Wave2;
    this->Wave2 = tmpWave;
    this->Wave2->Reset();
    } //while wave still propagating

  return;
}

//
//  Mark polygons around vertex.  Create new vertex (if necessary) and
//  replace (i.e., split mesh).
//
void vtkPolyDataNormals::MarkAndSplit (vtkIdType ptId)
{
  int i,j;

  // Get the cells using this point and make sure that we have to do something
  unsigned short ncells;
  vtkIdType *cells;
  this->OldMesh->GetPointCells(ptId,ncells,cells);
  if ( ncells <= 1 )
    {
    return; //point does not need to be further disconnected
    }

  // Start moving around the "cycle" of points using the point. Label
  // each point as requiring a visit. Then label each subregion of cells
  // connected to this point that are connected (and not separated by
  // a feature edge) with a given region number. For each N regions
  // created, N-1 duplicate (split) points are created. The split point
  // replaces the current point ptId in the polygons connectivity array.
  //
  // Start by initializing the cells as unvisited
  for (i=0; i<ncells; i++)
    {
    this->Visited[cells[i]] = -1;
    }

  // Loop over all cells and mark the region that each is in.
  //
  vtkIdType numPts;
  vtkIdType *pts;
  int numRegions = 0;
  vtkIdType spot, neiPt[2], nei, cellId, neiCellId;
  float *thisNormal, *neiNormal;
  for (j=0; j<ncells; j++) //for all cells connected to point
    {
    if ( this->Visited[cells[j]] < 0 ) //for all unvisited cells
      {
      this->Visited[cells[j]] = numRegions;
      //okay, mark all the cells connected to this seed cell and using ptId
      this->OldMesh->GetCellPoints(cells[j],numPts,pts);

      //find the two edges
      for (spot=0; spot < numPts; spot++)
        {
        if ( pts[spot] == ptId )
          {
          break;
          }
        }

      if ( spot == 0 ) 
        {
        neiPt[0] = pts[spot+1];
        neiPt[1] = pts[numPts-1];
        } 
      else if ( spot == (numPts-1) ) 
        {
        neiPt[0] = pts[spot-1];
        neiPt[1] = pts[0];
        } 
      else 
        {
        neiPt[0] = pts[spot+1];
        neiPt[1] = pts[spot-1];
        }

      for (i=0; i<2; i++) //for each of the two edges of the seed cell
        {
        cellId = cells[j];
        nei = neiPt[i];
        while ( cellId >= 0 ) //while we can grow this region
          {
          this->OldMesh->GetCellEdgeNeighbors(cellId,ptId,nei,this->CellIds);
          if ( this->CellIds->GetNumberOfIds() == 1 && 
               this->Visited[(neiCellId=this->CellIds->GetId(0))] < 0 )
            {
            thisNormal = this->PolyNormals->GetNormal(cellId);
            neiNormal =  this->PolyNormals->GetNormal(neiCellId);

            if ( vtkMath::Dot(thisNormal,neiNormal) > CosAngle )
              {
              //visit and arrange to visit next edge neighbor
              this->Visited[neiCellId] = numRegions;
              cellId = neiCellId;
              this->OldMesh->GetCellPoints(cellId,numPts,pts);

              for (spot=0; spot < numPts; spot++)
                {
                if ( pts[spot] == ptId )
                  {
                  break;
                  }
                }

              if (spot == 0) 
                {
                nei = (pts[spot+1] != nei ? pts[spot+1] : pts[numPts-1]);
                } 
              else if (spot == (numPts-1)) 
                {
                nei = (pts[spot-1] != nei ? pts[spot-1] : pts[0]);
                } 
              else 
                {
                nei = (pts[spot+1] != nei ? pts[spot+1] : pts[spot-1]);
                }

              }//if not separated by edge angle
            else
              {
              cellId = -1; //separated by edge angle
              }
            }//if can move to edge neighbor
          else
            {
            cellId = -1;//separated by previous visit, bounary, or non-manifold
            }
          }//while visit wave is propagating
        }//for each of the two edges of the starting cell
      numRegions++;
      }//if cell is unvisited
    }//for all cells connected to point ptId
  
  if ( numRegions <=1 )
    {
    return; //a single region, no splitting ever required
    }

  // Okay, for all cells not in the first region, the ptId is
  // replaced with a new ptId, which is a duplicate of the first
  // point, but disconnected topologically.
  //
  vtkIdType lastId = this->Map->GetNumberOfIds();
  vtkIdType replacementPoint;
  for (j=0; j<ncells; j++)
    {
    if (this->Visited[cells[j]] > 0 ) //replace point if splitting needed
      {
      replacementPoint = lastId + this->Visited[cells[j]] - 1;
      
      this->Map->InsertId(replacementPoint, ptId);

      this->NewMesh->GetCellPoints(cells[j],numPts,pts);
      for (i=0; i < numPts; i++) 
        {
        if ( pts[i] == ptId ) 
          {
          pts[i] = replacementPoint; // this is very nasty! direct write!
          break;
          }
        }//replace ptId with split point
      }//if not in first regions and requiring splitting
    }//for all cells connected to ptId

  return;
}

void vtkPolyDataNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Splitting: " << (this->Splitting ? "On\n" : "Off\n");
  os << indent << "Consistency: " << (this->Consistency ? "On\n" : "Off\n"); 
  os << indent << "Flip Normals: " << (this->FlipNormals ? "On\n" : "Off\n");
  os << indent << "Compute Point Normals: " 
     << (this->ComputePointNormals ? "On\n" : "Off\n");
  os << indent << "Compute Cell Normals: " 
     << (this->ComputeCellNormals ? "On\n" : "Off\n");
  os << indent << "Non-manifold Traversal: " 
     << (this->NonManifoldTraversal ? "On\n" : "Off\n");
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
