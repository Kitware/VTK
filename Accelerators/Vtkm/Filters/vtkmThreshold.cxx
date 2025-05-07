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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/entity_extraction/Threshold.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmThreshold);

namespace
{

class MaskBits
{
public:
  // We are not using `= default` here as nvcc does not allow it.
  // NOLINTNEXTLINE(modernize-use-equals-default)
  VISKORES_EXEC_CONT MaskBits() {}

  explicit MaskBits(int mask)
    : Mask(mask)
  {
  }

  VISKORES_EXEC_CONT int operator()(unsigned char in) const
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

  auto ah = tovtkm::vtkAOSDataArrayToFlatArrayHandle(ghostArray);
  int result = viskores::cont::Algorithm::Reduce(
    viskores::cont::make_ArrayHandleTransform(ah, MaskBits(flags)), 0, viskores::LogicalOr());
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
  viskores::cont::Field::Association vtkmAssoc;
  switch (association)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      vtkmAssoc = viskores::cont::Field::Association::Points;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      vtkmAssoc = viskores::cont::Field::Association::Cells;
      break;
    default:
      vtkErrorMacro("Only point and cell fields are supported");
      return 1;
  }

  try
  {
    // currently, viskores::filter::entity_extraction::Threshold always generates single precision
    // points
    // auto pointSet = vtkPointSet::SafeDownCast(input);
    // if ((this->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION) ||
    //   (this->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION && pointSet &&
    //     pointSet->GetPoints()->GetDataType() != VTK_FLOAT))
    // {
    //   throw viskores::cont::ErrorFilterExecution(
    //     "vtkmThreshold only supports generating single precision output points.");
    // }
    if (this->GetOutputPointsPrecision() != vtkAlgorithm::DEFAULT_PRECISION)
    {
      throw viskores::cont::ErrorFilterExecution(
        "Only `vtkAlgorithm::DEFAULT_PRECISION` is supported for `OutputPointsPrecision`");
    }

    if (this->GetUseContinuousCellRange())
    {
      throw viskores::cont::ErrorFilterExecution(
        "vtkmThreshold currently does not support UseContinuousCellRange.");
    }

    if (this->GetComponentMode() == VTK_COMPONENT_MODE_USE_SELECTED &&
      this->GetSelectedComponent() == inputArray->GetNumberOfComponents())
    {
      throw viskores::cont::ErrorFilterExecution(
        "vtkmThreshold currently does not support Magnitude.");
    }

    if (HasGhostFlagsSet(input->GetCellData()->GetGhostArray(), vtkDataSetAttributes::HIDDENCELL) ||
      HasGhostFlagsSet(input->GetPointData()->GetGhostArray(), vtkDataSetAttributes::HIDDENPOINT))
    {
      throw viskores::cont::ErrorFilterExecution("hidden points/cells not supported.");
    }

    // convert the input dataset to a viskores::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);

    const char* activeFieldName = inputArray->GetName();
    if (!activeFieldName || activeFieldName[0] == '\0')
    {
      activeFieldName = tovtkm::NoNameVTKFieldName();
    }

    viskores::filter::entity_extraction::Threshold filter;
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
    viskores::filter::clean_grid::CleanGrid clean;
    clean.SetCompactPointFields(true);
    clean.SetMergePoints(false);
    clean.SetRemoveDegenerateCells(false);
    result = clean.Execute(result);

    // now we are done the algorithm and conversion of arrays so
    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      throw viskores::cont::ErrorFilterExecution(
        "Unable to convert Viskores result dataSet back to VTK.");
    }
  }
  catch (const viskores::cont::Error& e)
  {
    if (this->ForceVTKm)
    {
      vtkErrorMacro(<< "Viskores error: " << e.GetMessage());
      return 0;
    }
    else
    {
      vtkWarningMacro(<< "Viskores failed with message: " << e.GetMessage() << "\n"
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
