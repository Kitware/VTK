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
#include <vtkm/filter/Contour.h>

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
bool vtkmContour::IsSupportedInput(vtkDataSet* input)
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
int vtkmContour::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Find the scalar array:
  int association = this->GetInputArrayAssociation(0, inputVector);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != vtkDataObject::FIELD_ASSOCIATION_POINTS || inputArray == nullptr ||
    inputArray->GetName() == nullptr || inputArray->GetName()[0] == '\0')
  {
    vtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 0;
  }

  const int numContours = this->GetNumberOfContours();
  if (numContours == 0)
  {
    return 1;
  }

  try
  {
    if (!vtkmContour::IsSupportedInput(input))
    {
      throw vtkm::cont::ErrorFilterExecution("Input dataset is not supported by vtkmContour.");
    }

    vtkm::filter::Contour filter;
    filter.SetActiveField(inputArray->GetName(), vtkm::cont::Field::Association::POINTS);
    filter.SetGenerateNormals(this->GetComputeNormals() != 0);
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
      filter.SetFieldsToPass(vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::MODE_NONE));
    }

    vtkm::cont::DataSet result = filter.Execute(in);

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
    vtkWarningMacro(<< "VTK-m error: " << e.GetMessage() << "\n"
                    << "Falling back to the default VTK implementation.");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // we got this far, everything is good
  return 1;
}
