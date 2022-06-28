//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "vtkmGradient.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/field_conversion/PointAverage.h>
#include <vtkm/filter/vector_analysis/Gradient.h>

vtkStandardNewMacro(vtkmGradient);

namespace
{

inline vtkm::cont::DataSet CopyDataSetStructure(const vtkm::cont::DataSet& ds)
{
  vtkm::cont::DataSet cp;
  cp.CopyStructure(ds);
  return cp;
}

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
  auto ah =
    tovtkm::DataArrayToArrayHandle<vtkAOSDataArrayTemplate<unsigned char>, 1>::Wrap(ghostArray);
  int result = vtkm::cont::Algorithm::Reduce(
    vtkm::cont::make_ArrayHandleTransform(ah, MaskBits(flags)), 0, vtkm::LogicalOr());
  return result;
}

} // anonymous namespace

//------------------------------------------------------------------------------
vtkmGradient::vtkmGradient() = default;

//------------------------------------------------------------------------------
vtkmGradient::~vtkmGradient() = default;

//------------------------------------------------------------------------------
void vtkmGradient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkmGradient::CanProcessInput(vtkDataSet* input)
{
  auto unstructuredGrid = vtkUnstructuredGrid::SafeDownCast(input);
  if (unstructuredGrid)
  {
    auto cellTypes = unstructuredGrid->GetDistinctCellTypesArray();
    if (cellTypes)
    {
      for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
      {
        unsigned char cellType = cellTypes->GetValue(i);
        // VTK-m only supports some cell types
        if (cellType == VTK_EMPTY_CELL || cellType == VTK_POLY_VERTEX ||
          cellType == VTK_POLY_LINE || cellType == VTK_TRIANGLE_STRIP || cellType > VTK_PYRAMID)
        {
          return false;
        }
      }
    }
    return true;
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkmGradient::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // grab the input array to process to determine the field want to compute
  // the gradient for
  int association = this->GetInputArrayAssociation(0, inputVector);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);

  // Some early exit checks
  if (input->GetNumberOfCells() == 0)
  {
    // need cells to compute the gradient so if we don't have cells. we can't compute anything.
    // if we have points and an array with values provide a warning letting the user know that
    // no gradient will be computed because of the lack of cells. otherwise the dataset is
    // assumed empty and we can skip providing a warning message to the user.
    if (input->GetNumberOfPoints() && inputArray && inputArray->GetNumberOfTuples())
    {
      vtkWarningMacro("Cannot compute gradient for datasets without cells");
    }
    output->ShallowCopy(input);
    return 1;
  }
  if (inputArray == nullptr)
  {
    vtkErrorMacro("No input array. If this dataset is part of a composite dataset"
      << " check to make sure that all non-empty blocks have this array.");
    return 0;
  }
  if (inputArray->GetNumberOfComponents() == 0)
  {
    vtkErrorMacro("Input array must have at least one component.");
    return 0;
  }

  output->ShallowCopy(input);

  try
  {
    if (!this->CanProcessInput(input))
    {
      throw vtkm::cont::ErrorFilterExecution(
        "Input dataset/parameters not supported by vtkmGradient.");
    }

    // convert the input dataset to a vtkm::cont::DataSet. We explicitly drop
    // all arrays from the conversion as this algorithm doesn't change topology
    // and therefore doesn't need input fields converted through the VTK-m filter
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::None);
    vtkm::cont::Field field = tovtkm::Convert(inputArray, association);
    in.AddField(field);

    const bool fieldIsPoint = field.GetAssociation() == vtkm::cont::Field::Association::Points;
    const bool fieldIsCell = field.GetAssociation() == vtkm::cont::Field::Association::Cells;
    const bool fieldIsVec = (inputArray->GetNumberOfComponents() == 3);
    const bool fieldIsScalar =
      inputArray->GetDataType() == VTK_FLOAT || inputArray->GetDataType() == VTK_DOUBLE;
    const bool fieldValid =
      (fieldIsPoint || fieldIsCell) && fieldIsScalar && !field.GetName().empty();

    // ignore cell gradients on structured and rectilinear grids as the algorithm for
    // VTK-m differs from VTK. Once VTK-m is able to do stencil based
    // gradients for points and cells, we can remove this check.
    if (fieldIsCell && (input->IsA("vtkStructuredGrid") || input->IsA("vtkRectilinearGrid")))
    {
      throw vtkm::cont::ErrorFilterExecution(
        std::string("cell gradient of ") + input->GetClassName() + " is not supported.");
    }

    if (input->IsA("vtkImageData") || input->IsA("vtkStructuredGrid") ||
      input->IsA("vtkRectilinearGrid"))
    {
      vtkUnsignedCharArray* ghostArray = nullptr;
      int hidden = 0;
      if (fieldIsCell)
      {
        ghostArray = input->GetCellData()->GetGhostArray();
        hidden = vtkDataSetAttributes::HIDDENCELL;
      }
      else if (fieldIsPoint)
      {
        ghostArray = input->GetPointData()->GetGhostArray();
        hidden = vtkDataSetAttributes::HIDDENPOINT;
      }

      if (ghostArray && HasGhostFlagsSet(ghostArray, hidden))
      {
        throw vtkm::cont::ErrorFilterExecution("hidden points/cells not supported.");
      }
    }

    if (!fieldValid)
    {
      throw vtkm::cont::ErrorFilterExecution("Unsupported field type.");
    }

    auto passNoFields = vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::Mode::None);
    vtkm::filter::vector_analysis::Gradient filter;
    filter.SetFieldsToPass(passNoFields);
    filter.SetColumnMajorOrdering();

    if (fieldIsVec)
    { // this properties are only valid when processing a vec<3> field
      filter.SetComputeDivergence(this->ComputeDivergence != 0);
      filter.SetComputeVorticity(this->ComputeVorticity != 0);
      filter.SetComputeQCriterion(this->ComputeQCriterion != 0);
    }
    else if (this->ComputeQCriterion || this->ComputeVorticity || this->ComputeDivergence)
    {
      vtkWarningMacro("Input array must have exactly three components with "
        << "ComputeDivergence, ComputeVorticity or ComputeQCriterion flag enabled."
        << "Skipping divergence, vorticity and Q-criterion computation.");
    }

    if (this->ResultArrayName)
    {
      filter.SetOutputFieldName(this->ResultArrayName);
    }

    if (this->DivergenceArrayName)
    {
      filter.SetDivergenceName(this->DivergenceArrayName);
    }

    if (this->VorticityArrayName)
    {
      filter.SetVorticityName(this->VorticityArrayName);
    }

    if (this->QCriterionArrayName)
    {
      filter.SetQCriterionName(this->QCriterionArrayName);
    }
    else
    {
      filter.SetQCriterionName("Q-criterion");
    }

    // Run the VTK-m Gradient Filter
    // -----------------------------
    vtkm::cont::DataSet result;
    if (fieldIsPoint)
    {
      filter.SetComputePointGradient(!this->FasterApproximation);
      filter.SetActiveField(field.GetName(), vtkm::cont::Field::Association::Points);
      result = filter.Execute(in);

      // When we have faster approximation enabled the VTK-m gradient will output
      // a cell field not a point field. So at that point we will need to convert
      // back to a point field
      if (this->FasterApproximation)
      {
        vtkm::filter::field_conversion::PointAverage cellToPoint;
        cellToPoint.SetFieldsToPass(passNoFields);

        auto c2pIn = result;
        result = CopyDataSetStructure(result);

        if (this->ComputeGradient)
        {
          cellToPoint.SetActiveField(
            filter.GetOutputFieldName(), vtkm::cont::Field::Association::Cells);
          auto ds = cellToPoint.Execute(c2pIn);
          result.AddField(ds.GetField(0));
        }
        if (this->ComputeDivergence && fieldIsVec)
        {
          cellToPoint.SetActiveField(
            filter.GetDivergenceName(), vtkm::cont::Field::Association::Cells);
          auto ds = cellToPoint.Execute(c2pIn);
          result.AddField(ds.GetField(0));
        }
        if (this->ComputeVorticity && fieldIsVec)
        {
          cellToPoint.SetActiveField(
            filter.GetVorticityName(), vtkm::cont::Field::Association::Cells);
          auto ds = cellToPoint.Execute(c2pIn);
          result.AddField(ds.GetField(0));
        }
        if (this->ComputeQCriterion && fieldIsVec)
        {
          cellToPoint.SetActiveField(
            filter.GetQCriterionName(), vtkm::cont::Field::Association::Cells);
          auto ds = cellToPoint.Execute(c2pIn);
          result.AddField(ds.GetField(0));
        }
      }
    }
    else
    {
      // we need to convert the field to be a point field
      vtkm::filter::field_conversion::PointAverage cellToPoint;
      cellToPoint.SetFieldsToPass(passNoFields);
      cellToPoint.SetActiveField(field.GetName(), field.GetAssociation());
      cellToPoint.SetOutputFieldName(field.GetName());
      in = cellToPoint.Execute(in);

      filter.SetComputePointGradient(false);
      filter.SetActiveField(field.GetName(), vtkm::cont::Field::Association::Points);
      result = filter.Execute(in);
    }

    // Remove gradient field from result if it was not requested.
    auto requestedResult = result;
    if (!this->ComputeGradient)
    {
      requestedResult = CopyDataSetStructure(result);
      vtkm::Id numOfFields = static_cast<vtkm::Id>(result.GetNumberOfFields());
      for (vtkm::Id i = 0; i < numOfFields; ++i)
      {
        if (result.GetField(i).GetName() != filter.GetOutputFieldName())
        {
          requestedResult.AddField(result.GetField(i));
        }
      }
    }

    // convert arrays back to VTK
    if (!fromvtkm::ConvertArrays(result, output))
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
      vtkWarningMacro(<< "VTK-m error: " << e.GetMessage()
                      << " Falling back to VTK implementation.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }
  }

  return 1;
}
