/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariant.h

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
/**
 * @class   vtkVariant
 * @brief   A atomic type representing the union of many types
 *
 *
 *
 * @par Thanks:
 * Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
 * Sandia National Laboratories for their help in developing this class.
*/

#ifndef vtkVariant_h
#define vtkVariant_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkType.h"           // To define type IDs and VTK_TYPE_USE_* flags
#include "vtkSystemIncludes.h" // To define ostream
#include "vtkSetGet.h"         // For vtkNotUsed macro
#include "vtkObject.h"         // For vtkObject's warning support
#include "vtkStdString.h"
#include "vtkUnicodeString.h"

//
// The following should be eventually placed in vtkSetGet.h
//

// This is same as extended template macro with an additional case for VTK_VARIANT
#define vtkExtraExtendedTemplateMacro(call)                                 \
  vtkExtendedTemplateMacro(call);                                            \
  vtkTemplateMacroCase(VTK_VARIANT, vtkVariant, call)

// This is same as Iterator Template macro with an additional case for VTK_VARIANT
#define vtkExtendedArrayIteratorTemplateMacro(call)                                      \
  vtkArrayIteratorTemplateMacro(call);                                                   \
  vtkArrayIteratorTemplateMacroCase(VTK_VARIANT, vtkVariant, call);

class vtkStdString;
class vtkUnicodeString;
class vtkObjectBase;
class vtkAbstractArray;
class vtkVariant;
struct vtkVariantLessThan;

VTKCOMMONCORE_EXPORT ostream& operator << ( ostream& os, const vtkVariant& val );

class VTKCOMMONCORE_EXPORT vtkVariant
{
public:

  /**
   * Create an invalid variant.
   */
  vtkVariant();

  /**
   * Destruct the variant.
   */
  ~vtkVariant();

  /**
   * Copy constructor.
   */
  vtkVariant(const vtkVariant & other);

  /**
   * Create a bool variant. Internally store it as char.
   */
  vtkVariant(bool value);

  /**
   * Create a char variant.
   */
  vtkVariant(char value);

  /**
   * Create an unsigned char variant.
   */
  vtkVariant(unsigned char value);

  /**
   * Create a signed char variant.
   */
  vtkVariant(signed char value);

  /**
   * Create a short variant.
   */
  vtkVariant(short value);

  /**
   * Create an unsigned short variant.
   */
  vtkVariant(unsigned short value);

  /**
   * Create an integer variant.
   */
  vtkVariant(int value);

  /**
   * Create an unsigned integer variant.
   */
  vtkVariant(unsigned int value);

  /**
   * Create an long variant.
   */
  vtkVariant(long value);

  /**
   * Create an unsigned long variant.
   */
  vtkVariant(unsigned long value);

  /**
   * Create a long long variant.
   */
  vtkVariant(long long value);

  /**
   * Create an unsigned long long variant.
   */
  vtkVariant(unsigned long long value);

  /**
   * Create a float variant.
   */
  vtkVariant(float value);

  /**
   * Create a double variant.
   */
  vtkVariant(double value);

  /**
   * Create a string variant from a const char*.
   */
  vtkVariant(const char* value);

  /**
   * Create a string variant from a std string.
   */
  vtkVariant(vtkStdString value);

  /**
   * Create a Unicode string variant
   */
  vtkVariant(const vtkUnicodeString& value);

  /**
   * Create a vtkObjectBase variant.
   */
  vtkVariant(vtkObjectBase* value);

  /**
   * Create a variant of a specific type.
   */
  vtkVariant(const vtkVariant &other, unsigned int type);

  /**
   * Copy the value of one variant into another.
   */
  vtkVariant & operator= (const vtkVariant & other);

  /**
   * Get whether the variant value is valid.
   */
  bool IsValid() const;

  /**
   * Get whether the variant is a string.
   */
  bool IsString() const;

  /**
   * Get whether the variant is a Unicode string.
   */
  bool IsUnicodeString() const;

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
   * Legacy.  Returns false.  The variant is never an __int64.
   */
  bool Is__Int64() const;

  /**
   * Legacy.  Returns false.  The variant is never an unsigned __int64.
   */
  bool IsUnsigned__Int64() const;

  /**
   * Get whether the variant is long long.
   */
  bool IsLongLong() const;

  /**
   * Get whether the variant is unsigned long long.
   */
  bool IsUnsignedLongLong() const;

  /**
   * Get whether the variant is a VTK object pointer.
   */
  bool IsVTKObject() const;

