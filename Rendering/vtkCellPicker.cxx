/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellPicker.h"

#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkImageActor.h"

vtkCxxRevisionMacro(vtkCellPicker, "1.39");
vtkStandardNewMacro(vtkCellPicker);

vtkCellPicker::vtkCellPicker()
{
  this->CellId = -1;
  this->SubId = -1;
  for (int i=0; i<3; i++)
    {
    this->PCoords[i] = 0.0;
    }
  this->Cell = vtkGenericCell::New();
}

vtkCellPicker::~vtkCellPicker()
{
  this->Cell->Delete();
}

double vtkCellPicker::IntersectWithLine(double p1[3], double p2[3], double tol, 
                                       vtkAssemblyPath *path, 
                                       vtkProp3D *prop3D, 
                                       vtkAbstractMapper3D *m)
{
  vtkIdType cellId, numCells, minCellId;
  int i, minSubId, subId;
  double x[3], tMin, t, pcoords[3], minXYZ[3], minPcoords[3];
  vtkDataSet *input;
  vtkMapper *mapper;
  vtkAbstractVolumeMapper *volumeMapper;
  vtkImageActor *imageActor = NULL;

  // Get the underlying dataset
  if ( (mapper=vtkMapper::SafeDownCast(m)) != NULL )
    {
    input = mapper->GetInput();
    }
  else if ( (volumeMapper=vtkAbstractVolumeMapper::SafeDownCast(m)) != NULL )
    {
    input = volumeMapper->GetDataSetInput();
    }
  else if ( (imageActor=vtkImageActor::SafeDownCast(prop3D)) != NULL )
    {
    input = imageActor->GetInput();
    }
  else
    {
    return VTK_DOUBLE_MAX;
    }

  cellId = 0;
  numCells = input->GetNumberOfCells();

  if ( imageActor != NULL )
    {
    // Restrict the search to the displayed slice
    int extent[6], displayExtent[6], kMin, kMax;
    vtkImageData *imageData = static_cast<vtkImageData *>(input);
    imageData->GetExtent(extent);
    imageActor->GetDisplayExtent(displayExtent);
    kMin = ((displayExtent[4] > extent[4]) ? displayExtent[4] : extent[4]); 
    kMax = ((displayExtent[5] < extent[5]) ? displayExtent[5] : extent[5]); 
    cellId = kMin - extent[4];
    cellId *= extent[3] - extent[2];
    cellId *= extent[1] - extent[0];
    numCells = kMax - extent[4] + 1;
    numCells *= extent[3] - extent[2];
    numCells *= extent[1] - extent[0];
    }

  if ( numCells <= cellId )
    {
    return 2.0;
    }

  // Intersect each cell with ray.  Keep track of one closest to
  // the eye (within the tolerance tol) and within the clipping range). 
  // Note that we fudge the "closest to" (tMin+this->Tolerance) a little and
  // keep track of the cell with the best pick based on parametric
  // coordinate (pick the minimum, maximum parametric distance). This 
  // breaks ties in a reasonable way when cells are the same distance 
  // from the eye (like cells lying on a 2D plane).
  //
  minCellId = -1;
  minSubId = -1;
  pcoords[0] = pcoords[1] = pcoords[2] = 0;
  double pDistMin=VTK_DOUBLE_MAX, pDist;
  for (tMin=VTK_DOUBLE_MAX; cellId<numCells; cellId++) 
    {
    input->GetCell(cellId, this->Cell);

    if ( this->Cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId) 
    && t <= (tMin+this->Tolerance) )
      {
      pDist = this->Cell->GetParametricDistance(pcoords);
      if ( pDist < pDistMin || (pDist == pDistMin && t < tMin) )
        {
        minCellId = cellId;
        minSubId = subId;
        for (i=0; i<3; i++)
          {
          minXYZ[i] = x[i];
          minPcoords[i] = pcoords[i];
          }
        tMin = t;
        pDistMin = pDist;
//        cout << "cell id: " << minCellId << "\n";
        }//if minimum, maximum
      }//if a close cell
    }//for all cells
  
  //  Now compare this against other actors.
  //
  if ( minCellId>(-1) && tMin < this->GlobalTMin ) 
    {
    this->MarkPicked(path, prop3D, m, tMin, minXYZ);
    this->CellId = minCellId;
    this->SubId = minSubId;
    for (i=0; i<3; i++)
      {
      this->PCoords[i] = minPcoords[i];
      }
    vtkDebugMacro("Picked cell id= " << minCellId);
    }
  return tMin;
}

void vtkCellPicker::Initialize()
{
  this->CellId = (-1);
  this->SubId = (-1);
  for (int i=0; i<3; i++)
    {
    this->PCoords[i] = 0.0;
    }
  this->vtkPicker::Initialize();
}

void vtkCellPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Cell Id: " << this->CellId << "\n";
  os << indent << "SubId: " << this->SubId << "\n";
  os << indent << "PCoords: (" << this->PCoords[0] << ", " 
     << this->PCoords[1] << ", " << this->PCoords[2] << ")\n";
}
