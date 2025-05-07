// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkmContour.h"

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
#include "vtkmlib/PolyDataConverter.h"

#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/ErrorUserAbort.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/worklet/WorkletMapField.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmContour);

//------------------------------------------------------------------------------
vtkmContour::vtkmContour() = default;

//------------------------------------------------------------------------------
vtkmContour::~vtkmContour() = default;

//------------------------------------------------------------------------------
void vtkmContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkmContour::CanProcessInput(vtkDataSet* input)
{
  // viskores::filter::contour::Contour currently does not support gradient field generation
  if (this->GetComputeGradients())
  {
    return false;
  }

  // currently, viskores::filter::contour::Contour always generates single precision points
  auto pointSet = vtkPointSet::SafeDownCast(input);
  if ((this->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION) ||
    (this->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION && pointSet &&
      pointSet->GetPoints()->GetDataType() != VTK_FLOAT))
  {
    return false;
  }

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

//------------------------------------------------------------------------------
int vtkmContour::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Nothing to process, return early
  if (this->GetNumberOfContours() == 0 || input->GetNumberOfCells() == 0)
  {
    return 1;
  }

  // Find the scalar array:
  int association = this->GetInputArrayAssociation(0, inputVector);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != vtkDataObject::FIELD_ASSOCIATION_POINTS || inputArray == nullptr)
  {
    vtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 0;
  }

  try
  {
    viskores::cont::ScopedRuntimeDeviceTracker rtdt([&]() { return this->CheckAbort(); });

    if (!this->CanProcessInput(input))
    {
      throw viskores::cont::ErrorFilterExecution(
        "Input dataset/parameters not supported by vtkmContour.");
    }

    const char* scalarFieldName = inputArray->GetName();
    if (!scalarFieldName || scalarFieldName[0] == '\0')
    {
      scalarFieldName = tovtkm::NoNameVTKFieldName();
    }

    const int numContours = this->GetNumberOfContours();
    viskores::filter::contour::Contour filter;
    filter.SetActiveField(scalarFieldName, viskores::cont::Field::Association::Points);
    filter.SetGenerateNormals(this->GetComputeNormals() != 0);
    filter.SetNormalArrayName("Normals");
    filter.SetNumberOfIsoValues(numContours);
    for (int i = 0; i < numContours; ++i)
    {
      filter.SetIsoValue(i, this->GetValue(i));
    }

    // convert the input dataset to a viskores::cont::DataSet
    viskores::cont::DataSet in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    if (!this->ComputeScalars)
    {
      // don't pass the scalar field
      filter.SetFieldsToPass(viskores::filter::FieldSelection(
        scalarFieldName, viskores::filter::FieldSelection::Mode::Exclude));
    }

    viskores::cont::DataSet result = filter.Execute(in);
    ChangeTriangleOrientation(result);

    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      throw viskores::cont::ErrorFilterExecution(
        "Unable to convert Viskores result dataSet back to VTK.");
    }

    if (this->ComputeScalars)
    {
      output->GetPointData()->SetActiveScalars(inputArray->GetName());
    }
    if (this->ComputeNormals)
    {
      output->GetPointData()->SetActiveAttribute(
        filter.GetNormalArrayName().c_str(), vtkDataSetAttributes::NORMALS);
    }
  }
  catch (const viskores::cont::ErrorUserAbort&)
  {
    // viskores detected an abort request, clear the output
    output->Initialize();
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
