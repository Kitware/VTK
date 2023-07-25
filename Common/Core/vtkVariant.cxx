// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkVariant.h"

#include "vtkAbstractArray.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkDataArray.h"
#include "vtkMath.h"
#include "vtkObjectBase.h"
#include "vtkSetGet.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkType.h"
#include "vtkValueFromString.h"
#include "vtkVariantArray.h"

#include "vtksys/SystemTools.hxx"

#include <cassert>
#include <cctype> // std::isspace
#include <locale> // C++ locale
#include <sstream>

//------------------------------------------------------------------------------

// Implementation of vtkVariant's
// fast-but-potentially-counterintuitive < operation
VTK_ABI_NAMESPACE_BEGIN
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

    case VTK_LONG_LONG:
      return (s1.Data.LongLong < s2.Data.LongLong);

    case VTK_UNSIGNED_LONG_LONG:
      return (s1.Data.UnsignedLongLong < s2.Data.UnsignedLongLong);

    case VTK_FLOAT:
      return (s1.Data.Float < s2.Data.Float);

    case VTK_DOUBLE:
      return (s1.Data.Double < s2.Data.Double);

    default:
      cerr << "ERROR: Unhandled type " << s1.Type << " in vtkVariantStrictWeakOrder\n";
      return false;
  }
}

//------------------------------------------------------------------------------

bool vtkVariantStrictEquality::operator()(const vtkVariant& s1, const vtkVariant& s2) const
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
        cerr << "Strings differ: '" << *(s1.Data.String) << "' and '" << *(s2.Data.String) << "'\n";
      }
      return (*(s1.Data.String) == *(s2.Data.String));
    }

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

    case VTK_LONG_LONG:
      return (s1.Data.LongLong == s2.Data.LongLong);

    case VTK_UNSIGNED_LONG_LONG:
      return (s1.Data.UnsignedLongLong == s2.Data.UnsignedLongLong);

    case VTK_FLOAT:
      return (s1.Data.Float == s2.Data.Float);

    case VTK_DOUBLE:
      return (s1.Data.Double == s2.Data.Double);

    default:
      cerr << "ERROR: Unhandled type " << s1.Type << " in vtkVariantStrictEquality\n";
      return false;
  }
}

//------------------------------------------------------------------------------

bool vtkVariantLessThan::operator()(const vtkVariant& v1, const vtkVariant& v2) const
{
  return v1.operator<(v2);
}

//------------------------------------------------------------------------------

bool vtkVariantEqual::operator()(const vtkVariant& v1, const vtkVariant& v2) const
{
  return v1.operator==(v2);
}

//------------------------------------------------------------------------------
vtkVariant::vtkVariant()
{
  this->Valid = false;
  this->Type = VTK_VOID;

  // Zero the Data union by setting any field.
  this->Data.UnsignedLongLong = 0;
}

vtkVariant::vtkVariant(const vtkVariant& other)
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
        this->Data.VTKObject->Register(nullptr);
        break;
    }
  }
}

vtkVariant::vtkVariant(const vtkVariant& s2, unsigned int type)
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

      case VTK_OBJECT:
        this->Data.VTKObject = s2.ToVTKObject();
        if (this->Data.VTKObject)
        {
          this->Data.VTKObject->Register(nullptr);
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

      case VTK_LONG_LONG:
        this->Data.LongLong = s2.ToLongLong(&valid);
        break;

      case VTK_UNSIGNED_LONG_LONG:
        this->Data.UnsignedLongLong = s2.ToUnsignedLongLong(&valid);
        break;

      case VTK_FLOAT:
        this->Data.Float = s2.ToFloat(&valid);
        break;

      case VTK_DOUBLE:
        this->Data.Double = s2.ToDouble(&valid);
        break;

      // Other types are not allowed.
      default:
        assert(0);
        break;
    }
  }

  this->Type = (valid ? type : VTK_VOID);
  this->Valid = valid;
}

vtkVariant& vtkVariant::operator=(const vtkVariant& other)
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
        this->Data.VTKObject->Register(nullptr);
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

vtkVariant::vtkVariant(bool value)
{
  this->Data.Char = value;
  this->Valid = true;
  this->Type = VTK_CHAR;
}

vtkVariant::vtkVariant(char value)
{
  this->Data.Char = value;
  this->Valid = true;
  this->Type = VTK_CHAR;
}

vtkVariant::vtkVariant(unsigned char value)
{
  this->Data.UnsignedChar = value;
  this->Valid = true;
  this->Type = VTK_UNSIGNED_CHAR;
}

