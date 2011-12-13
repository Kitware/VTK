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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#ifdef VTK_VARIANT_IMPL
/*
All code in this source file is conditionally compiled to work-around
a build problem on the SGI with MIPSpro 7.4.4.  The prelinker loads
vtkVariant.cxx while looking for the template definition of
vtkVariant::ToNumeric<> declared in the vtkVariant.h header.  It wants
the definition in order to instantiate it in vtkCharArray.o or some
other object to which the instantiation has been assigned.  For some
reason providing the explicit instantiation in vtkVariant.o is not
enough to prevent the prelinker from assigning the instantiation to
another object.  When vtkVariant.cxx is included in the translation
unit by the prelinker, it causes additional instantiations of other
templates, like vtkVariantStringToNumeric.  This sends the prelinker
into an infinite loop of instantiation requests.  By placing all the
code in this preprocessing condition, we hide it from the prelinker.
The CMakeLists.txt file defines the macro to compile this code when
really building the vtkVariant.o object.
*/
#include "vtkVariant.h"

#include "vtkStdString.h"
#include "vtkUnicodeString.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkVariantArray.h"
#include "vtkType.h"
#include "vtkSetGet.h"
#include "vtkObjectBase.h"
#include "vtkStringArray.h"
#include "vtkMath.h"

#include "vtksys/ios/sstream"
#include "vtksys/SystemTools.hxx"
#include <locale> // C++ locale

//----------------------------------------------------------------------------

// Implementation of vtkVariant's
// fast-but-potentially-counterintuitive < operation
bool vtkVariantStrictWeakOrder::operator()(const vtkVariant& s1, const vtkVariant& s2) const
{
  // First sort on type if they are different
  if (s1.Type != s2.Type)
    {
    return s1.Type < s2.Type;
    }

  // Next check for nulls
  if (!(s1.Valid && s2.Valid))
    {
    if (!(s1.Valid || s2.Valid))
      {
      return false; // nulls are equal to one another
      }
    else if (!s1.Valid)
      {
      return true; // null is less than any valid value
      }
    else
      {
      return false;
      }
    }

  switch (s1.Type)
    {
    case VTK_STRING:
      return (*(s1.Data.String) < *(s2.Data.String));

    case VTK_UNICODE_STRING:
      return (*(s1.Data.UnicodeString) < *(s2.Data.UnicodeString));

    case VTK_OBJECT:
      return (s1.Data.VTKObject < s2.Data.VTKObject);

    case VTK_CHAR:
      return (s1.Data.Char < s2.Data.Char);

    case VTK_SIGNED_CHAR:
      return (s1.Data.SignedChar < s2.Data.SignedChar);

    case VTK_UNSIGNED_CHAR:
      return (s1.Data.UnsignedChar < s2.Data.UnsignedChar);

    case VTK_SHORT:
      return (s1.Data.Short < s2.Data.Short);

    case VTK_UNSIGNED_SHORT:
      return (s1.Data.UnsignedShort < s2.Data.UnsignedShort);

    case VTK_INT:
      return (s1.Data.Int < s2.Data.Int);

    case VTK_UNSIGNED_INT:
      return (s1.Data.UnsignedInt < s2.Data.UnsignedInt);

    case VTK_LONG:
      return (s1.Data.Long < s2.Data.Long);

    case VTK_UNSIGNED_LONG:
      return (s1.Data.UnsignedLong < s2.Data.UnsignedLong);

#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:
      return (s1.Data.__Int64 < s2.Data.__Int64);

    case VTK_UNSIGNED___INT64:
      return (s1.Data.Unsigned__Int64 < s2.Data.Unsigned__Int64);
#endif

#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_LONG_LONG:
      return (s1.Data.LongLong < s2.Data.LongLong);

    case VTK_UNSIGNED_LONG_LONG:
      return (s1.Data.UnsignedLongLong < s2.Data.UnsignedLongLong);
#endif

    case VTK_FLOAT:
      return (s1.Data.Float < s2.Data.Float);

    case VTK_DOUBLE:
      return (s1.Data.Double < s2.Data.Double);

    default:
      cerr << "ERROR: Unhandled type " << s1.Type << " in vtkVariantStrictWeakOrder\n";
      return false;
    }
}

