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

#include "vtkmFilterPolicy.h"

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/worklet/WorkletMapField.h>

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
  // vtkm::filter::contour::Contour currently does not support gradient field generation
  if (this->GetComputeGradients())
  {
    return false;
  }

  // currently, vtkm::filter::contour::Contour always generates single precision points
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
    // Currently, vtkm's flying edges implementation crashes for some cases.
    // Temporarily disabling this code path
    return false;
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
    if (!this->CanProcessInput(input))
    {
      throw vtkm::cont::ErrorFilterExecution(
        "Input dataset/parameters not supported by vtkmContour.");
    }

    const char* scalarFieldName = inputArray->GetName();
    if (!scalarFieldName || scalarFieldName[0] == '\0')
    {
      scalarFieldName = tovtkm::NoNameVTKFieldName();
    }

    const int numContours = this->GetNumberOfContours();
    vtkm::filter::contour::Contour filter;
    filter.SetActiveField(scalarFieldName, vtkm::cont::Field::Association::Points);
    filter.SetGenerateNormals(this->GetComputeNormals() != 0);
    filter.SetNormalArrayName("Normals");
    filter.SetNumberOfIsoValues(numContours);
    for (int i = 0; i < numContours; ++i)
    {
      filter.SetIsoValue(i, this->GetValue(i));
    }

    // convert the input dataset to a vtkm::cont::DataSet
    vtkm::cont::DataSet in;
    if (this->ComputeScalars)
    {
      in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    }
    else
    {
      in = tovtkm::Convert(input, tovtkm::FieldsFlag::None);
      // explicitly convert just the field we need
      auto inField = tovtkm::Convert(inputArray, association);
      in.AddField(inField);
      // don't pass this field
      filter.SetFieldsToPass(
        vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::Mode::None));
    }

    vtkm::cont::DataSet result = filter.Execute(in);
    ChangeTriangleOrientation(result);

    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      throw vtkm::cont::ErrorFilterExecution("Unable to convert VTKm result dataSet back to VTK.");
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
  catch (const vtkm::cont::Error& e)
  {
    vtkWarningMacro(<< "VTK-m failed with message: " << e.GetMessage() << "\n"
                    << "Falling back to the default VTK implementation.");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // we got this far, everything is good
  return 1;
}
