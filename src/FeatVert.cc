/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FeatVert.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FeatVert.hh"
#include "vtkMath.hh"

// Description:
// Construct object with feature angle = 30; all types of vertices extracted
// and colored.
vtkFeatureVertices::vtkFeatureVertices()
{
  this->FeatureAngle = 30.0;
  this->BoundaryVertices = 1;
  this->FeatureVertices = 1;
  this->NonManifoldVertices = 1;
  this->Coloring = 1;
}

// Generate feature vertices for mesh
void vtkFeatureVertices::Execute()
{
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkFloatScalars *newScalars;
  vtkCellArray *newVerts;
  vtkPolyData Mesh;
  int i, j, numCells, cellId, numPts;
  int numBVertices, numNonManifoldVertices, numFvertices;
  float scalar, *x1, x[3], xPrev[3], xNext[3], cosAngle;
  float vPrev[3], vNext[3];
  vtkMath math;
  int vertId[1];
  int npts, *pts;
  vtkCellArray *inLines;
  vtkIdList cells(MAX_CELL_SIZE);

  vtkDebugMacro(<<"Executing feature vertices");
  this->Initialize();
//
//  Check input
//
  if ( (numPts=input->GetNumberOfPoints()) < 1 || 
  (inPts=input->GetPoints()) == NULL || 
  (inLines=input->GetPolys()) == NULL )
    {
    vtkErrorMacro(<<"No input data!");
    return;
    }

  if ( !this->BoundaryVertices && !this->NonManifoldVertices && !this->FeatureVertices) 
    {
    vtkWarningMacro(<<"All vertex types turned off!");
    return;
    }

  // build cell structure.  Only operate with polygons.
  Mesh.SetPoints(inPts);
  Mesh.SetLines(inLines);
  Mesh.BuildLinks();
//
//  Allocate storage for lines/points
//
  newPts = new vtkFloatPoints(numPts/10,numPts); // arbitrary allocations size 
  newScalars = new vtkFloatScalars(numPts/10,numPts);
  newVerts = new vtkCellArray(numPts/10);
//
//  Loop over all lines generating boundary, non-manifold, and feature vertices
//
  cosAngle = cos ((double) math.DegreesToRadians() * this->FeatureAngle);

  numBVertices = numNonManifoldVertices = numFvertices = 0;
  for (cellId=0, inLines->InitTraversal(); inLines->GetNextCell(npts,pts); 
  cellId++)
    {
    for (i=0; i < npts; i++) 
      {

      Mesh.GetPointCells(pts[i],cells);
      numCells = cells.GetNumberOfIds();

      if ( this->NonManifoldVertices && numCells > 2 )
        {
        numNonManifoldVertices++;
        scalar = 0.33333;
        }

      else if ( this->BoundaryVertices && numCells == 1 )
        {
        numBVertices++;
        scalar = 0.0;
        }

      else if ( this->FeatureVertices && numCells == 2 )
        {
        if ( i == 0 && npts > 1 )
          {
          inPts->GetPoint(pts[i],x);
          inPts->GetPoint(pts[i+1],xNext);
          }
        else if ( i > 0 && i < (npts-1) )
          {
          for (j=0; j<3; j++)
            {
            xPrev[j] = x[j];
            x[j] = xNext[j];
            }
          inPts->GetPoint(pts[i+1],xNext);
          for (j=0; j<3; j++)
            {
            vPrev[j] = vNext[j];
            vNext[j] = xNext[j] - x[j];
            }
          if ( math.Normalize(vNext) == 0.0 || 
          math.Dot(vPrev,vNext) <= cosAngle )
            {
            numFvertices++;
            scalar = 0.66667;
            }
          }
        }

      else continue; // don't add point/vertex

      // Add vertex to output
      x1 = inPts->GetPoint(pts[i]);

      vertId[0] = newPts->InsertNextPoint(x1);

      newVerts->InsertNextCell(1,vertId);

      newScalars->InsertScalar(vertId[0], scalar);
      }
    }

  vtkDebugMacro(<<"Created " << numBVertices << " boundary vertices, " <<
               numNonManifoldVertices << " non-manifold vertices, " <<
               numFvertices << " feature vertices");

//
//  Update ourselves.
//
  this->SetPoints(newPts);
  this->SetVerts(newVerts);
  if ( this->Coloring )
    this->PointData.SetScalars(newScalars);
  else
    delete newScalars;
}

void vtkFeatureVertices::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Boundary Vertices: " << (this->BoundaryVertices ? "On\n" : "Off\n");
  os << indent << "Feature Vertices: " << (this->FeatureVertices ? "On\n" : "Off\n"); 
  os << indent << "Non-Manifold Vertices: " << (this->NonManifoldVertices ? "On\n" : "Off\n");
  os << indent << "Coloring: " << (this->Coloring ? "On\n" : "Off\n");
}

