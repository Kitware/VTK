/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDoubleDispatcher.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any
//     purpose is hereby granted without fee, provided that the above copyright
//     notice appear in all copies and that both that copyright notice and this
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the
//     suitability of this software for any purpose. It is provided "as is"
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

// .NAME vtkDoubleDispatcher - Dispatch to functor based on two pointer types.
// .SECTION Description
// vtkDoubleDispatcher is a class that allows calling a functor based
// on the derived types of two pointers. This form of dynamic dispatching
// allows the conversion of runtime polymorphism to a compile time polymorphism.
// For example it can be used as a replacement for the vtkTemplateMacro when
// you want to know multiple parameter types, or need to call a specialized implementation
// for a subset
//
// Note: By default the return type is void.
//
// The functors that are passed around can contain state, and are allowed
// to be const or non const. If you are using a functor that does have state,
// make sure your copy constructor is correct.
//
// \code
// struct functor{
//   template<typename T,typename U>
//   void operator()(T& t,U& u) const
//   {
//
//   }
// };
//
// Here is an example of using the double dispatcher.
//  \code
//  vtkDoubleDispatcher<vtkObject,vtkObject,vtkPoints*> dispatcher;
//  dispatcher.Add<vtkPoints,vtkDoubleArray>(makePointsWrapperFunctor());
//  dispatcher.Add<vtkPoints,vtkPoints>(straightCopyFunctor());
//  dispatcher.Go(ptr1,ptr2); //this will return a vtkPoints pointer
//  \endcode

//
// .SECTION See Also
// vtkDispatcher

#ifndef __vtkDoubleDispatcher_h
#define __vtkDoubleDispatcher_h

#include "vtkDispatcher_Private.h" //needed for Functor,CastingPolicy,TypeInfo
#include <map> //Required for the storage of template params to runtime params

template
<
    class BaseLhs,
    class BaseRhs = BaseLhs,
    typename ReturnType = void,
    template <class, class> class CastingPolicy = vtkDispatcherCommon::vtkCaster
    >
class vtkDoubleDispatcher
{
public:
  // Description:
  // Add in a functor that is mapped to the combination of the
  // two template parameters passed in. When instances of the two parameters
  // are passed in on the Go method we will call the functor and pass along
  // the given parameters.
  // Note: This copies the functor so pass stateful functors by pointer.
  //
  // \code
  // vtkDoubleDispatcher<vtkDataModel,vtkCell> dispatcher;
  // dispatcher.Add<vtkImageData,vtkVoxel>(exampleFunctor());
  // dispatcher.Add<vtkImageData,vtkVoxel>(&exampleFunctorWithState);
  // \endcode
  template <class SomeLhs, class SomeRhs, class Functor>
  void Add(Functor fun) { this->AddInternal<SomeLhs,SomeRhs>(fun, 1); }

  // Description:
  // Remove a functor that is bound to the given parameter types. Will
  // return true if we did remove a functor.
  template <class SomeLhs, class SomeRhs>
  bool Remove() { return DoRemove(typeid(SomeLhs), typeid(SomeRhs)); }

  // Description:
  // Given two pointers of objects that derive from the BaseLhs and BaseRhs
  // we find the matching functor that was added, and call it passing along
  // the given parameters. It should be noted that the functor will be called
  // with the parameters being the derived type that Functor was registered with.
  //
  // Note: This will only find exact matches. So if you add functor to find
  // vtkDataArray,vtkDataArray, it will not be called if passed with
  // vtkDoubleArray,vtkDoubleArray.
  //
  // \code
  //
  // vtkDoubleDispatcher<vtkDataArray,vtkDataArray> dispatcher;
  // dispatcher.Add(vtkFloatArray,vtkFloatArray>(floatFunctor())
  // dispatcher.Add(vtkFloatArray,vtkDoubleArray>(mixedTypeFunctor())
  // dispatcher.Go( dataArray1, dataArray2);
  // \endcode
  ReturnType Go(BaseLhs* lhs, BaseRhs* rhs);

protected:
  typedef vtkDispatcherCommon::TypeInfo TypeInfo;
  typedef vtkDoubleDispatcherPrivate::Functor<ReturnType,BaseLhs,BaseRhs>
                                              MappedType;

