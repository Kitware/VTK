// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformationDataObjectMetaDataKey.h"

#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkInformationDataObjectMetaDataKey::vtkInformationDataObjectMetaDataKey(
  const char* name, const char* location)
  : vtkInformationDataObjectKey(name, location)
{
}

//------------------------------------------------------------------------------
vtkInformationDataObjectMetaDataKey::~vtkInformationDataObjectMetaDataKey() = default;

//------------------------------------------------------------------------------
void vtkInformationDataObjectMetaDataKey::CopyDefaultInformation(
  vtkInformation* request, vtkInformation* fromInfo, vtkInformation* toInfo)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->ShallowCopy(fromInfo, toInfo);
  }
}

//------------------------------------------------------------------------------
void vtkInformationDataObjectMetaDataKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