// ----------------------------------------------------------------------

bool
vtkVariantStrictEquality::operator()(const vtkVariant &s1, const vtkVariant &s2) const
{
  // First sort on type if they are different
  if (s1.Type != s2.Type)
    {
    cerr << "Types differ: " << s1.Type << " and " << s2.Type << "\n";
    return false;
    }

  // Next check for nulls
  if (!(s1.Valid && s2.Valid))
    {
    cerr << "Validity may differ: " << s1.Valid << " and " << s2.Valid << "\n";
    return (s1.Valid == s2.Valid);
    }

  // At this point we know that both variants contain a valid value.
  switch (s1.Type)
    {
    case VTK_STRING:
    {
    if (*(s1.Data.String) != *(s2.Data.String))
      {
      cerr << "Strings differ: '"
           << *(s1.Data.String) << "' and '"
           << *(s2.Data.String) << "'\n";
      }
      return (*(s1.Data.String) == *(s2.Data.String));
    };

    case VTK_UNICODE_STRING:
      return (*(s1.Data.UnicodeString) == *(s2.Data.UnicodeString));

    case VTK_OBJECT:
      return (s1.Data.VTKObject == s2.Data.VTKObject);

    case VTK_CHAR:
      return (s1.Data.Char == s2.Data.Char);

    case VTK_SIGNED_CHAR:
      return (s1.Data.SignedChar == s2.Data.SignedChar);

    case VTK_UNSIGNED_CHAR:
      return (s1.Data.UnsignedChar == s2.Data.UnsignedChar);

    case VTK_SHORT:
      return (s1.Data.Short == s2.Data.Short);

    case VTK_UNSIGNED_SHORT:
      return (s1.Data.UnsignedShort == s2.Data.UnsignedShort);

    case VTK_INT:
      return (s1.Data.Int == s2.Data.Int);

    case VTK_UNSIGNED_INT:
      return (s1.Data.UnsignedInt == s2.Data.UnsignedInt);

    case VTK_LONG:
      return (s1.Data.Long == s2.Data.Long);

    case VTK_UNSIGNED_LONG:
      return (s1.Data.UnsignedLong == s2.Data.UnsignedLong);

#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:
      return (s1.Data.__Int64 == s2.Data.__Int64);

    case VTK_UNSIGNED___INT64:
      return (s1.Data.Unsigned__Int64 == s2.Data.Unsigned__Int64);
#endif

#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_LONG_LONG:
      return (s1.Data.LongLong == s2.Data.LongLong);

    case VTK_UNSIGNED_LONG_LONG:
      return (s1.Data.UnsignedLongLong == s2.Data.UnsignedLongLong);
#endif

    case VTK_FLOAT:
      return (s1.Data.Float == s2.Data.Float);

    case VTK_DOUBLE:
      return (s1.Data.Double == s2.Data.Double);

    default:
      cerr << "ERROR: Unhandled type " << s1.Type << " in vtkVariantStrictEquality\n";
      return false;
    }
}

// ----------------------------------------------------------------------

bool
vtkVariantLessThan::operator()(const vtkVariant &v1, const vtkVariant &v2) const
{
  return v1.operator<(v2);
}

// ----------------------------------------------------------------------

bool
vtkVariantEqual::operator()(const vtkVariant &v1, const vtkVariant &v2) const
{
  return v1.operator==(v2);
}

//----------------------------------------------------------------------------
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
      case VTK_UNICODE_STRING:
        this->Data.UnicodeString = new vtkUnicodeString(*other.Data.UnicodeString);
        break;
      case VTK_OBJECT:
        this->Data.VTKObject->Register(0);
        break;
      }
    }
}

