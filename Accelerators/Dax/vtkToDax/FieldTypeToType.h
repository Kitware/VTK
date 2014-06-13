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

namespace internal {

template<int NUM_COMP>
struct IdType
{
  static const int NUM_COMPONENTS = NUM_COMP;
  typedef dax::Tuple<dax::Id,NUM_COMPONENTS> DaxValueType;
  typedef dax::Id DaxComponentType;
};

template<>
struct IdType<1>
{
  static const int NUM_COMPONENTS = 1;
  typedef dax::Id DaxValueType;
  typedef dax::Id DaxComponentType;
};

template<int NUM_COMP>
struct ScalarType
{
  static const int NUM_COMPONENTS = NUM_COMP;
  typedef dax::Tuple<dax::Scalar,NUM_COMPONENTS> DaxValueType;
  typedef dax::Scalar DaxComponentType;
};

template<>
struct ScalarType<1>
{
  static const int NUM_COMPONENTS = 1;
  typedef dax::Scalar DaxValueType;
  typedef dax::Scalar DaxComponentType;
};

} // namespace internal

template<typename VTKArrayType, int NUM_COMP> struct FieldTypeToType;

template<int NUM_COMP>
struct FieldTypeToType<vtkIntArray,NUM_COMP> : internal::IdType<NUM_COMP>
{
  typedef int VTKComponentType;
};

template<int NUM_COMP>
struct FieldTypeToType<vtkIdTypeArray,NUM_COMP> : internal::IdType<NUM_COMP>
{
  typedef vtkIdType VTKComponentType;
};


template<int NUM_COMP>
struct FieldTypeToType<vtkFloatArray,NUM_COMP> : internal::ScalarType<NUM_COMP>
{
  typedef float VTKComponentType;
};

template<int NUM_COMP>
struct FieldTypeToType<vtkDoubleArray,NUM_COMP> : internal::ScalarType<NUM_COMP>
{
  typedef double VTKComponentType;
};

template<int NUM_COMP>
struct FieldTypeToType<vtkUnsignedCharArray,NUM_COMP>
{
  static const int NUM_COMPNENTS = NUM_COMP;
  typedef dax::Tuple<uint8_t,NUM_COMPNENTS> DaxValueType;
  typedef uint8_t DaxComponentType;
  typedef uint8_t VTKComponentType;
};

template<>
struct FieldTypeToType<vtkUnsignedCharArray,1>
{
  static const int NUM_COMPNENTS = 1;
  typedef uint8_t DaxValueType;
  typedef uint8_t DaxComponentType;
  typedef uint8_t VTKComponentType;
};
}

#endif // vtkToDax_FieldTypeToType_h
