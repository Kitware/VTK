/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayDispatcher.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkDataArrayDispatcher
 * @brief   Dispatch to functor vtkDataArrayType
 *
 * vtkDataArrayDispatcher is a class that allows calling a functor based
 * on the data type of the vtkDataArray subclass. This is a wrapper
 * around the vtkTemplateMacro (VTK_TT) to allow easier implementation and
 * readibility, while at the same time the ability to use statefull functors.
 *
 * Note: By default the return type is void.
 * Note: The functor parameter must be of type vtkDataArrayDispatcherPointer
 *
 * The functors that are passed around can contain state, and are allowed
 * to be const or non const. If you are using a functor that does have state,
 * make sure your copy constructor is correct.
 *
 * \code
 * struct sizeOfFunctor{
 *   template<typename T>
 *   int operator()(const vtkDataArrayDispatcherPointer<T>& t) const
 *   {
 *   return t.NumberOfComponents * t.NumberOfTuples;
 *   }
 * };
 *
 * Here is an example of using the dispatcher.
 *  \code
 *  vtkDataArrayDispatcher<sizeOfFunctor,int> dispatcher;
 *  int arrayLength = dispatcher.Go(vtkDataArrayPtr);
 *  \endcode
 *
 *
 * @sa
 * vtkDispatcher vtkDoubleDispatcher
*/

#ifndef vtkDataArrayDispatcher_h
#define vtkDataArrayDispatcher_h

#include "vtkType.h" //Required for vtkIdType
#include "vtkDataArray.h" //required for constructor of the vtkDataArrayFunctor
#include <map> //Required for the storage of template params to runtime params

////////////////////////////////////////////////////////////////////////////////
// Object that is passed to all functor that are used with this class
// This allows the user the ability to find info about the size
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct vtkDataArrayDispatcherPointer
{
  typedef T ValueType;

  vtkIdType NumberOfTuples;
  vtkIdType NumberOfComponents;
  ValueType* RawPointer;

  explicit vtkDataArrayDispatcherPointer(vtkDataArray* array):
    NumberOfTuples(array->GetNumberOfTuples()),
    NumberOfComponents(array->GetNumberOfComponents()),
    RawPointer(static_cast<ValueType*>(array->GetVoidPointer(0)))
    {}
};

////////////////////////////////////////////////////////////////////////////////
// class template FunctorDispatcher
////////////////////////////////////////////////////////////////////////////////
template
  <
    class DefaultFunctorType,
    typename ReturnType = void
  >
class vtkDataArrayDispatcher
{
public:

  /**
   * Specify the functor that is to be used when dispatching. This allows
   * you to specify a statefull functor.

   * \code

   * struct storeLengthFunctor
   * {
   * int length;
   * storeLengthFunctor():length(0){}

   * template<typename T>
   * void operator()(vtkDataArrayDispatcherPointer<T> array)
   * {
   * length += array.NumberOfComponents * array.NumberOfTuples;
   * }
   * };

   * storeLengthFunctor storedLength;
   * vtkDataArrayDispatcher<storeLengthFunctor> dispatcher(storedLength);
   * dispatcher.Go(exampleDataArray);

   * \endcode
   */
  vtkDataArrayDispatcher(DefaultFunctorType& f);

  /**
   * Default constructor which will create an instance of the DefaultFunctorType
   * and use that single instance for all calls.
   */
  vtkDataArrayDispatcher();

  virtual ~vtkDataArrayDispatcher();

  /**
   * Execute the default functor with the passed in vtkDataArray;
   */
  ReturnType Go(vtkDataArray* lhs);

protected:
  DefaultFunctorType* DefaultFunctor;
  bool OwnsFunctor;
};

//We are making all these method non-inline to reduce compile time overhead

//----------------------------------------------------------------------------
template<class DefaultFunctorType,typename ReturnType>
vtkDataArrayDispatcher<DefaultFunctorType,ReturnType>::vtkDataArrayDispatcher(DefaultFunctorType& fun):
  DefaultFunctor(&fun),
  OwnsFunctor(false)
{
}

//----------------------------------------------------------------------------
template<class DefaultFunctorType,typename ReturnType>
vtkDataArrayDispatcher<DefaultFunctorType,ReturnType>::vtkDataArrayDispatcher():
  DefaultFunctor(new DefaultFunctorType()),
  OwnsFunctor(true)
{
}

//----------------------------------------------------------------------------
template<class DefaultFunctorType,typename ReturnType>
vtkDataArrayDispatcher<DefaultFunctorType,ReturnType>::~vtkDataArrayDispatcher()
{
  if(OwnsFunctor)
  {
    delete this->DefaultFunctor;
  }
}

//----------------------------------------------------------------------------
template <class DefaultFunctorType,typename ReturnType>
ReturnType vtkDataArrayDispatcher<DefaultFunctorType,ReturnType>
::Go(vtkDataArray* lhs)
{
  switch(lhs->GetDataType())
  {
      vtkTemplateMacro(return (*this->DefaultFunctor) (
                      vtkDataArrayDispatcherPointer<VTK_TT>(lhs) ));
  }
  return ReturnType();
}

#endif // vtkDataArrayDispatcher_h
// VTK-HeaderTest-Exclude: vtkDataArrayDispatcher.h