vtkVariant::vtkVariant(const vtkVariant &s2, unsigned int type)
{
  bool valid = false;

  if (s2.Valid)
    {
    switch (type)
      {
      case VTK_STRING:
        this->Data.String = new vtkStdString(s2.ToString());
        valid = true;
        break;

      case VTK_UNICODE_STRING:
        this->Data.UnicodeString =
          new vtkUnicodeString(s2.ToUnicodeString());
        valid = true;
        break;

      case VTK_OBJECT:
        this->Data.VTKObject = s2.ToVTKObject();
        if (this->Data.VTKObject)
          {
          this->Data.VTKObject->Register(0);
          valid = true;
          }
        break;

      case VTK_CHAR:
        this->Data.Char = s2.ToChar(&valid);
        break;

      case VTK_SIGNED_CHAR:
        this->Data.SignedChar = s2.ToSignedChar(&valid);
        break;

      case VTK_UNSIGNED_CHAR:
        this->Data.UnsignedChar = s2.ToUnsignedChar(&valid);
        break;

      case VTK_SHORT:
        this->Data.Short = s2.ToShort(&valid);
        break;

      case VTK_UNSIGNED_SHORT:
        this->Data.UnsignedShort = s2.ToUnsignedShort(&valid);
        break;

      case VTK_INT:
        this->Data.Int = s2.ToInt(&valid);
        break;

      case VTK_UNSIGNED_INT:
        this->Data.UnsignedInt = s2.ToUnsignedInt(&valid);
        break;

      case VTK_LONG:
        this->Data.Long = s2.ToLong(&valid);
        break;

      case VTK_UNSIGNED_LONG:
        this->Data.UnsignedLong = s2.ToUnsignedLong(&valid);
        break;

#if defined(VTK_TYPE_USE___INT64)
      case VTK___INT64:
        this->Data.__Int64 = s2.To__Int64(&valid);
        break;

      case VTK_UNSIGNED___INT64:
        this->Data.Unsigned__Int64 = s2.ToUnsigned__Int64(&valid);
        break;
#endif

#if defined(VTK_TYPE_USE_LONG_LONG)
      case VTK_LONG_LONG:
        this->Data.LongLong = s2.ToLongLong(&valid);
        break;

      case VTK_UNSIGNED_LONG_LONG:
        this->Data.UnsignedLongLong = s2.ToUnsignedLongLong(&valid);
        break;
#endif

      case VTK_FLOAT:
        this->Data.Float = s2.ToFloat(&valid);
        break;

      case VTK_DOUBLE:
        this->Data.Double = s2.ToDouble(&valid);
        break;
      }
    }

  this->Type = (valid ? type : 0);
  this->Valid = valid;
}

const vtkVariant & vtkVariant::operator= (const vtkVariant & other)
{
  // Short circuit if assigning to self:
  if (this == &other)
    {
    return *this;
    }

  // First delete current variant item.
  if (this->Valid)
    {
    switch (this->Type)
      {
      case VTK_STRING:
        delete this->Data.String;
        break;
      case VTK_UNICODE_STRING:
        delete this->Data.UnicodeString;
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
      case VTK_UNICODE_STRING:
        this->Data.UnicodeString = new vtkUnicodeString(*other.Data.UnicodeString);
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
      case VTK_UNICODE_STRING:
        delete this->Data.UnicodeString;
        break;
      case VTK_OBJECT:
        this->Data.VTKObject->Delete();
        break;
      }
    }
}

vtkVariant::vtkVariant(bool value)
{
  this->Data.Char = value;
  this->Valid = 1;
  this->Type = VTK_CHAR;
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
  this->Valid = 0;
  this->Type = 0;
  if (value)
    {
    this->Data.String = new vtkStdString(value);
    this->Valid = 1;
    this->Type = VTK_STRING;
    }
}

