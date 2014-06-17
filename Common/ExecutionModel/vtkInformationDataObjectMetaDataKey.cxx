/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDataObjectMetaDataKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationDataObjectMetaDataKey.h"

#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
vtkInformationDataObjectMetaDataKey::vtkInformationDataObjectMetaDataKey(const char* name, const char* location) :
  vtkInformationDataObjectKey(name, location)
{
}

//----------------------------------------------------------------------------
vtkInformationDataObjectMetaDataKey::~vtkInformationDataObjectMetaDataKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectMetaDataKey::CopyDefaultInformation(
  vtkInformation* request,
  vtkInformation* fromInfo,
  vtkInformation* toInfo)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->ShallowCopy(fromInfo, toInfo);
    }
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectMetaDataKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
