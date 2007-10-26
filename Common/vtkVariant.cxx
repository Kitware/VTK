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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
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

vtkVariantLessThan::vtkVariantLessThan()
{
}

// Implementation of vtkVariant's less than operation
const bool vtkVariantLessThan::operator()(const vtkVariant& s1, const vtkVariant& s2) const
{
  if (s1.IsString() && s2.IsString())
    {
    return s1.ToString() < s2.ToString();
    }
  return s1.ToDouble() < s2.ToDouble();
}

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

vtkVariant::vtkVariant(char value)
{
  this->Data.Char = value;
  this->Valid = 1;
  this->Type = VTK_CHAR;
}

vtkVariant::vtkVariant(unsigned char value)
{
  this->Data.UnsignedChar = value;
  this->Valid = 1;
  this->Type = VTK_UNSIGNED_CHAR;
}

vtkVariant::vtkVariant(signed char value)
{
  this->Data.SignedChar = value;
  this->Valid = 1;
  this->Type = VTK_SIGNED_CHAR;
}

vtkVariant::vtkVariant(short value)
{
  this->Data.Short = value;
  this->Valid = 1;
  this->Type = VTK_SHORT;
}

vtkVariant::vtkVariant(unsigned short value)
{
  this->Data.UnsignedShort = value;
  this->Valid = 1;
  this->Type = VTK_UNSIGNED_SHORT;
}

vtkVariant::vtkVariant(int value)
{
  this->Data.Int = value;
  this->Valid = 1;
  this->Type = VTK_INT;
}

vtkVariant::vtkVariant(unsigned int value)
{
  this->Data.UnsignedInt = value;
  this->Valid = 1;
  this->Type = VTK_UNSIGNED_INT;
}

vtkVariant::vtkVariant(long value)
{
  this->Data.Long = value;
  this->Valid = 1;
  this->Type = VTK_LONG;
}

vtkVariant::vtkVariant(unsigned long value)
{
  this->Data.UnsignedLong = value;
  this->Valid = 1;
  this->Type = VTK_UNSIGNED_LONG;
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

vtkVariant::vtkVariant(const char* value)
{
  this->Data.String = new vtkStdString(value);
  this->Valid = 1;
  this->Type = VTK_STRING;
}

vtkVariant::vtkVariant(vtkStdString value)
{
  this->Data.String = new vtkStdString(value);
  this->Valid = 1;
  this->Type = VTK_STRING;
}

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
    || this->IsChar()
    || this->IsUnsignedChar()
    || this->IsSignedChar()
    || this->IsShort()
    || this->IsUnsignedShort()
    || this->IsInt() 
    || this->IsUnsignedInt()
    || this->IsLong() 
    || this->IsUnsignedLong()
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

bool vtkVariant::IsChar() const
{
  return this->Type == VTK_CHAR;
}

bool vtkVariant::IsUnsignedChar() const
{
  return this->Type == VTK_UNSIGNED_CHAR;
}

bool vtkVariant::IsSignedChar() const
{
  return this->Type == VTK_SIGNED_CHAR;
}

bool vtkVariant::IsShort() const
{
  return this->Type == VTK_SHORT;
}

bool vtkVariant::IsUnsignedShort() const
{
  return this->Type == VTK_UNSIGNED_SHORT;
}

bool vtkVariant::IsInt() const
{
  return this->Type == VTK_INT;
}

bool vtkVariant::IsUnsignedInt() const
{
  return this->Type == VTK_UNSIGNED_INT;
}

bool vtkVariant::IsLong() const
{
  return this->Type == VTK_LONG;
}

bool vtkVariant::IsUnsignedLong() const
{
  return this->Type == VTK_UNSIGNED_LONG;
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
    return vtkStdString(*(this->Data.String));
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
  if (this->IsChar())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.Char;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsignedChar())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.UnsignedChar;
    return vtkStdString(ostr.str());
    }
  if (this->IsSignedChar())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.SignedChar;
    return vtkStdString(ostr.str());
    }
  if (this->IsShort())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.Short;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsignedShort())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.UnsignedShort;
    return vtkStdString(ostr.str());
    }
  if (this->IsInt())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.Int;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsignedInt())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.UnsignedInt;
    return vtkStdString(ostr.str());
    }
  if (this->IsLong())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.Long;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsignedLong())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.UnsignedLong;
    return vtkStdString(ostr.str());
    }
