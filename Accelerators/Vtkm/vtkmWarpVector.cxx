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
#include "vtkmWarpVector.h"
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

#include <vtkm/filter/WarpVector.h>

vtkStandardNewMacro(vtkmWarpVector);

//------------------------------------------------------------------------------
vtkmWarpVector::vtkmWarpVector()
  : vtkWarpVector()
{
}

//------------------------------------------------------------------------------
vtkmWarpVector::~vtkmWarpVector() {}

//------------------------------------------------------------------------------
int vtkmWarpVector::RequestData(vtkInformation* vtkNotUsed(request),
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
  vtkIdType numPts = input->GetPoints()->GetNumberOfPoints();

  vtkDataArray* vectors = this->GetInputArrayToProcess(0, inputVector);
  int vectorsAssociation = this->GetInputArrayAssociation(0, inputVector);

  if (!vectors || !numPts)
  {
    vtkDebugMacro(<< "no input data");
    return 1;
  }

  output->CopyStructure(input);

  try
  {
    vtkm::cont::DataSet in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    vtkm::cont::Field vectorField = tovtkm::Convert(vectors, vectorsAssociation);
    in.AddField(vectorField);

    vtkmInputFilterPolicy policy;
    vtkm::filter::WarpVector warpVector(this->ScaleFactor);
    warpVector.SetUseCoordinateSystemAsField(true);
    warpVector.SetVectorField(vectorField.GetName(), vectorField.GetAssociation());
    auto result = warpVector.Execute(in, policy);

    vtkDataArray* warpVectorResult =
      fromvtkm::Convert(result.GetField("warpvector", vtkm::cont::Field::Association::POINTS));
    vtkNew<vtkPoints> newPts;

    newPts->SetNumberOfPoints(warpVectorResult->GetNumberOfTuples());
    newPts->SetData(warpVectorResult);
    output->SetPoints(newPts);
    warpVectorResult->FastDelete();
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
void vtkmWarpVector::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
