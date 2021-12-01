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
#include "vtkmClipInternals.h"

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

#include <vtkm/cont/DataSet.h>

#include <algorithm>

vtkStandardNewMacro(vtkmClip);

//------------------------------------------------------------------------------
void vtkmClip::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ClipValue: " << this->GetClipValue() << "\n";
  os << indent << "ClipFunction: \n";
  this->GetClipFunction()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ComputeScalars: " << this->GetComputeScalars() << "\n";
}

//------------------------------------------------------------------------------
vtkmClip::vtkmClip()
  : Internals(new vtkmClip::internals)
{
  this->Internals->ClipFunctionConverter.reset(new tovtkm::ImplicitFunctionConverter());
  // Clip active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkmClip::~vtkmClip() = default;

//------------------------------------------------------------------------------
vtkMTimeType vtkmClip::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->GetClipFunction())
  {
    mTime = std::max(mTime, this->GetClipFunction()->GetMTime());
  }
  return mTime;
}

//------------------------------------------------------------------------------
double vtkmClip::GetClipValue()
{
  return this->Internals->ClipValue;
}

//------------------------------------------------------------------------------
void vtkmClip::SetClipValue(double val)
{
  this->Internals->ClipValue = val;
}

//------------------------------------------------------------------------------
bool vtkmClip::GetComputeScalars()
{
  return this->Internals->ComputeScalars;
}

//------------------------------------------------------------------------------
void vtkmClip::SetComputeScalars(bool val)
{
  this->Internals->ComputeScalars = val;
}

//------------------------------------------------------------------------------
void vtkmClip::SetClipFunction(vtkImplicitFunction* clipFunction)
{
  if (this->GetClipFunction() != clipFunction)
  {
    this->Internals->ClipFunction = clipFunction;
    this->Internals->ClipFunctionConverter->Set(clipFunction);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkImplicitFunction* vtkmClip::GetClipFunction()
{
  return this->Internals->ClipFunction;
}

//------------------------------------------------------------------------------
int vtkmClip::RequestData(
  vtkInformation*, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
  vtkInformation* outInfo = outInfoVec->GetInformationObject(0);

  // Extract data objects from info:
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Find the scalar array:
  int assoc = this->GetInputArrayAssociation(0, inInfoVec);
  vtkDataArray* scalars = this->GetInputArrayToProcess(0, inInfoVec);
  if (!this->GetClipFunction() &&
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
      this->GetComputeScalars() ? tovtkm::FieldsFlag::PointsAndCells : tovtkm::FieldsFlag::None;
    auto in = tovtkm::Convert(input, fieldsFlag);

    // Run filter:
    vtkm::cont::DataSet result;
    if (this->GetClipFunction())
    {
      result = this->Internals->ExecuteClipWithImplicitFunction(in);
    }
    else
    {
      result = this->Internals->ExecuteClipWithField(in, scalars, assoc);
    }

    // Convert result to output:
    if (!fromvtkm::Convert(result, output, input))
    {
      vtkErrorMacro("Error generating vtkUnstructuredGrid from vtkm's result.");
      return 0;
    }

    if (!this->GetClipFunction() && this->GetComputeScalars())
    {
      output->GetPointData()->SetActiveScalars(scalars->GetName());
    }

    return 1;
  }
  catch (const vtkm::cont::Error& e)
  {
    if (this->ForceVTKm)
    {
      vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
      return 0;
    }
    else
    {
      vtkWarningMacro(<< "VTK-m error: " << e.GetMessage()
                      << "Falling back to serial implementation.");

      vtkNew<vtkTableBasedClipDataSet> filter;
      filter->SetClipFunction(this->GetClipFunction());
      filter->SetValue(this->GetClipValue());
      filter->SetInputData(input);
      filter->Update();
      output->ShallowCopy(filter->GetOutput());
      return 1;
    }
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