  /**
   * Get whether the variant is a VTK array (i.e. a subclass of vtkAbstractArray).
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

  /**
   * Convert the variant to a string.
   */
  vtkStdString ToString() const;

  /**
   * convert the variant to a Unicode string.
   */
  vtkUnicodeString ToUnicodeString() const;

  //@{
  /**
   * Convert the variant to a numeric type:
   * If it holds a numeric, cast to the appropriate type.
   * If it holds a string, attempt to convert the string to the appropriate type;
   * set the valid flag to false when the conversion fails.
   * If it holds an array type, cast the first value of the array
   * to the appropriate type.
   * Fail if it holds a VTK object which is not an array.
   */
  float ToFloat(bool *valid) const;
  float ToFloat() const {
    return this->ToFloat(nullptr); };
  double ToDouble(bool *valid) const;
  double ToDouble() const {
    return this->ToDouble(nullptr); };
  char ToChar(bool *valid) const;
  char ToChar() const {
    return this->ToChar(nullptr); };
  unsigned char ToUnsignedChar(bool *valid) const;
  unsigned char ToUnsignedChar() const {
    return this->ToUnsignedChar(nullptr); };
  signed char ToSignedChar(bool *valid) const;
  signed char ToSignedChar() const {
    return this->ToSignedChar(nullptr); };
  short ToShort(bool *valid) const;
  short ToShort() const {
    return this->ToShort(nullptr); };
  unsigned short ToUnsignedShort(bool *valid) const;
  unsigned short ToUnsignedShort() const {
    return this->ToUnsignedShort(nullptr); };
  int ToInt(bool *valid) const;
  int ToInt() const {
    return this->ToInt(nullptr); };
  unsigned int ToUnsignedInt(bool *valid) const;
  unsigned int ToUnsignedInt() const {
    return this->ToUnsignedInt(nullptr); };
  long ToLong(bool *valid) const;
  long ToLong() const {
    return this->ToLong(nullptr); };
  unsigned long ToUnsignedLong(bool *valid) const;
  unsigned long ToUnsignedLong() const {
    return this->ToUnsignedLong(nullptr); };
  long long ToLongLong(bool *valid) const;
  long long ToLongLong() const {
    return this->ToLongLong(nullptr); };
  unsigned long long ToUnsignedLongLong(bool *valid) const;
  unsigned long long ToUnsignedLongLong() const {
    return this->ToUnsignedLongLong(nullptr); };
  vtkTypeInt64 ToTypeInt64(bool *valid) const;
  vtkTypeInt64 ToTypeInt64() const {
    return this->ToTypeInt64(nullptr); };
  vtkTypeUInt64 ToTypeUInt64(bool *valid) const;
  vtkTypeUInt64 ToTypeUInt64() const {
    return this->ToTypeUInt64(nullptr); };
  //@}

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

  //@{
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
  bool operator==(const vtkVariant &other) const;
  bool operator!=(const vtkVariant &other) const;
  bool operator<(const vtkVariant &other) const;
  bool operator>(const vtkVariant &other) const;
  bool operator<=(const vtkVariant &other) const;
  bool operator>=(const vtkVariant &other) const;
  //@}

  friend VTKCOMMONCORE_EXPORT ostream& operator << ( ostream& os, const vtkVariant& val );

private:

  template <typename T>
  T ToNumeric(bool *valid, T* vtkNotUsed(ignored)) const;

  union
  {
    vtkStdString* String;
    vtkUnicodeString* UnicodeString;
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

  unsigned char Valid;
  unsigned char Type;

  friend struct vtkVariantLessThan;
  friend struct vtkVariantEqual;
  friend struct vtkVariantStrictWeakOrder;
  friend struct vtkVariantStrictEquality;

};

#include "vtkVariantInlineOperators.h" // needed for operator== and company

// A STL-style function object so you can compare two variants using
// comp(s1,s2) where comp is an instance of vtkVariantStrictWeakOrder.
// This is a faster version of operator< that makes no attempt to
// compare values.  It satisfies the STL requirement for a comparison
// function for ordered containers like map and set.

struct VTKCOMMONCORE_EXPORT vtkVariantLessThan
{
public:
  bool operator()(const vtkVariant &s1, const vtkVariant &s2) const;
};

struct VTKCOMMONCORE_EXPORT vtkVariantEqual
{
public:
  bool operator()(const vtkVariant &s1, const vtkVariant &s2) const;
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
  bool operator()(const vtkVariant &s1, const vtkVariant &s2) const;
};

#endif
// VTK-HeaderTest-Exclude: vtkVariant.h
