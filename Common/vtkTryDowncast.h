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
class vtkTryDowncastHelper
{
public:
  vtkTryDowncastHelper(vtkObject* source, FunctorT functor, bool& succeeded) :
    Source(source),
    Functor(functor),
    Succeeded(succeeded)
  {
  }

  template<typename ValueT>
  void operator()(ValueT) const
  {
    if(Succeeded)
      return;

    if(TargetT<ValueT>* const target = TargetT<ValueT>::SafeDownCast(Source))
      {
      Succeeded = true;
      this->Functor(target);
      }
  }

  vtkObject* Source;
  FunctorT Functor;
  bool& Succeeded;

private:
  vtkTryDowncastHelper& operator=(const vtkTryDowncastHelper&);
};

template<template <typename> class TargetT, typename FunctorT, typename Arg1T>
class vtkTryDowncastHelper1
{
public:
  vtkTryDowncastHelper1(vtkObject* source, FunctorT functor, Arg1T arg1, bool& succeeded) :
    Source(source),
    Functor(functor),
    Arg1(arg1),
    Succeeded(succeeded)
  {
  }

  template<typename ValueT>
  void operator()(ValueT) const
  {
    if(Succeeded)
      return;

    if(TargetT<ValueT>* const target = TargetT<ValueT>::SafeDownCast(Source))
      {
      Succeeded = true;
      this->Functor(target, this->Arg1);
      }
  }

  vtkObject* Source;
  FunctorT Functor;
  Arg1T Arg1;
  bool& Succeeded;

private:
  vtkTryDowncastHelper1& operator=(const vtkTryDowncastHelper1&);
};

template<template <typename> class TargetT, typename TypesT, typename FunctorT>
bool vtkTryDowncast(vtkObject* source, FunctorT functor)
{
  bool succeeded = false;
  boost::mpl::for_each<TypesT>(vtkTryDowncastHelper<TargetT, FunctorT>(source, functor, succeeded));
  return succeeded;
}

template<template <typename> class TargetT, typename TypesT, typename FunctorT, typename Arg1T>
bool vtkTryDowncast(vtkObject* source, FunctorT functor, Arg1T arg1)
{
  bool succeeded = false;
  boost::mpl::for_each<TypesT>(vtkTryDowncastHelper1<TargetT, FunctorT, Arg1T>(source, functor, arg1, succeeded));
  return succeeded;
}