vtkVariant::vtkVariant(vtkStdString value)
{
  this->Data.String = new vtkStdString(value);
  this->Valid = 1;
  this->Type = VTK_STRING;
}

vtkVariant::vtkVariant(const vtkUnicodeString& value)
{
  this->Data.UnicodeString = new vtkUnicodeString(value);
  this->Valid = 1;
  this->Type = VTK_UNICODE_STRING;
}

vtkVariant::vtkVariant(vtkObjectBase* value)
{
  this->Valid = 0;
  this->Type = 0;
  if (value)
    {
    value->Register(0);
    this->Data.VTKObject = value;
    this->Valid = 1;
    this->Type = VTK_OBJECT;
    }
}

bool vtkVariant::IsValid() const
{
  return this->Valid != 0;
}

bool vtkVariant::IsString() const
{
  return this->Type == VTK_STRING;
}

bool vtkVariant::IsUnicodeString() const
{
  return this->Type == VTK_UNICODE_STRING;
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
  if (this->IsUnicodeString())
    {
    return vtkUnicodeString(*(this->Data.UnicodeString)).utf8_str();
    }
  if (this->IsFloat())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.Float;
    return vtkStdString(ostr.str());
    }
  if (this->IsDouble())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
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
    ostr << static_cast<unsigned int>(this->Data.UnsignedChar);
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
    ostr.imbue(std::locale::classic());
    ostr << this->Data.Int;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsignedInt())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.UnsignedInt;
    return vtkStdString(ostr.str());
    }
  if (this->IsLong())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.Long;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsignedLong())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.UnsignedLong;
    return vtkStdString(ostr.str());
    }
