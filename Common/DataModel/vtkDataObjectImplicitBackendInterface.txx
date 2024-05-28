// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObjectImplicitBackendInterface.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <typename ValueType>
vtkDataObjectImplicitBackendInterface<ValueType>::vtkDataObjectImplicitBackendInterface(
  vtkDataObject* dataobject, const std::string& arrayName, int attributeType)
  : DataObject(dataobject)
  , ArrayName(arrayName)
  , AttributeType(attributeType)
{
  this->DataObject->AddObserver(
    vtkCommand::DeleteEvent, this, &vtkDataObjectImplicitBackendInterface::OnDataObjectDeleted);
}

//------------------------------------------------------------------------------
template <typename ValueType>
vtkDataObjectImplicitBackendInterface<ValueType>::~vtkDataObjectImplicitBackendInterface() =
  default;

//------------------------------------------------------------------------------
template <typename ValueType>
void vtkDataObjectImplicitBackendInterface<ValueType>::OnDataObjectDeleted(
  vtkObject* vtkNotUsed(caller), unsigned long eventId, void* vtkNotUsed(calldata))
{
  if (eventId != vtkCommand::DeleteEvent)
  {
    return;
  }

  this->Cache = vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>>::New();
  vtkDataArray* self = this->GetArray();
  this->Cache->DeepCopy(self);
}

//------------------------------------------------------------------------------
template <typename ValueType>
ValueType vtkDataObjectImplicitBackendInterface<ValueType>::operator()(vtkIdType index) const
{
  if (this->DataObject)
  {
    return this->GetValueFromDataObject(index);
  }

  return this->Cache->GetValue(index);
}

//------------------------------------------------------------------------------
template <typename ValueType>
vtkDataArray* vtkDataObjectImplicitBackendInterface<ValueType>::GetArray()
{
  return this->DataObject->GetAttributesAsFieldData(this->AttributeType)
    ->GetArray(this->ArrayName.c_str());
}

VTK_ABI_NAMESPACE_END
