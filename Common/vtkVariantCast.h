/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariantCast.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkVariantCast_h
#define __vtkVariantCast_h


// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

// Description:
// Performs an explicit conversion from a vtkVariant to the type that it contains.  Implicit
// conversions are *not* performed, so casting a vtkVariant containing one type (e.g. double)
// to a different type (e.g. string) will not convert between types.
//
// The optional 'valid' flag can be used by callers to verify whether conversion succeeded.
template<typename T>
T vtkVariantCast(const vtkVariant& value, bool* valid = 0)
{
  vtkGenericWarningMacro(<< "cannot cast vtkVariant containing " << value.GetTypeAsString() << " to unsupported type.");

  if(valid)
    *valid = false;
  
  static T dummy;
  return dummy;
}

template<>
inline char vtkVariantCast<char>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsChar();

  return value.IsChar() ? value.ToChar() : 0;
}

template<>
inline unsigned char vtkVariantCast<unsigned char>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsUnsignedChar();

  return value.IsUnsignedChar() ? value.ToUnsignedChar() : 0;
}

template<>
inline short vtkVariantCast<short>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsShort();

  return value.IsShort() ? value.ToShort() : 0;
}

template<>
inline unsigned short vtkVariantCast<unsigned short>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsUnsignedShort();

  return value.IsUnsignedShort() ? value.ToUnsignedShort() : 0;
}

template<>
inline int vtkVariantCast<int>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsInt();

  return value.IsInt() ? value.ToInt() : 0;
}

template<>
inline unsigned int vtkVariantCast<unsigned int>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsUnsignedInt();

  return value.IsUnsignedInt() ? value.ToUnsignedInt() : 0;
}

template<>
inline long vtkVariantCast<long>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsLong();

  return value.IsLong() ? value.ToLong() : 0;
}

template<>
inline unsigned long vtkVariantCast<unsigned long>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsUnsignedLong();

  return value.IsUnsignedLong() ? value.ToUnsignedLong() : 0;
}

#ifdef VTK_TYPE_USE___INT64

template<>
inline __int64 vtkVariantCast<__int64>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.Is__Int64();

  return value.Is__Int64() ? value.To__Int64() : 0;
}

template<>
inline unsigned __int64 vtkVariantCast<unsigned __int64>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsUnsigned__Int64();

  return value.IsUnsigned__Int64() ? value.ToUnsigned__Int64() : 0;
}

#endif


#ifdef VTK_TYPE_USE_LONG_LONG

template<>
inline long long vtkVariantCast<long long>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsLongLong();
    
  return value.IsLongLong() ? value.ToLongLong() : 0;
}

template<>
inline unsigned long long vtkVariantCast<unsigned long long>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsUnsignedLongLong();

  return value.IsUnsignedLongLong() ? value.ToUnsignedLongLong() : 0;
}

#endif

template<>
inline float vtkVariantCast<float>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsFloat();

  return value.IsFloat() ? value.ToFloat() : 0.0f;
}

template<>
inline double vtkVariantCast<double>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsDouble();

  return value.IsDouble() ? value.ToDouble() : 0.0;
}

template<>
inline vtkStdString vtkVariantCast<vtkStdString>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = value.IsString();

  return value.IsString() ? value.ToString() : vtkStdString();
}

template<>
inline vtkVariant vtkVariantCast<vtkVariant>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = true;

  return value;
}

#endif

