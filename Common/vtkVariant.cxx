/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariant.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVariant.h"

#include "vtkStdString.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkVariantArray.h"
#include "vtkType.h"
#include "vtkSetGet.h"
#include "vtkObjectBase.h"
#include "vtkStringArray.h"

#include "vtksys/ios/sstream"

vtkVariant::vtkVariant()
{
  this->Valid = 0;
  this->Type = 0;
}

vtkVariant::vtkVariant(const vtkVariant & other)
{
  this->Valid = other.Valid;
  this->Type = other.Type;
  this->Data = other.Data;
  if (this->Valid)
    {
    switch (other.Type)
      {
      case VTK_STRING:
        this->Data.String = new vtkStdString(*other.Data.String);
        break;
      case VTK_OBJECT:
        this->Data.VTKObject->Register(0);
        break;
      }
    }
}


const vtkVariant & vtkVariant::operator= (const vtkVariant & other)
{
  // First delete current variant item.
  if (this->Valid)
    {
    switch (this->Type)
      {
      case VTK_STRING:
        delete this->Data.String;
        break;
      case VTK_OBJECT:
        this->Data.VTKObject->Delete();
        break;
      }
    }

  // Then set the appropriate value.
  this->Valid = other.Valid;
  this->Type = other.Type;
  this->Data = other.Data;
  if (this->Valid)
    {
    switch (other.Type)
      {
      case VTK_STRING:
        this->Data.String = new vtkStdString(*other.Data.String);
        break;
      case VTK_OBJECT:
        this->Data.VTKObject->Register(0);
        break;
      }
    }
  return *this;
}

vtkVariant::~vtkVariant()
{
  if (this->Valid)
    {
    switch (this->Type)
      {
      case VTK_STRING:
        delete this->Data.String;
        break;
      case VTK_OBJECT:
        this->Data.VTKObject->Delete();
        break;
      }
    }
}


vtkVariant::vtkVariant(vtkStdString value)
{
  this->Data.String = new vtkStdString(value);
  this->Valid = 1;
  this->Type = VTK_STRING;
}

vtkVariant::vtkVariant(const char* value)
{
  this->Data.String = new vtkStdString(value);
  this->Valid = 1;
  this->Type = VTK_STRING;
}

vtkVariant::vtkVariant(float value)
{
  this->Data.Float = value;
  this->Valid = 1;
  this->Type = VTK_FLOAT;
}

vtkVariant::vtkVariant(double value)
{
  this->Data.Double = value;
  this->Valid = 1;
  this->Type = VTK_DOUBLE;
}

vtkVariant::vtkVariant(int value)
{
  this->Data.Int = value;
  this->Valid = 1;
  this->Type = VTK_INT;
}

#if defined(VTK_TYPE_USE___INT64)
vtkVariant::vtkVariant(__int64 value)
{
  this->Data.__Int64 = value;
  this->Valid = 1;
  this->Type = VTK___INT64;
}

vtkVariant::vtkVariant(unsigned __int64 value)
{
  this->Data.Unsigned__Int64 = value;
  this->Valid = 1;
  this->Type = VTK_UNSIGNED___INT64;
}
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
vtkVariant::vtkVariant(long long value)
{
  this->Data.LongLong = value;
  this->Valid = 1;
  this->Type = VTK_LONG_LONG;
}

vtkVariant::vtkVariant(unsigned long long value)
{
  this->Data.UnsignedLongLong = value;
  this->Valid = 1;
  this->Type = VTK_UNSIGNED_LONG_LONG;
}
#endif

vtkVariant::vtkVariant(vtkObjectBase* value)
{
  value->Register(0);
  this->Data.VTKObject = value;
  this->Valid = 1;
  this->Type = VTK_OBJECT;
}

bool vtkVariant::IsValid() const
{
  return this->Valid != 0;
}

bool vtkVariant::IsString() const
{
  return this->Type == VTK_STRING;
}

bool vtkVariant::IsNumeric() const
{
  return this->IsFloat() 
    || this->IsDouble() 
    || this->IsInt() 
    || this->Is__Int64() 
    || this->IsUnsigned__Int64() 
    || this->IsLongLong() 
    || this->IsUnsignedLongLong();
}

bool vtkVariant::IsFloat() const
{
  return this->Type == VTK_FLOAT;
}

bool vtkVariant::IsDouble() const
{
  return this->Type == VTK_DOUBLE;
}

bool vtkVariant::IsInt() const
{
  return this->Type == VTK_INT;
}

bool vtkVariant::Is__Int64() const
{
  return this->Type == VTK___INT64;
}

bool vtkVariant::IsUnsigned__Int64() const
{
  return this->Type == VTK_UNSIGNED___INT64;
}

bool vtkVariant::IsLongLong() const
{
  return this->Type == VTK_LONG_LONG;
}

bool vtkVariant::IsUnsignedLongLong() const
{
  return this->Type == VTK_UNSIGNED_LONG_LONG;
}

bool vtkVariant::IsVTKObject() const
{
  return this->Type == VTK_OBJECT;
}

bool vtkVariant::IsArray() const
{
  return this->Type == VTK_OBJECT 
    && this->Valid 
    && this->Data.VTKObject->IsA("vtkAbstractArray");
}

unsigned int vtkVariant::GetType() const
{
  return this->Type;
}

const char* vtkVariant::GetTypeAsString() const
{
  if (this->Type == VTK_OBJECT && this->Valid)
    {
    return this->Data.VTKObject->GetClassName();
    }
  return vtkImageScalarTypeNameMacro(this->Type);
}

