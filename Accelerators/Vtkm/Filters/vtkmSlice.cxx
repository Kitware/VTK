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
#include "vtkmSlice.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
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

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/contour/Slice.h>
#include <vtkm/worklet/WorkletMapField.h>

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

struct OrientationTransform : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ConnPortal>
  VTKM_EXEC void operator()(vtkm::Id idx, ConnPortal conn) const
  {
    auto temp = conn.Get(idx);
    conn.Set(idx, conn.Get(idx + 2));
    conn.Set(idx + 2, temp);
  }
};

struct Negate : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  template <typename T>
  VTKM_EXEC void operator()(T& v) const
  {
    v *= T(-1);
  }
};

void ChangeTriangleOrientation(vtkm::cont::DataSet& dataset)
{
  vtkm::cont::Invoker invoke;

  vtkm::cont::CellSetSingleType<> cs;
  dataset.GetCellSet().AsCellSet(cs);
  vtkm::cont::ArrayHandle<vtkm::Id> conn =
    cs.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  invoke(OrientationTransform{},
    vtkm::cont::make_ArrayHandleCounting(0, 3, conn.GetNumberOfValues() / 3), conn);

  auto numPoints = cs.GetNumberOfPoints();
  cs.Fill(numPoints, vtkm::CellShapeTagTriangle::Id, 3, conn);
  dataset.SetCellSet(cs);

  if (dataset.HasPointField("Normals"))
  {
    vtkm::cont::ArrayHandle<vtkm::Vec3f> normals;
    dataset.GetPointField("Normals").GetData().AsArrayHandle(normals);
    invoke(Negate{}, normals);
  }
}

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
      throw vtkm::cont::ErrorFilterExecution("Input dataset not supported by vtkmSlice.");
    }

    if (!this->GenerateTriangles)
    {
      throw vtkm::cont::ErrorFilterExecution("vtkmSlice only generates triangles in the output.");
    }

    if (this->SortBy != VTK_SORT_BY_VALUE)
    {
      throw vtkm::cont::ErrorFilterExecution(
        "vtkmSlice currently only supports `VTK_SORT_BY_VALUE`.");
    }

    // currently, vtkm::filter::contour::Slice always generates single precision points
    auto pointSet = vtkPointSet::SafeDownCast(input);
    if ((this->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION) ||
      (this->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION && pointSet &&
        pointSet->GetPoints()->GetDataType() != VTK_FLOAT))
    {
      throw vtkm::cont::ErrorFilterExecution(
        "vtkmSlice only supports generating single precision output points.");
    }

    tovtkm::ImplicitFunctionConverter clipFunctionConverter;
    clipFunctionConverter.Set(this->CutFunction);
    auto function = clipFunctionConverter.Get();

    const int numContours = this->GetNumberOfContours();
    vtkm::filter::contour::Slice filter;
    filter.SetImplicitFunction(function);
    filter.SetNumberOfIsoValues(numContours);
    for (int i = 0; i < numContours; ++i)
    {
      filter.SetIsoValue(i, this->GetValue(i));
    }

    // convert the input dataset to a vtkm::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    vtkm::cont::DataSet result = filter.Execute(in);
    ChangeTriangleOrientation(result);

    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      throw vtkm::cont::ErrorFilterExecution("Unable to convert VTKm result dataSet back to VTK.");
    }

    output->GetPointData()->GetAbstractArray("sliceScalars")->SetName("cutScalars");
    if (this->GenerateCutScalars)
    {
      output->GetPointData()->SetActiveScalars("cutScalars");
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkWarningMacro(<< "VTK-m failed with message: " << e.GetMessage() << "\n"
                    << "Falling back to the default VTK implementation.");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // we got this far, everything is good
  return 1;
}
VTK_ABI_NAMESPACE_END
