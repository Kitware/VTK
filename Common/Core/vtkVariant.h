// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkVariant
 * @brief   A type representing the union of many types.
 *
 * @par Thanks:
 * Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
 * Sandia National Laboratories for their help in developing this class.
 */

#ifndef vtkVariant_h
#define vtkVariant_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"           // For vtkObject's warning support
#include "vtkSetGet.h"           // For vtkNotUsed macro
#include "vtkStdString.h"
#include "vtkSystemIncludes.h" // To define ostream
#include "vtkType.h"           // To define type IDs and VTK_TYPE_USE_* flags

//
// The following should be eventually placed in vtkSetGet.h
//

// This is same as extended template macro with an additional case for VTK_VARIANT
#define vtkExtraExtendedTemplateMacro(call)                                                        \
  vtkExtendedTemplateMacro(call);                                                                  \
  vtkTemplateMacroCase(VTK_VARIANT, vtkVariant, call)

// This is same as Iterator Template macro with an additional case for VTK_VARIANT
#define vtkExtendedArrayIteratorTemplateMacro(call)                                                \
  vtkArrayIteratorTemplateMacro(call);                                                             \
  vtkArrayIteratorTemplateMacroCase(VTK_VARIANT, vtkVariant, call)

VTK_ABI_NAMESPACE_BEGIN
class vtkStdString;
class vtkObjectBase;
class vtkAbstractArray;
class vtkVariant;
struct vtkVariantLessThan;

VTKCOMMONCORE_EXPORT ostream& operator<<(ostream& os, const vtkVariant& val);

class VTKCOMMONCORE_EXPORT vtkVariant
{
public:
  /**
   * Create an invalid variant. The type will be VTK_VOID and IsValid will return false.
   */
  vtkVariant();

  /**
   * Destruct the variant.
   * For the VTK_STRING case, invokes delete on the string.
   * For the VTK_OBJECT case, invokes Delete() on the object.
   */
  ~vtkVariant();

  /**
   * Copy constructor.
   */
  vtkVariant(const vtkVariant& other);

  /**
   * Create a bool variant. Internally store it as char. The type will be VTK_CHAR and IsValid will
   * return true.
   */
  vtkVariant(bool value);

  /**
   * Create a char variant. The type will be VTK_CHAR and IsValid will return true.
   */
  vtkVariant(char value);

  /**
   * Create an unsigned char variant. The type will be VTK_UNSIGNED_CHAR and IsValid will return
   * true.
   */
  vtkVariant(unsigned char value);

  /**
   * Create a signed char variant. The type will be VTK_SIGNED_CHAR and IsValid will return true.
   */
  vtkVariant(signed char value);

  /**
   * Create a short variant. The type will be VTK_SHORT and IsValid will return true.
   */
  vtkVariant(short value);

  /**
   * Create an unsigned short variant. The type will be VTK_UNSIGNED_SHORT and IsValid will return
   * true.
   */
  vtkVariant(unsigned short value);

  /**
   * Create an integer variant. The type will be VTK_INT and IsValid will return true.
   */
  vtkVariant(int value);

  /**
   * Create an unsigned integer variant. The type will be VTK_UNSIGNED_INT and IsValid will return
   * true.
   */
  vtkVariant(unsigned int value);

  /**
   * Create an long variant. The type will be VTK_LONG and IsValid will return true.
   */
  vtkVariant(long value);

  /**
   * Create an unsigned long variant. The type will be VTK_UNSIGNED_LONG and IsValid will return
   * true.
   */
  vtkVariant(unsigned long value);

  /**
   * Create a long long variant. The type will be VTK_LONG_LONG and IsValid will return true.
   */
  vtkVariant(long long value);

  /**
   * Create an unsigned long long variant. The type will be VTK_UNSIGNED_LONG_LONG and IsValid will
   * return true.
   */
  vtkVariant(unsigned long long value);

  /**
   * Create a float variant. The type will be VTK_FLOAT and IsValid will return true.
   */
  vtkVariant(float value);

  /**
   * Create a double variant. The type will be VTK_DOUBLE and IsValid will return true.
   */
  vtkVariant(double value);

  /**
   * Create a string variant from a const char*.
   * If nullptr is passed, the type will be VTK_VOID and IsValid will return false;
   * else the type will be VTK_STRING and IsValid will return true.
   */
  vtkVariant(const char* value);

  /**
   * Create a string variant from a std string.
   * The type will be VTK_STRING and IsValid will return true.
   */
  vtkVariant(vtkStdString value);

  /**
   * Create a vtkObjectBase variant.
   * If nullptr is passed, the type will be VTK_VOID and IsValid will return false;
   * else the type will be VTK_OBJECT and IsValid will return true.
   */
  vtkVariant(vtkObjectBase* value);

