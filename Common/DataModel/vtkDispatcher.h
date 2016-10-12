/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDispatcher.h

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

/**
 * @class   vtkDispatcher
 * @brief   Dispatch to functor based on a pointer type.
 *
 * vtkDispatcher is a class that allows calling a functor based
 * on the derived types of a pointer. This form of dynamic dispatching
 * allows the conversion of runtime polymorphism to a compile time polymorphism.
 * For example it can be used as a replacement for the vtkTemplateMacro, with
 * bonus of being easier to understand
 *
 * Note: By default the return type is void.
 *
 * The functors that are passed around can contain state, and are allowed
 * to be const or non const. If you are using a functor that does have state,
 * make sure your copy constructor is correct.
 *
 * \code
 * struct functor{
 *   template<typename T>
 *   void operator()(T& t) const
 *   {
 *
 *   }
 * };
 *
 * Here is an example of using the dispatcher.
 *  \code
 *  statefull functor;
 *  vtkDispatcher<vtkDataArray> dispatcher;
 *  dispatcher.Add<vtkCharArray>(&functor);
 *  dispatcher.Add<vtkDoubleArray>(&functor);
 *  dispatcher.Add<vtkIdTypeArray>(&functor);
 *  dispatcher.Add<vtkIntArray>(&functor);
 *  dispatcher.Add<vtkFloatArray>(&functor);
 *  dispatcher.Go(ptr1);
 *  \endcode
 *
 *
 * @sa
 * vtkDispatcher
*/

#ifndef vtkDispatcher_h
#define vtkDispatcher_h

#include "vtkDispatcher_Private.h" //needed for Functor,CastingPolicy,TypeInfo
#include <map> //Required for the storage of template params to runtime params

////////////////////////////////////////////////////////////////////////////////
// class template FunctorDispatcher
////////////////////////////////////////////////////////////////////////////////
template
  <
    class BaseLhs,
    typename ReturnType = void,
    template <class, class> class CastingPolicy = vtkDispatcherCommon::vtkCaster
  >
class vtkDispatcher
{
public:
  /**
   * Add in a functor that is mapped to the template SomeLhs parameter.
   * When instances of the parameter is passed in on the Go method we will call
   * the functor and pass along the given parameter.
   * Note: This copies the functor so pass stateful functors by pointer.

   * \code
   * vtkDispatcher<vtkDataModel> dispatcher;
   * dispatcher.Add<vtkImageData>(exampleFunctor());
   * dispatcher.Add<vtkImageData>(&exampleFunctorWithState);
   * \endcode
   */
  template <class SomeLhs, class Functor>
  void Add(Functor fun) { this->AddInternal<SomeLhs>(fun, 1); }

  /**
   * Remove a functor that is bound to the given parameter type. Will
   * return true if we did remove a functor.
   */
  template <class SomeLhs>
  bool Remove() { return DoRemove(typeid(SomeLhs)); }

  /**
   * Given a pointer to an object that derives from the BaseLhs
   * we find the matching functor that was added, and call it passing along
   * the given parameter. It should be noted that the functor will be called
   * with the parameter being the derived type that Functor was registered with.

   * Note: This will only find exact matches. So if you add functor to find
   * vtkDataArray, it will not be called if passed with a vtkDoubleArray.

   * \code

   * vtkDispatcher<vtkDataArray> dispatcher;
   * dispatcher.Add(vtkFloatArray>(floatFunctor())
   * dispatcher.Add(vtkDoubleArray>(doubleFunctor())
   * dispatcher.Go(dataArray1);
   * dispatcher.Go(dataArray2);
   * \endcode
   */
  ReturnType Go(BaseLhs* lhs);

protected:
  typedef vtkDispatcherCommon::TypeInfo TypeInfo;
  typedef vtkDispatcherPrivate::Functor<ReturnType,BaseLhs> MappedType;

  void DoAddFunctor(TypeInfo lhs, MappedType fun);
  bool DoRemove(TypeInfo lhs);
  typedef std::map<TypeInfo, MappedType > MapType;
  MapType FunctorMap;
private:
  template <class SomeLhs, class Functor>
  void AddInternal(Functor const& fun, long);
  template <class SomeLhs, class Functor>
  void AddInternal(Functor* fun, int);
};

//We are making all these method non-inline to reduce compile time overhead
//----------------------------------------------------------------------------
template<class BaseLhs,typename ReturnType,
         template <class, class> class CastingPolicy>
template <class SomeLhs, class Functor>
void vtkDispatcher<BaseLhs,ReturnType,CastingPolicy>::AddInternal(const Functor& fun, long)
{
  typedef vtkDispatcherPrivate::FunctorDispatcherHelper<
      BaseLhs,
      SomeLhs,
      ReturnType,
      CastingPolicy<SomeLhs, BaseLhs>,
      Functor> Adapter;
  Adapter ada(fun);
  MappedType mt(ada);
  DoAddFunctor(typeid(SomeLhs),mt);
}


//----------------------------------------------------------------------------
template<class BaseLhs,typename ReturnType,
         template <class, class> class CastingPolicy>
template <class SomeLhs, class Functor>
void vtkDispatcher<BaseLhs,ReturnType,CastingPolicy>::AddInternal(Functor* fun, int)
{
  typedef vtkDispatcherPrivate::FunctorRefDispatcherHelper<
      BaseLhs,
      SomeLhs,
      ReturnType,
      CastingPolicy<SomeLhs, BaseLhs>,
      Functor> Adapter;
  Adapter ada(*fun);
  MappedType mt(ada);
  DoAddFunctor(typeid(SomeLhs),mt);
}

//----------------------------------------------------------------------------
template<class BaseLhs,typename ReturnType,
         template <class, class> class CastingPolicy>
void vtkDispatcher<BaseLhs,ReturnType,CastingPolicy>
::DoAddFunctor(TypeInfo lhs, MappedType fun)
{
  FunctorMap[TypeInfo(lhs)] = fun;
}

//----------------------------------------------------------------------------
template <class BaseLhs, typename ReturnType,
          template <class, class> class CastingPolicy>
bool vtkDispatcher<BaseLhs,ReturnType,CastingPolicy>
::DoRemove(TypeInfo lhs)
{
  return FunctorMap.erase(TypeInfo(lhs)) == 1;
}

//----------------------------------------------------------------------------
template <class BaseLhs,typename ReturnType,
          template <class, class> class CastingPolicy>
ReturnType vtkDispatcher<BaseLhs,ReturnType,CastingPolicy>
::Go(BaseLhs* lhs)
{
  typename MapType::key_type k(typeid(*lhs));
  typename MapType::iterator i = FunctorMap.find(k);
  if (i == FunctorMap.end())
  {
    //we return a default type, currently i don't want exceptions thrown
    return ReturnType();
  }
  return (i->second)(*lhs);
}

#endif // vtkDispatcher_h
// VTK-HeaderTest-Exclude: vtkDispatcher.h