vtkVariant::vtkVariant(signed char value)
{
  this->Data.SignedChar = value;
  this->Valid = true;
  this->Type = VTK_SIGNED_CHAR;
}

vtkVariant::vtkVariant(short value)
{
  this->Data.Short = value;
  this->Valid = true;
  this->Type = VTK_SHORT;
}

vtkVariant::vtkVariant(unsigned short value)
{
  this->Data.UnsignedShort = value;
  this->Valid = true;
  this->Type = VTK_UNSIGNED_SHORT;
}

vtkVariant::vtkVariant(int value)
{
  this->Data.Int = value;
  this->Valid = true;
  this->Type = VTK_INT;
}

vtkVariant::vtkVariant(unsigned int value)
{
  this->Data.UnsignedInt = value;
  this->Valid = true;
  this->Type = VTK_UNSIGNED_INT;
}

vtkVariant::vtkVariant(long value)
{
  this->Data.Long = value;
  this->Valid = true;
  this->Type = VTK_LONG;
}

vtkVariant::vtkVariant(unsigned long value)
{
  this->Data.UnsignedLong = value;
  this->Valid = true;
  this->Type = VTK_UNSIGNED_LONG;
}

vtkVariant::vtkVariant(long long value)
{
  this->Data.LongLong = value;
  this->Valid = true;
  this->Type = VTK_LONG_LONG;
}

vtkVariant::vtkVariant(unsigned long long value)
{
  this->Data.UnsignedLongLong = value;
  this->Valid = true;
  this->Type = VTK_UNSIGNED_LONG_LONG;
}

vtkVariant::vtkVariant(float value)
{
  this->Data.Float = value;
  this->Valid = true;
  this->Type = VTK_FLOAT;
}

vtkVariant::vtkVariant(double value)
{
  this->Data.Double = value;
  this->Valid = true;
  this->Type = VTK_DOUBLE;
}

vtkVariant::vtkVariant(const char* value)
{
  this->Valid = false;
  this->Type = VTK_VOID;
  if (value)
  {
    this->Data.String = new vtkStdString(value);
    this->Valid = true;
    this->Type = VTK_STRING;
  }
}

vtkVariant::vtkVariant(vtkStdString value)
{
  this->Data.String = new vtkStdString(value);
  this->Valid = true;
  this->Type = VTK_STRING;
}

vtkVariant::vtkVariant(vtkObjectBase* value)
{
  this->Valid = false;
  this->Type = VTK_VOID;
  if (value)
  {
    value->Register(nullptr);
    this->Data.VTKObject = value;
    this->Valid = true;
    this->Type = VTK_OBJECT;
  }
}

bool vtkVariant::IsValid() const
{
  return this->Valid;
}

bool vtkVariant::IsString() const
{
  return this->Type == VTK_STRING;
}

bool vtkVariant::IsNumeric() const
{
  return this->IsFloat() || this->IsDouble() || this->IsChar() || this->IsUnsignedChar() ||
    this->IsSignedChar() || this->IsShort() || this->IsUnsignedShort() || this->IsInt() ||
    this->IsUnsignedInt() || this->IsLong() || this->IsUnsignedLong() || this->IsLongLong() ||
    this->IsUnsignedLongLong();
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
  return this->Type == VTK_OBJECT && this->Data.VTKObject &&
    this->Data.VTKObject->IsA("vtkAbstractArray");
}

unsigned int vtkVariant::GetType() const
{
  return this->Type;
}

const char* vtkVariant::GetTypeAsString() const
{
  if (this->Type == VTK_OBJECT && this->Data.VTKObject)
  {
    return this->Data.VTKObject->GetClassName();
  }
  return vtkImageScalarTypeNameMacro(this->Type);
}

void SetFormattingOnStream(int formatting, std::ostringstream& ostr)
{
  switch (formatting)
  {
    case (vtkVariant::FIXED_FORMATTING):
      ostr << std::fixed;
      return;
    case (vtkVariant::SCIENTIFIC_FORMATTING):
      ostr << std::scientific;
      return;
    case (vtkVariant::DEFAULT_FORMATTING):
      // GCC 4.8.1 does not support std::defaultfloat or std::hexfloat
      VTK_FALLTHROUGH;
    default:
      return;
  }
}

