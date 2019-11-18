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
#include "vtkmWarpScalar.h"
#include "vtkmConfig.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkm/cont/DataSetFieldAdd.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/WarpScalar.h>

vtkStandardNewMacro(vtkmWarpScalar);

//------------------------------------------------------------------------------
vtkmWarpScalar::vtkmWarpScalar()
  : vtkWarpScalar()
{
}

//------------------------------------------------------------------------------
vtkmWarpScalar::~vtkmWarpScalar() {}

//------------------------------------------------------------------------------
int vtkmWarpScalar::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkSmartPointer<vtkPointSet> output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      vtkNew<vtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }
  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  output->CopyStructure(input);

  // Get the scalar field info
  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  int inScalarsAssociation = this->GetInputArrayAssociation(0, inputVector);
  // Get the normal field info
  vtkDataArray* inNormals = input->GetPointData()->GetNormals();
  vtkPoints* inPts = input->GetPoints();

  // InScalars is not used when XYPlane is on
  if (!inPts || (!inScalars && !this->XYPlane))
  {
    vtkDebugMacro(<< "No data to warp");
    return 1;
  }

  try
  {
    vtkm::cont::DataSet in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    if (inScalars)
    {
      auto scalarFactor = tovtkm::Convert(inScalars, inScalarsAssociation);
      in.AddField(scalarFactor);
    }
    vtkm::Id numberOfPoints = in.GetCoordinateSystem().GetData().GetNumberOfValues();

    // ScaleFactor in vtk is the scalarAmount in vtk-m.
    vtkm::filter::WarpScalar warpScalar(this->ScaleFactor);
    warpScalar.SetUseCoordinateSystemAsField(true);

    // Get/generate the normal field
    if (inNormals && !this->UseNormal)
    { // DataNormal
      auto inNormalsField = tovtkm::Convert(inNormals, vtkDataObject::FIELD_ASSOCIATION_POINTS);
      in.AddField(inNormalsField);
      warpScalar.SetNormalField(inNormals->GetName());
    }
    else if (this->XYPlane)
    {
      using vecType = vtkm::Vec<vtkm::FloatDefault, 3>;
      vecType normal = vtkm::make_Vec<vtkm::FloatDefault>(0.0, 0.0, 1.0);
      vtkm::cont::ArrayHandleConstant<vecType> vectorAH =
        vtkm::cont::make_ArrayHandleConstant(normal, numberOfPoints);
      vtkm::cont::DataSetFieldAdd::AddPointField(in, "zNormal", vectorAH);
      warpScalar.SetNormalField("zNormal");
    }
    else
    {
      using vecType = vtkm::Vec<vtkm::FloatDefault, 3>;
      vecType normal =
        vtkm::make_Vec<vtkm::FloatDefault>(this->Normal[0], this->Normal[1], this->Normal[2]);
      vtkm::cont::ArrayHandleConstant<vecType> vectorAH =
        vtkm::cont::make_ArrayHandleConstant(normal, numberOfPoints);
      vtkm::cont::DataSetFieldAdd::AddPointField(in, "instanceNormal", vectorAH);
      warpScalar.SetNormalField("instanceNormal");
    }

    if (this->XYPlane)
    { // Just use the z value to warp the surface. Ignore the input scalars.
      std::vector<vtkm::FloatDefault> zValues;
      zValues.reserve(static_cast<size_t>(input->GetNumberOfPoints()));
      for (vtkIdType i = 0; i < input->GetNumberOfPoints(); i++)
      {
        zValues.push_back(input->GetPoints()->GetPoint(i)[2]);
      }
      vtkm::cont::DataSetFieldAdd::AddPointField(in, "scalarfactor", zValues);
      warpScalar.SetScalarFactorField("scalarfactor");
    }
    else
    {
      warpScalar.SetScalarFactorField(std::string(inScalars->GetName()));
    }

    vtkmInputFilterPolicy policy;
    auto result = warpScalar.Execute(in, policy);
    vtkDataArray* warpScalarResult =
      fromvtkm::Convert(result.GetField("warpscalar", vtkm::cont::Field::Association::POINTS));
    vtkPoints* newPts = vtkPoints::New();
    // Update points
    newPts->SetNumberOfPoints(warpScalarResult->GetNumberOfTuples());
    newPts->SetData(warpScalarResult);
    output->SetPoints(newPts);
    newPts->Delete();
    warpScalarResult->FastDelete();
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }

  // Update ourselves and release memory
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//------------------------------------------------------------------------------
void vtkmWarpScalar::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