#if defined(VTK_TYPE_USE___INT64)
  if (this->Is__Int64())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.__Int64;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsigned__Int64())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.Unsigned__Int64;
    return vtkStdString(ostr.str());
    }
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
  if (this->IsLongLong())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.LongLong;
    return vtkStdString(ostr.str());
    }
  if (this->IsUnsignedLongLong())
    {
    vtksys_ios::ostringstream ostr;
    ostr.imbue(std::locale::classic());
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

vtkUnicodeString vtkVariant::ToUnicodeString() const
{
  if (!this->IsValid())
    {
    return vtkUnicodeString();
    }
  if (this->IsString())
    {
    return vtkUnicodeString::from_utf8(*this->Data.String);
    }
  if (this->IsUnicodeString())
    {
    return *this->Data.UnicodeString;
    }

  return vtkUnicodeString::from_utf8(this->ToString());
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

// Used internally by vtkVariantStringToNumeric to find non-finite numbers.
// Most numerics do not support non-finite numbers, hence the default simply
// fails.  Overload for doubles and floats detect non-finite numbers they
// support
template <typename T>
T vtkVariantStringToNonFiniteNumeric(vtkStdString vtkNotUsed(str), bool *valid)
{
  if (valid) *valid = 0;
  return 0;
}

template<> double vtkVariantStringToNonFiniteNumeric<double>(vtkStdString str,
                                                             bool *valid)
{
  if (vtksys::SystemTools::Strucmp(str.c_str(), "nan") == 0)
    {
    if (valid) *valid = true;
    return vtkMath::Nan();
    }
  if (   (vtksys::SystemTools::Strucmp(str.c_str(), "infinity") == 0)
      || (vtksys::SystemTools::Strucmp(str.c_str(), "inf") == 0) )
    {
    if (valid) *valid = true;
    return vtkMath::Inf();
    }
  if (   (vtksys::SystemTools::Strucmp(str.c_str(), "-infinity") == 0)
      || (vtksys::SystemTools::Strucmp(str.c_str(), "-inf") == 0) )
    {
    if (valid) *valid = true;
    return vtkMath::NegInf();
    }
  if (valid) *valid = false;
  return vtkMath::Nan();
}

template<> float vtkVariantStringToNonFiniteNumeric<float>(vtkStdString str,
                                                           bool *valid)
{
  return static_cast<float>(
                        vtkVariantStringToNonFiniteNumeric<double>(str, valid));
}

template <typename T>
T vtkVariantStringToNumeric(vtkStdString str, bool* valid, T* vtkNotUsed(ignored) = 0)
{
  vtksys_ios::istringstream vstr(str);
  T data;
  vstr >> data;
  // Check for a valid result
  bool v = (   ((vstr.rdstate() & ios::badbit) == 0)
            && ((vstr.rdstate() & ios::failbit) == 0)
            && vstr.eof() );
  //v = (vstr.rdstate() == ios::goodbit);
  if (valid) *valid = v;
  if (!v)
    {
    data = vtkVariantStringToNonFiniteNumeric<T>(str, valid);
    }
  return data;
}

//----------------------------------------------------------------------------
// Definition of ToNumeric

// Visual Studio 6 chokes with "error C2893: Failed to specialize function
// template" when the definition of ToNumeric is included here and it
// encounters the various callers of ToNumeric below. So for VS6 include the
// the implementation of ToNumeric at the bottom of the file.
//
#if !defined(_MSC_VER) || (_MSC_VER >= 1300)
# define VTK_VARIANT_TO_NUMERIC_CXX_INCLUDED
# include "vtkVariantToNumeric.cxx"
#endif

//----------------------------------------------------------------------------
// Explicit instantiations of ToNumeric

// Visual Studio 6 chokes with "fatal error C1001: INTERNAL COMPILER ERROR"
// when trying to use the vtkVariantToNumericInstantiateMacro, so don't...
//
#if defined(_MSC_VER) && (_MSC_VER < 1300)
# define VTK_VARIANT_NO_INSTANTIATE
#endif

//----------------------------------------------------------------------------
// Explicitly instantiate the ToNumeric member template to make sure
// the symbols are exported from this object file.
// This explicit instantiation exists to resolve VTK issue #5791.

#if !defined(VTK_VARIANT_NO_INSTANTIATE)

#define vtkVariantToNumericInstantiateMacro(x)                          \
  template x vtkVariant::ToNumeric< x >(bool*, x*) const

vtkVariantToNumericInstantiateMacro(char);
vtkVariantToNumericInstantiateMacro(float);
vtkVariantToNumericInstantiateMacro(double);
vtkVariantToNumericInstantiateMacro(unsigned char);
vtkVariantToNumericInstantiateMacro(signed char);
vtkVariantToNumericInstantiateMacro(short);
vtkVariantToNumericInstantiateMacro(unsigned short);
vtkVariantToNumericInstantiateMacro(int);
vtkVariantToNumericInstantiateMacro(unsigned int);
vtkVariantToNumericInstantiateMacro(long);
vtkVariantToNumericInstantiateMacro(unsigned long);

#if defined(VTK_TYPE_USE___INT64)
vtkVariantToNumericInstantiateMacro(__int64);
vtkVariantToNumericInstantiateMacro(unsigned __int64);
#endif

#if defined(VTK_TYPE_USE_LONG_LONG)
vtkVariantToNumericInstantiateMacro(long long);
vtkVariantToNumericInstantiateMacro(unsigned long long);
#endif

#endif

//----------------------------------------------------------------------------
// Callers causing implicit instantiations of ToNumeric

float vtkVariant::ToFloat(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<float *>(0));
}

double vtkVariant::ToDouble(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<double *>(0));
}

char vtkVariant::ToChar(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<char *>(0));
}

unsigned char vtkVariant::ToUnsignedChar(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned char *>(0));
}

signed char vtkVariant::ToSignedChar(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<signed char *>(0));
}

short vtkVariant::ToShort(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<short *>(0));
}

unsigned short vtkVariant::ToUnsignedShort(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned short *>(0));
}

