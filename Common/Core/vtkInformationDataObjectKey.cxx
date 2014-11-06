/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDataObjectKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationDataObjectKey.h"

#if defined(vtkCommonDataModel_ENABLED)
# include "../DataModel/vtkDataObject.h"
#endif


//----------------------------------------------------------------------------
vtkInformationDataObjectKey::vtkInformationDataObjectKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationDataObjectKey::~vtkInformationDataObjectKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectKey::Set(vtkInformation* info,
                                      vtkDataObject* value)
{
#if defined(vtkCommonDataModel_ENABLED)
  this->SetAsObjectBase(info, value);
#endif
}

//----------------------------------------------------------------------------
vtkDataObject* vtkInformationDataObjectKey::Get(vtkInformation* info)
{
#if defined(vtkCommonDataModel_ENABLED)
  return static_cast<vtkDataObject *>(this->GetAsObjectBase(info));
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectKey::ShallowCopy(vtkInformation* from,
                                              vtkInformation* to)
{
  this->Set(to, this->Get(from));
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectKey::Report(vtkInformation* info,
                                         vtkGarbageCollector* collector)
{
  this->ReportAsObjectBase(info, collector);
}
