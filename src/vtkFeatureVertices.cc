/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFeatureVertices.cc
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
#include "vtkFeatureVertices.hh"
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
  float scalar = 0;
  float *x1, x[3], xPrev[3], xNext[3], cosAngle;
  float vPrev[3], vNext[3];
  int vertId[1];
  int npts, *pts;
  vtkCellArray *inLines;
  vtkIdList cells(VTK_CELL_SIZE);
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<<"Executing feature vertices");
//
//  Check input
//
  inPts=input->GetPoints();
  inLines=input->GetPolys();
  if ( (numPts=input->GetNumberOfPoints()) < 1 || inPts == NULL || 
       inLines == NULL)
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
  cosAngle = cos ((double) vtkMath::DegreesToRadians() * this->FeatureAngle);

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
          if ( vtkMath::Normalize(vNext) == 0.0 || 
          vtkMath::Dot(vPrev,vNext) <= cosAngle )
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
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetVerts(newVerts);
  newVerts->Delete();

  if ( this->Coloring ) output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  output->Squeeze();
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

