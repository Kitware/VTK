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
#include "vtkmlib/ImplicitFunctionConverter.h"
#include "vtkmlib/PolyDataConverter.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/filter/ClipWithField.h>
#include <vtkm/filter/ClipWithField.hxx>
#include <vtkm/filter/ClipWithImplicitFunction.h>
#include <vtkm/filter/ClipWithImplicitFunction.hxx>

#include <algorithm>

vtkStandardNewMacro(vtkmClip);

//------------------------------------------------------------------------------
void vtkmClip::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ClipValue: " << this->ClipValue << "\n";
  os << indent << "ClipFunction: \n";
  this->ClipFunction->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ComputeScalars: " << this->ComputeScalars << "\n";
}

//------------------------------------------------------------------------------
vtkmClip::vtkmClip()
  : ClipValue(0.)
  , ComputeScalars(true)
  , ClipFunction(nullptr)
  , ClipFunctionConverter(new tovtkm::ImplicitFunctionConverter)
{
  // Clip active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkmClip::~vtkmClip() {}

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
void vtkmClip::SetClipFunction(vtkImplicitFunction* clipFunction)
{
  if (this->ClipFunction != clipFunction)
  {
    this->ClipFunction = clipFunction;
    this->ClipFunctionConverter->Set(clipFunction);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkmClip::RequestData(
  vtkInformation*, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  vtkm::cont::ScopedRuntimeDeviceTracker tracker(
    vtkm::cont::DeviceAdapterTagCuda{}, vtkm::cont::RuntimeDeviceTrackerMode::Disable);

  vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
  vtkInformation* outInfo = outInfoVec->GetInformationObject(0);

  // Extract data objects from info:
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Find the scalar array:
  int assoc = this->GetInputArrayAssociation(0, inInfoVec);
  vtkDataArray* scalars = this->GetInputArrayToProcess(0, inInfoVec);
  if (!this->ClipFunction &&
    (assoc != vtkDataObject::FIELD_ASSOCIATION_POINTS || scalars == nullptr ||
      scalars->GetName() == nullptr || scalars->GetName()[0] == '\0'))
  {
    vtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 0;
  }

  // Validate input objects:
  if (input->GetNumberOfPoints() == 0 || input->GetNumberOfCells() == 0)
  {
    return 1; // nothing to do
  }

  try
  {
    // Convert inputs to vtkm objects:
    auto fieldsFlag =
      this->ComputeScalars ? tovtkm::FieldsFlag::PointsAndCells : tovtkm::FieldsFlag::None;
    auto in = tovtkm::Convert(input, fieldsFlag);

    // Run filter:
    vtkm::cont::DataSet result;
    vtkmInputFilterPolicy policy;
    if (this->ClipFunction)
    {
      vtkm::filter::ClipWithImplicitFunction functionFilter;
      auto function = this->ClipFunctionConverter->Get();
      if (function.GetValid())
      {
        functionFilter.SetImplicitFunction(function);
        result = functionFilter.Execute(in, policy);
      }
    }
    else
    {
      vtkm::filter::ClipWithField fieldFilter;
      if (!this->ComputeScalars)
      {
        // explicitly convert just the field we need
        auto inField = tovtkm::Convert(scalars, assoc);
        in.AddField(inField);
        // don't pass this field
        fieldFilter.SetFieldsToPass(
          vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::MODE_NONE));
      }

      fieldFilter.SetActiveField(scalars->GetName(), vtkm::cont::Field::Association::POINTS);
      fieldFilter.SetClipValue(this->ClipValue);
      result = fieldFilter.Execute(in, policy);
    }

    // Convert result to output:
    if (!fromvtkm::Convert(result, output, input))
    {
      vtkErrorMacro("Error generating vtkUnstructuredGrid from vtkm's result.");
      return 0;
    }

    if (!this->ClipFunction && this->ComputeScalars)
    {
      output->GetPointData()->SetActiveScalars(scalars->GetName());
    }

    return 1;
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkWarningMacro(<< "VTK-m error: " << e.GetMessage()
                    << "Falling back to serial implementation.");

    vtkNew<vtkTableBasedClipDataSet> filter;
    filter->SetClipFunction(this->ClipFunction);
    filter->SetValue(this->ClipValue);
    filter->SetInputData(input);
    filter->Update();
    output->ShallowCopy(filter->GetOutput());
    return 1;
  }
}

//------------------------------------------------------------------------------
int vtkmClip::FillInputPortInformation(int, vtkInformation* info)
{
  // These are the types supported by tovtkm::Convert:
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUniformGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
