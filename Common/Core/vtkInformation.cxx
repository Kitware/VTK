/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformation.h"

#include "vtkCommand.h"
#include "vtkGarbageCollector.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationInformationKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerPointerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKeyVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationObjectBaseVectorKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkInformationVariantKey.h"
#include "vtkInformationVariantVectorKey.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "vtkInformationInternals.h"

vtkStandardNewMacro(vtkInformation);

//----------------------------------------------------------------------------
vtkInformation::vtkInformation()
{
  // Allocate the internal representation.
  this->Internal = new vtkInformationInternals;

  // There is no request key stored initially.
  this->Request = 0;
}

//----------------------------------------------------------------------------
vtkInformation::~vtkInformation()
{
  // Delete the internal representation.
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // Print the request if one is set.
  if(this->Request)
  {
    os << indent << "Request: " << this->Request->GetName() << "\n";
  }
  this->PrintKeys(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformation::PrintKeys(ostream& os, vtkIndent indent)
{
  typedef vtkInformationInternals::MapType MapType;
  for(MapType::const_iterator i = this->Internal->Map.begin();
      i != this->Internal->Map.end(); ++i)
  {
    // Print the key name first.
    vtkInformationKey* key = i->first;
    os << indent << key->GetName() << ": ";

    // Ask the key to print its value.
    key->Print(os, this);
    os << "\n";
  }
}


//----------------------------------------------------------------------------
// call modified on superclass
void vtkInformation::Modified()
{
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
// Update MTime and invoke a modified event with
// the information key as call data
void vtkInformation::Modified(vtkInformationKey* key)
{
  this->MTime.Modified();
  this->InvokeEvent(vtkCommand::ModifiedEvent, key);
}

//----------------------------------------------------------------------------
// Return the number of keys as a result of iteration.
int vtkInformation::GetNumberOfKeys()
{
  vtkSmartPointer<vtkInformationIterator> infoIterator =
    vtkSmartPointer<vtkInformationIterator>::New();
  infoIterator->SetInformation( this );

  int numberOfKeys = 0;
  for (infoIterator->InitTraversal(); !infoIterator->IsDoneWithTraversal();
    infoIterator->GoToNextItem())
  {
    numberOfKeys++;
  }
  return numberOfKeys;
}

//----------------------------------------------------------------------------
void vtkInformation::SetAsObjectBase(vtkInformationKey* key,
                                     vtkObjectBase* newvalue)
{
  if(!key)
  {
    return;
  }
  typedef vtkInformationInternals::MapType MapType;
  MapType::iterator i = this->Internal->Map.find(key);
  if(i != this->Internal->Map.end())
  {
    vtkObjectBase* oldvalue = i->second;
    if(newvalue)
    {
      i->second = newvalue;
      newvalue->Register(0);
    }
    else
    {
      this->Internal->Map.erase(i);
    }
    oldvalue->UnRegister(0);
  }
  else if(newvalue)
  {
    MapType::value_type entry(key, newvalue);
    this->Internal->Map.insert(entry);
    newvalue->Register(0);
  }
  this->Modified(key);
}

//----------------------------------------------------------------------------
const vtkObjectBase* vtkInformation::GetAsObjectBase(
  const vtkInformationKey* key) const
{
  if(key)
  {
    typedef vtkInformationInternals::MapType MapType;
    MapType::const_iterator i = this->Internal->Map.find(const_cast<vtkInformationKey*>(key));
    if(i != this->Internal->Map.end())
    {
      return i->second;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkInformation::GetAsObjectBase(vtkInformationKey* key)
{
  if(key)
  {
    typedef vtkInformationInternals::MapType MapType;
    MapType::const_iterator i = this->Internal->Map.find(key);
    if(i != this->Internal->Map.end())
    {
      return i->second;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkInformation::Clear()
{
  this->Copy(0);
}

//----------------------------------------------------------------------------
void vtkInformation::Copy(vtkInformation* from, int deep)
{
  vtkInformationInternals* oldInternal = this->Internal;
  this->Internal = new vtkInformationInternals;
  if(from)
  {
    typedef vtkInformationInternals::MapType MapType;
    for(MapType::const_iterator i = from->Internal->Map.begin();
        i != from->Internal->Map.end(); ++i)
    {
      this->CopyEntry(from, i->first, deep);
    }
  }
  delete oldInternal;
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformation* from, int deep)
{
  if(from)
  {
    typedef vtkInformationInternals::MapType MapType;
    for(MapType::const_iterator i = from->Internal->Map.begin();
        i != from->Internal->Map.end(); ++i)
    {
      this->CopyEntry(from, i->first, deep);
    }
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationDataObjectKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationInformationKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationInformationVectorKey* key,
                               int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationIntegerKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationRequestKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationIntegerVectorKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation *from,
                               vtkInformationObjectBaseVectorKey *key,
                               int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationDoubleVectorKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationVariantKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationVariantVectorKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationStringKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationUnsignedLongKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from,
                               vtkInformationStringVectorKey* key, int deep)
{
  if (!deep)
  {
    key->ShallowCopy(from, this);
  }
  else
  {
    key->DeepCopy(from, this);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntries(vtkInformation* from,
                                 vtkInformationKeyVectorKey* key, int deep)
{
  int numberOfKeys = from->Length(key);
  vtkInformationKey** keys = from->Get(key);
  for(int i=0; i < numberOfKeys; ++i)
  {
    this->CopyEntry(from, keys[i], deep);
  }
}

//----------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationKey* key)
{
  // Use the virtual interface in case this is a special-cased key.
  return key->Has(this)?1:0;
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationKey* key)
{
  // Use the virtual interface in case this is a special-cased key.
  key->Remove(this);
}

void vtkInformation::Set(vtkInformationRequestKey* key)
{
  key->Set(this);
}
void vtkInformation::Remove(vtkInformationRequestKey* key)
{
  key->vtkInformationRequestKey::Remove(this);
}
int vtkInformation::Has(vtkInformationRequestKey* key)
{
  return key->vtkInformationRequestKey::Has(this);
}

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(name, type)                  \
  void vtkInformation::Set(vtkInformation##name##Key* key, type value)      \
  {                                                                       \
    key->Set(this, value);                                                  \
  }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##Key* key)               \
  {                                                                       \
    key->vtkInformation##name##Key::Remove(this);                           \
  }                                                                       \
  type vtkInformation::Get(vtkInformation##name##Key* key)                  \
  {                                                                       \
    return key->Get(this);                                                  \
  }                                                                       \
  int vtkInformation::Has(vtkInformation##name##Key* key)                   \
  {                                                                       \
    return key->vtkInformation##name##Key::Has(this);                       \
  }
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(IdType, vtkIdType);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Double, double);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(UnsignedLong, unsigned long);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(String, const char*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(DataObject, vtkDataObject*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Information, vtkInformation*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(InformationVector, vtkInformationVector*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(ObjectBase, vtkObjectBase*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Variant, const vtkVariant&);
#undef VTK_INFORMATION_DEFINE_SCALAR_PROPERTY

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(name, type)                  \
  void vtkInformation::Append(vtkInformation##name##VectorKey* key,         \
                              type value)                                   \
  {                                                                       \
    key->Append(this, value);                                               \
  }                                                                       \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           type const* value, int length)                         \
  {                                                                       \
    key->Set(this, value, length);                                          \
  }                                                                       \
  type* vtkInformation::Get(vtkInformation##name##VectorKey* key)           \
  {                                                                       \
    return key->Get(this);                                                  \
  }                                                                       \
  type vtkInformation::Get(vtkInformation##name##VectorKey* key, int idx)   \
  {                                                                       \
    return key->Get(this, idx);                                             \
  }                                                                       \
  void vtkInformation::Get(vtkInformation##name##VectorKey* key,            \
                           type* value)                                     \
  {                                                                       \
    key->Get(this, value);                                                  \
  }                                                                       \
  int vtkInformation::Length(vtkInformation##name##VectorKey* key)          \
  {                                                                       \
    return key->Length(this);                                               \
  }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##VectorKey* key)         \
  {                                                                       \
    key->vtkInformation##name##VectorKey::Remove(this);                     \
  }                                                                       \
  int vtkInformation::Has(vtkInformation##name##VectorKey* key)             \
  {                                                                       \
    return key->vtkInformation##name##VectorKey::Has(this);                 \
  }
VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Double, double);

// String keys can accept std::string.
void vtkInformation::Append(vtkInformationStringVectorKey *key,
                            const std::string &value)
{
  this->Append(key, value.c_str());
}
void vtkInformation::Set(vtkInformationStringVectorKey* key,
                         const std::string &value, int idx)
{
  this->Set(key, value.c_str(), idx);
}
void vtkInformation::Set(vtkInformationStringKey* key,
                         const std::string &value)
{
  this->Set(key, value.c_str());
}

// Variant vector key is slightly different to accommodate efficient
// pass-by-reference instead of pass-by-value calls.
void vtkInformation::Append(vtkInformationVariantVectorKey* key,
  const vtkVariant& value)
{
  key->Append(this, value);
}
void vtkInformation::Set(vtkInformationVariantVectorKey* key,
  const vtkVariant* value, int length)
{
  key->Set(this, value, length);
}
const vtkVariant* vtkInformation::Get(vtkInformationVariantVectorKey* key)
{
  return key->Get(this);
}
const vtkVariant& vtkInformation::Get(vtkInformationVariantVectorKey* key, int idx)
{
  return key->Get(this, idx);
}
void vtkInformation::Get(vtkInformationVariantVectorKey* key,
  vtkVariant* value)
{
  key->Get(this, value);
}
int vtkInformation::Length(vtkInformationVariantVectorKey* key)
{
  return key->Length(this);
}
void vtkInformation::Remove(vtkInformationVariantVectorKey* key)
{
  key->vtkInformationVariantVectorKey::Remove(this);
}
int vtkInformation::Has(vtkInformationVariantVectorKey* key)
{
  return key->vtkInformationVariantVectorKey::Has(this);
}

// String vector key is slightly different to make it backwards compatible with
// the scalar string key.
void vtkInformation::Append(vtkInformationStringVectorKey* key,
                            const char* value)
{
  key->Append(this, value);
}
void vtkInformation::Set(vtkInformationStringVectorKey* key,
    const char* value, int idx)
{
  key->Set(this, value, idx);
}
const char* vtkInformation::Get(vtkInformationStringVectorKey* key, int idx)
{
  return key->Get(this, idx);
}
int vtkInformation::Length(vtkInformationStringVectorKey* key)
{
  return key->Length(this);
}
void vtkInformation::Remove(vtkInformationStringVectorKey* key)
{
  key->vtkInformationStringVectorKey::Remove(this);
}
int vtkInformation::Has(vtkInformationStringVectorKey* key)
{
  return key->vtkInformationStringVectorKey::Has(this);
}


//------------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationObjectBaseVectorKey *key,
                            vtkObjectBase *data)
{
  key->Append(this, data);
}

//------------------------------------------------------------------------------
void vtkInformation::Set(vtkInformationObjectBaseVectorKey *key,
                         vtkObjectBase *value, int idx)
{
  key->Set(this, value, idx);
}

//------------------------------------------------------------------------------
vtkObjectBase *vtkInformation::Get(vtkInformationObjectBaseVectorKey *key,
                                   int idx)
{
  return key->Get(this, idx);
}

//------------------------------------------------------------------------------
int vtkInformation::Length(vtkInformationObjectBaseVectorKey *key)
{
  return key->Length(this);
}

//------------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationObjectBaseVectorKey *key)
{
  key->Remove(this);
}

//------------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationObjectBaseVectorKey *key,
                            vtkObjectBase *objectToRemove)
{
  key->Remove(this, objectToRemove);
}

//------------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationObjectBaseVectorKey *key,
                            int indexToRemove)
{
  key->Remove(this, indexToRemove);
}

//------------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationObjectBaseVectorKey *key)
{
  return key->Has(this);
}

VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Key, vtkInformationKey*);
#define VTK_INFORMATION_DEFINE_VECTOR_VALUE2_PROPERTY(name, type, atype)    \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           atype value1, atype value2, atype value3,        \
                           atype value4, atype value5, atype value6)        \
  {                                                                       \
    type value[6];                                                          \
    value[0] = value1;                                                      \
    value[1] = value2;                                                      \
    value[2] = value3;                                                      \
    value[3] = value4;                                                      \
    value[4] = value5;                                                      \
    value[5] = value6;                                                      \
    key->Set(this, value, 6);                                               \
  }                                                                       \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           atype value1, atype value2, atype value3)        \
  {                                                                       \
    type value[3];                                                          \
    value[0] = value1;                                                      \
    value[1] = value2;                                                      \
    value[2] = value3;                                                      \
    key->Set(this, value, 3);                                               \
  }
#define VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(name, type)            \
        VTK_INFORMATION_DEFINE_VECTOR_VALUE2_PROPERTY(name, type, type)
VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(Double, double);
VTK_INFORMATION_DEFINE_VECTOR_VALUE2_PROPERTY(Variant, vtkVariant, const vtkVariant&);
#undef VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY

#undef VTK_INFORMATION_DEFINE_VECTOR_PROPERTY

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_POINTER_PROPERTY(name, type)                  \
  void vtkInformation::Set(vtkInformation##name##PointerKey* key,            \
                           type* value, int length)                         \
  {                                                                       \
    key->Set(this, value, length);                                          \
  }                                                                       \
  type* vtkInformation::Get(vtkInformation##name##PointerKey* key)           \
  {                                                                       \
    return key->Get(this);                                                  \
  }                                                                       \
  void vtkInformation::Get(vtkInformation##name##PointerKey* key,            \
                           type* value)                                     \
  {                                                                       \
    key->Get(this, value);                                                  \
  }                                                                       \
  int vtkInformation::Length(vtkInformation##name##PointerKey* key)          \
  {                                                                       \
    return key->Length(this);                                               \
  }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##PointerKey* key)         \
  {                                                                       \
    key->vtkInformation##name##PointerKey::Remove(this);                    \
  }                                                                       \
  int vtkInformation::Has(vtkInformation##name##PointerKey* key)             \
  {                                                                       \
    return key->vtkInformation##name##PointerKey::Has(this);                \
  }
VTK_INFORMATION_DEFINE_POINTER_PROPERTY(Integer, int);
#undef VTK_INFORMATION_DEFINE_POINTER_PROPERTY

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationDataObjectKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationDoubleKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationDoubleVectorKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationInformationKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationKeyVectorKey* key,
                            vtkInformationKey* value)
{
  key->RemoveItem(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationInformationVectorKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationIntegerKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationIntegerVectorKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationStringKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationUnsignedLongKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationObjectBaseKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationStringVectorKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationDataObjectKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationDoubleKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationDoubleVectorKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationInformationKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationInformationVectorKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationIntegerKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationIntegerVectorKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationStringKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationUnsignedLongKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationObjectBaseKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationStringVectorKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationDataObjectKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationInformationKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationInformationVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationIntegerKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationRequestKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationDoubleKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationIntegerVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationDoubleVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationStringKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationStringVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationUnsignedLongKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationVariantKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationVariantVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
void vtkInformation::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkInformation::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkInformation::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // Ask each key/value pair to report any references it holds.
  typedef vtkInformationInternals::MapType MapType;
  for(MapType::const_iterator i = this->Internal->Map.begin();
      i != this->Internal->Map.end(); ++i)
  {
    i->first->Report(this, collector);
  }
}

//----------------------------------------------------------------------------
void vtkInformation::ReportAsObjectBase(vtkInformationKey* key,
                                        vtkGarbageCollector* collector)
{
  if(key)
  {
    typedef vtkInformationInternals::MapType MapType;
    MapType::iterator i = this->Internal->Map.find(key);
    if(i != this->Internal->Map.end())
    {
      vtkGarbageCollectorReport(collector, i->second, key->GetName());
    }
  }
}

//----------------------------------------------------------------------------
void vtkInformation::SetRequest(vtkInformationRequestKey* request)
{
  this->Request = request;
}

//----------------------------------------------------------------------------
vtkInformationRequestKey* vtkInformation::GetRequest()
{
  return this->Request;
}
