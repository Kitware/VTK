/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataGeometryFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkCompositeDataGeometryFilter);

//-----------------------------------------------------------------------------
vtkCompositeDataGeometryFilter::vtkCompositeDataGeometryFilter()
{
}

//-----------------------------------------------------------------------------
vtkCompositeDataGeometryFilter::~vtkCompositeDataGeometryFilter()
{
}

//-----------------------------------------------------------------------------
int vtkCompositeDataGeometryFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCompositeDataGeometryFilter::ProcessRequest(
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

//-----------------------------------------------------------------------------
int vtkCompositeDataGeometryFilter::RequestCompositeData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector*  outputVector)
{
  vtkCompositeDataSet* input = vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (!input)
  {
    vtkErrorMacro("No input composite dataset provided.");
    return 0;
  }

  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);
  if (!output)
  {
    vtkErrorMacro("No output polydata provided.");
    return 0;
  }

  vtkNew<vtkAppendPolyData> append;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());

  for(iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds && ds->GetNumberOfPoints() > 0)
    {
      vtkNew<vtkDataSetSurfaceFilter> dssf;
      dssf->SetInputData(ds);
      dssf->Update();
      append->AddInputDataObject(dssf->GetOutputDataObject(0));
    }
  }
  if (append->GetNumberOfInputConnections(0) > 0)
  {
    append->Update();
    output->ShallowCopy(append->GetOutput());
  }

  return 1;
}

//-----------------------------------------------------------------------------
vtkExecutive* vtkCompositeDataGeometryFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
void vtkCompositeDataGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