  /**
   * Create a new variant by copying the given variant but converting it to the given type.
   * \p type must be one of: VTK_STRING, VTK_OBJECT, VTK_CHAR, VTK_SIGNED_CHAR, VTK_UNSIGNED_CHAR,
   * VTK_SHORT, VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG, VTK_UNSIGNED_LONG,
   * VTK_LONG_LONG, VTK_UNSIGNED_LONG_LONG, VTK_FLOAT, VTK_DOUBLE.
   */
  vtkVariant(const vtkVariant& other, unsigned int type);

  /**
   * Copy the value of one variant into another.
   */
  vtkVariant& operator=(const vtkVariant& other);

  /**
   * Get whether the variant value is valid. Simple scalar types are always considered valid.
   * Strings and pointers are considered valid only if non-nullptr.
   */
  bool IsValid() const;

  /**
   * Get whether the variant is a string.
   */
  bool IsString() const;

  /**
   * Get whether the variant is any numeric type.
   */
  bool IsNumeric() const;

  /**
   * Get whether the variant is a float.
   */
  bool IsFloat() const;

  /**
   * Get whether the variant is a double.
   */
  bool IsDouble() const;

  /**
   * Get whether the variant is an char.
   */
  bool IsChar() const;

  /**
   * Get whether the variant is an unsigned char.
   */
  bool IsUnsignedChar() const;

  /**
   * Get whether the variant is an signed char.
   */
  bool IsSignedChar() const;

  /**
   * Get whether the variant is an short.
   */
  bool IsShort() const;

  /**
   * Get whether the variant is an unsigned short.
   */
  bool IsUnsignedShort() const;

  /**
   * Get whether the variant is an int.
   */
  bool IsInt() const;

  /**
   * Get whether the variant is an unsigned int.
   */
  bool IsUnsignedInt() const;

  /**
   * Get whether the variant is an long.
   */
  bool IsLong() const;

  /**
   * Get whether the variant is an unsigned long.
   */
  bool IsUnsignedLong() const;

  /**
   * Get whether the variant is long long.
   */
  bool IsLongLong() const;

  /**
   * Get whether the variant is unsigned long long.
   */
  bool IsUnsignedLongLong() const;

  /**
   * Get whether the variant is a VTK object pointer (i.e. vtkObjectBase or a subclass thereof).
   */
  bool IsVTKObject() const;

  /**
   * Get whether the variant is a VTK array (i.e. vtkAbstractArray or a subclass thereof).
   */
  bool IsArray() const;

  /**
   * Get the type of the variant.
   */
  unsigned int GetType() const;

  /**
   * Get the type of the variant as a string.
   */
  const char* GetTypeAsString() const;

  enum StringFormatting
  {
    DEFAULT_FORMATTING = 0,
    FIXED_FORMATTING = 1,
    SCIENTIFIC_FORMATTING = 2
  };

  /**
   * Convert the variant to a string.
   * Set the formatting argument to either DEFAULT_FORMATTING, FIXED_FORMATTING,
   * SCIENTIFIC_FORMATTING to control the formatting. Set the precision
   * argument to control the precision of the output. These two parameters have no effect when the
   * variant is not a floating-point value or an array of floating-point values.
   * See the std doc for more information.
   */
  vtkStdString ToString(int formatting = DEFAULT_FORMATTING, int precision = 6) const;

  ///@{
  /**
   * Convert the variant to a numeric type:
   * If it holds a numeric, cast to the appropriate type.
   * If it holds a string, attempt to convert the string to the appropriate type;
   * set the valid flag to false when the conversion fails.
   * If it holds an array type, cast the first value of the array
   * to the appropriate type.
   * Fail if it holds a VTK object which is not an array.
   */
  float ToFloat(bool* valid) const;
  float ToFloat() const { return this->ToFloat(nullptr); }
  double ToDouble(bool* valid) const;
  double ToDouble() const { return this->ToDouble(nullptr); }
  char ToChar(bool* valid) const;
  char ToChar() const { return this->ToChar(nullptr); }
  unsigned char ToUnsignedChar(bool* valid) const;
  unsigned char ToUnsignedChar() const { return this->ToUnsignedChar(nullptr); }
  signed char ToSignedChar(bool* valid) const;
  signed char ToSignedChar() const { return this->ToSignedChar(nullptr); }
  short ToShort(bool* valid) const;
  short ToShort() const { return this->ToShort(nullptr); }
  unsigned short ToUnsignedShort(bool* valid) const;
  unsigned short ToUnsignedShort() const { return this->ToUnsignedShort(nullptr); }
  int ToInt(bool* valid) const;
  int ToInt() const { return this->ToInt(nullptr); }
  unsigned int ToUnsignedInt(bool* valid) const;
  unsigned int ToUnsignedInt() const { return this->ToUnsignedInt(nullptr); }
  long ToLong(bool* valid) const;
  long ToLong() const { return this->ToLong(nullptr); }
  unsigned long ToUnsignedLong(bool* valid) const;
  unsigned long ToUnsignedLong() const { return this->ToUnsignedLong(nullptr); }
  long long ToLongLong(bool* valid) const;
  long long ToLongLong() const { return this->ToLongLong(nullptr); }
  unsigned long long ToUnsignedLongLong(bool* valid) const;
  unsigned long long ToUnsignedLongLong() const { return this->ToUnsignedLongLong(nullptr); }
  vtkTypeInt64 ToTypeInt64(bool* valid) const;
  vtkTypeInt64 ToTypeInt64() const { return this->ToTypeInt64(nullptr); }
  vtkTypeUInt64 ToTypeUInt64(bool* valid) const;
  vtkTypeUInt64 ToTypeUInt64() const { return this->ToTypeUInt64(nullptr); }
  ///@}