template <typename iterT>
vtkStdString vtkVariantArrayToString(iterT* it, int formatting, int precision)
{
  vtkIdType maxInd = it->GetNumberOfValues();
  std::ostringstream ostr;
  SetFormattingOnStream(formatting, ostr);
  ostr << std::setprecision(precision);

  for (vtkIdType i = 0; i < maxInd; ++i)
  {
    if (i > 0)
    {
      ostr << " ";
    }
    ostr << it->GetValue(i);
  }
  return ostr.str();
}

vtkStdString vtkVariant::ToString(int formatting, int precision) const
{
  if (!this->IsValid())
  {
    return {};
  }
  if (this->IsString())
  {
    return *this->Data.String;
  }
  if (this->IsFloat())
  {
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    SetFormattingOnStream(formatting, ostr);
    ostr << std::setprecision(precision);
    ostr << this->Data.Float;
    return ostr.str();
  }
  if (this->IsDouble())
  {
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    SetFormattingOnStream(formatting, ostr);
    ostr << std::setprecision(precision);
    ostr << this->Data.Double;
    return ostr.str();
  }
  if (this->IsChar())
  {
    std::ostringstream ostr;
    ostr << this->Data.Char;
    return ostr.str();
  }
  if (this->IsUnsignedChar())
  {
    std::ostringstream ostr;
    ostr << static_cast<unsigned int>(this->Data.UnsignedChar);
    return ostr.str();
  }
  if (this->IsSignedChar())
  {
    std::ostringstream ostr;
    ostr << this->Data.SignedChar;
    return ostr.str();
  }
  if (this->IsShort())
  {
    std::ostringstream ostr;
    ostr << this->Data.Short;
    return ostr.str();
  }
  if (this->IsUnsignedShort())
  {
    std::ostringstream ostr;
    ostr << this->Data.UnsignedShort;
    return ostr.str();
  }
  if (this->IsInt())
  {
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.Int;
    return ostr.str();
  }
  if (this->IsUnsignedInt())
  {
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.UnsignedInt;
    return ostr.str();
  }
  if (this->IsLong())
  {
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.Long;
    return ostr.str();
  }
  if (this->IsUnsignedLong())
  {
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.UnsignedLong;
    return ostr.str();
  }
  if (this->IsLongLong())
  {
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.LongLong;
    return ostr.str();
  }
  if (this->IsUnsignedLongLong())
  {
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    ostr << this->Data.UnsignedLongLong;
    return ostr.str();
  }
  if (this->IsArray())
  {
    vtkAbstractArray* arr = vtkAbstractArray::SafeDownCast(this->Data.VTKObject);
    vtkArrayIterator* iter = arr->NewIterator();
    std::string str;
    switch (arr->GetDataType())
    {
      vtkArrayIteratorTemplateMacro(
        str = vtkVariantArrayToString(static_cast<VTK_TT*>(iter), formatting, precision));
    }
    iter->Delete();
    return str;
  }
  vtkGenericWarningMacro(<< "Cannot convert unknown type (" << this->GetTypeAsString()
                         << ") to a string.");
  return {};
}

vtkObjectBase* vtkVariant::ToVTKObject() const
{
  if (this->IsVTKObject())
  {
    return this->Data.VTKObject;
  }
  return nullptr;
}

vtkAbstractArray* vtkVariant::ToArray() const
{
  if (this->IsArray())
  {
    return vtkAbstractArray::SafeDownCast(this->Data.VTKObject);
  }
  return nullptr;
}

template <typename T>
struct isChar : std::false_type
{
};
template <>
struct isChar<char> : std::true_type
{
};
template <>
struct isChar<unsigned char> : std::true_type
{
};
template <>
struct isChar<signed char> : std::true_type
{
};

template <typename T, typename std::enable_if<!isChar<T>::value>::type* = nullptr>
std::size_t vtkVariantStringToNumericInternal(const char* it, const char* end, T& output)
{
  return vtkValueFromString(it, end, output);
}

template <typename T, typename std::enable_if<isChar<T>::value>::type* = nullptr>
std::size_t vtkVariantStringToNumericInternal(const char* it, const char* end, T& output)
{
  if (it != end)
  {
    output = static_cast<T>(*it);
    return 1;
  }

  return 0;
}

