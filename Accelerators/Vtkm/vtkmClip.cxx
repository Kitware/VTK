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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"
#include "vtkmlib/Storage.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/Clip.h>

vtkStandardNewMacro(vtkmClip)

//------------------------------------------------------------------------------
void vtkmClip::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ClipValue: " << this->ClipValue << "\n";
  os << indent << "ComputeScalars: " << this->ComputeScalars << "\n";
}

//------------------------------------------------------------------------------
vtkmClip::vtkmClip()
  : ClipValue(0.),
    ComputeScalars(true)
{
  // Clip active point scalars by default
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkmClip::~vtkmClip()
{
}

//------------------------------------------------------------------------------
int vtkmClip::RequestDataObject(vtkInformation *,
                                vtkInformationVector **inVec,
                                vtkInformationVector *outVec)
{
  vtkInformation* inInfo = inVec[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataSet *input =
      vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input)
  {
    // If the input has 3D cells, we will produce an unstructured grid. Otherwise,
    // we'll make a polydata:
    bool has3DCells = false;
    vtkCellIterator *i = input->NewCellIterator();
    for (i->InitTraversal(); !i->IsDoneWithTraversal(); i->GoToNextCell())
    {
      if (i->GetCellDimension() == 3)
      {
        has3DCells = true;
        break;
      }
    }
    i->Delete();

    vtkInformation* info = outVec->GetInformationObject(0);
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

    if (has3DCells && (!output || !output->IsA("vtkUnstructuredGrid")))
    {
      vtkNew<vtkUnstructuredGrid> newOutput;
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput.Get());
    }
    else if (!has3DCells && (!output || !output->IsA("vtkPolyData")))
    {
      vtkNew<vtkPolyData> newOutput;
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput.Get());
    }

    return 1;
  }

  return 0;
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
  vtkDataObject *outputDO = outInfo->Get(vtkDataObject::DATA_OBJECT());

  // Find the scalar array:
  int assoc = this->GetInputArrayAssociation(0, inInfoVec);
  vtkDataArray *scalars = this->GetInputArrayToProcess(0, inInfoVec);

  // Validate input objects:
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
  vtkm::filter::Clip filter;
  filter.SetClipValue(this->ClipValue);

  // Run filter:
  vtkm::filter::ResultDataSet result;
  vtkmInputFilterPolicy policy;
  result = filter.Execute(in, field, policy);

  if (!result.IsValid())
  {
    vtkErrorMacro("vtkm Clip filter failed to run.");
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
        if (!filter.MapFieldOntoOutput(result, pField, policy))
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
  vtkUnstructuredGrid *outputUG = vtkUnstructuredGrid::SafeDownCast(outputDO);
  vtkPolyData *outputPD = outputUG ? NULL : vtkPolyData::SafeDownCast(outputDO);

  bool outputValid = false;
  if (outputUG)
  {
    outputValid = fromvtkm::Convert(result.GetDataSet(), outputUG, input);
  }
  else if (outputPD)
  {
    outputValid = fromvtkm::Convert(result.GetDataSet(), outputPD, input);
  }
  else
  {
    vtkErrorMacro("Unsupported output data object type: "
                  << (outputDO ? outputDO->GetClassName() : "(NULL)"));
    return 1;
  }

  if (!outputValid)
  {
    vtkErrorMacro("Error generating vtkPolyData from vtkm's result.");
    outputDO->Initialize();
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

//------------------------------------------------------------------------------
int vtkmClip::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPointSet");
  return 1;
}