  void DoAddFunctor(TypeInfo lhs,TypeInfo rhs, MappedType fun);
  bool DoRemove(TypeInfo lhs, TypeInfo rhs);

  typedef std::pair<TypeInfo,TypeInfo> KeyType;
  typedef std::map<KeyType, MappedType > MapType;
  MapType FunctorMap;
private:
  template <class SomeLhs, class SomeRhs, class Functor>
  void AddInternal(const Functor& fun, long);
  template <class SomeLhs, class SomeRhs, class Functor>
  void AddInternal(Functor* fun, int);
};

//We are making all these method non-inline to reduce compile time overhead
//----------------------------------------------------------------------------
template<class BaseLhs, class BaseRhs, typename ReturnType,
         template <class, class> class CastingPolicy>
template <class SomeLhs, class SomeRhs, class Functor>
void vtkDoubleDispatcher<BaseLhs,BaseRhs,ReturnType,CastingPolicy>
::AddInternal(const Functor& fun, long)
  {
  typedef vtkDoubleDispatcherPrivate::FunctorDoubleDispatcherHelper<
      BaseLhs, BaseRhs,
      SomeLhs, SomeRhs,
      ReturnType,
      CastingPolicy<SomeLhs, BaseLhs>,
      CastingPolicy<SomeRhs, BaseRhs>,
      Functor> Adapter;
  Adapter ada(fun);
  MappedType mt(ada);
  DoAddFunctor(typeid(SomeLhs), typeid(SomeRhs),mt);
  }

//----------------------------------------------------------------------------
template<class BaseLhs, class BaseRhs, typename ReturnType,
         template <class, class> class CastingPolicy>
template <class SomeLhs, class SomeRhs, class Functor>
void vtkDoubleDispatcher<BaseLhs,BaseRhs,ReturnType,CastingPolicy>
::AddInternal(Functor* fun, int)
  {
  typedef vtkDoubleDispatcherPrivate::FunctorRefDispatcherHelper<
      BaseLhs, BaseRhs,
      SomeLhs, SomeRhs,
      ReturnType,
      CastingPolicy<SomeLhs, BaseLhs>,
      CastingPolicy<SomeRhs, BaseRhs>,
      Functor> Adapter;
  Adapter ada(*fun);
  MappedType mt(ada);
  DoAddFunctor(typeid(SomeLhs), typeid(SomeRhs),mt);
  }

//----------------------------------------------------------------------------
template<class BaseLhs, class BaseRhs, typename ReturnType,
         template <class, class> class CastingPolicy>
void vtkDoubleDispatcher<BaseLhs,BaseRhs,ReturnType,CastingPolicy>
::DoAddFunctor(TypeInfo lhs, TypeInfo rhs, MappedType fun)
  {
  FunctorMap[KeyType(lhs, rhs)] = fun;
  }

//----------------------------------------------------------------------------
template <class BaseLhs, class BaseRhs, typename ReturnType,
          template <class, class> class CastingPolicy>
bool vtkDoubleDispatcher<BaseLhs,BaseRhs,ReturnType,CastingPolicy>
::DoRemove(TypeInfo lhs, TypeInfo rhs)
  {
  return FunctorMap.erase(KeyType(lhs, rhs)) == 1;
  }

//----------------------------------------------------------------------------
template <class BaseLhs, class BaseRhs, typename ReturnType,
          template <class, class> class CastingPolicy>
ReturnType vtkDoubleDispatcher<BaseLhs,BaseRhs,ReturnType,CastingPolicy>
::Go(BaseLhs* lhs, BaseRhs* rhs)
  {
  typename MapType::key_type k(typeid(*lhs),typeid(*rhs));
  typename MapType::iterator i = FunctorMap.find(k);
  if (i == FunctorMap.end())
    {
    //we don't want to throw exceptions so we have two options.
    //we can return the default, or make a lightweight struct for return value
    return ReturnType();
    }
  return (i->second)(*lhs,*rhs);
  }

#endif // __vtkDoubleDispatcher_h
// VTK-HeaderTest-Exclude: vtkDoubleDispatcher.h
