/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPOutlineFilter.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPOutlineFilterInternals.h"

vtkStandardNewMacro(vtkPOutlineFilter);
vtkCxxSetObjectMacro(vtkPOutlineFilter, Controller, vtkMultiProcessController);

vtkPOutlineFilter::vtkPOutlineFilter ()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->OutlineSource = vtkOutlineSource::New();
}

vtkPOutlineFilter::~vtkPOutlineFilter ()
{
  this->SetController(0);
  if (this->OutlineSource != NULL)
  {
    this->OutlineSource->Delete ();
    this->OutlineSource = NULL;
  }
}

int vtkPOutlineFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkPOutlineFilterInternals internals;
  internals.SetIsCornerSource(false);
  internals.SetController(this->Controller);

  return internals.RequestData(request,inputVector,outputVector);
}

int vtkPOutlineFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

void vtkPOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
}
