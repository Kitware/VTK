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
#include "vtkmProbe.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkmFilterPolicy.h"

#include "vtkm/filter/Probe.h"

vtkStandardNewMacro(vtkmProbe);

//------------------------------------------------------------------------------
vtkmProbe::vtkmProbe()
{
  this->SetNumberOfInputPorts(2);
  this->PassCellArrays = false;
  this->PassPointArrays = false;
  this->PassFieldArrays = true;
  this->ValidPointMaskArrayName = "vtkValidPointMask";
  this->ValidCellMaskArrayName = "vtkValidCellMask";
}

//------------------------------------------------------------------------------
void vtkmProbe::SetSourceData(vtkDataObject* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkmProbe::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(1, 0);
}

//------------------------------------------------------------------------------
void vtkmProbe::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
int vtkmProbe::RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataSet::DATA_OBJECT()));
  vtkDataSet* source = vtkDataSet::SafeDownCast(sourceInfo->Get(vtkDataSet::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataSet::DATA_OBJECT()));

  // Copy the input to the output as a starting point
  output->CopyStructure(input);

  try
  {
    // Convert the input dataset to a vtkm::cont::DataSet
    vtkm::cont::DataSet in = tovtkm::Convert(input);
    // VTK-m's probe filter requires the source to have at least a cellSet.
    vtkm::cont::DataSet so = tovtkm::Convert(source, tovtkm::FieldsFlag::PointsAndCells);
    if (so.GetNumberOfCells() <= 0)
    {
      vtkErrorMacro(<< "The source geometry does not have any cell set,"
                       "aborting vtkmProbe filter");
      return 0;
    }

    vtkmInputFilterPolicy policy;
    vtkm::filter::Probe probe;
    // The input in VTK is the geometry in VTKM and the source in VTK is the input
    // in VTKM.
    probe.SetGeometry(in);

    auto result = probe.Execute(so, policy);
    for (vtkm::Id i = 0; i < result.GetNumberOfFields(); i++)
    {
      const vtkm::cont::Field& field = result.GetField(i);
      vtkDataArray* fieldArray = fromvtkm::Convert(field);
      if (field.GetAssociation() == vtkm::cont::Field::Association::POINTS)
      {
        if (strcmp(fieldArray->GetName(), "HIDDEN") == 0)
        {
          fieldArray->SetName(this->ValidPointMaskArrayName.c_str());
        }
        output->GetPointData()->AddArray(fieldArray);
      }
      else if (field.GetAssociation() == vtkm::cont::Field::Association::CELL_SET)
      {
        if (strcmp(fieldArray->GetName(), "HIDDEN") == 0)
        {
          fieldArray->SetName(this->ValidCellMaskArrayName.c_str());
        }
        output->GetCellData()->AddArray(fieldArray);
      }
      fieldArray->FastDelete();
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }

  this->PassAttributeData(input, source, output);

  return 1;
}

//------------------------------------------------------------------------------
int vtkmProbe::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Update the whole extent in the output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int wholeExtent[6];
  if (inInfo && outInfo)
  {
    outInfo->CopyEntry(sourceInfo, vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->CopyEntry(sourceInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);

    // Make sure that the scalar type and number of components
    // are propagated from the source not the input.
    if (vtkImageData::HasScalarType(sourceInfo))
    {
      vtkImageData::SetScalarType(vtkImageData::GetScalarType(sourceInfo), outInfo);
    }
    if (vtkImageData::HasNumberOfScalarComponents(sourceInfo))
    {
      vtkImageData::SetNumberOfScalarComponents(
        vtkImageData::GetNumberOfScalarComponents(sourceInfo), outInfo);
    }
    return 1;
  }
  vtkErrorMacro("Missing input or output info!");
  return 0;
}

//------------------------------------------------------------------------------
int vtkmProbe::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (inInfo && outInfo)
  { // Source's update exetent should be independent of the resampling extent
    inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
    sourceInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    if (sourceInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
    }
    return 1;
  }
  vtkErrorMacro("Missing input or output info!");
  return 0;
}

//------------------------------------------------------------------------------
void vtkmProbe::PassAttributeData(
  vtkDataSet* input, vtkDataObject* vtkNotUsed(source), vtkDataSet* output)
{
  if (this->PassPointArrays)
  { // Copy point data arrays
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i = 0; i < numPtArrays; i++)
    {
      vtkDataArray* da = input->GetPointData()->GetArray(i);
      if (da && !output->GetPointData()->HasArray(da->GetName()))
      {
        output->GetPointData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      vtkAbstractArray* da = input->GetPointData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetPointData()->GetAttribute(i))
      {
        output->GetPointData()->SetAttribute(da, i);
      }
    }
  }

  // copy cell data arrays
  if (this->PassCellArrays)
  {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i = 0; i < numCellArrays; ++i)
    {
      vtkDataArray* da = input->GetCellData()->GetArray(i);
      if (!output->GetCellData()->HasArray(da->GetName()))
      {
        output->GetCellData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      vtkAbstractArray* da = input->GetCellData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetCellData()->GetAttribute(i))
      {
        output->GetCellData()->SetAttribute(da, i);
      }
    }
  }

  if (this->PassFieldArrays)
  {
    // nothing to do, vtkDemandDrivenPipeline takes care of that.
  }
  else
  {
    output->GetFieldData()->Initialize();
  }
}

//------------------------------------------------------------------------------
void vtkmProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PassPointArrays: " << this->PassPointArrays << "\n";
  os << indent << "PassCellArrays: " << this->PassCellArrays << "\n";
  os << indent << "PassFieldArray: " << this->PassFieldArrays << "\n";
}
