// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware SAS
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkForceStaticMesh.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkForceStaticMesh);

//------------------------------------------------------------------------------
void vtkForceStaticMesh::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ForceCacheComputation: " << (this->ForceCacheComputation ? "on" : "off") << endl;
}

//------------------------------------------------------------------------------
int vtkForceStaticMesh::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkForceStaticMesh::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = nullptr;

  // Retrieve the input data object to process
  vtkDataObject* inputObj = vtkDataObject::GetData(inputVector[0]);
  vtkMultiBlockDataSet* inputMB = vtkMultiBlockDataSet::GetData(inputVector[0]);
  if (inputMB && inputMB->GetNumberOfBlocks() >= 1)
  {
    // Recover the first block
    if (inputMB->GetNumberOfBlocks() > 1)
    {
      vtkWarningMacro("Only the first block will be passed");
    }

    inputObj = inputMB->GetBlock(0);
  }

  // cast into supporte type
  if (auto inputUG = vtkUnstructuredGrid::SafeDownCast(inputObj))
  {
    input = inputUG;
  }
  else if (auto inputPD = vtkPolyData::SafeDownCast(inputObj))
  {
    input = inputPD;
  }

  if (!input)
  {
    vtkErrorMacro("Input is invalid, it should be either a polydata, an unstructured grid"
                  "or a multiblock with an unstructured grid in the first block");
    return 0;
  }

  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  bool validCache = this->CacheInitialized;
  if (validCache)
  {
    if (!this->Cache)
    {
      // Not initialized
      validCache = false;
    }
    if (input->GetNumberOfPoints() != this->Cache->GetNumberOfPoints())
    {
      vtkWarningMacro("Cache has been invalidated, the number of points in input changed, from "
        << this->Cache->GetNumberOfPoints() << " to " << input->GetNumberOfPoints());
      validCache = false;
    }
    if (input->GetNumberOfCells() != this->Cache->GetNumberOfCells())
    {
      vtkWarningMacro("Cache has been invalidated, the number of cells in input changed, from "
        << this->Cache->GetNumberOfCells() << " to " << input->GetNumberOfCells());
      validCache = false;
    }
  }

  if (this->ForceCacheComputation || !validCache)
  {
    // Cache is invalid
    vtkDebugMacro("Building static mesh cache");

    this->Cache.TakeReference(input->NewInstance());
    this->Cache->DeepCopy(input);
    this->CacheInitialized = true;
  }
  else
  {
    // Cache mesh is up to date, use it to generate data
    vtkDebugMacro("Using static mesh cache");

    this->Cache->GetPointData()->ShallowCopy(input->GetPointData());
    this->Cache->GetCellData()->ShallowCopy(input->GetCellData());
    this->Cache->GetFieldData()->ShallowCopy(input->GetFieldData());
  }

  output->ShallowCopy(this->Cache);
  return 1;
}
VTK_ABI_NAMESPACE_END
