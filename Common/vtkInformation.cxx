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

#include "vtkDataObject.h"
#include "vtkDebugLeaks.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationDataObjectVectorKey.h"
#include "vtkInformationInformationKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKeyVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkInformation, "1.6");
vtkStandardNewMacro(vtkInformation);

#ifdef VTK_DEBUG_LEAKS
static void vtkInformationConstructClass(const char* name)
{
  vtkDebugLeaks::ConstructClass(name);
}
#else
static void vtkInformationConstructClass(const char*)
{
}
#endif

class vtkInformationInternals
{
public:
  typedef vtkstd::map<vtkInformationKey*, vtkSmartPointer<vtkObjectBase> > MapType;
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
    os << indent << i->first << ": " << i->second.GetPointer() << "\n";
    }
}

//----------------------------------------------------------------------------
void vtkInformation::SetAsObjectBase(vtkInformationKey* key,
                                     vtkObjectBase* value)
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
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkInformation::GetAsObjectBase(vtkInformationKey* key)
{
  vtkInformationInternals::MapType::const_iterator i =
    this->Internal->Map.find(key);
  if(i != this->Internal->Map.end())
    {
    return i->second.GetPointer();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkInformation::Clear()
{
  this->Copy(0);
}

//----------------------------------------------------------------------------
void vtkInformation::Copy(vtkInformation* from)
{
  if(from)
    {
    this->Internal->Map = from->Internal->Map;
    }
  else
    {
    this->Internal->Map.clear();
    }
}

//----------------------------------------------------------------------------
class vtkInformationStringValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationStringValue, vtkObjectBase);
  vtkstd::string Value;
};

//----------------------------------------------------------------------------
void vtkInformation::Set(vtkInformationStringKey* key, const char* value)
{
  if(value)
    {
    vtkInformationStringValue* v = new vtkInformationStringValue;
    vtkInformationConstructClass("vtkInformationStringValue");
    v->Value = value;
    this->SetAsObjectBase(key, v);
    v->Delete();
    }
  else
    {
    this->SetAsObjectBase(key, 0);
    }
}

