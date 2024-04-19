// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformationDataObjectKey.h"

// This file does not include vtkDataObject.h, because doing so introduces a
// dependency cycle between CommonCore and CommonDataModel that complicates
// both compilation and linking.  For example, some tools such as UBSan
// complain about undefined symbols for "typeinfo for vtkDataObject" when
// building the vtkCommonCore library.

// Since vtkDataObject.h is not included, static_cast<> cannot be used to cast
// between vtkObjectBase* and vtkDataObject*, and reinterpret_cast<> is used
// instead.  This is done on the assumption that, for single-inheritance,
// the address of an object is not changed by upcasting or downcasting.  The
// C++ standard does not guarantee this to be true, but every compiler that
// is supported by VTK implements single-inheritance polymorphism this way.

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkInformationDataObjectKey::vtkInformationDataObjectKey(const char* name, const char* location)
  : vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//------------------------------------------------------------------------------
vtkInformationDataObjectKey::~vtkInformationDataObjectKey() = default;

//------------------------------------------------------------------------------
void vtkInformationDataObjectKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkInformationDataObjectKey::Set(vtkInformation* info, vtkDataObject* value)
{
  // see comments at top of file regarding the reinterpret_cast
  this->SetAsObjectBase(info, reinterpret_cast<vtkObjectBase*>(value));
}

//------------------------------------------------------------------------------
vtkDataObject* vtkInformationDataObjectKey::Get(vtkInformation* info)
{
  // see comments at top of file regarding the reinterpret_cast
  return reinterpret_cast<vtkDataObject*>(this->GetAsObjectBase(info));
}

//------------------------------------------------------------------------------
void vtkInformationDataObjectKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
{
  this->Set(to, this->Get(from));
}

//------------------------------------------------------------------------------
void vtkInformationDataObjectKey::Report(vtkInformation* info, vtkGarbageCollector* collector)
{
  this->ReportAsObjectBase(info, collector);
}
VTK_ABI_NAMESPACE_END
