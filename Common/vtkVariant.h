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
// .NAME vtkVariant - A atomic type representing the union of many types
//
// .SECTION Description
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class.


#ifndef __vtkVariant_h
#define __vtkVariant_h

#include "vtkType.h"           // To define type IDs and VTK_TYPE_USE_* flags
#include "vtkSystemIncludes.h" // To define ostream
#include "vtkSetGet.h"         // For vtkNotUsed macro
#include "vtkStdString.h"

//
// The following should be eventually placed in vtkType.h
//

/* These types are required by vtkVariant and vtkVariantArray */
#define VTK_VARIANT 20
#define VTK_OBJECT 21

//
// The following should be eventually placed in vtkSetGet.h
//

// This is same as extended template macro with additional cases for 
// VTK_VARIANT, and VTK_OBJECT.
#define vtkExtraExtendedTemplateMacro(call)                                 \
  vtExtendedTemplateMacro(call);                                            \
  vtkTemplateMacroCase(VTK_VARIANT, vtkVariant, call)                       \
  vtkTemplateMacroCase(VTK_OBJECT, vtkObjectBase, call)

// This is same as Iterator Template macro with additional case for VTK_VARIANT
#define vtkExtendedArrayIteratorTemplateMacro(call)                                      \
  vtkArrayIteratorTemplateMacro(call);                                                   \
  vtkArrayIteratorTemplateMacroCase(VTK_VARIANT, vtkVariant, call);

class vtkStdString;
class vtkObjectBase;
class vtkAbstractArray;

class VTK_COMMON_EXPORT vtkVariant
{
public:

  // Description:
  // Create an invalid variant.
  vtkVariant();

  // Description:
  // Destruct the variant.
  ~vtkVariant();

  // Description:
  // Copy constructor.
  vtkVariant(const vtkVariant & other);

  // Description:
  // Create an integer variant.
  vtkVariant(int value);

  // Description:
  // Create an unsigned integer variant.
  vtkVariant(unsigned int value);

  // Description:
  // Create an long variant.
  vtkVariant(long value);

  // Description:
  // Create an unsigned long variant.
  vtkVariant(unsigned long value);

#if defined(VTK_TYPE_USE___INT64)
  // Description:
  // Create an __int64 variant.
  vtkVariant(__int64 value);
  
  // Description:
  // Create an unsigned __int64 variant.
  vtkVariant(unsigned __int64 value);
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
  // Description:
  // Create a long long variant.
  vtkVariant(long long value);

  // Description:
  // Create an unsigned long long variant.
  vtkVariant(unsigned long long value);
#endif

  // Description:
  // Create a float variant.
  vtkVariant(float value);

  // Description:
  // Create a double variant.
  vtkVariant(double value);

  // Description:
  // Create a string variant from a const char*.
  vtkVariant(const char* value);

  // Description:
  // Create a string variant from a std string.
  vtkVariant(vtkStdString value);

  // Description:
  // Create a vtkObjectBase variant.
  vtkVariant(vtkObjectBase* value);

  // Description:
  // Copy the value of one variant into another.
  const vtkVariant & operator= (const vtkVariant & other);

  // Description:
  // Get whether the variant value is valid.
  bool IsValid() const;

  // Description:
  // Get whether the variant is a string.
  bool IsString() const;

  // Description:
  // Get whether the variant is any numeric type.
  bool IsNumeric() const;

  // Description:
  // Get whether the variant is a float.
  bool IsFloat() const;

  // Description:
  // Get whether the variant is a double.
  bool IsDouble() const;

  // Description:
  // Get whether the variant is an int.
  bool IsInt() const;

  // Description:
  // Get whether the variant is an unsigned int.
  bool IsUnsignedInt() const;

  // Description:
  // Get whether the variant is an long.
  bool IsLong() const;

  // Description:
  // Get whether the variant is an unsigned long.
  bool IsUnsignedLong() const;

  // Description:
  // Get whether the variant is an __int64.
  bool Is__Int64() const;

  // Description:
  // Get whether the variant is an unsigned __int64.
  bool IsUnsigned__Int64() const;

  // Description:
  // Get whether the variant is long long.
  bool IsLongLong() const;

  // Description:
  // Get whether the variant is unsigned long long.
  bool IsUnsignedLongLong() const;

  // Description:
  // Get whether the variant is a VTK object pointer.
  bool IsVTKObject() const;

  // Description:
  // Get whether the variant is a VTK array (i.e. a subclass of vtkAbstractArray).
  bool IsArray() const;

  // Description:
  // Get the type of the variant.
  unsigned int GetType() const;

  // Description:
  // Get the type of the variant as a string.
  const char* GetTypeAsString() const;

  // Description:
  // Convert the variant to a string.
  vtkStdString ToString() const;

  // Description:
  // Convert the variant to a numeric type:
  // If it holds a numeric, cast to the appropriate type.
  // If it holds a string, attempt to convert the string to the appropriate type;
  //   set the valid flag to false when the conversion fails.
  // If it holds an array type, cast the first value of the array
  //   to the appropriate type.
  // Fail if it holds a VTK object which is not an array.
  float ToFloat(bool* valid = 0) const;
  double ToDouble(bool* valid = 0) const;
  int ToInt(bool* valid = 0) const;
  unsigned int ToUnsignedInt(bool* valid = 0) const;
  long ToLong(bool* valid = 0) const;
  unsigned long ToUnsignedLong(bool* valid = 0) const;
#if defined(VTK_TYPE_USE___INT64)
  __int64 To__Int64(bool* valid = 0) const;
  unsigned __int64 ToUnsigned__Int64(bool* valid = 0) const;
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
  long long ToLongLong(bool* valid = 0) const;
  unsigned long long ToUnsignedLongLong(bool* valid = 0) const;
#endif
  vtkTypeInt64 ToTypeInt64(bool* valid = 0) const;
  vtkTypeUInt64 ToTypeUInt64(bool* valid = 0) const;

  // Description:
  // Return the VTK object, or NULL if not of that type.
  vtkObjectBase* ToVTKObject() const;

  // Description:
  // Return the array, or NULL if not of that type.
  vtkAbstractArray* ToArray() const;

private:
  template <typename T>
  T ToNumeric(bool* valid, T* vtkNotUsed(ignored)) const;

  union
  {
    vtkStdString* String;
    float Float;
    double Double;
    int Int;
    unsigned int UnsignedInt;
    long Long;
    unsigned long UnsignedLong;
#if defined(VTK_TYPE_USE___INT64)
    __int64 __Int64;
    unsigned __int64 Unsigned__Int64;
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
    long long LongLong;
    unsigned long long UnsignedLongLong;
#endif
    vtkObjectBase* VTKObject;
  } Data;

  unsigned char Valid;
  unsigned char Type;
};

#endif