template <typename iterT>
vtkStdString vtkVariantArrayToString(iterT* it)
{
  vtkIdType maxInd = it->GetNumberOfValues();
  vtksys_ios::ostringstream ostr;
  for (vtkIdType i = 0; i < maxInd; i++)
    {
    if (i > 0)
      {
      ostr << " ";
      }
    ostr << it->GetValue(i);
    }
  return ostr.str();
}

vtkStdString vtkVariant::ToString() const
{
  if (!this->IsValid())
    {
    return vtkStdString();
    }
  if (this->IsString())
    {
    return vtkStdString(this->Data.String->c_str());
    }
  if (this->IsFloat())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.Float;
    return vtkStdString(ostr.str());
    }
  if (this->IsDouble())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.Double;
    return vtkStdString(ostr.str());
    }
  if (this->IsInt())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.Int;
    return vtkStdString(ostr.str());
    }
#if defined(VTK_TYPE_USE___INT64)
  if (this->Is__Int64())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.__Int64;
    return vtkStdString(ostr.str());
    }
#endif
  if (this->IsArray())
    {
    vtkAbstractArray* arr = vtkAbstractArray::SafeDownCast(this->Data.VTKObject);
    vtkArrayIterator* iter = arr->NewIterator();
    vtkStdString str;
    switch(arr->GetDataType())
      {
      vtkArrayIteratorTemplateMacro(
        str = vtkVariantArrayToString(static_cast<VTK_TT*>(iter)));
      }
    iter->Delete();
    return str;
    }
  return vtkStdString();
}

float vtkVariant::ToFloat(bool* valid) const
{
  return this->ToNumeric(valid, (float *)0);
}

double vtkVariant::ToDouble(bool* valid) const
{
  return this->ToNumeric(valid, (double *)0);
}

int vtkVariant::ToInt(bool* valid) const
{
  return this->ToNumeric(valid, (int *)0);
}

#if defined(VTK_TYPE_USE___INT64)
__int64 vtkVariant::To__Int64(bool* valid) const
{
  return this->ToNumeric(valid, (__int64 *)0);
}

unsigned __int64 vtkVariant::ToUnsigned__Int64(bool* valid) const
{
  return this->ToNumeric(valid, (unsigned __int64 *)0);
}
#endif

#if defined(VTK_TYPE_USE_LONG_LONG)
long long vtkVariant::ToLongLong(bool* valid) const
{
  return this->ToNumeric(valid, (long long *)0);
}

unsigned long long vtkVariant::ToUnsignedLongLong(bool* valid) const
{
  return this->ToNumeric(valid, (unsigned long long *)0);
}
#endif

vtkObjectBase* vtkVariant::ToVTKObject() const
{
  if (this->IsVTKObject())
    {
    return this->Data.VTKObject;
    }
  return 0;
}

vtkAbstractArray* vtkVariant::ToArray() const
{
  if (this->IsArray())
    {
    return vtkAbstractArray::SafeDownCast(this->Data.VTKObject);
    }
  return 0;
}

ostream& operator << (ostream& out, vtkVariant v)
{
  out << v.ToString();
  return out;
}

template <typename T>
T vtkVariantStringToNumeric(vtkStdString str, bool* valid, T* vtkNotUsed(ignored) = 0)
{
  vtkstd::istringstream vstr(str);
  T data;
  vstr >> data;
  // Check for a valid result
  if (valid)
    {
    *valid =  ((vstr.rdstate() & ios::badbit) == 0
      && (vstr.rdstate() & ios::failbit) == 0);
    //*valid = (vstr.rdstate() == ios::goodbit);
    }
  return data;
}

template <typename T>
T vtkVariant::ToNumeric(bool* valid, T*) const
{
  if (valid)
    {
    *valid = true;
    }
  if (this->IsString())
    {
    return vtkVariantStringToNumeric<T>(* this->Data.String, valid);
    }
  if (this->IsFloat())
    {
    return static_cast<T>(this->Data.Float);
    }
  if (this->IsDouble())
    {
    return static_cast<T>(this->Data.Double);
    }
  if (this->IsInt())
    {
    return static_cast<T>(this->Data.Int);
    }
#if defined(VTK_TYPE_USE___INT64)
  if (this->Is__Int64())
    {
    return static_cast<T>(this->Data.__Int64);
    }
  if (this->IsUnsigned__Int64())
    {
    return static_cast<T>(this->Data.Unsigned__Int64);
    }
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
  if (this->IsLongLong())
    {
    return static_cast<T>(this->Data.LongLong);
    }
  if (this->IsUnsignedLongLong())
    {
    return static_cast<T>(this->Data.UnsignedLongLong);
    }
#endif
  // For arrays, convert the first value to the appropriate type.
  if (this->IsArray())
    {
    if (this->Data.VTKObject->IsA("vtkDataArray"))
      {
      // Note: This are not the best conversion.
      //       We covert the first value to double, then
      //       cast it back to the appropriate numeric type.
      vtkDataArray* da = vtkDataArray::SafeDownCast(this->Data.VTKObject);
      return static_cast<T>(da->GetTuple1(0));
      }
    if (this->Data.VTKObject->IsA("vtkVariantArray"))
      {
      // Note: This are not the best conversion.
      //       We covert the first value to double, then
      //       cast it back to the appropriate numeric type.
      vtkVariantArray* va = vtkVariantArray::SafeDownCast(this->Data.VTKObject);
      return static_cast<T>(va->GetValue(0).ToDouble());
      }
    if (this->Data.VTKObject->IsA("vtkStringArray"))
      {
      vtkStringArray* sa = vtkStringArray::SafeDownCast(this->Data.VTKObject);
      return vtkVariantStringToNumeric<T>(sa->GetValue(0), valid);
      }
    }
  if (valid)
    {
    *valid = false;
    }
  return static_cast<T>(0);
}
