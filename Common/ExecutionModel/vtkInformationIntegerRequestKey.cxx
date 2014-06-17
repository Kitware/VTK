/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerRequestKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationIntegerRequestKey.h"

#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
vtkInformationIntegerRequestKey::vtkInformationIntegerRequestKey(const char* name, const char* location) :
  vtkInformationIntegerKey(name, location)
{
  this->DataKey = 0;
}

//----------------------------------------------------------------------------
vtkInformationIntegerRequestKey::~vtkInformationIntegerRequestKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationIntegerRequestKey::CopyDefaultInformation(
  vtkInformation* request,
  vtkInformation* fromInfo,
  vtkInformation* toInfo)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    this->ShallowCopy(fromInfo, toInfo);
    }
}

//----------------------------------------------------------------------------
bool vtkInformationIntegerRequestKey::NeedToExecute(vtkInformation* pipelineInfo,
                                                    vtkInformation* dobjInfo)
{
  if (!dobjInfo->Has(this->DataKey) ||
    dobjInfo->Get(this->DataKey) != pipelineInfo->Get(this))
    {
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkInformationIntegerRequestKey::StoreMetaData(vtkInformation*,
                                                    vtkInformation* pipelineInfo,
                                                    vtkInformation* dobjInfo)
{
  dobjInfo->Set(this->DataKey, pipelineInfo->Get(this));
}

//----------------------------------------------------------------------------
void vtkInformationIntegerRequestKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
