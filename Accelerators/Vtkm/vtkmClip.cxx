/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkmClip.h"

#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"
#include "vtkmlib/Storage.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/ClipWithField.h>
#include <vtkm/filter/ClipWithImplicitFunction.h>

#include <algorithm>


vtkStandardNewMacro(vtkmClip)

//------------------------------------------------------------------------------
void vtkmClip::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ClipValue: " << this->ClipValue << "\n";
  os << indent << "ClipFunction: \n";
  this->ClipFunction->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ComputeScalars: " << this->ComputeScalars << "\n";
}

//------------------------------------------------------------------------------
vtkmClip::vtkmClip()
  : ClipValue(0.),
    ComputeScalars(true),
    ClipFunction(nullptr)
{
  // Clip active point scalars by default
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkmClip::~vtkmClip()
{
}

//----------------------------------------------------------------------------
vtkMTimeType vtkmClip::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->ClipFunction)
  {
    mTime = std::max(mTime, this->ClipFunction->GetMTime());
  }
  return mTime;
}

//----------------------------------------------------------------------------
void vtkmClip::SetClipFunction(vtkImplicitFunction *clipFunction)
{
  if (this->ClipFunction != clipFunction)
  {
    this->ClipFunction = clipFunction;
    this->ClipFunctionConverter.Set(clipFunction);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkmClip::RequestData(vtkInformation *,
                          vtkInformationVector **inInfoVec,
                          vtkInformationVector *outInfoVec)
{
  vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
  vtkInformation* outInfo = outInfoVec->GetInformationObject(0);

  // Extract data objects from info:
  vtkDataSet *input =
    vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Find the scalar array:
  int assoc = this->GetInputArrayAssociation(0, inInfoVec);
  vtkDataArray *scalars = this->GetInputArrayToProcess(0, inInfoVec);

  // Validate input objects:
  if (!scalars)
  {
    vtkErrorMacro("Specified scalar array not found.");
    return 1;
  }
  if (input->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro("No points in input dataset!");
    return 1;
  }
  if (input->GetNumberOfCells() == 0)
  {
    vtkErrorMacro("No cells in input dataset!");
    return 1;
  }

  // Convert inputs to vtkm objects:
  vtkm::cont::DataSet in = tovtkm::Convert(input);
  vtkm::cont::Field field = tovtkm::Convert(scalars, assoc);

  if (field.GetAssociation() != vtkm::cont::Field::ASSOC_POINTS ||
      field.GetName() == std::string())
  {
    vtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 1;
  }

  // Configure vtkm filter:
  vtkm::filter::ClipWithField fieldFilter;
  vtkm::filter::ClipWithImplicitFunction functionFilter;

  // Run filter:
  vtkm::filter::ResultDataSet result;
  vtkmInputFilterPolicy policy;
  if (this->ClipFunction)
  {
    auto function = this->ClipFunctionConverter.Get();
    if (function.get())
    {
      functionFilter.SetImplicitFunction(function);
      result = functionFilter.Execute(in, policy);
    }
  }
  else
  {
    fieldFilter.SetClipValue(this->ClipValue);
    result = fieldFilter.Execute(in, field, policy);
  }

  if (!result.IsValid())
  {
    vtkWarningMacro(<< "vtkm Clip filter failed to run.\n"
                    << "Falling back to serial implementation.");

    vtkNew<vtkTableBasedClipDataSet> filter;
    filter->SetClipFunction(this->ClipFunction);
    filter->SetValue(this->ClipValue);
    filter->SetInputData(input);
    filter->Update();
    output->ShallowCopy(filter->GetOutput());

    return 1;
  }

  // ComputeScalars:
  vtkPointData *pd = input->GetPointData();
  if (this->ComputeScalars && pd)
  {
    for (vtkIdType i = 0; i < pd->GetNumberOfArrays(); ++i)
    {
      vtkDataArray *array = pd->GetArray(i);
      if (!array)
      {
        continue;
      }

      vtkm::cont::Field pField =
          tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_POINTS);

      try
      {
        bool success = this->ClipFunction ?
                       functionFilter.MapFieldOntoOutput(result, pField, policy) :
                       fieldFilter.MapFieldOntoOutput(result, pField, policy);
        if (!success)
        {
          throw vtkm::cont::ErrorBadValue("MapFieldOntoOutput returned false.");
        }
      }
      catch (vtkm::cont::Error &e)
      {
        vtkWarningMacro("Failed to map input point array '"
                        << array->GetName() << "' (ValueType: "
                        << array->GetDataTypeAsString() << ", "
                        << array->GetNumberOfComponents() << " components) "
                        << "onto output dataset: "
                        << e.what());
      }
    }
  }

  // Convert result to output:
  bool outputValid = fromvtkm::Convert(result.GetDataSet(), output, input);
  if (!outputValid)
  {
    vtkErrorMacro("Error generating vtkUnstructuredGrid from vtkm's result.");
    output->Initialize();
    return 1;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkmClip::FillInputPortInformation(int, vtkInformation *info)
{
  // These are the types supported by tovtkm::Convert:
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUniformGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