template <typename T>
T vtkVariantStringToNumeric(const vtkStdString& str, bool* valid, T* vtkNotUsed(ignored) = nullptr)
{
  auto it = str.data();
  const auto end = str.data() + str.size();

  const auto consumeWhitespaces = [&it, end]() {
    it = std::find_if(it, end, [](char c) { return !std::isspace(static_cast<unsigned char>(c)); });
  };

  // discard leading whitespace (as if done by std::istream::operator>>)
  consumeWhitespaces();

  // Convert value
  T output{};
  const auto consumed = vtkVariantStringToNumericInternal(it, end, output);
  if (consumed == 0) // failed to parse any value
  {
    if (valid)
    {
      *valid = false;
    }

    return output;
  }
  it += consumed;

  // check that we consumed all non-whitespace input
  consumeWhitespaces();

  if (valid)
  {
    *valid = (it == end);
  }

  return output;
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
    return vtkVariantStringToNumeric<T>(*this->Data.String, valid);
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
  if (this->IsLongLong())
  {
    return static_cast<T>(this->Data.LongLong);
  }
  if (this->IsUnsignedLongLong())
  {
    return static_cast<T>(this->Data.UnsignedLongLong);
  }
  // For arrays, convert the first value to the appropriate type.
  if (this->IsArray())
  {
    if (this->Data.VTKObject->IsA("vtkDataArray"))
    {
      // Note: This are not the best conversion.
      //       We convert the first value to double, then
      //       cast it back to the appropriate numeric type.
      vtkDataArray* da = vtkDataArray::SafeDownCast(this->Data.VTKObject);
      if (da->GetNumberOfTuples() > 0)
      {
        return static_cast<T>(da->GetTuple1(0));
      }
    }
    else if (this->Data.VTKObject->IsA("vtkVariantArray"))
    {
      // Note: This are not the best conversion.
      //       We convert the first value to double, then
      //       cast it back to the appropriate numeric type.
      vtkVariantArray* va = vtkVariantArray::SafeDownCast(this->Data.VTKObject);
      if (va->GetNumberOfValues() > 0)
      {
        return static_cast<T>(va->GetValue(0).ToDouble());
      }
    }
    else if (this->Data.VTKObject->IsA("vtkStringArray"))
    {
      vtkStringArray* sa = vtkStringArray::SafeDownCast(this->Data.VTKObject);
      if (sa->GetNumberOfValues() > 0)
      {
        return vtkVariantStringToNumeric<T>(sa->GetValue(0), valid);
      }
    }
  }
  if (valid)
  {
    *valid = false;
  }
  return static_cast<T>(0);
}

//------------------------------------------------------------------------------
// Callers causing implicit instantiations of ToNumeric

float vtkVariant::ToFloat(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<float*>(nullptr));
}

double vtkVariant::ToDouble(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<double*>(nullptr));
}

char vtkVariant::ToChar(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<char*>(nullptr));
}

unsigned char vtkVariant::ToUnsignedChar(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned char*>(nullptr));
}

signed char vtkVariant::ToSignedChar(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<signed char*>(nullptr));
}

short vtkVariant::ToShort(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<short*>(nullptr));
}

unsigned short vtkVariant::ToUnsignedShort(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned short*>(nullptr));
}

int vtkVariant::ToInt(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<int*>(nullptr));
}

unsigned int vtkVariant::ToUnsignedInt(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned int*>(nullptr));
}

long vtkVariant::ToLong(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<long*>(nullptr));
}

unsigned long vtkVariant::ToUnsignedLong(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned long*>(nullptr));
}

long long vtkVariant::ToLongLong(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<long long*>(nullptr));
}

unsigned long long vtkVariant::ToUnsignedLongLong(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<unsigned long long*>(nullptr));
}

vtkTypeInt64 vtkVariant::ToTypeInt64(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<vtkTypeInt64*>(nullptr));
}

vtkTypeUInt64 vtkVariant::ToTypeUInt64(bool* valid) const
{
  return this->ToNumeric(valid, static_cast<vtkTypeUInt64*>(nullptr));
}

bool vtkVariant::IsEqual(const vtkVariant& other) const
{
  return this->operator==(other);
}

ostream& operator<<(ostream& os, const vtkVariant& val)
{
  if (!val.Valid)
  {
    os << "(invalid)";
    return os;
  }
  switch (val.Type)
  {
    case VTK_STRING:
      if (val.Data.String)
      {
        os << "\"" << *val.Data.String << "\"";
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
    case VTK_LONG_LONG:
      os << val.Data.LongLong;
      break;
    case VTK_UNSIGNED_LONG_LONG:
      os << val.Data.UnsignedLongLong;
      break;
    case VTK_OBJECT:
      if (val.Data.VTKObject)
      {
        os << "(" << val.Data.VTKObject->GetClassName() << ")" << hex << val.Data.VTKObject << dec;
      }
      else
      {
        os << "(vtkObjectBase)0x0";
      }
      break;
  }
  return os;
}
VTK_ABI_NAMESPACE_END
