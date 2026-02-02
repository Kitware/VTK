// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageMultiVariateToRGBA.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

#include <cmath>
#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageMultiVariateToRGBA);

namespace
{
/**
 * Normalize each modality group together.
 * For example, if Indices `[0, 1, 2]` are the same modality, we will
 * normalize them all together.
 * In this example, all channels are in the same modality, so they all
 * get normalized together.
 */
struct NormalizeDataWorker
{
  NormalizeDataWorker(const std::vector<int>& indices, double newMin = 0, double newMax = 1)
    : Indices(indices)
    , NewMin(newMin)
    , NewMax(newMax)
  {
  }

  template <typename Input, typename Output>
  void operator()(Input* inputArray, Output* outputArray)
  {
    double minVal = inputArray->GetDataTypeMin();
    double maxVal = inputArray->GetDataTypeMax();
    vtkDataArrayAccessor<Input> input(inputArray);
    vtkDataArrayAccessor<Output> output(outputArray);

    vtkIdType n = inputArray->GetNumberOfTuples();
    for (vtkIdType tupleIdx = 0; tupleIdx < n; ++tupleIdx)
    {
      for (int i = 0; i < this->Indices.size(); ++i)
      {
        output.Set(tupleIdx, this->Indices[i],
          (this->NewMax - this->NewMin) * (input.Get(tupleIdx, this->Indices[i]) - minVal) /
              (maxVal - minVal) +
            this->NewMin);
      }
    }
  }
  std::vector<int> Indices;
  double NewMin = 0;
  double NewMax = 1;
};

}

//==============================================================================
vtkImageMultiVariateToRGBA::vtkImageMultiVariateToRGBA()
{
  this->ColorWheelRotation = 0;
}

//==============================================================================
vtkImageMultiVariateToRGBA::~vtkImageMultiVariateToRGBA() {}

//==============================================================================
void vtkImageMultiVariateToRGBA::SetModalityGroups(std::vector<std::vector<int>>& modalityGroups)
{
  vtkDebugMacro(<< " setting ModalityGroups to " << fmt::format("{}", this->ModalityGroups));
  if (this->ModalityGroups != modalityGroups)
  {
    this->ModalityGroups = modalityGroups;
    this->Modified();
  }
}

//==============================================================================
std::vector<std::vector<int>> vtkImageMultiVariateToRGBA::GetModalityGroups() VTK_FUTURE_CONST
{
  vtkDebugMacro(<< " returning ModalityGroups of " << fmt::format("{}", this->ModalityGroups));
  return this->ModalityGroups;
}

//==============================================================================
int vtkImageMultiVariateToRGBA::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData* inData = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* outData = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  outData->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  outData->AllocateScalars(outInfo);

  int inExt[6];
  inData->GetExtent(inExt);
  if (inExt[1] < inExt[0] || inExt[3] < inExt[2] || inExt[5] < inExt[4])
  {
    vtkErrorMacro("A scalar input with variables as a components "
                  "is needed for this filter");
    return 0;
  }

  auto* pointData = inData->GetPointData();
  vtkDataArray* scalars = pointData->GetScalars();
  if (!pointData || !scalars)
  {
    vtkErrorMacro("A scalar input with variables as a components is needed for this filter");
    return 0;
  }
  int numComponents = scalars->GetNumberOfComponents();
  if (this->ModalityGroups.empty())
  {
    // add one group with all variables
    this->ModalityGroups.resize(1);
    this->ModalityGroups[0].resize(numComponents);
    std::iota(this->ModalityGroups[0].begin(), this->ModalityGroups[0].end(), 0);
  }
  vtkNew<vtkDoubleArray> normalizedScalars;
  normalizedScalars->SetNumberOfComponents(scalars->GetNumberOfComponents());
  normalizedScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  typedef vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>
    Dispatcher;
  for (int i = 0; i < this->ModalityGroups.size(); ++i)
  {
    NormalizeDataWorker worker(this->ModalityGroups[i]);
    if (!Dispatcher::Execute(scalars, normalizedScalars, worker))
    {
      worker(scalars, normalizedScalars.GetPointer());
    }
  }

  return 1;
}

//==============================================================================
void vtkImageMultiVariateToRGBA::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
