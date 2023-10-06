// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkmThreshold.h"
#include "vtkmConfigFilters.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/entity_extraction/Threshold.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmThreshold);

namespace
{

class MaskBits
{
public:
  // We are not using `= default` here as nvcc does not allow it.
  // NOLINTNEXTLINE(modernize-use-equals-default)
  VTKM_EXEC_CONT MaskBits() {}

  explicit MaskBits(int mask)
    : Mask(mask)
  {
  }

  VTKM_EXEC_CONT int operator()(unsigned char in) const
  {
    return static_cast<int>(in) & this->Mask;
  }

private:
  int Mask = 0;
};

inline bool HasGhostFlagsSet(vtkUnsignedCharArray* ghostArray, int flags)
{
  if (!ghostArray)
  {
    return false;
  }

  auto ah =
    tovtkm::DataArrayToArrayHandle<vtkAOSDataArrayTemplate<unsigned char>, 1>::Wrap(ghostArray);
  int result = vtkm::cont::Algorithm::Reduce(
    vtkm::cont::make_ArrayHandleTransform(ah, MaskBits(flags)), 0, vtkm::LogicalOr());
  return result;
}

} // anonymous namespace

//------------------------------------------------------------------------------
vtkmThreshold::vtkmThreshold() = default;

//------------------------------------------------------------------------------
vtkmThreshold::~vtkmThreshold() = default;

//------------------------------------------------------------------------------
int vtkmThreshold::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (inputArray == nullptr)
  {
    vtkDebugMacro("No scalar data to threshold");
    return 1;
  }

  int association = this->GetInputArrayAssociation(0, inputVector);
  vtkm::cont::Field::Association vtkmAssoc;
  switch (association)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      vtkmAssoc = vtkm::cont::Field::Association::Points;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      vtkmAssoc = vtkm::cont::Field::Association::Cells;
      break;
    default:
      vtkErrorMacro("Only point and cell fields are supported");
      return 1;
  }

  try
  {
    // currently, vtkm::filter::entity_extraction::Threshold always generates single precision
    // points
    // auto pointSet = vtkPointSet::SafeDownCast(input);
    // if ((this->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION) ||
    //   (this->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION && pointSet &&
    //     pointSet->GetPoints()->GetDataType() != VTK_FLOAT))
    // {
    //   throw vtkm::cont::ErrorFilterExecution(
    //     "vtkmThreshold only supports generating single precision output points.");
    // }
    if (this->GetOutputPointsPrecision() != vtkAlgorithm::DEFAULT_PRECISION)
    {
      throw vtkm::cont::ErrorFilterExecution(
        "Only `vtkAlgorithm::DEFAULT_PRECISION` is supported for `OutputPointsPrecision`");
    }

    if (this->GetUseContinuousCellRange())
    {
      throw vtkm::cont::ErrorFilterExecution(
        "vtkmThreshold currently does not support UseContinuousCellRange.");
    }

    if (this->GetComponentMode() == VTK_COMPONENT_MODE_USE_SELECTED &&
      this->GetSelectedComponent() == inputArray->GetNumberOfComponents())
    {
      throw vtkm::cont::ErrorFilterExecution("vtkmThreshold currently does not support Magnitude.");
    }

    if (HasGhostFlagsSet(input->GetCellData()->GetGhostArray(), vtkDataSetAttributes::HIDDENCELL) ||
      HasGhostFlagsSet(input->GetPointData()->GetGhostArray(), vtkDataSetAttributes::HIDDENPOINT))
    {
      throw vtkm::cont::ErrorFilterExecution("hidden points/cells not supported.");
    }

    // convert the input dataset to a vtkm::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);

    const char* activeFieldName = inputArray->GetName();
    if (!activeFieldName || activeFieldName[0] == '\0')
    {
      activeFieldName = tovtkm::NoNameVTKFieldName();
    }

    vtkm::filter::entity_extraction::Threshold filter;
    filter.SetActiveField(activeFieldName, vtkmAssoc);

    switch (this->GetThresholdFunction())
    {
      case vtkThreshold::THRESHOLD_BETWEEN:
        filter.SetThresholdBetween(this->GetLowerThreshold(), this->GetUpperThreshold());
        break;
      case vtkThreshold::THRESHOLD_LOWER:
        filter.SetThresholdBelow(this->GetLowerThreshold());
        break;
      case vtkThreshold::THRESHOLD_UPPER:
        filter.SetThresholdAbove(this->GetUpperThreshold());
        break;
      default:
        assert(false); // unreachable
    }

    switch (this->GetComponentMode())
    {
      case VTK_COMPONENT_MODE_USE_SELECTED:
        filter.SetComponentToTest(this->SelectedComponent);
        break;
      case VTK_COMPONENT_MODE_USE_ALL:
        filter.SetComponentToTestToAll();
        break;
      case VTK_COMPONENT_MODE_USE_ANY:
        filter.SetComponentToTestToAny();
        break;
      default:
        assert(false); // unreachable
    }

    filter.SetAllInRange(this->AllScalars);
    filter.SetInvert(this->Invert);

    auto result = filter.Execute(in);

    // clean the output to remove unused points
    vtkm::filter::clean_grid::CleanGrid clean;
    clean.SetCompactPointFields(true);
    clean.SetMergePoints(false);
    clean.SetRemoveDegenerateCells(false);
    result = clean.Execute(result);

    // now we are done the algorithm and conversion of arrays so
    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      throw vtkm::cont::ErrorFilterExecution("Unable to convert VTKm result dataSet back to VTK.");
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    if (this->ForceVTKm)
    {
      vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
      return 0;
    }
    else
    {
      vtkWarningMacro(<< "VTK-m failed with message: " << e.GetMessage() << "\n"
                      << "Falling back to the default VTK implementation.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkmThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
