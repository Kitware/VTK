/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSetGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockDataSetGeometryFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiBlockDataSetGeometryFilter, "1.1");
vtkStandardNewMacro(vtkMultiBlockDataSetGeometryFilter);

vtkMultiBlockDataSetGeometryFilter::vtkMultiBlockDataSetGeometryFilter()
{
}

vtkMultiBlockDataSetGeometryFilter::~vtkMultiBlockDataSetGeometryFilter()
{
}

int vtkMultiBlockDataSetGeometryFilter::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  info->Remove(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME());
  return 1;
}

int vtkMultiBlockDataSetGeometryFilter::RequestCompositeData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector*  outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiBlockDataSet *input = vtkMultiBlockDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_SET()));
  if (!input) {return 0;}

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

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

vtkExecutive* vtkMultiBlockDataSetGeometryFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

vtkPolyData* vtkMultiBlockDataSetGeometryFilter::GetOutput()
{
  return this->GetOutput(0);
}

vtkPolyData* vtkMultiBlockDataSetGeometryFilter::GetOutput(int port)
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(port));
}

void vtkMultiBlockDataSetGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

