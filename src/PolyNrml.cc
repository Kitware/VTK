/*=========================================================================

  Program:   Visualization Library
  Module:    PolyNrml.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PolyNrml.hh"
#include "Polygon.hh"
#include "vlMath.hh"
#include "FNormals.hh"

// Description:
// Construct with feature angle=30, splitting and consistency turned on, 
// and flipNormals turned off.
vlPolyNormals::vlPolyNormals()
{
  this->FeatureAngle = 30.0;
  this->Splitting = 1;
  this->Consistency = 1;
  this->FlipNormals = 0;
  this->MaxRecursionDepth = 10000;
}

static  int NumFlips=0, NumExceededMaxDepth=0;
static  int *Visited;
static  vlPolyData *OldMesh, *NewMesh;
static  int RecursionDepth;
static  int Mark;    
static  vlFloatNormals *PolyNormals;
static	float	CosAngle;
static  vlIdList *Seeds, *Map;

// Generate normals for polygon meshes

void vlPolyNormals::Execute()
{
  int i, j;
  int *pts, npts;
  int numNewPts;
  float *polyNormal, *vertNormal, length;
  float flipDirection=1.0;
  int replacementPoint, numPolys;
  int cellId, numPts;
  vlPoints *inPts;
  vlCellArray *inPolys;
  vlPolygon poly;
  vlMath math;
  vlFloatPoints *newPts;
  vlFloatNormals *newNormals;
  vlPointData *pd;
  float n[3];
  vlCellArray *newPolys;
  int ptId, oldId;
  vlIdList cellIds(MAX_CELL_SIZE);

  vlDebugMacro(<<"Generating surface normals");
  this->Initialize();

  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 || 
  (numPolys=this->Input->GetNumberOfPolys()) < 1 )
    {
    vlErrorMacro(<<"No data to generate normals for!");
    return;
    }
//
// Load data into cell structure.  We need two copies: one is a 
// non-writable mesh used to perform topological queries.  The other 
// is used to write into and modify the connectivity of the mesh.
//
  inPts = this->Input->GetPoints();
  inPolys = this->Input->GetPolys();

  OldMesh = new vlPolyData;
  OldMesh->SetPoints(inPts);
  OldMesh->SetPolys(inPolys);
  
  pd = this->Input->GetPointData();
    
  NewMesh = new vlPolyData;
  NewMesh->SetPoints(inPts);
  // create a copy because we're modifying it
  newPolys = new vlCellArray(*(inPolys));
  NewMesh->SetPolys(newPolys);
//
// The visited array keeps track of which polygons have been visited.
//
  if ( this->Consistency || this->Splitting ) 
    {
    Visited = new int[numPolys];
    for ( i=0; i < numPolys; i++) Visited[i] = 0;
    Mark = 1;
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
  if ( this->Consistency ) 
    {    
    NumFlips = 0;
    Seeds = new vlIdList(1000,1000);

    for (cellId=0; cellId < numPolys; cellId++)
      {
      if ( ! Visited[cellId] ) 
        {
        if ( this->FlipNormals ) 
          {
          NumFlips++;
          NewMesh->ReverseCell(cellId);
          }
        RecursionDepth = 0;
        this->TraverseAndOrder(cellId);
        }

      for (i=0; i < Seeds->GetNumberOfIds(); i++) 
        {
        RecursionDepth = 0;
        this->TraverseAndOrder (Seeds->GetId(i));
        }

      Seeds->Reset();
      }
    vlDebugMacro(<<"Reversed ordering of " << NumFlips << " polygons");
    vlDebugMacro(<<"Exceeded recursion depth " << NumExceededMaxDepth
                 <<" times");

    delete Seeds;
    }
//
//  Compute polygon normals
//
  PolyNormals = new vlFloatNormals(numPolys);

  for (cellId=0, newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts); 
  cellId++ )
    {
    poly.ComputeNormal(inPts, npts, pts, n);
    PolyNormals->SetNormal(cellId,n);
    }
//
//  Traverse all nodes; evaluate loops and feature edges.  If feature
//  edges found, split mesh creating new nodes.  Update element connectivity.
//
  if ( this->Splitting ) 
    {
    CosAngle = cos ((double) math.DegreesToRadians() * this->FeatureAngle);
//
//  Splitting will create new points.  Have to create index array to map
//  new points into old points.
//
    Map = new vlIdList(numPts,numPts);
    for (i=0; i < numPts; i++) Map->SetId(i,i);

    for (ptId=0; ptId < OldMesh->GetNumberOfPoints(); ptId++)
      {
      Mark++;
      replacementPoint = ptId;
      OldMesh->GetPointCells(ptId,cellIds);
      for (j=0; j < cellIds.GetNumberOfIds(); j++)
        {
        if ( Visited[cellIds.GetId(j)] != Mark )
          this->MarkAndReplace (cellIds.GetId(j), ptId, replacementPoint);

        replacementPoint = Map->GetNumberOfIds();
        }
      }

    Map->Squeeze();
    numNewPts = Map->GetNumberOfIds() - 1;

    vlDebugMacro(<<"Created " << numNewPts-numPts << " new points");
//
//  Now need to map values of old points into new points.
//
    this->PointData.CopyNormalsOff();
    this->PointData.CopyAllocate(pd,numNewPts);

    newPts = new vlFloatPoints(numNewPts);
    for (ptId=0; ptId < numNewPts; ptId++)
      {
      oldId = Map->GetId(ptId);
      newPts->SetPoint(ptId,inPts->GetPoint(oldId));
      this->PointData.CopyData(pd,oldId,ptId);
      }
    delete Map;
    } 
  else //no splitting
    {
    numNewPts = numPts;
    }
//
//  Finally, traverse all elements, computing polygon normals and
//  accumalating them at the vertices.
//
  if ( Visited ) delete Visited;

  if ( this->FlipNormals && ! this->Consistency ) flipDirection = -1.0;

  newNormals = new vlFloatNormals(numNewPts);
  n[0] = n[1] = n[2] = 0.0;
  for (i=0; i < numNewPts; i++) newNormals->SetNormal(i,n);

  for (cellId=0, newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts); 
  cellId++ )
    {
    polyNormal = PolyNormals->GetNormal(cellId);

    for (i=0; i < npts; i++) 
      {
      vertNormal = newNormals->GetNormal(pts[i]);
      for (j=0; j < 3; j++) n[j] = vertNormal[j] + polyNormal[j];
      newNormals->SetNormal(pts[i],n);
      }
    }

  for (i=0; i < numNewPts; i++) 
    {
    vertNormal = newNormals->GetNormal(i);
    length = math.Norm(vertNormal);
    if (length != 0.0) 
      {
      for (j=0; j < 3; j++) n[j] = vertNormal[j] / length * flipDirection;
      }
    newNormals->SetNormal(i,n);
    }
  delete PolyNormals;
//
//  Update ourselves.  If no new nodes have been created (i.e., no
//  splitting), can simply pass data through.
//
  if ( ! this->Splitting ) 
    {
    this->SetPoints(inPts);
    }
//
//  If there is splitting, then have to send down the new data.
//
  else
    {
    this->SetPoints(newPts);
    }

  this->PointData.SetNormals(newNormals);
  this->SetPolys(newPolys);

  delete OldMesh;
  delete NewMesh;
}

//
//  Mark current polygon as visited, make sure that all neighboring
//  polygons are ordered consistent with this one.
//
void vlPolyNormals::TraverseAndOrder (int cellId)
{
  int p1, p2;
  int j, k, l;
  vlIdList ptIds(MAX_CELL_SIZE), neiPtIds(MAX_CELL_SIZE);
  vlIdList cellIds(MAX_CELL_SIZE);
  vlIdList edge(2);
  int npts, neighbor;

  Visited[cellId] = Mark; //means that it's been ordered properly

  if ( RecursionDepth++ > this->MaxRecursionDepth ) 
    {
    Seeds->InsertNextId(cellId);
    NumExceededMaxDepth++;
    return;
    }

  NewMesh->GetCellPoints(cellId, ptIds);
  npts = ptIds.GetNumberOfIds();

  for (j=0; j < npts; j++) 
    {
    p1 = ptIds.GetId(j);
    p2 = ptIds.GetId((j+1)%npts);

    edge.SetId(0,p1);
    edge.SetId(1,p2);

    OldMesh->GetCellNeighbors(cellId, edge, cellIds);
//
//  Check the direction of the neighbor ordering.  Should be
//  consistent with us (i.e., if we are n1->n2, neighbor should be n2->n1).
//
    for (k=0; k < cellIds.GetNumberOfIds(); k++) 
      {
      if ( ! Visited[cellIds.GetId(k)] ) 
        {
        neighbor = cellIds.GetId(k);
        NewMesh->GetCellPoints(neighbor,neiPtIds);
        for (l=0; l < neiPtIds.GetNumberOfIds(); l++)
          if (neiPtIds.GetId(l) == p2)
             break;
//
//  Have to reverse ordering if neighbor not consistent
//
         if ( neiPtIds.GetId((l+1)%neiPtIds.GetNumberOfIds()) != p1 ) 
           {
           NewMesh->ReverseCell(neighbor);
           }
         this->TraverseAndOrder (neighbor);
        }
      } // for each edge neighbor
    } // for all edges of this polygon

  RecursionDepth--;
  return;
}

//
//  Mark polygons around vertex.  Create new vertex (if necessary) and
//  replace (i.e., split mesh).
//
void vlPolyNormals::MarkAndReplace (int cellId, int n, int replacementPoint)
{
  int i, spot;
  int neiNode[2];
  float *thisNormal, *neiNormal;
  int npts;
  vlIdList oldPts(MAX_CELL_SIZE), ptIds(MAX_CELL_SIZE);
  vlIdList edge(2), cellIds(MAX_CELL_SIZE), newPtIds;
  vlMath math;

  Visited[cellId] = Mark;
  OldMesh->GetCellPoints(cellId,oldPts);
  npts = oldPts.GetNumberOfIds();
//
//  Replace the node if necessary
//
  if ( n != replacementPoint ) 
    {
    Map->InsertId(replacementPoint, n);

    NewMesh->GetCellPoints(cellId,ptIds);
    newPtIds = ptIds;
    for (i=0; i < newPtIds.GetNumberOfIds(); i++) 
      {
      if ( newPtIds.GetId(i) == n ) 
        {
        newPtIds.SetId(i,replacementPoint);
        NewMesh->ReplaceCell(cellId,newPtIds);
        break;
        }
      }
    }
//
//  Look at neighbors who share central point and see whether a
//  feature edge separates us.  If not, can recusrsively call this
//  routine. 
//
  for (spot=0; spot < npts; spot++)
    if ( oldPts.GetId(spot) == n )
      break;

  if ( spot == 0 ) 
    {
    neiNode[0] = oldPts.GetId(spot+1);
    neiNode[1] = oldPts.GetId(npts-1);
    } 
  else if ( spot == (npts-1) ) 
    {
    neiNode[0] = oldPts.GetId(spot-1);
    neiNode[1] = oldPts.GetId(0);
    } 
  else 
    {
    neiNode[0] = oldPts.GetId(spot+1);
    neiNode[1] = oldPts.GetId(spot-1);
    }

  edge.SetId(0,n);

  for (i=0; i<2; i++) 
    {
    edge.SetId(1,neiNode[i]);
    OldMesh->GetCellNeighbors(cellId, edge, cellIds);
    if ( cellIds.GetNumberOfIds() == 1 && Visited[cellIds.GetId(0)] != Mark ) 
      {
      thisNormal = PolyNormals->GetNormal(cellId);
      neiNormal =  PolyNormals->GetNormal(cellIds.GetId(0));

      if ( math.Dot(thisNormal,neiNormal) > CosAngle )
        this->MarkAndReplace (cellIds.GetId(0), n, replacementPoint);
      }
    }

  return;
}

void vlPolyNormals::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPolyNormals::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
    os << indent << "Splitting: " << (this->Splitting ? "On\n" : "Off\n");
    os << indent << "Consistency: " << (this->Consistency ? "On\n" : "Off\n"); 
    os << indent << "Flip Normals: " << (this->FlipNormals ? "On\n" : "Off\n");
    os << indent << "Maximum Recursion Depth: " << this->MaxRecursionDepth << "\n";
   }
}

