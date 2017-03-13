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
#include "vtkmlib/Storage.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/MarchingCubes.h>

vtkStandardNewMacro(vtkmContour)

//------------------------------------------------------------------------------
vtkmContour::vtkmContour()
{
}

//------------------------------------------------------------------------------
vtkmContour::~vtkmContour()
{
}

//------------------------------------------------------------------------------
void vtkmContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkmContour::RequestData(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input =
      vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData* output =
      vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  const int numContours = this->GetNumberOfContours();
  if(numContours == 0)
  {
    return 1;
  }

  vtkm::filter::MarchingCubes filter;

  // set local variables
  filter.SetGenerateNormals(this->GetComputeNormals() != 0);

  filter.SetNumberOfIsoValues(numContours);
  for(int i = 0; i < numContours; ++i)
  {
    filter.SetIsoValue(i, this->GetValue(i));
  }


  // convert the input dataset to a vtkm::cont::DataSet
  vtkm::cont::DataSet in = tovtkm::Convert(input);

  // we need to map the given property to the data set
  int association = this->GetInputArrayAssociation(0, inputVector);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  vtkm::cont::Field field = tovtkm::Convert(inputArray, association);

  const bool dataSetValid =
      in.GetNumberOfCoordinateSystems() > 0 && in.GetNumberOfCellSets() > 0;
  const bool fieldValid =
      (field.GetAssociation() != vtkm::cont::Field::ASSOC_ANY) &&
      (field.GetName() != std::string());


  if (!dataSetValid)
  {
    vtkWarningMacro(<< "Will not be able to use VTKm dataset type is unknown");
  }
  if (!fieldValid)
  {
    vtkWarningMacro(<< "Will not be able to use VTKm field type is unknown");
  }


  vtkm::filter::ResultDataSet result;
  bool convertedDataSet = false;
  if (dataSetValid && fieldValid)
  {
    vtkmInputFilterPolicy policy;
    result = filter.Execute(in, field, policy);

    if (!result.IsValid())
    {
      vtkWarningMacro(<< "VTKm contour algorithm was failed to run. \n"
                      << "Falling back to serial implementation.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

    // convert other scalar arrays
    if (this->GetComputeScalars())
    {
      vtkPointData* pd = input->GetPointData();
      for (vtkIdType i = 0; i < pd->GetNumberOfArrays(); i++)
      {
        vtkDataArray* array = pd->GetArray(i);
        if (array == NULL)
        {
          continue;
        }

        vtkm::cont::Field pfield =
            tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_POINTS);
        try
        {
          filter.MapFieldOntoOutput(result, pfield, policy);
        }
        catch (vtkm::cont::Error&)
        { // nothing to do for now
          vtkWarningMacro(<< "Unable to use VTKm to convert field( "
                          << array->GetName() << " ) to the MarchingCubes"
                          << "output.");
        }
      }
    }

    // convert back the dataset to VTK
    convertedDataSet = fromvtkm::Convert(result.GetDataSet(), output, input);

    if (!convertedDataSet)
    {
      vtkWarningMacro(<< "Unable to convert VTKm DataSet back to VTK.\n"
                      << "Falling back to serial implementation.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

  }

  if (this->ComputeNormals)
  {
    output->GetPointData()->SetActiveAttribute(
          filter.GetNormalArrayName().c_str(), vtkDataSetAttributes::NORMALS);
  }

  // we got this far, everything is good
  return 1;
}