#if defined(VTK_TYPE_USE___INT64)
  if (this->Is__Int64())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.__Int64;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsigned__Int64())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.Unsigned__Int64;
    return vtkStdString(ostr.str());
    }
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
  if (this->IsLongLong())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.LongLong;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsignedLongLong())
    {
    vtksys_ios::ostringstream ostr;
    ostr << this->Data.UnsignedLongLong;
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
  vtkGenericWarningMacro(<< "Cannot convert unknown type (" << this->Type << ") to a string.");
  return vtkStdString();
}

float vtkVariant::ToFloat(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<float *>(0));
}

double vtkVariant::ToDouble(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<double *>(0));
}

char vtkVariant::ToChar(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<char *>(0));
}

unsigned char vtkVariant::ToUnsignedChar(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned char *>(0));
}

signed char vtkVariant::ToSignedChar(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<signed char *>(0));
}

short vtkVariant::ToShort(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<short *>(0));
}

unsigned short vtkVariant::ToUnsignedShort(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned short *>(0));
}

int vtkVariant::ToInt(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<int *>(0));
}

unsigned int vtkVariant::ToUnsignedInt(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned int *>(0));
}

long vtkVariant::ToLong(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<long *>(0));
}

unsigned long vtkVariant::ToUnsignedLong(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned long *>(0));
}

#if defined(VTK_TYPE_USE___INT64)
__int64 vtkVariant::To__Int64(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<__int64 *>(0));
}

unsigned __int64 vtkVariant::ToUnsigned__Int64(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned __int64 *>(0));
}
#endif

#if defined(VTK_TYPE_USE_LONG_LONG)
long long vtkVariant::ToLongLong(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<long long *>(0));
}

unsigned long long vtkVariant::ToUnsignedLongLong(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned long long *>(0));
}
#endif

vtkTypeInt64 vtkVariant::ToTypeInt64(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<vtkTypeInt64 *>(0));
}

vtkTypeUInt64 vtkVariant::ToTypeUInt64(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<vtkTypeUInt64 *>(0));
}

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

template <typename T>
T vtkVariantStringToNumeric(vtkStdString str, bool* valid, T* vtkNotUsed(ignored) = 0)
{
  vtksys_ios::istringstream vstr(str);
  T data;
  vstr >> data;
  // Check for a valid result
  if (valid)
    {
    *valid =  ((vstr.rdstate() & ios::badbit) == 0
      && (vstr.rdstate() & ios::failbit) == 0);
    *valid = *valid && vstr.eof();
    //*valid = (vstr.rdstate() == ios::goodbit);
    }
  return data;
}

template <typename T>
T vtkVariant::ToNumeric(bool* valid, T* vtkNotUsed(ignored)) const
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
  if (this->IsChar())
    {
    return static_cast<T>(this->Data.Char);
    }
  if (this->IsUnsignedChar())
    {
    return static_cast<T>(this->Data.UnsignedChar);
    }
  if (this->IsSignedChar())
    {
    return static_cast<T>(this->Data.SignedChar);
    }
  if (this->IsShort())
    {
    return static_cast<T>(this->Data.Short);
    }
  if (this->IsUnsignedShort())
    {
    return static_cast<T>(this->Data.UnsignedShort);
    }
  if (this->IsInt())
    {
    return static_cast<T>(this->Data.Int);
    }
  if (this->IsUnsignedInt())
    {
    return static_cast<T>(this->Data.UnsignedInt);
    }
  if (this->IsLong())
    {
    return static_cast<T>(this->Data.Long);
    }
  if (this->IsUnsignedLong())
    {
    return static_cast<T>(this->Data.UnsignedLong);
    }
#if defined(VTK_TYPE_USE___INT64)
  if (this->Is__Int64())
    {
    return static_cast<T>(this->Data.__Int64);
    }
  if (this->IsUnsigned__Int64())
    {
    return static_cast<T>(static_cast<__int64>(this->Data.Unsigned__Int64));
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
