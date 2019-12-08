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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/filter/Contour.h>
#include <vtkm/filter/Contour.hxx>

vtkStandardNewMacro(vtkmContour);

//------------------------------------------------------------------------------
vtkmContour::vtkmContour() {}

//------------------------------------------------------------------------------
vtkmContour::~vtkmContour() {}

//------------------------------------------------------------------------------
void vtkmContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkmContour::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkm::cont::ScopedRuntimeDeviceTracker tracker(
    vtkm::cont::DeviceAdapterTagCuda{}, vtkm::cont::RuntimeDeviceTrackerMode::Disable);

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

    vtkm::cont::DataSet result;
    vtkmInputFilterPolicy policy;

    result = filter.Execute(in, policy);

    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      vtkWarningMacro(<< "Unable to convert VTKm DataSet back to VTK.\n"
                      << "Falling back to serial implementation.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
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
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }

  // we got this far, everything is good
  return 1;
}