  /**
   * Return the VTK object, or nullptr if not of that type.
   */
  vtkObjectBase* ToVTKObject() const;

  /**
   * Return the array, or nullptr if not of that type.
   */
  vtkAbstractArray* ToArray() const;

  /**
   * Determines whether two variants have the same value. They do
   * not need to be storing exactly the same type to have the same
   * value.  In practice you don't need to use this method: just use
   * operator== instead.  If you want precise equality down to the bit
   * level use the following idiom:

   * vtkVariantStrictEquality comparator;
   * bool variantsEqual = comparator(firstVariant, secondVariant);
   */
  bool IsEqual(const vtkVariant& other) const;

  ///@{
  /**
   * Compare two variants for equality, greater than, and less than.
   * These operators use the value represented by the variant instead
   * of the particular type/bit pattern used to represent it.  This
   * behavior is similar to the default behavior in C and C++,
   * including type promotion, with the following caveats:

   * * When comparing type X with a string, type X will first be
   * converted to string, then compared lexically (the usual
   * behavior of string::operator< and company).

   * * vtkObject pointers will be converted to an unsigned integer of
   * appropriate size.  If both variants contain vtkObjects then
   * they are comparable directly.

   * * Comparing char values with strings will not work the way you
   * might expect if you're treating a char as a numeric type.  Char
   * values are written to strings as literal ASCII characters
   * instead of numbers.

   * This approach follows the principle of least surprise at the
   * expense of speed.  Casting integers to floating-point values is
   * relatively slow.  Casting numeric types to strings is very slow.
   * If you prefer speed at the expense of counterintuitive behavior
   * -- for example, when using vtkVariants as keys in STL containers
   * -- you can use the functors described at the bottom of this file.

   * The actual definitions of these operators are in
   * vtkVariantInlineOperators.cxx.
   */
  bool operator==(const vtkVariant& other) const;
  bool operator!=(const vtkVariant& other) const;
  bool operator<(const vtkVariant& other) const;
  bool operator>(const vtkVariant& other) const;
  bool operator<=(const vtkVariant& other) const;
  bool operator>=(const vtkVariant& other) const;
  ///@}

  friend VTKCOMMONCORE_EXPORT ostream& operator<<(ostream& os, const vtkVariant& val);

private:
  template <typename T>
  T ToNumeric(bool* valid, T* vtkNotUsed(ignored)) const;

  union
  {
    vtkStdString* String;
    float Float;
    double Double;
    char Char;
    unsigned char UnsignedChar;
    signed char SignedChar;
    short Short;
    unsigned short UnsignedShort;
    int Int;
    unsigned int UnsignedInt;
    long Long;
    unsigned long UnsignedLong;
    long long LongLong;
    unsigned long long UnsignedLongLong;
    vtkObjectBase* VTKObject;
  } Data;

  bool Valid;
  unsigned int Type;

  friend struct vtkVariantLessThan;
  friend struct vtkVariantEqual;
  friend struct vtkVariantStrictWeakOrder;
  friend struct vtkVariantStrictEquality;
};

VTK_ABI_NAMESPACE_END
#include "vtkVariantInlineOperators.h" // needed for operator== and company

// A STL-style function object so you can compare two variants using
// comp(s1,s2) where comp is an instance of vtkVariantStrictWeakOrder.
// This is a faster version of operator< that makes no attempt to
// compare values.  It satisfies the STL requirement for a comparison
// function for ordered containers like map and set.

VTK_ABI_NAMESPACE_BEGIN
struct VTKCOMMONCORE_EXPORT vtkVariantLessThan
{
public:
  bool operator()(const vtkVariant& s1, const vtkVariant& s2) const;
};

struct VTKCOMMONCORE_EXPORT vtkVariantEqual
{
public:
  bool operator()(const vtkVariant& s1, const vtkVariant& s2) const;
};

struct VTKCOMMONCORE_EXPORT vtkVariantStrictWeakOrder
{
public:
  bool operator()(const vtkVariant& s1, const vtkVariant& s2) const;
};

// Similarly, this is a fast version of operator== that requires that
// the types AND the values be equal in order to admit equality.

struct VTKCOMMONCORE_EXPORT vtkVariantStrictEquality
{
public:
  bool operator()(const vtkVariant& s1, const vtkVariant& s2) const;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkVariant.h
