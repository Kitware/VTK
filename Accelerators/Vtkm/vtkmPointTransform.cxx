/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkmPointTransform.h"

#include "vtkCellData.h"
#include "vtkHomogeneousTransform.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkm/cont/Error.h"
#include "vtkm/filter/PointTransform.h"
#include "vtkm/filter/PointTransform.hxx"

#include "vtkmFilterPolicy.h"

vtkStandardNewMacro(vtkmPointTransform);
vtkCxxSetObjectMacro(vtkmPointTransform, Transform, vtkHomogeneousTransform);

//------------------------------------------------------------------------------
vtkmPointTransform::vtkmPointTransform()
{
  this->Transform = nullptr;
}

//------------------------------------------------------------------------------
vtkmPointTransform::~vtkmPointTransform()
{
  this->SetTransform(nullptr);
}

//------------------------------------------------------------------------------
int vtkmPointTransform::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmPointTransform::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
  vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

//------------------------------------------------------------------------------
int vtkmPointTransform::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkSmartPointer<vtkPointSet> output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting rectilinear grid
    vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rectToPoints;
      rectToPoints->SetInputData(inRect);
      rectToPoints->Update();
      input = rectToPoints->GetOutput();
    }
  }
  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  output->CopyStructure(input);

  vtkPoints* inPts = input->GetPoints();

  if (!inPts || !this->Transform)
  {
    vtkDebugMacro(<< "Miss input points or transform matrix");
    return 0;
  }

  try
  {
    vtkm::cont::DataSet in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    vtkMatrix4x4* matrix = this->Transform->GetMatrix();
    vtkm::Matrix<vtkm::FloatDefault, 4, 4> vtkmMatrix;
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        vtkmMatrix[i][j] = static_cast<vtkm::FloatDefault>(matrix->GetElement(i, j));
      }
    }

    vtkm::filter::PointTransform pointTransform;
    pointTransform.SetUseCoordinateSystemAsField(true);
    pointTransform.SetTransform(vtkmMatrix);

    vtkmInputFilterPolicy policy;
    auto result = pointTransform.Execute(in, policy);
    vtkDataArray* pointTransformResult =
      fromvtkm::Convert(result.GetField("transform", vtkm::cont::Field::Association::POINTS));
    vtkPoints* newPts = vtkPoints::New();
    // Update points
    newPts->SetNumberOfPoints(pointTransformResult->GetNumberOfTuples());
    newPts->SetData(pointTransformResult);
    output->SetPoints(newPts);
    newPts->FastDelete();
    pointTransformResult->FastDelete();
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
  }

  // Update ourselves and release memory
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//------------------------------------------------------------------------------
void vtkmPointTransform::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Transform: " << this->Transform << "\n";
}
