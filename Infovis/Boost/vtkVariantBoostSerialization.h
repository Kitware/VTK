/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariantBoostSerialization.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
// .NAME vtkVariantBoostSerialization - Serialization support for
// vtkVariant and vtkVariantArray using the Boost.Serialization
// library.
//
// .SECTION Description
// The header includes the templates required to serialize the
// vtkVariant and vtkVariantArray with the Boost.Serialization
// library. Just including the header suffices to get serialization
// support; no other action is needed.

#ifndef __vtkVariantBoostSerialization_h
#define __vtkVariantBoostSerialization_h

#include "vtkSetGet.h"
#include "vtkType.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

// This include fixes header-ordering issues in Boost.Serialization
// prior to Boost 1.35.0.
#include <boost/archive/binary_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/extended_type_info_no_rtti.hpp>
#include <boost/serialization/split_free.hpp>

//----------------------------------------------------------------------------
// vtkStdString serialization code
//----------------------------------------------------------------------------
template<typename Archiver>
void serialize(Archiver& ar, vtkStdString& str,
               const unsigned int vtkNotUsed(version))
{
  ar & boost::serialization::base_object<std::string>(str);
}

//----------------------------------------------------------------------------
// vtkVariant serialization code
//----------------------------------------------------------------------------

template<typename Archiver>
void save(Archiver& ar, const vtkVariant& variant,
          const unsigned int vtkNotUsed(version))
{
  if (!variant.IsValid())
    {
    char null = 0;
    ar & null;
    return;
    }

  // Output the type
  char Type = variant.GetType();
  ar & Type;

  // Output the value
#define VTK_VARIANT_SAVE(Value,Type,Function)   \
   case Value:                                  \
     {                                          \
       Type value = variant.Function();         \
       ar & value;                              \
     }                                          \
     return

  switch (Type)
    {
    VTK_VARIANT_SAVE(VTK_STRING,vtkStdString,ToString);
    VTK_VARIANT_SAVE(VTK_FLOAT,float,ToFloat);
    VTK_VARIANT_SAVE(VTK_DOUBLE,double,ToDouble);
    VTK_VARIANT_SAVE(VTK_CHAR,char,ToChar);
    VTK_VARIANT_SAVE(VTK_UNSIGNED_CHAR,unsigned char,ToUnsignedChar);
    VTK_VARIANT_SAVE(VTK_SHORT,short,ToShort);
    VTK_VARIANT_SAVE(VTK_UNSIGNED_SHORT,unsigned short,ToUnsignedShort);
    VTK_VARIANT_SAVE(VTK_INT,int,ToInt);
    VTK_VARIANT_SAVE(VTK_UNSIGNED_INT,unsigned int,ToUnsignedInt);
    VTK_VARIANT_SAVE(VTK_LONG,long,ToLong);
    VTK_VARIANT_SAVE(VTK_UNSIGNED_LONG,unsigned long,ToUnsignedLong);
#if defined(VTK_TYPE_USE___INT64)
    VTK_VARIANT_SAVE(VTK___INT64,vtkTypeInt64,ToTypeInt64);
    VTK_VARIANT_SAVE(VTK_UNSIGNED___INT64,vtkTypeUInt64,ToTypeUInt64);
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
    VTK_VARIANT_SAVE(VTK_LONG_LONG,long long,ToLongLong);
    VTK_VARIANT_SAVE(VTK_UNSIGNED_LONG_LONG,unsigned long long,
                     ToUnsignedLongLong);
#endif
    default:
      cerr << "cannot serialize variant with type " << variant.GetType()
           << '\n';
    }
#undef VTK_VARIANT_SAVE
}

template<typename Archiver>
void load(Archiver& ar, vtkVariant& variant,
          const unsigned int vtkNotUsed(version))
{
  char Type;
  ar & Type;

#define VTK_VARIANT_LOAD(Value,Type)            \
    case Value:                                 \
      {                                         \
        Type value;                             \
        ar & value;                             \
        variant = vtkVariant(value);            \
      }                                         \
      return

  switch (Type)
    {
    case 0: variant = vtkVariant(); return;
    VTK_VARIANT_LOAD(VTK_STRING,vtkStdString);
    VTK_VARIANT_LOAD(VTK_FLOAT,float);
    VTK_VARIANT_LOAD(VTK_DOUBLE,double);
    VTK_VARIANT_LOAD(VTK_CHAR,char);
    VTK_VARIANT_LOAD(VTK_UNSIGNED_CHAR,unsigned char);
    VTK_VARIANT_LOAD(VTK_SHORT,short);
    VTK_VARIANT_LOAD(VTK_UNSIGNED_SHORT,unsigned short);
    VTK_VARIANT_LOAD(VTK_INT,int);
    VTK_VARIANT_LOAD(VTK_UNSIGNED_INT,unsigned int);
    VTK_VARIANT_LOAD(VTK_LONG,long);
    VTK_VARIANT_LOAD(VTK_UNSIGNED_LONG,unsigned long);
#if defined(VTK_TYPE_USE___INT64)
    VTK_VARIANT_LOAD(VTK___INT64,vtkTypeInt64);
    VTK_VARIANT_LOAD(VTK_UNSIGNED___INT64,vtkTypeUInt64);
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
    VTK_VARIANT_LOAD(VTK_LONG_LONG,long long);
    VTK_VARIANT_LOAD(VTK_UNSIGNED_LONG_LONG,unsigned long long);
#endif
    default:
      cerr << "cannot deserialize variant with type " << Type << '\n';
      variant = vtkVariant();
    }
#undef VTK_VARIANT_LOAD
}

BOOST_SERIALIZATION_SPLIT_FREE(vtkVariant)

//----------------------------------------------------------------------------
// vtkVariantArray serialization code
//----------------------------------------------------------------------------

template<typename Archiver>
void save(Archiver& ar, const vtkVariantArray& c_array,
          const unsigned int vtkNotUsed(version))
{
  vtkVariantArray& array = const_cast<vtkVariantArray&>(c_array);

  // Array name
  vtkStdString name;
  if(array.GetName()!=NULL) name=array.GetName();
  ar & name;

  // Array data
  vtkIdType n = array.GetNumberOfTuples();
  ar & n;
  for (vtkIdType i = 0; i < n; ++i)
    {
    ar & array.GetValue(i);
    }
}

template<typename Archiver>
void load(Archiver& ar, vtkVariantArray& array,
          const unsigned int vtkNotUsed(version))
{
  // Array name
  vtkStdString name;
  ar & name;
  array.SetName(name.c_str());

  if(name.empty())
    {
    array.SetName(0);
    }
  else
    {
    array.SetName(name.c_str());
    }

  // Array data
  vtkIdType n;
  ar & n;
  array.SetNumberOfTuples(n);
  vtkVariant value;
  for (vtkIdType i = 0; i < n; ++i)
    {
    ar & value;
    array.SetValue(i, value);
    }
}

BOOST_SERIALIZATION_SPLIT_FREE(vtkVariantArray)

#endif