//----------------------------------------------------------------------------
const char* vtkInformation::Get(vtkInformationStringKey* key)
{
  vtkInformationStringValue* v =
    vtkInformationStringValue::SafeDownCast(this->GetAsObjectBase(key));
  return v?v->Value.c_str():0;
}

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_OBJECT_PROPERTY(type)                        \
  void vtkInformation::Set(vtkInformation##type##Key* key,                  \
                           vtk##type* value)                                \
    {                                                                       \
    this->SetAsObjectBase(key, value);                                      \
    }                                                                       \
  vtk##type* vtkInformation::Get(vtkInformation##type##Key* key)            \
    {                                                                       \
    return vtk##type::SafeDownCast(this->GetAsObjectBase(key));             \
    }

VTK_INFORMATION_DEFINE_OBJECT_PROPERTY(DataObject);
VTK_INFORMATION_DEFINE_OBJECT_PROPERTY(Information);
VTK_INFORMATION_DEFINE_OBJECT_PROPERTY(InformationVector);

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(name, type)                  \
  class vtkInformation##name##Value: public vtkObjectBase                   \
  {                                                                         \
  public:                                                                   \
    vtkTypeMacro(vtkInformation##name##Value, vtkObjectBase);               \
    type Value;                                                             \
  };                                                                        \
  void vtkInformation::Set(vtkInformation##name##Key* key, type value)      \
    {                                                                       \
    vtkInformation##name##Value* v = new vtkInformation##name##Value;       \
    vtkInformationConstructClass("vtkInformation" #name "Value");           \
    v->Value = value;                                                       \
    this->SetAsObjectBase(key, v);                                          \
    v->Delete();                                                            \
    }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##Key* key)               \
    {                                                                       \
    this->SetAsObjectBase(key, 0);                                          \
    }                                                                       \
  type vtkInformation::Get(vtkInformation##name##Key* key)                  \
    {                                                                       \
    vtkInformation##name##Value* v =                                        \
      vtkInformation##name##Value::SafeDownCast(                            \
        this->GetAsObjectBase(key));                                        \
    return v?v->Value:0;                                                    \
    }                                                                       \
  int vtkInformation::Has(vtkInformation##name##Key* key)                   \
    {                                                                       \
    vtkInformation##name##Value* v =                                        \
      vtkInformation##name##Value::SafeDownCast(                            \
        this->GetAsObjectBase(key));                                        \
    return v?1:0;                                                           \
    }

VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Integer, int);
//VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Float, float);

#define VTK_INFORMATION_DEFINE_KEY_METHOD(NAME, type)                       \
  vtkInformation##type##Key* vtkInformation::NAME()                         \
    {                                                                       \
    static vtkInformation##type##Key instance;                              \
    return &instance;                                                       \
    }

VTK_INFORMATION_DEFINE_KEY_METHOD(INPUT_REQUIRED_DATA_TYPE, String);
VTK_INFORMATION_DEFINE_KEY_METHOD(INPUT_IS_OPTIONAL, Integer);
VTK_INFORMATION_DEFINE_KEY_METHOD(INPUT_IS_REPEATABLE, Integer);
VTK_INFORMATION_DEFINE_KEY_METHOD(INPUT_REQUIRED_FIELDS, InformationVector);
VTK_INFORMATION_DEFINE_KEY_METHOD(DATA_TYPE, Integer);
VTK_INFORMATION_DEFINE_KEY_METHOD(OUTPUT_PROVIDED_FIELDS, InformationVector);
VTK_INFORMATION_DEFINE_KEY_METHOD(DATA_OBJECT, DataObject);
VTK_INFORMATION_DEFINE_KEY_METHOD(DATA_OBJECTS, DataObjectVector);
VTK_INFORMATION_DEFINE_KEY_METHOD(FIELD_NAME, String);
VTK_INFORMATION_DEFINE_KEY_METHOD(FIELD_ASSOCIATION, Integer);
VTK_INFORMATION_DEFINE_KEY_METHOD(FIELD_ATTRIBUTE_TYPE, Integer);
VTK_INFORMATION_DEFINE_KEY_METHOD(FIELD_OPERATION, Integer);
VTK_INFORMATION_DEFINE_KEY_METHOD(UPDATE_EXTENT, IntegerVector);
VTK_INFORMATION_DEFINE_KEY_METHOD(SUPPORTED_UPSTREAM_REQUESTS, KeyVector);
VTK_INFORMATION_DEFINE_KEY_METHOD(SUPPORTED_DOWNSTREAM_REQUESTS, KeyVector);

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(name, type)                  \
  class vtkInformation##name##VectorValue: public vtkObjectBase             \
  {                                                                         \
  public:                                                                   \
    vtkTypeMacro(vtkInformation##name##VectorValue, vtkObjectBase);         \
    vtkstd::vector<type> Value;                                             \
  };                                                                        \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           type* value, int length)                         \
    {                                                                       \
    vtkInformation##name##VectorValue* v =                                  \
      new vtkInformation##name##VectorValue;                                \
    vtkInformationConstructClass("vtkInformation" #name "VectorValue");     \
    v->Value.insert(v->Value.begin(), value, value+length);                 \
    this->SetAsObjectBase(key, v);                                          \
    v->Delete();                                                            \
    }                                                                       \
  type* vtkInformation::Get(vtkInformation##name##VectorKey* key)           \
    {                                                                       \
    vtkInformation##name##VectorValue* v =                                  \
      vtkInformation##name##VectorValue::SafeDownCast(                      \
        this->GetAsObjectBase(key));                                        \
    return v?(&v->Value[0]):0;                                              \
    }                                                                       \
  void vtkInformation::Get(vtkInformation##name##VectorKey* key,            \
                           type* value)                                     \
    {                                                                       \
    vtkInformation##name##VectorValue* v =                                  \
      vtkInformation##name##VectorValue::SafeDownCast(                      \
        this->GetAsObjectBase(key));                                        \
    if(v)                                                                   \
      {                                                                     \
      for(vtkstd::vector<type>::size_type i = 0; i < v->Value.size(); ++i)  \
        {                                                                   \
        value[i] = v->Value[i];                                             \
        }                                                                   \
      }                                                                     \
    }                                                                       \
  int vtkInformation::Length(vtkInformation##name##VectorKey* key)          \
    {                                                                       \
    vtkInformation##name##VectorValue* v =                                  \
      vtkInformation##name##VectorValue::SafeDownCast(                      \
        this->GetAsObjectBase(key));                                        \
    return v?static_cast<int>(v->Value.size()):0;                           \
    }

VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Key, vtkInformationKey*);

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_OBJECT_VECTOR_PROPERTY(name, type)           \
  class vtkInformation##name##VectorValue: public vtkObjectBase             \
  {                                                                         \
  public:                                                                   \
    vtkTypeMacro(vtkInformation##name##VectorValue, vtkObjectBase);         \
    vtkstd::vector< vtkSmartPointer<vtk##type> > References;                \
    vtkstd::vector< vtk##type* > Value;                                     \
  };                                                                        \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           vtk##type** value, int length)                   \
    {                                                                       \
    vtkInformation##name##VectorValue* v =                                  \
      new vtkInformation##name##VectorValue;                                \
    vtkInformationConstructClass("vtkInformation" #name "VectorValue");     \
    v->Value.insert(v->Value.begin(), value, value+length);                 \
    for(vtkstd::vector<vtk##type*>::size_type i = 0;                        \
        i < v->Value.size(); ++i)                                           \
      {                                                                     \
      v->References.push_back(v->Value[i]);                                 \
      }                                                                     \
    this->SetAsObjectBase(key, v);                                          \
    v->Delete();                                                            \
    }                                                                       \
  vtk##type** vtkInformation::Get(vtkInformation##name##VectorKey* key)     \
    {                                                                       \
    vtkInformation##name##VectorValue* v =                                  \
      vtkInformation##name##VectorValue::SafeDownCast(                      \
        this->GetAsObjectBase(key));                                        \
    return v?(&v->Value[0]):0;                                              \
    }                                                                       \
  void vtkInformation::Get(vtkInformation##name##VectorKey* key,            \
                           vtk##type** value)                               \
    {                                                                       \
    vtkInformation##name##VectorValue* v =                                  \
      vtkInformation##name##VectorValue::SafeDownCast(                      \
        this->GetAsObjectBase(key));                                        \
    if(v)                                                                   \
      {                                                                     \
      for(vtkstd::vector<vtk##type*>::size_type i = 0;                      \
          i < v->Value.size(); ++i)                                         \
        {                                                                   \
        value[i] = v->Value[i];                                             \
        }                                                                   \
      }                                                                     \
    }                                                                       \
  int vtkInformation::Length(vtkInformation##name##VectorKey* key)          \
    {                                                                       \
    vtkInformation##name##VectorValue* v =                                  \
      vtkInformation##name##VectorValue::SafeDownCast(                      \
        this->GetAsObjectBase(key));                                        \
    return v?static_cast<int>(v->Value.size()):0;                           \
    }

VTK_INFORMATION_DEFINE_OBJECT_VECTOR_PROPERTY(DataObject, DataObject);

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
vtkInformationKey* vtkInformation::GetKey(vtkInformationDataObjectVectorKey* key)
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
vtkInformationKey* vtkInformation::GetKey(vtkInformationIntegerVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationStringKey* key)
{
  return key;
}
