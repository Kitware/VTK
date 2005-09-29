/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataGeometryFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiGroupDataGeometryFilter, "1.1");
vtkStandardNewMacro(vtkMultiGroupDataGeometryFilter);

vtkMultiGroupDataGeometryFilter::vtkMultiGroupDataGeometryFilter()
{
}

vtkMultiGroupDataGeometryFilter::~vtkMultiGroupDataGeometryFilter()
{
}

int vtkMultiGroupDataGeometryFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkMultiGroupDataSet");
  return 1;
}

int vtkMultiGroupDataGeometryFilter::ProcessRequest(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkCompositeDataPipeline::REQUEST_DATA()))
    {
    int retVal = this->RequestCompositeData(request, inputVector, outputVector);
    return retVal;
    }

 return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkMultiGroupDataGeometryFilter::RequestCompositeData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector*  outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiGroupDataSet *input = vtkMultiGroupDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!input) 
    {
    vtkErrorMacro("No input composite dataset provided.");
    return 0;
    }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) 
    {
    vtkErrorMacro("No output polydata provided.");
    return 0;
    }

  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->GoToFirstItem();
  vtkAppendPolyData* append = vtkAppendPolyData::New();
  while (!iter->IsDoneWithTraversal())
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      vtkGeometryFilter* geom = vtkGeometryFilter::New();
      geom->SetInput(ds);
      geom->Update();
      append->AddInput(geom->GetOutput());
      geom->Delete();
      }
    iter->GoToNextItem();
    }
  iter->Delete();
  append->Update();

  output->ShallowCopy(append->GetOutput());

  append->Delete();

  return 1;
}

vtkExecutive* vtkMultiGroupDataGeometryFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

void vtkMultiGroupDataGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

