/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFeatureEdges.cc
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
#include "vtkFeatureEdges.hh"
#include "vtkMath.hh"
#include "vtkPolygon.hh"
#include "vtkFloatNormals.hh"

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
  float scalar, n[3], *x1, *x2;
  float cosAngle = 0;
  vtkPolygon poly;
  int lineIds[2];
  int npts, *pts;
  vtkCellArray *inPolys;
  vtkFloatNormals *polyNormals = NULL;
  int numPts, nei;
  vtkIdList neighbors(VTK_CELL_SIZE);
  int p1, p2;
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<<"Executing feature edges");
//
//  Check input
//
  inPts=input->GetPoints();
  inPolys=input->GetPolys();
  if ( (numPts=input->GetNumberOfPoints()) < 1 || inPts == NULL ||
       inPolys == NULL )
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

    cosAngle = cos ((double) vtkMath::DegreesToRadians() * this->FeatureAngle);
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
        if ( vtkMath::Dot(polyNormals->GetNormal(nei),polyNormals->GetNormal(cellId)) <= cosAngle ) 
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
  if ( this->FeatureEdges ) polyNormals->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  if ( this->Coloring ) output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
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

