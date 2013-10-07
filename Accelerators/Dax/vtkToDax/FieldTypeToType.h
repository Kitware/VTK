//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkToDax_FieldTypeToType_h
#define vtkToDax_FieldTypeToType_h

#include <stdint.h>

class vtkIntArray;
class vtkIdTypeArray;
class vtkFloatArray;
class vtkUnsignedCharArray;

#include <dax/Types.h>

namespace vtkToDax
{

template<typename T, int NUM_COMP> struct FieldTypeToType;

template<int NUM_COMP>
struct FieldTypeToType<vtkIntArray,NUM_COMP>
{
  static const int NUM_COMPNENTS = NUM_COMP;
  typedef dax::Tuple<dax::Id,NUM_COMPNENTS> FieldType;
  typedef dax::Id ComponentType;
};

template<>
struct FieldTypeToType<vtkIntArray,1>
{
  static const int NUM_COMPNENTS = 1;
  typedef dax::Id FieldType;
  typedef dax::Id ComponentType;
};

template<int NUM_COMP>
struct FieldTypeToType<vtkIdTypeArray,NUM_COMP>
{
  static const int NUM_COMPNENTS = NUM_COMP;
  typedef dax::Tuple<dax::Id,NUM_COMPNENTS> FieldType;
  typedef dax::Id ComponentType;
};

template<>
struct FieldTypeToType<vtkIdTypeArray,1>
{
  static const int NUM_COMPNENTS = 1;
  typedef dax::Id FieldType;
  typedef dax::Id ComponentType;
};


template<int NUM_COMP>
struct FieldTypeToType<vtkFloatArray,NUM_COMP>
{
  static const int NUM_COMPNENTS = NUM_COMP;
  typedef dax::Tuple<dax::Scalar,NUM_COMPNENTS> FieldType;
  typedef dax::Scalar ComponentType;
};

template<>
struct FieldTypeToType<vtkFloatArray,1>
{
  static const int NUM_COMPNENTS = 1;
  typedef dax::Scalar FieldType;
  typedef dax::Scalar ComponentType;
};

template<int NUM_COMP>
struct FieldTypeToType<vtkUnsignedCharArray,NUM_COMP>
{
  static const int NUM_COMPNENTS = NUM_COMP;
  typedef dax::Tuple<uint8_t,NUM_COMPNENTS> FieldType;
  typedef uint8_t ComponentType;
};

template<>
struct FieldTypeToType<vtkUnsignedCharArray,1>
{
  static const int NUM_COMPNENTS = 1;
  typedef uint8_t FieldType;
  typedef uint8_t ComponentType;
};
}

#endif // vtkToDax_FieldTypeToType_h
