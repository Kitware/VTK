// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware SAS
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkForceStaticMesh.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
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
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkForceStaticMesh::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve the input data object to process
  vtkDataObject* inputObj = vtkDataObject::GetData(inputVector[0]);
  vtkCompositeDataSet* inputComposite = vtkCompositeDataSet::SafeDownCast(inputObj);
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(inputObj);
  vtkDataObject* output = vtkDataObject::GetData(outputVector);

  if (!inputComposite && !inputDS)
  {
    vtkErrorMacro("Unsupported input data type.");
    return 0;
  }

  bool validCache =
    (inputComposite) ? this->IsValidCache(inputComposite) : this->IsValidCache(inputDS);

  if (this->ForceCacheComputation || !validCache)
  {
    // Cache is invalid
    vtkDebugMacro("Building static mesh cache");

    this->Cache.TakeReference(inputObj->NewInstance());
    this->Cache->DeepCopy(inputObj);
    this->CacheInitialized = true;
  }
  else
  {
    // Cache mesh is up to date, use it to generate data
    vtkDebugMacro("Using static mesh cache");

    if (inputComposite)
    {
      this->InputToCache(inputComposite);
    }
    else
    {
      this->InputToCache(inputDS);
    }
  }

  output->ShallowCopy(this->Cache);

  return 1;
}

//------------------------------------------------------------------------------
bool vtkForceStaticMesh::IsValidCache(vtkDataSet* input)
{
  bool validCache = this->CacheInitialized;
  if (validCache)
  {
    if (!this->Cache)
    {
      // Not initialized
      validCache = false;
    }
    vtkDataSet* internalCache = vtkDataSet::SafeDownCast(this->Cache);
    if (input->GetNumberOfPoints() != internalCache->GetNumberOfPoints())
    {
      vtkWarningMacro("Cache has been invalidated, the number of points in input changed, from "
        << internalCache->GetNumberOfPoints() << " to " << input->GetNumberOfPoints());
      validCache = false;
    }
    if (input->GetNumberOfCells() != internalCache->GetNumberOfCells())
    {
      vtkWarningMacro("Cache has been invalidated, the number of cells in input changed, from "
        << internalCache->GetNumberOfCells() << " to " << input->GetNumberOfCells());
      validCache = false;
    }
  }
  return validCache;
}

//------------------------------------------------------------------------------
bool vtkForceStaticMesh::IsValidCache(vtkCompositeDataSet* input)
{
  bool validCache = this->CacheInitialized;
  if (validCache)
  {
    if (!this->Cache)
    {
      // Not initialized
      validCache = false;
    }
    vtkCompositeDataSet* internalCache = vtkCompositeDataSet::SafeDownCast(this->Cache);
    assert(internalCache);

    // Global parameters
    if (input->GetNumberOfPoints() != internalCache->GetNumberOfPoints())
    {
      vtkWarningMacro("Cache has been invalidated, the number of points in input changed, from "
        << internalCache->GetNumberOfPoints() << " to " << input->GetNumberOfPoints());
      validCache = false;
    }
    if (input->GetNumberOfCells() != internalCache->GetNumberOfCells())
    {
      vtkWarningMacro("Cache has been invalidated, the number of cells in input changed, from "
        << internalCache->GetNumberOfCells() << " to " << input->GetNumberOfCells());
      validCache = false;
    }

    // Per block parameters

    auto compIterator =
      vtkSmartPointer<vtkCompositeDataIterator>::Take(internalCache->NewIterator());
    for (compIterator->InitTraversal(); !compIterator->IsDoneWithTraversal();
         compIterator->GoToNextItem())
    {
      // Both composite must have the same structure by construction,
      // we can use the GetDataSet with iterator from other composite
      vtkDataSet* cacheBlock = vtkDataSet::SafeDownCast(internalCache->GetDataSet(compIterator));
      vtkDataSet* inputBlock = vtkDataSet::SafeDownCast(input->GetDataSet(compIterator));

      if ((cacheBlock == nullptr) != (inputBlock == nullptr))
      {
        // if one of them is dataset and not the other:
        // not the same internal structure, invalid cache
        validCache = false;
        break;
      }

      if (cacheBlock /*&& inputBlock */)
      {
        if (inputBlock->GetNumberOfPoints() != cacheBlock->GetNumberOfPoints())
        {
          vtkWarningMacro(
            "Cache has been invalidated, the number of points in a block changed, from "
            << cacheBlock->GetNumberOfPoints() << " to " << inputBlock->GetNumberOfPoints());
          validCache = false;
          break;
        }
        if (inputBlock->GetNumberOfCells() != cacheBlock->GetNumberOfCells())
        {
          vtkWarningMacro(
            "Cache has been invalidated, the number of cells in a block changed, from "
            << cacheBlock->GetNumberOfCells() << " to " << inputBlock->GetNumberOfCells());
          validCache = false;
          break;
        }
      }
    }
  }
  return validCache;
}

//------------------------------------------------------------------------------
void vtkForceStaticMesh::InputToCache(vtkDataSet* input)
{
  vtkDataSet* internalCache = vtkDataSet::SafeDownCast(this->Cache);
  assert(internalCache);
  internalCache->GetPointData()->ShallowCopy(input->GetPointData());
  internalCache->GetCellData()->ShallowCopy(input->GetCellData());
  internalCache->GetFieldData()->ShallowCopy(input->GetFieldData());
}

//------------------------------------------------------------------------------
void vtkForceStaticMesh::InputToCache(vtkCompositeDataSet* input)
{
  vtkCompositeDataSet* internalCache = vtkCompositeDataSet::SafeDownCast(this->Cache);
  assert(internalCache);
  auto compIterator = vtkSmartPointer<vtkCompositeDataIterator>::Take(internalCache->NewIterator());
  for (compIterator->InitTraversal(); !compIterator->IsDoneWithTraversal();
       compIterator->GoToNextItem())
  {
    // Both composite must have the same structure by construction,
    // we can use the GetDataSet with iterator from other composite
    vtkDataSet* cacheBlock = vtkDataSet::SafeDownCast(internalCache->GetDataSet(compIterator));
    vtkDataSet* inputBlock = vtkDataSet::SafeDownCast(input->GetDataSet(compIterator));

    if (!cacheBlock || !inputBlock)
    { // intermediate non dataset block are ignored
      continue;
    }
    cacheBlock->GetPointData()->ShallowCopy(inputBlock->GetPointData());
    cacheBlock->GetCellData()->ShallowCopy(inputBlock->GetCellData());
    cacheBlock->GetFieldData()->ShallowCopy(inputBlock->GetFieldData());
  }
}

VTK_ABI_NAMESPACE_END
