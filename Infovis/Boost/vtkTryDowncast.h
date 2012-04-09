/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTryDowncast.h

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

#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/vector.hpp>

// These are lists of standard VTK types.  End-users will have to choose these when they implement
// their algorithms.

// Description:
// Enumerates all integer VTK types
typedef boost::mpl::vector<vtkTypeUInt8, vtkTypeInt8, vtkTypeUInt16, vtkTypeInt16, vtkTypeUInt32, vtkTypeInt32, vtkTypeUInt64, vtkTypeInt64, vtkIdType> vtkIntegerTypes;
// Description:
// Enumerates all floating-point VTK types
typedef boost::mpl::vector<vtkTypeFloat32, vtkTypeFloat64> vtkFloatingPointTypes;
// Description:
// Enumerates all numeric VTK types
typedef boost::mpl::joint_view<vtkIntegerTypes, vtkFloatingPointTypes> vtkNumericTypes;
// Description:
// Enumerates all string VTK types
typedef boost::mpl::vector<vtkStdString, vtkUnicodeString> vtkStringTypes;
// Description:
// Enumerates all VTK types
typedef boost::mpl::joint_view<vtkNumericTypes, vtkStringTypes> vtkAllTypes;

// End-users can ignore these, they're the guts of the beast ...
template<template <typename> class TargetT, typename FunctorT>
class vtkTryDowncastHelper1
{
public:
  vtkTryDowncastHelper1(vtkObject* source1, FunctorT functor, bool& succeeded) :
    Source1(source1),
    Functor(functor),
    Succeeded(succeeded)
  {
  }

  template<typename ValueT>
  void operator()(ValueT) const
  {
    if(Succeeded)
      return;

    TargetT<ValueT>* const target1 = TargetT<ValueT>::SafeDownCast(Source1);
    if(target1)
      {
      Succeeded = true;
      this->Functor(target1);
      }
  }

  vtkObject* Source1;
  FunctorT Functor;
  bool& Succeeded;

private:
  vtkTryDowncastHelper1& operator=(const vtkTryDowncastHelper1&);
};

template<template <typename> class TargetT, typename FunctorT>
class vtkTryDowncastHelper2
{
public:
  vtkTryDowncastHelper2(vtkObject* source1, vtkObject* source2, FunctorT functor, bool& succeeded) :
    Source1(source1),
    Source2(source2),
    Functor(functor),
    Succeeded(succeeded)
  {
  }

  template<typename ValueT>
  void operator()(ValueT) const
  {
    if(Succeeded)
      return;

    TargetT<ValueT>* const target1 = TargetT<ValueT>::SafeDownCast(Source1);
    TargetT<ValueT>* const target2 = TargetT<ValueT>::SafeDownCast(Source2);
    if(target1 && target2)
      {
      Succeeded = true;
      this->Functor(target1, target2);
      }
  }

  vtkObject* Source1;
  vtkObject* Source2;
  FunctorT Functor;
  bool& Succeeded;

private:
  vtkTryDowncastHelper2& operator=(const vtkTryDowncastHelper2&);
};

template<template <typename> class TargetT, typename FunctorT>
class vtkTryDowncastHelper3
{
public:
  vtkTryDowncastHelper3(vtkObject* source1, vtkObject* source2, vtkObject* source3, FunctorT functor, bool& succeeded) :
    Source1(source1),
    Source2(source2),
    Source3(source3),
    Functor(functor),
    Succeeded(succeeded)
  {
  }

  template<typename ValueT>
  void operator()(ValueT) const
  {
    if(Succeeded)
      return;

    TargetT<ValueT>* const target1 = TargetT<ValueT>::SafeDownCast(Source1);
    TargetT<ValueT>* const target2 = TargetT<ValueT>::SafeDownCast(Source2);
    TargetT<ValueT>* const target3 = TargetT<ValueT>::SafeDownCast(Source3);
    if(target1 && target2 && target3)
      {
      Succeeded = true;
      this->Functor(target1, target2, target3);
      }
  }

  vtkObject* Source1;
  vtkObject* Source2;
  vtkObject* Source3;
  FunctorT Functor;
  bool& Succeeded;

private:
  vtkTryDowncastHelper3& operator=(const vtkTryDowncastHelper3&);
};

template<template <typename> class TargetT, typename TypesT, typename FunctorT>
bool vtkTryDowncast(vtkObject* source1, FunctorT functor)
{
  bool succeeded = false;
  boost::mpl::for_each<TypesT>(vtkTryDowncastHelper1<TargetT, FunctorT>(source1, functor, succeeded));
  return succeeded;
}

template<template <typename> class TargetT, typename TypesT, typename FunctorT>
bool vtkTryDowncast(vtkObject* source1, vtkObject* source2, FunctorT functor)
{
  bool succeeded = false;
  boost::mpl::for_each<TypesT>(vtkTryDowncastHelper2<TargetT, FunctorT>(source1, source2, functor, succeeded));
  return succeeded;
}


template<template <typename> class TargetT, typename TypesT, typename FunctorT>
bool vtkTryDowncast(vtkObject* source1, vtkObject* source2, vtkObject* source3, FunctorT functor)
{
  bool succeeded = false;
  boost::mpl::for_each<TypesT>(vtkTryDowncastHelper3<TargetT, FunctorT>(source1, source2, source3, functor, succeeded));
  return succeeded;
}

// VTK-HeaderTest-Exclude: vtkTryDowncast.h
