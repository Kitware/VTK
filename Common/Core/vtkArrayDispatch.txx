// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkArrayDispatch_txx
#define vtkArrayDispatch_txx

#include "vtkArrayDispatch.h"

#include "vtkDebug.h"  // For warning macro settings.
#include "vtkSetGet.h" // For warning macros.

#include <utility> // For std::forward

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
VTK_ABI_NAMESPACE_END

namespace vtkArrayDispatch
{
namespace impl
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Implementation of the single-array dispatch mechanism.
template <typename ArrayList>
struct Dispatch;

// Terminal case:
template <>
struct Dispatch<vtkTypeList::NullType>
{
  template <typename... T>
  static bool Execute(T&&...)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Array dispatch failed.");
#endif
    return false;
  }
};

// Recursive case:
template <typename ArrayHead, typename ArrayTail>
struct Dispatch<vtkTypeList::TypeList<ArrayHead, ArrayTail>>
{
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* inArray, Worker&& worker, Params&&... params)
  {
    if (ArrayHead* array = vtkArrayDownCast<ArrayHead>(inArray))
    {
      worker(array, std::forward<Params>(params)...);
      return true;
    }
    else
    {
      return Dispatch<ArrayTail>::Execute(
        inArray, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 2 array dispatch mechanism.
template <typename ArrayList1, typename ArrayList2>
struct Dispatch2;

//----------------------------//
// First dispatch trampoline: //
//----------------------------//
template <typename Array1T, typename ArrayList2>
struct Dispatch2Trampoline;

// Dispatch2 Terminal case:
template <typename ArrayList2>
struct Dispatch2<vtkTypeList::NullType, ArrayList2>
{
  template <typename... T>
  static bool Execute(T&&...)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch2 Recursive case:
template <typename Array1Head, typename Array1Tail, typename ArrayList2>
struct Dispatch2<vtkTypeList::TypeList<Array1Head, Array1Tail>, ArrayList2>
{
  typedef Dispatch2<Array1Tail, ArrayList2> NextDispatch;
  typedef Dispatch2Trampoline<Array1Head, ArrayList2> Trampoline;

  template <typename Worker, typename... Params>
  static bool Execute(
    vtkDataArray* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    if (Array1Head* array = vtkArrayDownCast<Array1Head>(array1))
    {
      return Trampoline::Execute(
        array, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
    else
    {
      return NextDispatch::Execute(
        array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
  }
};

// Dispatch2 Trampoline terminal case:
template <typename Array1T>
struct Dispatch2Trampoline<Array1T, vtkTypeList::NullType>
{
  template <typename... T>
  static bool Execute(T&&...)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch2 Trampoline recursive case:
template <typename Array1T, typename Array2Head, typename Array2Tail>
struct Dispatch2Trampoline<Array1T, vtkTypeList::TypeList<Array2Head, Array2Tail>>
{
  typedef Dispatch2Trampoline<Array1T, Array2Tail> NextDispatch;

  template <typename Worker, typename... Params>
  static bool Execute(Array1T* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    if (Array2Head* array = vtkArrayDownCast<Array2Head>(array2))
    {
      worker(array1, array, std::forward<Params>(params)...);
      return true;
    }
    else
    {
      return NextDispatch::Execute(
        array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 2 array same-type dispatch mechanism.
template <typename ArrayList1, typename ArrayList2>
struct Dispatch2Same;

// Terminal case:
template <typename ArrayList2>
struct Dispatch2Same<vtkTypeList::NullType, ArrayList2>
{
  template <typename... T>
  static bool Execute(T&&...)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
    return false;
  }
};

// Recursive case:
template <typename ArrayHead, typename ArrayTail, typename ArrayList2>
struct Dispatch2Same<vtkTypeList::TypeList<ArrayHead, ArrayTail>, ArrayList2>
{
  typedef Dispatch2Same<ArrayTail, ArrayList2> NextDispatch;
  typedef vtkTypeList::Create<typename ArrayHead::ValueType> ValueType;
  typedef typename FilterArraysByValueType<ArrayList2, ValueType>::Result ValueArrayList;
  typedef Dispatch2Trampoline<ArrayHead, ValueArrayList> Trampoline;

  template <typename Worker, typename... Params>
  static bool Execute(
    vtkDataArray* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    if (ArrayHead* array = vtkArrayDownCast<ArrayHead>(array1))
    {
      return Trampoline::Execute(
        array, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
    else
    {
      return NextDispatch::Execute(
        array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 3 array dispatch mechanism.
template <typename ArrayList1, typename ArrayList2, typename ArrayList3>
struct Dispatch3;

//-----------------------------//
// First dispatch trampoline: //
//---------------------------//
template <typename Array1T, typename ArrayList2, typename ArrayList3>
struct Dispatch3Trampoline1;

//------------------------------//
// Second dispatch trampoline: //
//----------------------------//
template <typename Array1T, typename Array2T, typename ArrayList3>
struct Dispatch3Trampoline2;

// Dispatch3 Terminal case:
template <typename ArrayList2, typename ArrayList3>
struct Dispatch3<vtkTypeList::NullType, ArrayList2, ArrayList3>
{
  template <typename... T>
  static bool Execute(T&&...)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch3 Recursive case:
template <typename ArrayHead, typename ArrayTail, typename ArrayList2, typename ArrayList3>
struct Dispatch3<vtkTypeList::TypeList<ArrayHead, ArrayTail>, ArrayList2, ArrayList3>
{
private:
  typedef Dispatch3Trampoline1<ArrayHead, ArrayList2, ArrayList3> Trampoline;
  typedef Dispatch3<ArrayTail, ArrayList2, ArrayList3> NextDispatch;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array1, vtkDataArray* array2, vtkDataArray* array3,
    Worker&& worker, Params&&... params)
  {
    if (ArrayHead* array = vtkArrayDownCast<ArrayHead>(array1))
    {
      return Trampoline::Execute(
        array, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
    else
    {
      return NextDispatch::Execute(
        array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
  }
};

// Dispatch3 Trampoline1 terminal case:
template <typename Array1T, typename ArrayList3>
struct Dispatch3Trampoline1<Array1T, vtkTypeList::NullType, ArrayList3>
{
  template <typename... T>
  static bool Execute(T&&...)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch3 Trampoline1 recursive case:
template <typename Array1T, typename ArrayHead, typename ArrayTail, typename ArrayList3>
struct Dispatch3Trampoline1<Array1T, vtkTypeList::TypeList<ArrayHead, ArrayTail>, ArrayList3>
{
private:
  typedef Dispatch3Trampoline2<Array1T, ArrayHead, ArrayList3> Trampoline;
  typedef Dispatch3Trampoline1<Array1T, ArrayTail, ArrayList3> NextDispatch;

public:
  template <typename Worker, typename... Params>
  static bool Execute(Array1T* array1, vtkDataArray* array2, vtkDataArray* array3, Worker&& worker,
    Params&&... params)
  {
    if (ArrayHead* array = vtkArrayDownCast<ArrayHead>(array2))
    {
      return Trampoline::Execute(
        array1, array, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
    else
    {
      return NextDispatch::Execute(
        array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
  }
};

// Dispatch3 Trampoline2 terminal case:
template <typename Array1T, typename Array2T>
struct Dispatch3Trampoline2<Array1T, Array2T, vtkTypeList::NullType>
{
  template <typename... T>
  static bool Execute(T&&...)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch3 Trampoline2 recursive case:
template <typename Array1T, typename Array2T, typename ArrayHead, typename ArrayTail>
struct Dispatch3Trampoline2<Array1T, Array2T, vtkTypeList::TypeList<ArrayHead, ArrayTail>>
{
private:
  typedef Dispatch3Trampoline2<Array1T, Array2T, ArrayTail> NextDispatch;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    Array1T* array1, Array2T* array2, vtkDataArray* array3, Worker&& worker, Params&&... params)
  {
    if (ArrayHead* array = vtkArrayDownCast<ArrayHead>(array3))
    {
      worker(array1, array2, array, std::forward<Params>(params)...);
      return true;
    }
    else
    {
      return NextDispatch::Execute(
        array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
  }
};

//------------------------------------------------------------------------------
// Description:
// Dispatch three arrays, enforcing that all three have the same ValueType.
// Initially, set both ArraysToTest and ArrayList to the same TypeList.
// ArraysToTest is iterated through, while ArrayList is preserved for later
// dispatches.
template <typename ArrayList1, typename ArrayList2, typename ArrayList3>
struct Dispatch3Same;

// Dispatch3Same terminal case:
template <typename ArrayList2, typename ArrayList3>
struct Dispatch3Same<vtkTypeList::NullType, ArrayList2, ArrayList3>
{
  template <typename... T>
  static bool Execute(T&&...)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch3Same recursive case:
template <typename ArrayHead, typename ArrayTail, typename ArrayList2, typename ArrayList3>
struct Dispatch3Same<vtkTypeList::TypeList<ArrayHead, ArrayTail>, ArrayList2, ArrayList3>
{
private:
  typedef vtkTypeList::Create<typename ArrayHead::ValueType> ValueType;
  typedef typename FilterArraysByValueType<ArrayList2, ValueType>::Result ValueArrays2;
  typedef typename FilterArraysByValueType<ArrayList3, ValueType>::Result ValueArrays3;
  typedef Dispatch3Trampoline1<ArrayHead, ValueArrays2, ValueArrays3> Trampoline;
  typedef Dispatch3Same<ArrayTail, ArrayList2, ArrayList3> NextDispatch;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array1, vtkDataArray* array2, vtkDataArray* array3,
    Worker&& worker, Params&&... params)
  {
    if (ArrayHead* array = vtkArrayDownCast<ArrayHead>(array1))
    {
      return Trampoline::Execute(
        array, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
    else
    {
      return NextDispatch::Execute(
        array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
    }
  }
};

VTK_ABI_NAMESPACE_END
} // end namespace impl

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// FilterArraysByValueType implementation:
//------------------------------------------------------------------------------

// Terminal case:
template <typename ValueList>
struct FilterArraysByValueType<vtkTypeList::NullType, ValueList>
{
  typedef vtkTypeList::NullType Result;
};

// Recursive case:
template <typename ArrayHead, typename ArrayTail, typename ValueList>
struct FilterArraysByValueType<vtkTypeList::TypeList<ArrayHead, ArrayTail>, ValueList>
{
private:
  typedef typename ArrayHead::ValueType ValueType;
  enum
  {
    ValueIsAllowed = vtkTypeList::IndexOf<ValueList, ValueType>::Result >= 0
  };
  typedef typename FilterArraysByValueType<ArrayTail, ValueList>::Result NewTail;

public:
  typedef typename vtkTypeList::Select<ValueIsAllowed, vtkTypeList::TypeList<ArrayHead, NewTail>,
    NewTail>::Result Result;
};

//------------------------------------------------------------------------------
// DispatchByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch:
template <typename ArrayHead, typename ArrayTail>
struct DispatchByArray<vtkTypeList::TypeList<ArrayHead, ArrayTail>>
{
private:
  typedef vtkTypeList::TypeList<ArrayHead, ArrayTail> ArrayList;
  typedef typename vtkTypeList::Unique<ArrayList>::Result UniqueArrays;
  typedef typename vtkTypeList::DerivedToFront<UniqueArrays>::Result SortedUniqueArrays;
  typedef impl::Dispatch<SortedUniqueArrays> ArrayDispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* inArray, Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      inArray, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch implementation:
// (defined after DispatchByArray to prevent 'incomplete type' errors)
//------------------------------------------------------------------------------
struct Dispatch
{
private:
  typedef DispatchByArray<Arrays> Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array, Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// DispatchByValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch
template <typename ArrayList, typename ValueTypeHead, typename ValueTypeTail>
struct DispatchByValueTypeUsingArrays<ArrayList,
  vtkTypeList::TypeList<ValueTypeHead, ValueTypeTail>>
{
private:
  typedef vtkTypeList::TypeList<ValueTypeHead, ValueTypeTail> ValueTypeList;
  typedef typename FilterArraysByValueType<ArrayList, ValueTypeList>::Result FilteredArrayList;
  typedef typename vtkTypeList::Unique<FilteredArrayList>::Result UniqueArrays;
  typedef typename vtkTypeList::DerivedToFront<UniqueArrays>::Result SortedUniqueArrays;
  typedef impl::Dispatch<SortedUniqueArrays> ArrayDispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* inArray, Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      inArray, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
template <typename ValueTypeList>
struct DispatchByValueType : public DispatchByValueTypeUsingArrays<Arrays, ValueTypeList>
{
};

//------------------------------------------------------------------------------
// Dispatch2ByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2:
template <typename ArrayList1, typename ArrayList2>
struct Dispatch2ByArray
{
private:
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef impl::Dispatch2<SortedUniqueArray1, SortedUniqueArray2> ArrayDispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkDataArray* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch2 implementation:
//------------------------------------------------------------------------------
struct Dispatch2
{
private:
  typedef Dispatch2ByArray<Arrays, Arrays> Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkDataArray* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch2ByValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2
template <typename ArrayList, typename ValueTypeList1, typename ValueTypeList2>
struct Dispatch2ByValueTypeUsingArrays
{
private:
  typedef typename FilterArraysByValueType<ArrayList, ValueTypeList1>::Result FilteredArrayList1;
  typedef typename FilterArraysByValueType<ArrayList, ValueTypeList2>::Result FilteredArrayList2;
  typedef typename vtkTypeList::Unique<FilteredArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<FilteredArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef impl::Dispatch2<SortedUniqueArray1, SortedUniqueArray2> ArrayDispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkDataArray* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
template <typename ValueTypeList1, typename ValueTypeList2>
struct Dispatch2ByValueType
  : Dispatch2ByValueTypeUsingArrays<Arrays, ValueTypeList1, ValueTypeList2>
{
};

//------------------------------------------------------------------------------
// Dispatch2BySameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2Same
template <typename ArrayList, typename ValueTypeList>
struct Dispatch2BySameValueTypeUsingArrays
{
private:
  typedef typename FilterArraysByValueType<ArrayList, ValueTypeList>::Result FilteredArrayList;
  typedef typename vtkTypeList::Unique<FilteredArrayList>::Result UniqueArray;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray>::Result SortedUniqueArray;
  typedef impl::Dispatch2Same<SortedUniqueArray, SortedUniqueArray> Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkDataArray* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
template <typename ValueTypeList>
struct Dispatch2BySameValueType : Dispatch2BySameValueTypeUsingArrays<Arrays, ValueTypeList>
{
};

//------------------------------------------------------------------------------
// Dispatch2ByArrayWithSameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2Same
template <typename ArrayList1, typename ArrayList2>
struct Dispatch2ByArrayWithSameValueType
{
private:
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef impl::Dispatch2Same<SortedUniqueArray1, SortedUniqueArray2> Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkDataArray* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch2SameValueType implementation:
//------------------------------------------------------------------------------
template <typename ArrayList>
struct Dispatch2SameValueTypeUsingArrays
{
private:
  typedef Dispatch2ByArrayWithSameValueType<ArrayList, ArrayList> Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkDataArray* array1, vtkDataArray* array2, Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
struct Dispatch2SameValueType : public Dispatch2SameValueTypeUsingArrays<Arrays>
{
};

//------------------------------------------------------------------------------
// Dispatch3ByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3:
template <typename ArrayList1, typename ArrayList2, typename ArrayList3>
struct Dispatch3ByArray
{
private:
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::Unique<ArrayList3>::Result UniqueArray3;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray3>::Result SortedUniqueArray3;
  typedef impl::Dispatch3<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3>
    ArrayDispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array1, vtkDataArray* array2, vtkDataArray* array3,
    Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch3 implementation:
//------------------------------------------------------------------------------
struct Dispatch3
{
private:
  typedef Dispatch3ByArray<Arrays, Arrays, Arrays> Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array1, vtkDataArray* array2, vtkDataArray* array3,
    Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch3ByValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3
template <typename ArrayList, typename ValueTypeList1, typename ValueTypeList2,
  typename ValueTypeList3>
struct Dispatch3ByValueTypeUsingArrays
{
private:
  typedef typename FilterArraysByValueType<ArrayList, ValueTypeList1>::Result FilteredArrayList1;
  typedef typename FilterArraysByValueType<ArrayList, ValueTypeList2>::Result FilteredArrayList2;
  typedef typename FilterArraysByValueType<ArrayList, ValueTypeList3>::Result FilteredArrayList3;
  typedef typename vtkTypeList::Unique<FilteredArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<FilteredArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::Unique<FilteredArrayList3>::Result UniqueArray3;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray3>::Result SortedUniqueArray3;
  typedef impl::Dispatch3<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3>
    ArrayDispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array1, vtkDataArray* array2, vtkDataArray* array3,
    Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
template <typename ValueTypeList1, typename ValueTypeList2, typename ValueTypeList3>
struct Dispatch3ByValueType
  : public Dispatch3ByValueTypeUsingArrays<Arrays, ValueTypeList1, ValueTypeList2, ValueTypeList3>
{
};

//------------------------------------------------------------------------------
// Dispatch3BySameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3Same
template <typename ArrayList, typename ValueTypeList>
struct Dispatch3BySameValueTypeUsingArrays
{
private:
  typedef typename FilterArraysByValueType<ArrayList, ValueTypeList>::Result FilteredArrayList;
  typedef typename vtkTypeList::Unique<FilteredArrayList>::Result UniqueArray;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray>::Result SortedUniqueArray;
  typedef impl::Dispatch3Same<SortedUniqueArray, SortedUniqueArray, SortedUniqueArray> Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array1, vtkDataArray* array2, vtkDataArray* array3,
    Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
template <typename ValueTypeList>
struct Dispatch3BySameValueType : public Dispatch3BySameValueTypeUsingArrays<Arrays, ValueTypeList>
{
};

//------------------------------------------------------------------------------
// Dispatch3BySameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3Same
template <typename ArrayList1, typename ArrayList2, typename ArrayList3>
struct Dispatch3ByArrayWithSameValueType
{
private:
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::Unique<ArrayList3>::Result UniqueArray3;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray3>::Result SortedUniqueArray3;
  typedef impl::Dispatch3Same<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3>
    Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array1, vtkDataArray* array2, vtkDataArray* array3,
    Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch3SameValueType implementation:
//------------------------------------------------------------------------------
template <typename ArrayList>
struct Dispatch3SameValueTypeUsingArrays
{
private:
  typedef Dispatch3ByArrayWithSameValueType<ArrayList, ArrayList, ArrayList> Dispatcher;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkDataArray* array1, vtkDataArray* array2, vtkDataArray* array3,
    Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
struct Dispatch3SameValueType : Dispatch3SameValueTypeUsingArrays<Arrays>
{
};

VTK_ABI_NAMESPACE_END
} // end namespace vtkArrayDispatch

#endif // vtkArrayDispatch_txx
