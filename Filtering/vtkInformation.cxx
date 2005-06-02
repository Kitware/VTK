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
#include "vtkInformationInformationKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerPointerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKeyVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>

vtkCxxRevisionMacro(vtkInformation, "1.16");
vtkStandardNewMacro(vtkInformation);

//----------------------------------------------------------------------------
class vtkInformationInternals
{
public:
  typedef vtkstd::map<vtkInformationKey*,
                      vtkSmartPointer<vtkObjectBase> > MapType;
  MapType Map;
};

//----------------------------------------------------------------------------
vtkInformation::vtkInformation()
{
  this->Internal = new vtkInformationInternals;
}

//----------------------------------------------------------------------------
vtkInformation::~vtkInformation()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkInformationInternals::MapType::const_iterator i;
  for(i=this->Internal->Map.begin(); i != this->Internal->Map.end(); ++i)
    {
    vtkInformationKey* key = i->first;
    os << indent << key->GetName() << ": ";
    key->Print(os, this);
    os << "\n";
    }
}

//----------------------------------------------------------------------------
void vtkInformation::SetAsObjectBase(vtkInformationKey* key,
                                     vtkObjectBase* value)
{
  if(key)
    {
    vtkInformationInternals::MapType::iterator i =
      this->Internal->Map.find(key);
    if(i != this->Internal->Map.end())
      {
      if(value)
        {
        i->second = value;
        }
      else
        {
        this->Internal->Map.erase(i);
        }
      }
    else if(value)
      {
      this->Internal->Map[key] = value;
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkInformation::GetAsObjectBase(vtkInformationKey* key)
{
  if(key)
    {
    vtkInformationInternals::MapType::const_iterator i =
      this->Internal->Map.find(key);
    if(i != this->Internal->Map.end())
      {
      return i->second.GetPointer();
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
    vtkInformationInternals::MapType::const_iterator i;
    for(i=from->Internal->Map.begin(); i != from->Internal->Map.end(); ++i)
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
  vtkInformationInternals::MapType::const_iterator i;
  for(i=this->Internal->Map.begin(); i != this->Internal->Map.end(); ++i)
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
    vtkInformationInternals::MapType::iterator i =
      this->Internal->Map.find(key);
    if(i != this->Internal->Map.end())
      {
      vtkGarbageCollectorReport(collector, i->second, key->GetName());
      }
    }
}
