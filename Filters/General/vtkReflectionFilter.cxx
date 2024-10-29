// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkReflectionFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkHigherOrderHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkReflectionUtilities.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkReflectionFilter);

//------------------------------------------------------------------------------
vtkReflectionFilter::vtkReflectionFilter()
{
  this->Plane = USE_X_MIN;
  this->Center = 0.0;
  this->CopyInput = 1;
  this->FlipAllInputArrays = false;
}

//------------------------------------------------------------------------------
vtkReflectionFilter::~vtkReflectionFilter() = default;

//------------------------------------------------------------------------------
int vtkReflectionFilter::ComputeBounds(vtkDataObject* input, double bounds[6])
{
  // get the input and output
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);
  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(input);

  if (inputDS)
  {
    inputDS->GetBounds(bounds);
    return 1;
  }

  if (inputCD)
  {
    vtkBoundingBox bbox;

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(inputCD->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!ds)
      {
        vtkErrorMacro("Input composite dataset must be comprised for vtkDataSet "
                      "subclasses alone.");
        return 0;
      }
      bbox.AddBounds(ds->GetBounds());
    }
    if (bbox.IsValid())
    {
      bbox.GetBounds(bounds);
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkReflectionFilter::ReflectNon3DCell(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numInputPoints)
{
  return vtkReflectionUtilities::ReflectNon3DCellInternal(
    input, output, cellId, numInputPoints, this->CopyInput);
}

//------------------------------------------------------------------------------
int vtkReflectionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkDataSet* inputDS = vtkDataSet::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* outputUG = vtkUnstructuredGrid::GetData(outputVector, 0);

  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkCompositeDataSet* outputCD = vtkCompositeDataSet::GetData(outputVector, 0);

  if (inputDS && outputUG)
  {
    double bounds[6];
    this->ComputeBounds(inputDS, bounds);
    return this->RequestDataInternal(inputDS, outputUG, bounds);
  }

  if (inputCD && outputCD)
  {
    outputCD->CopyStructure(inputCD);
    double bounds[6];
    if (this->ComputeBounds(inputCD, bounds))
    {
      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(inputCD->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        if (this->CheckAbort())
        {
          break;
        }
        vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        if (!this->RequestDataInternal(ds, ug, bounds))
        {
          return 0;
        }

        outputCD->SetDataSet(iter, ug);
      }
    }
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkReflectionFilter::RequestDataInternal(
  vtkDataSet* input, vtkUnstructuredGrid* output, double bounds[6])
{
  double constant[3] = { 0.0, 0.0, 0.0 };
  int mirrorDir[3] = { 1, 1, 1 };
  int mirrorSymmetricTensorDir[6] = { 1, 1, 1, 1, 1, 1 };
  int mirrorTensorDir[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };

  // Compture transformation
  switch (this->Plane)
  {
    case USE_X_MIN:
      constant[0] = 2 * bounds[0];
      break;
    case USE_X_MAX:
      constant[0] = 2 * bounds[1];
      break;
    case USE_X:
      constant[0] = 2 * this->Center;
      break;
    case USE_Y_MIN:
      constant[1] = 2 * bounds[2];
      break;
    case USE_Y_MAX:
      constant[1] = 2 * bounds[3];
      break;
    case USE_Y:
      constant[1] = 2 * this->Center;
      break;
    case USE_Z_MIN:
      constant[2] = 2 * bounds[4];
      break;
    case USE_Z_MAX:
      constant[2] = 2 * bounds[5];
      break;
    case USE_Z:
      constant[2] = 2 * this->Center;
      break;
  }

  // Compute the element-wise multiplication needed for
  // vectors/sym tensors/tensors depending on the flipping axis
  //
  // For vectors it is as following
  // X axis
  // -1  1  1
  // Y axis
  //  1 -1  1
  // Z axis
  //  1  1 -1
  //
  // For symmetric tensor it is as following
  // X axis
  //  1 -1 -1
  //     1  1
  //        1
  // Y axis
  //  1 -1  1
  //     1 -1
  //        1
  // Z axis
  //  1  1 -1
  //     1 -1
  //        1
  //
  // For tensors it is as following :
  // X axis
  //  1 -1 -1
  // -1  1  1
  // -1  1  1
  // Y axis
  //  1 -1  1
  // -1  1 -1
  //  1 -1  1
  // Z axis
  //  1  1 -1
  //  1  1 -1
  // -1 -1  1
  //
  switch (this->Plane)
  {
    case USE_X_MIN:
    case USE_X_MAX:
    case USE_X:
      mirrorDir[0] = -1;
      mirrorSymmetricTensorDir[3] = -1;
      mirrorSymmetricTensorDir[5] = -1;
      break;
    case USE_Y_MIN:
    case USE_Y_MAX:
    case USE_Y:
      mirrorDir[1] = -1;
      mirrorSymmetricTensorDir[3] = -1;
      mirrorSymmetricTensorDir[4] = -1;
      break;
    case USE_Z_MIN:
    case USE_Z_MAX:
    case USE_Z:
      mirrorDir[2] = -1;
      mirrorSymmetricTensorDir[4] = -1;
      mirrorSymmetricTensorDir[5] = -1;
      break;
  }
  vtkMath::TensorFromSymmetricTensor(mirrorSymmetricTensorDir, mirrorTensorDir);

  vtkReflectionUtilities::ProcessUnstructuredGrid(input, output, constant, mirrorDir,
    mirrorSymmetricTensorDir, mirrorTensorDir, this->CopyInput, this->FlipAllInputArrays, this);

  return true;
}

//------------------------------------------------------------------------------
int vtkReflectionFilter::FillInputPortInformation(int, vtkInformation* info)
{
  // Input can be a dataset or a composite of datasets.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkReflectionFilter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (input)
  {
    vtkDataObject* output = vtkDataObject::GetData(outInfo);
    // If input is composite dataset, output is a vtkMultiBlockDataSet of
    // unstructrued grids.
    // If input is a dataset, output is an unstructured grid.
    if (!output || (input->IsA("vtkCompositeDataSet") && !output->IsA("vtkMultiBlockDataSet")) ||
      (input->IsA("vtkDataSet") && !output->IsA("vtkUnstructuredGrid")))
    {
      vtkDataObject* newOutput = nullptr;
      if (input->IsA("vtkCompositeDataSet"))
      {
        newOutput = vtkMultiBlockDataSet::New();
      }
      else // if (input->IsA("vtkDataSet"))
      {
        newOutput = vtkUnstructuredGrid::New();
      }
      outInfo->Set(vtkDataSet::DATA_OBJECT(), newOutput);
      newOutput->FastDelete();
    }
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkReflectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
  os << indent << "Center: " << this->Center << endl;
  os << indent << "CopyInput: " << this->CopyInput << endl;
}
VTK_ABI_NAMESPACE_END
