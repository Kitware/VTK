/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTerrainContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTerrainContourLineInterpolator.h"

#include "vtkObjectFactory.h"
#include "vtkContourRepresentation.h"
#include "vtkImageData.h"
#include "vtkProjectedTerrainPath.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkTerrainContourLineInterpolator);

//----------------------------------------------------------------------
vtkTerrainContourLineInterpolator::vtkTerrainContourLineInterpolator()
{
  this->ImageData       = NULL;
  this->Projector       = vtkProjectedTerrainPath::New();
  this->Projector->SetHeightOffset(0.0);
  this->Projector->SetHeightTolerance(5);
  this->Projector->SetProjectionModeToHug();
}

//----------------------------------------------------------------------
vtkTerrainContourLineInterpolator::~vtkTerrainContourLineInterpolator()
{
  this->SetImageData(NULL);
  this->Projector->Delete();
}

//----------------------------------------------------------------------
void vtkTerrainContourLineInterpolator::SetImageData(vtkImageData *image)
{
  if (this->ImageData != image)
  {
    vtkImageData *temp = this->ImageData;
    this->ImageData = image;
    if (this->ImageData != NULL)
    {
      this->ImageData->Register(this);
      this->Projector->SetSourceData(this->ImageData);
    }
    if (temp != NULL)
    {
      temp->UnRegister(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
int vtkTerrainContourLineInterpolator::InterpolateLine( vtkRenderer *,
                                           vtkContourRepresentation *rep,
                                                     int idx1, int idx2 )
{
  if (!this->ImageData)
  {
    return 0; // No interpolation done if height-field data isn't specified.
  }

  double p1[3], p2[3];
  rep->GetNthNodeWorldPosition( idx1, p1 );
  rep->GetNthNodeWorldPosition( idx2, p2 );

  vtkPoints *pts = vtkPoints::New();
  pts->InsertNextPoint(p1);
  pts->InsertNextPoint(p2);
  vtkCellArray *lines = vtkCellArray::New();
  lines-> InsertNextCell (2);
  lines-> InsertCellPoint(0);
  lines-> InsertCellPoint(1);

  vtkPolyData *terrainPath = vtkPolyData::New();
  terrainPath->SetPoints(pts);
  terrainPath->SetLines(lines);
  lines->Delete();
  pts->Delete();

  this->Projector->SetInputData(terrainPath);
  this->Projector->Update();
  terrainPath->Delete();

  vtkPolyData *interpolatedPd     = this->Projector->GetOutput();
  vtkPoints *interpolatedPts      = interpolatedPd->GetPoints();
  vtkCellArray *interpolatedCells = interpolatedPd->GetLines();

  vtkIdType *ptIdx, npts = 0;

  // Add an ordered set of lines to the representation...
  // The Projected path is a recursive filter and will not generate an ordered
  // set of points. It generates a vtkPolyData with several lines. Each line
  // contains 2 points. We will, from this polydata figure out the ordered set
  // of points that form the projected path..

  const double tolerance = 1.0;
  bool traversalDone = false;
  while (!traversalDone)
  {
    for (interpolatedCells->InitTraversal();
         interpolatedCells->GetNextCell(npts, ptIdx); )
    {

      double p[3];
      interpolatedPts->GetPoint(ptIdx[0], p);

      if ((p[0]-p1[0])*(p[0]-p1[0]) + (p[1]-p1[1])*(p[1]-p1[1]) < tolerance)
      {
        interpolatedPts->GetPoint(ptIdx[npts-1], p1);
        if ((p2[0]-p1[0])*(p2[0]-p1[0])
            + (p2[1]-p1[1])*(p2[1]-p1[1]) < tolerance)
        {
          --npts;
          traversalDone = true;
        }

        for (int i = 1; i < npts; i++)
        {
          rep->AddIntermediatePointWorldPosition(
              idx1, interpolatedPts->GetPoint(ptIdx[i]) );
        }
        continue;
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------
int vtkTerrainContourLineInterpolator::UpdateNode( vtkRenderer *,
                                      vtkContourRepresentation *,
                 double * vtkNotUsed(node), int vtkNotUsed(idx) )
{
  return 0;
}

//----------------------------------------------------------------------
void vtkTerrainContourLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ImageData: " << this->ImageData << endl;
  if (this->ImageData)
  {
    this->ImageData->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Projector: " << this->Projector << endl;
  if (this->Projector)
  {
    this->Projector->PrintSelf(os, indent.GetNextIndent());
  }
}
