/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFeatureEdges.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkFeatureEdges.h"
#include "vtkMath.h"
#include "vtkPolygon.h"
#include "vtkNormals.h"
#include "vtkMergePoints.h"

// Construct object with feature angle = 30; all types of edges extracted
// and colored.
vtkFeatureEdges::vtkFeatureEdges()
{
  this->FeatureAngle = 30.0;
  this->BoundaryEdges = 1;
  this->FeatureEdges = 1;
  this->NonManifoldEdges = 1;
  this->ManifoldEdges = 0;
  this->Coloring = 1;
  this->Locator = NULL;
}

vtkFeatureEdges::~vtkFeatureEdges()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

// Generate feature edges for mesh
void vtkFeatureEdges::Execute()
{
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkScalars *newScalars;
  vtkCellArray *newLines;
  vtkPolyData *Mesh;
  int i, j, numNei, cellId;
  int numBEdges, numNonManifoldEdges, numFedges, numManifoldEdges;
  float scalar, n[3], x1[3], x2[3];
  float cosAngle = 0;
  int lineIds[2];
  int npts, *pts;
  vtkCellArray *inPolys;
  vtkNormals *polyNormals = NULL;
  int numPts, numCells, nei;
  vtkIdList *neighbors;
  int p1, p2, newId;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  
  vtkDebugMacro(<<"Executing feature edges");

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
  numCells = input->GetNumberOfCells();

  if ( !this->BoundaryEdges && !this->NonManifoldEdges && 
  !this->FeatureEdges && !this->ManifoldEdges )
    {
    vtkWarningMacro(<<"All edge types turned off!");
    return;
    }

  // build cell structure.  Only operate with polygons.
  Mesh = vtkPolyData::New();
  Mesh->SetPoints(inPts);
  Mesh->SetPolys(inPolys);
  Mesh->BuildLinks();

  // Allocate storage for lines/points (arbitrary allocations size)
  //
  newPts = vtkPoints::New();
  newPts->Allocate(numPts/10,numPts); 
  newLines = vtkCellArray::New();
  newLines->Allocate(numPts/10);
  if ( this->Coloring )
    {
    newScalars = vtkScalars::New();
    newScalars->Allocate(numCells/10,numCells);
    outCD->CopyScalarsOff();
    }

  outPD->CopyAllocate(pd, numPts);
  outCD->CopyAllocate(cd, numCells);

  // Get our locator for merging points
  //
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, input->GetBounds());

  // Loop over all polygons generating boundary, non-manifold, 
  // and feature edges
  //
  if ( this->FeatureEdges ) 
    {    
    polyNormals = vtkNormals::New();
    polyNormals->Allocate(inPolys->GetNumberOfCells());

    for (cellId=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); 
    cellId++)
      {
      vtkPolygon::ComputeNormal(inPts,npts,pts,n);
      polyNormals->InsertNormal(cellId,n);
      }

    cosAngle = cos ((double) vtkMath::DegreesToRadians() * this->FeatureAngle);
    }

  neighbors = vtkIdList::New();
  neighbors->Allocate(VTK_CELL_SIZE);

  numBEdges = numNonManifoldEdges = numFedges = numManifoldEdges = 0;
  for (cellId=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); 
  cellId++)
    {
    if ( ! (cellId % 10000) ) //manage progress reports / early abort
      {
      this->UpdateProgress ((float)cellId / numCells);
      if ( this->GetAbortExecute() ) 
        {
        break;
        }
      }

    for (i=0; i < npts; i++) 
      {
      p1 = pts[i];
      p2 = pts[(i+1)%npts];

      Mesh->GetCellEdgeNeighbors(cellId,p1,p2, neighbors);
      numNei = neighbors->GetNumberOfIds();

      if ( this->BoundaryEdges && numNei < 1 )
        {
        numBEdges++;
        scalar = 0.0;
        }

      else if ( this->NonManifoldEdges && numNei > 1 )
        {
        // check to make sure that this edge hasn't been created before
        for (j=0; j < numNei; j++)
	  {
          if ( neighbors->GetId(j) < cellId )
	    {
            break;
	    }
	  }
        if ( j >= numNei )
          {
          numNonManifoldEdges++;
          scalar = 0.222222;
          }
        else
	  {
	  continue;
	  }
        }
      else if ( this->FeatureEdges && 
                numNei == 1 && (nei=neighbors->GetId(0)) > cellId ) 
        {
        if ( vtkMath::Dot(polyNormals->GetNormal(nei),
                          polyNormals->GetNormal(cellId)) <= cosAngle ) 
          {
          numFedges++;
          scalar = 0.444444;
          }
        else
	  {
	  continue;
	  }
        }
      else if ( this->ManifoldEdges )
        {
        numManifoldEdges++;
        scalar = 0.666667;
        }
      else
	{
	continue;
	}

      // Add edge to output
      Mesh->GetPoint(p1, x1);
      Mesh->GetPoint(p2, x2);

      if ( (lineIds[0] = this->Locator->IsInsertedPoint(x1)) )
        {
        lineIds[0] = this->Locator->InsertNextPoint(x1);
        outPD->CopyData (pd,p1,lineIds[0]);
        }
      
      if ( (lineIds[1] = this->Locator->IsInsertedPoint(x2)) )
        {
        lineIds[1] = this->Locator->InsertNextPoint(x2);
        outPD->CopyData (pd,p2,lineIds[1]);
        }

      newId = newLines->InsertNextCell(2,lineIds);
      outCD->CopyData (cd,cellId,newId);
      if ( this->Coloring )
	{
	newScalars->InsertScalar(newId, scalar);
	}
      }
    }

  vtkDebugMacro(<<"Created " << numBEdges << " boundary edges, "
                << numNonManifoldEdges << " non-manifold edges, "
                << numFedges << " feature edges, "
                << numManifoldEdges << " manifold edges");

  //  Update ourselves.
  //
  if ( this->FeatureEdges )
    {
    polyNormals->Delete();
    }

  Mesh->Delete();
  
  output->SetPoints(newPts);
  newPts->Delete();
  neighbors->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  if ( this->Coloring )
    {
    outCD->SetScalars(newScalars);
    newScalars->Delete();
    }
}

void vtkFeatureEdges::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkFeatureEdges::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator ) 
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( locator )
    {
    locator->Register(this);
    }
  this->Locator = locator;
  this->Modified();
}

unsigned long int vtkFeatureEdges::GetMTime()
{
  unsigned long mTime=this-> vtkPolyDataToPolyDataFilter::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}

void vtkFeatureEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Boundary Edges: " << (this->BoundaryEdges ? "On\n" : "Off\n");
  os << indent << "Feature Edges: " << (this->FeatureEdges ? "On\n" : "Off\n"); 
  os << indent << "Non-Manifold Edges: " << (this->NonManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Manifold Edges: " << (this->ManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Coloring: " << (this->Coloring ? "On\n" : "Off\n");

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}