int vtkVariant::ToInt(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<int *>(0));
}

unsigned int vtkVariant::ToUnsignedInt(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned int *>(0));
}

long vtkVariant::ToLong(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<long *>(0));
}

unsigned long vtkVariant::ToUnsignedLong(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned long *>(0));
}

#if defined(VTK_TYPE_USE___INT64)
__int64 vtkVariant::To__Int64(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<__int64 *>(0));
}

unsigned __int64 vtkVariant::ToUnsigned__Int64(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned __int64 *>(0));
}
#endif

#if defined(VTK_TYPE_USE_LONG_LONG)
long long vtkVariant::ToLongLong(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<long long *>(0));
}

unsigned long long vtkVariant::ToUnsignedLongLong(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned long long *>(0));
}
#endif

vtkTypeInt64 vtkVariant::ToTypeInt64(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<vtkTypeInt64 *>(0));
}

vtkTypeUInt64 vtkVariant::ToTypeUInt64(bool *valid) const
{
  return this->ToNumeric(valid, static_cast<vtkTypeUInt64 *>(0));
}

bool vtkVariant::IsEqual(const vtkVariant& other) const
{
  return this->operator==(other);
}

ostream& operator << ( ostream& os, const vtkVariant& val )
{
  if ( ! val.Valid )
    {
    os << "(invalid)";
    return os;
    }
  switch ( val.Type )
    {
  case VTK_STRING:
    if ( val.Data.String )
      {
      os << "\"" << val.Data.String->c_str() << "\"";
      }
    else
      {
      os << "\"\"";
      }
    break;
  case VTK_UNICODE_STRING:
    if ( val.Data.UnicodeString )
      {
      os << "\"" << val.Data.UnicodeString->utf8_str() << "\"";
      }
    else
      {
      os << "\"\"";
      }
    break;
  case VTK_FLOAT:
    os << val.Data.Float;
    break;
  case VTK_DOUBLE:
    os << val.Data.Double;
    break;
  case VTK_CHAR:
    os << val.Data.Char;
    break;
  case VTK_UNSIGNED_CHAR:
    os << val.Data.UnsignedChar;
    break;
  case VTK_SIGNED_CHAR:
    os << val.Data.SignedChar;
    break;
  case VTK_SHORT:
    os << val.Data.Short;
    break;
  case VTK_UNSIGNED_SHORT:
    os << val.Data.UnsignedShort;
    break;
  case VTK_INT:
    os << val.Data.Int;
    break;
  case VTK_UNSIGNED_INT:
    os << val.Data.UnsignedInt;
    break;
  case VTK_LONG:
    os << val.Data.Long;
    break;
  case VTK_UNSIGNED_LONG:
    os << val.Data.UnsignedLong;
    break;
#if defined(VTK_TYPE_USE___INT64)
  case VTK___INT64:
    os << val.Data.__Int64;
    break;
  case VTK_UNSIGNED___INT64:
    os << val.Data.Unsigned__Int64;
    break;
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
  case VTK_LONG_LONG:
    os << val.Data.LongLong;
    break;
  case VTK_UNSIGNED_LONG_LONG:
    os << val.Data.UnsignedLongLong;
    break;
#endif
  case VTK_OBJECT:
    if ( val.Data.VTKObject )
      {
      os << "(" << val.Data.VTKObject->GetClassName() << ")" << hex << val.Data.VTKObject;
      }
    else
      {
      os << "(vtkObjectBase)0x0";
      }
    break;
    }
  return os;
}

//----------------------------------------------------------------------------
// Definition of ToNumeric if not already included above:

#if !defined(VTK_VARIANT_TO_NUMERIC_CXX_INCLUDED)
# define VTK_VARIANT_TO_NUMERIC_CXX_INCLUDED
# include "vtkVariantToNumeric.cxx"
#endif
#endif
