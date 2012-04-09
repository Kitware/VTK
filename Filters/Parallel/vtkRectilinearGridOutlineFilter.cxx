/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearGridOutlineFilter.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkRectilinearGridOutlineFilter);


int vtkRectilinearGridOutlineFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // Although there may be overlap between piece outlines,
  // it is not worth requesting exact extents.
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);
  return 1;
}

int vtkRectilinearGridOutlineFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkRectilinearGrid *input = vtkRectilinearGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  float         bounds[6];
  double         *range;
  float         x[3];
  vtkIdType     pts[2];
  vtkPoints*    newPts;
  vtkCellArray* newLines;

  vtkDataArray* xCoords  = input->GetXCoordinates();
  vtkDataArray* yCoords  = input->GetYCoordinates();
  vtkDataArray* zCoords  = input->GetZCoordinates();
  int*          ext      = input->GetExtent();;
  int*          wholeExt =
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  if (xCoords == NULL || yCoords == NULL || zCoords == NULL ||
      input->GetNumberOfCells() == 0)
    {
    return 1;
    }

  // We could probably use just the input bounds ...
  range = xCoords->GetRange();
  bounds[0] = range[0];
  bounds[1] = range[1];
  range = yCoords->GetRange();
  bounds[2] = range[0];
  bounds[3] = range[1];
  range = zCoords->GetRange();
  bounds[4] = range[0];
  bounds[5] = range[1];

  //
  // Allocate storage and create outline
  //
  newPts = vtkPoints::New();
  newPts->Allocate(24);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(12,2));

  // xMin yMin
  if (ext[0] == wholeExt[0] && ext[2] == wholeExt[2])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMin yMax
  if (ext[0] == wholeExt[0] && ext[3] == wholeExt[3])
    {
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMin zMin
  if (ext[0] == wholeExt[0] && ext[4] == wholeExt[4])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMin zMax
  if (ext[0] == wholeExt[0] && ext[5] == wholeExt[5])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMax yMin
  if (ext[1] == wholeExt[1] && ext[2] == wholeExt[2])
    {
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMax yMax
  if (ext[1] == wholeExt[1] && ext[3] == wholeExt[3])
    {
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMax zMin
  if (ext[1] == wholeExt[1] && ext[4] == wholeExt[4])
    {
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMax zMax
  if (ext[1] == wholeExt[1] && ext[5] == wholeExt[5])
    {
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // yMin zMin
  if (ext[2] == wholeExt[2] && ext[4] == wholeExt[4])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // yMin zMax
  if (ext[2] == wholeExt[2] && ext[5] == wholeExt[5])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // yMax zMin
  if (ext[3] == wholeExt[3] && ext[4] == wholeExt[4])
    {
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // yMax zMax
  if (ext[3] == wholeExt[3] && ext[5] == wholeExt[5])
    {
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();

  return 1;
}

int vtkRectilinearGridOutlineFilter::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

void vtkRectilinearGridOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
