// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStitchImageDataWithGhosts.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStitchImageDataWithGhosts);

//----------------------------------------------------------------------------
vtkStitchImageDataWithGhosts::vtkStitchImageDataWithGhosts()
{
  this->Initialize();
}

//------------------------------------------------------------------------------
void vtkStitchImageDataWithGhosts::Initialize()
{
  this->Superclass::Initialize();
  this->BuildIfRequired = false;
}

//------------------------------------------------------------------------------
int vtkStitchImageDataWithGhosts::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkStitchImageDataWithGhosts::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  auto dualInputDO = vtkSmartPointer<vtkDataObject>::Take(inputDO->NewInstance());
  dualInputDO->ShallowCopy(inputDO);

  auto extractImageData = [](vtkDataObject* dObj) {
    if (auto cds = vtkCompositeDataSet::SafeDownCast(dObj))
    {
      return vtkCompositeDataSet::GetDataSets<vtkImageData>(cds);
    }
    else if (auto im = vtkImageData::SafeDownCast(dObj))
    {
      return std::vector<vtkImageData*>(1, im);
    }
    return std::vector<vtkImageData*>();
  };

  std::vector<vtkImageData*> inputs = extractImageData(dualInputDO);

  if (inputs.empty())
  {
    vtkLog(WARNING, "There are no vtkImageData in the input... Not generating anything.");
    return 1;
  }

  for (vtkImageData* im : inputs)
  {
    // Images cannot have cell data for this filter to work.
    // Indeed, what value are we going to assign to the cells in the gap being filled? We can't.
    if (im->GetCellData()->GetNumberOfArrays())
    {
      vtkLog(ERROR, "Input cannot have any Cell Data... Aborting.");
      return 0;
    }
  }

  // We transform the inputs into their dual meshes, effectively converting point data to cell data.
  // Such image data become connex if there was a one voxel gap between them.
  for (vtkImageData* im : inputs)
  {
    int* e = im->GetExtent();
    for (int dim = 0; dim < 3; ++dim)
    {
      if (e[2 * dim] != e[2 * dim + 1])
      {
        ++e[2 * dim + 1];
      }
    }
    im->SetExtent(e);
    im->GetCellData()->ShallowCopy(im->GetPointData());
    im->GetPointData()->Initialize();
  }

  // Running the filter with the input's dual
  int retVal = this->Execute(dualInputDO, outputVector);

  std::vector<vtkImageData*> outputs = extractImageData(outputDO);

  // We need to convert the output back to the primal mesh.
  for (vtkImageData* im : outputs)
  {
    int* e = im->GetExtent();
    for (int dim = 0; dim < 3; ++dim)
    {
      if (e[2 * dim] != e[2 * dim + 1])
      {
        --e[2 * dim + 1];
      }
    }
    im->SetExtent(e);
    im->GetPointData()->ShallowCopy(im->GetCellData());
    im->GetCellData()->Initialize();
  }

  return retVal;
}

//----------------------------------------------------------------------------
void vtkStitchImageDataWithGhosts::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
