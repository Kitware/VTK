/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FeatEdge.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FeatEdge.hh"
#include "vtkMath.hh"
#include "Polygon.hh"
#include "FNormals.hh"

// Description:
// Construct object with feature angle = 30; all types of edges extracted
// and colored.
vtkFeatureEdges::vtkFeatureEdges()
{
  this->FeatureAngle = 30.0;
  this->BoundaryEdges = 1;
  this->FeatureEdges = 1;
  this->NonManifoldEdges = 1;
  this->Coloring = 1;
}

// Generate feature edges for mesh
void vtkFeatureEdges::Execute()
{
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkFloatScalars *newScalars;
  vtkCellArray *newLines;
  vtkPolyData Mesh;
  int i, j, numNei, cellId;
  int numBEdges, numNonManifoldEdges, numFedges;
  float scalar, n[3], *x1, *x2, cosAngle;
  vtkMath math;
  vtkPolygon poly;
  int lineIds[2];
  int npts, *pts;
  vtkCellArray *inPolys;
  vtkFloatNormals *polyNormals;
  int numPts, nei;
  vtkIdList neighbors(MAX_CELL_SIZE);
  int p1, p2;

  vtkDebugMacro(<<"Executing feature edges");
  this->Initialize();
//
//  Check input
//
  if ( (numPts=input->GetNumberOfPoints()) < 1 || 
  (inPts=input->GetPoints()) == NULL || 
  (inPolys=input->GetPolys()) == NULL )
    {
    vtkErrorMacro(<<"No input data!");
    return;
    }

  if ( !this->BoundaryEdges && !this->NonManifoldEdges && !this->FeatureEdges) 
    {
    vtkWarningMacro(<<"All edge types turned off!");
    return;
    }

  // build cell structure.  Only operate with polygons.
  Mesh.SetPoints(inPts);
  Mesh.SetPolys(inPolys);
  Mesh.BuildLinks();
//
//  Allocate storage for lines/points
//
  newPts = new vtkFloatPoints(numPts/10,numPts); // arbitrary allocations size 
  newScalars = new vtkFloatScalars(numPts/10,numPts);
  newLines = new vtkCellArray(numPts/10);
//
//  Loop over all polygons generating boundary, non-manifold, and feature edges
//
  if ( this->FeatureEdges ) 
    {    
    polyNormals = new vtkFloatNormals(inPolys->GetNumberOfCells());

    for (cellId=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); 
    cellId++)
      {
      poly.ComputeNormal(inPts,npts,pts,n);
      polyNormals->InsertNormal(cellId,n);
      }

    cosAngle = cos ((double) math.DegreesToRadians() * this->FeatureAngle);
    }

  numBEdges = numNonManifoldEdges = numFedges = 0;
  for (cellId=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); 
  cellId++)
    {
    for (i=0; i < npts; i++) 
      {
      p1 = pts[i];
      p2 = pts[(i+1)%npts];

      Mesh.GetCellEdgeNeighbors(cellId,p1,p2,neighbors);
      numNei = neighbors.GetNumberOfIds();

      if ( this->BoundaryEdges && numNei < 1 )
        {
        numBEdges++;
        scalar = 0.0;
        }

      else if ( this->NonManifoldEdges && numNei > 1 )
        {
        // check to make sure that this edge hasn't been created before
        for (j=0; j < numNei; j++)
          if ( neighbors.GetId(j) < cellId )
            break;
        if ( j >= numNei )
          {
          numNonManifoldEdges++;
          scalar = 0.33333;
          }
        else continue;
        }

      else if ( this->FeatureEdges && 
      numNei == 1 && (nei=neighbors.GetId(0)) > cellId ) 
        {
        if ( math.Dot(polyNormals->GetNormal(nei),polyNormals->GetNormal(cellId)) <= cosAngle ) 
          {
          numFedges++;
          scalar = 0.66667;
          }
        else continue;
        }

      else continue;

      // Add edge to output
      x1 = Mesh.GetPoint(p1);
      x2 = Mesh.GetPoint(p2);

      lineIds[0] = newPts->InsertNextPoint(x1);
      lineIds[1] = newPts->InsertNextPoint(x2);

      newLines->InsertNextCell(2,lineIds);

      newScalars->InsertScalar(lineIds[0], scalar);
      newScalars->InsertScalar(lineIds[1], scalar);
      }
    }

  vtkDebugMacro(<<"Created " << numBEdges << " boundary edges, " <<
               numNonManifoldEdges << " non-manifold edges, " <<
               numFedges << " feature edges");

//
//  Update ourselves.
//
  if ( this->FeatureEdges ) delete polyNormals;

  this->SetPoints(newPts);
  this->SetLines(newLines);
  if ( this->Coloring )
    this->PointData.SetScalars(newScalars);
  else
    delete newScalars;
}

void vtkFeatureEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Boundary Edges: " << (this->BoundaryEdges ? "On\n" : "Off\n");
  os << indent << "Feature Edges: " << (this->FeatureEdges ? "On\n" : "Off\n"); 
  os << indent << "Non-Manifold Edges: " << (this->NonManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Coloring: " << (this->Coloring ? "On\n" : "Off\n");
}

