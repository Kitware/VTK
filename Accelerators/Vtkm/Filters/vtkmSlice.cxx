// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkmSlice.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/ImplicitFunctionConverter.h"
#include "vtkmlib/PolyDataConverter.h"

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/Invoker.h>
#include <viskores/filter/contour/Slice.h>
#include <viskores/filter/entity_extraction/Threshold.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmSlice);

//------------------------------------------------------------------------------
vtkmSlice::vtkmSlice() = default;

//------------------------------------------------------------------------------
vtkmSlice::~vtkmSlice() = default;

//------------------------------------------------------------------------------
void vtkmSlice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkmSlice::CanProcessInput(vtkDataSet* input)
{
  auto imageData = vtkImageData::SafeDownCast(input);
  if (imageData && imageData->GetDataDimension() == 3)
  {
    return true;
  }

  auto rectilinearGrid = vtkRectilinearGrid::SafeDownCast(input);
  if (rectilinearGrid && rectilinearGrid->GetDataDimension() == 3)
  {
    return true;
  }

  auto structuredGrid = vtkStructuredGrid::SafeDownCast(input);
  if (structuredGrid && structuredGrid->GetDataDimension() == 3)
  {
    return true;
  }

  auto unstructuredGrid = vtkUnstructuredGrid::SafeDownCast(input);
  if (unstructuredGrid)
  {
    auto cellTypes = unstructuredGrid->GetDistinctCellTypesArray();
    if (cellTypes)
    {
      for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
      {
        unsigned char cellType = cellTypes->GetValue(i);
        // Supports only 3D linear cell types
        if (cellType < VTK_TETRA || cellType > VTK_PYRAMID)
        {
          return false;
        }
      }
    }
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
namespace
{

struct OrientationTransform : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ConnPortal>
  VISKORES_EXEC void operator()(viskores::Id idx, ConnPortal conn) const
  {
    auto temp = conn.Get(idx);
    conn.Set(idx, conn.Get(idx + 2));
    conn.Set(idx + 2, temp);
  }
};

struct Negate : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  template <typename T>
  VISKORES_EXEC void operator()(T& v) const
  {
    v *= T(-1);
  }
};

void ChangeTriangleOrientation(viskores::cont::DataSet& dataset)
{
  viskores::cont::Invoker invoke;

  viskores::cont::CellSetSingleType<> cs;
  dataset.GetCellSet().AsCellSet(cs);
  viskores::cont::ArrayHandle<viskores::Id> conn = cs.GetConnectivityArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  invoke(OrientationTransform{},
    viskores::cont::make_ArrayHandleCounting(0, 3, conn.GetNumberOfValues() / 3), conn);

  auto numPoints = cs.GetNumberOfPoints();
  cs.Fill(numPoints, viskores::CellShapeTagTriangle::Id, 3, conn);
  dataset.SetCellSet(cs);

  if (dataset.HasPointField("Normals"))
  {
    viskores::cont::ArrayHandle<viskores::Vec3f> normals;
    dataset.GetPointField("Normals").GetData().AsArrayHandle(normals);
    invoke(Negate{}, normals);
  }
}

struct IdentifyCellsToDiscard : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellset, FieldInCell cellGhostFlag,
    FieldInPoint pointGhostFlags, FieldOutCell discard);

  using ExecutionSignature = _4(_2, _3, PointCount);

  template <typename VecType>
  VISKORES_EXEC viskores::UInt8 operator()(viskores::UInt8 cellGhostFlag,
    const VecType& pointGhostFlags, viskores::IdComponent numPoints) const
  {
    if (cellGhostFlag & (vtkDataSetAttributes::DUPLICATECELL | vtkDataSetAttributes::HIDDENCELL))
    {
      return 1;
    }

    for (viskores::IdComponent i = 0; i < numPoints; ++i)
    {
      if (pointGhostFlags[i] & vtkDataSetAttributes::HIDDENPOINT)
      {
        return 1;
      }
    }

    return 0;
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
int vtkmSlice::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->CutFunction)
  {
    vtkErrorMacro("No cut function specified");
    return 0;
  }

  // Nothing to process, return early
  if (this->GetNumberOfContours() == 0 || input->GetNumberOfCells() == 0)
  {
    return 1;
  }

  try
  {
    if (!this->CanProcessInput(input))
    {
      throw viskores::cont::ErrorFilterExecution("Input dataset not supported by vtkmSlice.");
    }

    if (!this->GenerateTriangles)
    {
      throw viskores::cont::ErrorFilterExecution(
        "vtkmSlice only generates triangles in the output.");
    }

    if (this->SortBy != VTK_SORT_BY_VALUE)
    {
      throw viskores::cont::ErrorFilterExecution(
        "vtkmSlice currently only supports `VTK_SORT_BY_VALUE`.");
    }

    // currently, viskores::filter::contour::Slice always generates single precision points
    auto pointSet = vtkPointSet::SafeDownCast(input);
    if ((this->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION) ||
      (this->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION && pointSet &&
        pointSet->GetPoints()->GetDataType() != VTK_FLOAT))
    {
      throw viskores::cont::ErrorFilterExecution(
        "vtkmSlice only supports generating single precision output points.");
    }

    tovtkm::ImplicitFunctionConverter clipFunctionConverter;
    clipFunctionConverter.Set(this->CutFunction);
    auto function = clipFunctionConverter.Get();

    const int numContours = this->GetNumberOfContours();
    viskores::filter::contour::Slice filter;
    filter.SetImplicitFunction(function);
    filter.SetNumberOfIsoValues(numContours);
    for (int i = 0; i < numContours; ++i)
    {
      filter.SetIsoValue(i, this->GetValue(i));
    }

    // convert the input dataset to a viskores::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    viskores::cont::DataSet result = filter.Execute(in);
    ChangeTriangleOrientation(result);

    // discard hidden and duplicate cells
    if (input->GetCellGhostArray() || input->GetPointGhostArray())
    {
      using GhostValueTypeList = viskores::List<viskores::UInt8>;
      using GhostStorageList =
        viskores::List<viskores::cont::StorageTagConstant, viskores::cont::StorageTagBasic>;
      viskores::cont::UncertainArrayHandle<GhostValueTypeList, GhostStorageList> cellGhostArray,
        pointGhostArray;
      if (input->GetCellGhostArray())
      {
        const auto& field = result.GetCellField(input->GetCellGhostArray()->GetName());

        // FIXME: The ghost fields get converted to float in the slice filter. This is fixed in
        // newer version of Viskores. The copy should be removed when we update and replaced with:
        // cellGhostArray =
        // field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::UInt8>>();
        viskores::cont::ArrayHandle<viskores::UInt8> copy;
        viskores::cont::ArrayCopy(
          field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>(),
          copy);
        cellGhostArray = copy;
      }
      else
      {
        cellGhostArray =
          viskores::cont::ArrayHandleConstant<viskores::UInt8>(0, result.GetNumberOfCells());
      }

      if (input->GetPointGhostArray())
      {
        const auto& field = result.GetPointField(input->GetPointGhostArray()->GetName());

        // FIXME: The ghost fields get converted to float in the slice filter. This is fixed in
        // newer version of Viskores. The copy should be removed when we update and replaced with:
        // pointGhostArray =
        // field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::UInt8>>();
        viskores::cont::ArrayHandle<viskores::UInt8> copy;
        viskores::cont::ArrayCopy(
          field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>(),
          copy);
        pointGhostArray = copy;
      }
      else
      {
        pointGhostArray =
          viskores::cont::ArrayHandleConstant<viskores::UInt8>(0, result.GetNumberOfPoints());
      }

      viskores::cont::ArrayHandle<viskores::UInt8> discard;
      viskores::cont::Invoker{}(
        IdentifyCellsToDiscard{}, result.GetCellSet(), cellGhostArray, pointGhostArray, discard);

      result.AddCellField("discard", discard);

      viskores::filter::entity_extraction::Threshold threshold;
      threshold.SetActiveField("discard", viskores::cont::Field::Association::Cells);
      threshold.SetThresholdBelow(0);
      threshold.SetFieldsToPass(viskores::filter::FieldSelection(
        "discard", viskores::filter::FieldSelection::Mode::Exclude));
      result = threshold.Execute(result);
    }

    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      throw viskores::cont::ErrorFilterExecution(
        "Unable to convert Viskores result dataSet back to VTK.");
    }

    output->GetPointData()->GetAbstractArray("sliceScalars")->SetName("cutScalars");
    if (this->GenerateCutScalars)
    {
      output->GetPointData()->SetActiveScalars("cutScalars");
    }
  }
  catch (const viskores::cont::Error& e)
  {
    vtkWarningMacro(<< "Viskores failed with message: " << e.GetMessage() << "\n"
                    << "Falling back to the default VTK implementation.");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // we got this far, everything is good
  return 1;
}
VTK_ABI_NAMESPACE_END
