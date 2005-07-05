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

#include "vtkGarbageCollector.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationInformationKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerPointerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKeyVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/algorithm>
#include <vtkstd/utility>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkInformation, "1.21");
vtkStandardNewMacro(vtkInformation);

//----------------------------------------------------------------------------
class vtkInformationInternals
{
public:
  // Vector to store ordered key/value pairs for efficient lookup with
  // a binary search.  Typically not many pairs are stored so linear
  // insertion time is okay.
  typedef vtkstd::pair<vtkInformationKey*, vtkObjectBase*> value_type;
  typedef vtkstd::vector<value_type> VectorType;
  VectorType Vector;

  // Comparison functor to order the values by key.
  struct CompareType
  {
    vtkstd_bool operator()(value_type const& l, value_type const& r) const
      {
      return l.first < r.first;
      }
  };
  CompareType Compare;

  // Methods to find the place for a value with a binary search.
  VectorType::iterator Find(value_type const& v)
    {
    return vtkstd::lower_bound(this->Vector.begin(), this->Vector.end(),
                               v, this->Compare);
    }
  VectorType::iterator Find(vtkInformationKey* key)
    {
    return this->Find(value_type(key, 0));
    }

  ~vtkInformationInternals()
    {
    // Delete all the values from the vector.
    for(VectorType::iterator i = this->Vector.begin();
        i != this->Vector.end(); ++i)
      {
      if(vtkObjectBase* value = i->second)
        {
        i->second = 0;
        value->UnRegister(0);
        }
      }
    }
};

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

  // Give each key a chance to print its value.
  for(vtkInformationInternals::VectorType::iterator i =
        this->Internal->Vector.begin();
      i != this->Internal->Vector.end(); ++i)
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
void vtkInformation::SetAsObjectBase(vtkInformationKey* key,
                                     vtkObjectBase* newvalue)
{
  if(key)
    {
    // Check for an existing entry.
    vtkInformationInternals::VectorType::value_type v(key, newvalue);
    vtkInformationInternals::VectorType::iterator i = this->Internal->Find(v);
    if(i != this->Internal->Vector.end() && i->first == key)
      {
      // There is already an entry with this key.  Update the value.
      vtkObjectBase* oldvalue = i->second;
      if(newvalue)
        {
        // There is a new value.  Replace the entry.
        i->second = newvalue;
        newvalue->Register(0);
        }
      else
        {
        // There is no new value.  Erase the entry.
        this->Internal->Vector.erase(i);
        }
      oldvalue->UnRegister(0);
      }
    else if(newvalue)
      {
      // There is no entry with this key.  Create one and store the
      // value.
      newvalue->Register(0);
      this->Internal->Vector.insert(i, v);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkInformation::GetAsObjectBase(vtkInformationKey* key)
{
  if(key)
    {
    // Look for an entry with this key.
    vtkInformationInternals::VectorType::iterator i =
      this->Internal->Find(key);
    if(i != this->Internal->Vector.end() && i->first == key)
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
    for(vtkInformationInternals::VectorType::iterator i =
          from->Internal->Vector.begin();
        i != from->Internal->Vector.end(); ++i)
      {
      this->CopyEntry(from, i->first, deep);
      }
    }
  delete oldInternal;
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationDataObjectKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationExecutivePortKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationInformationKey* key, int deep)
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
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationInformationVectorKey* key, int deep)
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
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationIntegerKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationRequestKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationIntegerVectorKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationDoubleVectorKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationStringKey* key, int)
{
  key->ShallowCopy(from, this);
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, vtkInformationUnsignedLongKey* key, int)
{
  key->ShallowCopy(from, this);
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

void vtkInformation::Set(vtkInformationRequestKey* key)
{
  key->Set(this);
}
void vtkInformation::Remove(vtkInformationRequestKey* key)
{
  key->Remove(this);
}
int vtkInformation::Has(vtkInformationRequestKey* key)
{
  return key->Has(this);
}

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(name, type)                  \
  void vtkInformation::Set(vtkInformation##name##Key* key, type value)      \
    {                                                                       \
    key->Set(this, value);                                                  \
    }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##Key* key)               \
    {                                                                       \
    key->Remove(this);                                                      \
    }                                                                       \
  type vtkInformation::Get(vtkInformation##name##Key* key)                  \
    {                                                                       \
    return key->Get(this);                                                  \
    }                                                                       \
  int vtkInformation::Has(vtkInformation##name##Key* key)                   \
    {                                                                       \
    return key->Has(this);                                                  \
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
#undef VTK_INFORMATION_DEFINE_SCALAR_PROPERTY

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(name, type)                  \
  void vtkInformation::Append(vtkInformation##name##VectorKey* key,         \
                              type value)                                   \
    {                                                                       \
    key->Append(this, value);                                               \
    }                                                                       \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           type* value, int length)                         \
    {                                                                       \
    key->Set(this, value, length);                                          \
    }                                                                       \
  type* vtkInformation::Get(vtkInformation##name##VectorKey* key)           \
    {                                                                       \
    return key->Get(this);                                                  \
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
    key->Remove(this);                                                      \
    }                                                                       \
  int vtkInformation::Has(vtkInformation##name##VectorKey* key)             \
    {                                                                       \
    return key->Has(this);                                                  \
    }
VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Double, double);
VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Key, vtkInformationKey*);
#define VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(name, type)            \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           type value1, type value2, type value3,           \
                           type value4, type value5, type value6)           \
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
                           type value1, type value2, type value3)           \
    {                                                                       \
    type value[3];                                                          \
    value[0] = value1;                                                      \
    value[1] = value2;                                                      \
    value[2] = value3;                                                      \
    key->Set(this, value, 3);                                               \
    }
VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(Double, double);
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
    key->Remove(this);                                                      \
    }                                                                       \
  int vtkInformation::Has(vtkInformation##name##PointerKey* key)             \
    {                                                                       \
    return key->Has(this);                                                  \
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
                            vtkInformationExecutivePortKey* value)
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
void vtkInformation::Set(vtkInformationExecutivePortKey* key,
                         vtkExecutive* executive, int port)
{
  key->Set(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortKey* key)
{
  key->Remove(this);
}

//----------------------------------------------------------------------------
vtkExecutive* vtkInformation::GetExecutive(vtkInformationExecutivePortKey* key)
{
  return key->GetExecutive(this);
}

//----------------------------------------------------------------------------
int vtkInformation::GetPort(vtkInformationExecutivePortKey* key)
{
  return key->GetPort(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Get(vtkInformationExecutivePortKey* key,
                         vtkExecutive*& executive, int &port)
{
  key->Get(this,executive,port);
}

//----------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationExecutivePortKey* key)
{
  return key->Has(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationExecutivePortVectorKey* key,
                            vtkExecutive* executive, int port)
{
  key->Append(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortVectorKey* key,
                            vtkExecutive* executive, int port)
{
  key->Remove(this, executive, port);
}

//----------------------------------------------------------------------------
void vtkInformation::Set(vtkInformationExecutivePortVectorKey* key,
                         vtkExecutive** executives, int* ports, int length)
{
  key->Set(this, executives, ports, length);
}

//----------------------------------------------------------------------------
vtkExecutive**
vtkInformation::GetExecutives(vtkInformationExecutivePortVectorKey* key)
{
  return key->GetExecutives(this);
}

//----------------------------------------------------------------------------
int*
vtkInformation::GetPorts(vtkInformationExecutivePortVectorKey* key)
{
  return key->GetPorts(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Get(vtkInformationExecutivePortVectorKey* key,
                         vtkExecutive** executives, int* ports)
{
  key->Get(this, executives, ports);
}

//----------------------------------------------------------------------------
int vtkInformation::Length(vtkInformationExecutivePortVectorKey* key)
{
  return key->Length(this);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationExecutivePortVectorKey* key)
{
  key->Remove(this);
}

//----------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationExecutivePortVectorKey* key)
{
  return key->Has(this);
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
vtkInformationKey* vtkInformation::GetKey(vtkInformationExecutivePortKey* key)
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
vtkInformationKey* vtkInformation::GetKey(vtkInformationUnsignedLongKey* key)
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
  for(vtkInformationInternals::VectorType::iterator i =
        this->Internal->Vector.begin();
      i != this->Internal->Vector.end(); ++i)
    {
    // TODO: Pass a reference to i->second as an argument to avoid
    // need for another lookup.
    i->first->Report(this, collector);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::ReportAsObjectBase(vtkInformationKey* key,
                                        vtkGarbageCollector* collector)
{
  if(key)
    {
    // Find the entry's value and report it.
    vtkInformationInternals::VectorType::iterator i =
      this->Internal->Find(key);
    if(i != this->Internal->Vector.end() && i->first == key)
      {
      vtkGarbageCollectorReport(collector, i->second, key->GetName());
      }
    }
}
