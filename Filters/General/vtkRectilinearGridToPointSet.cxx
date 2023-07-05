// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRectilinearGridToPointSet.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"

#include "vtkNew.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRectilinearGridToPointSet);

//------------------------------------------------------------------------------
vtkRectilinearGridToPointSet::vtkRectilinearGridToPointSet() = default;

vtkRectilinearGridToPointSet::~vtkRectilinearGridToPointSet() = default;

void vtkRectilinearGridToPointSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkRectilinearGridToPointSet::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkRectilinearGridToPointSet::CopyStructure(
  vtkStructuredGrid* outData, vtkRectilinearGrid* inData)
{
  vtkDataArray* xcoord = inData->GetXCoordinates();
  vtkDataArray* ycoord = inData->GetYCoordinates();
  vtkDataArray* zcoord = inData->GetZCoordinates();

  int extent[6];
  inData->GetExtent(extent);

  outData->SetExtent(extent);

  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(inData->GetNumberOfPoints());

  vtkIdType pointId = 0;
  int ijk[3];
  for (ijk[2] = extent[4]; ijk[2] <= extent[5]; ijk[2]++)
  {
    for (ijk[1] = extent[2]; ijk[1] <= extent[3]; ijk[1]++)
    {
      for (ijk[0] = extent[0]; ijk[0] <= extent[1]; ijk[0]++)
      {
        double coord[3];
        coord[0] = xcoord->GetComponent(ijk[0] - extent[0], 0);
        coord[1] = ycoord->GetComponent(ijk[1] - extent[2], 0);
        coord[2] = zcoord->GetComponent(ijk[2] - extent[4], 0);

        points->SetPoint(pointId, coord);
        pointId++;
      }
    }
  }

  if (pointId != points->GetNumberOfPoints())
  {
    vtkErrorMacro(<< "Somehow miscounted points");
    return 0;
  }

  outData->SetPoints(points);

  return 1;
}

//------------------------------------------------------------------------------
int vtkRectilinearGridToPointSet::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkRectilinearGrid* inData = vtkRectilinearGrid::GetData(inputVector[0]);
  vtkStructuredGrid* outData = vtkStructuredGrid::GetData(outputVector);

  if (inData == nullptr)
  {
    vtkErrorMacro(<< "Input data is nullptr.");
    return 0;
  }
  if (outData == nullptr)
  {
    vtkErrorMacro(<< "Output data is nullptr.");
    return 0;
  }

  int result = vtkRectilinearGridToPointSet::CopyStructure(outData, inData);
  if (!result)
  {
    return 0;
  }

  outData->GetPointData()->PassData(inData->GetPointData());
  outData->GetCellData()->PassData(inData->GetCellData());

  this->CheckAbort();

  return 1;
}
VTK_ABI_NAMESPACE_END
