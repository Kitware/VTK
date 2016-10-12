/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayDispatch.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkArrayDispatch_txx
#define vtkArrayDispatch_txx

#include "vtkArrayDispatch.h"

#include "vtkConfigure.h" // For warning macro settings.
#include "vtkSetGet.h" // For warning macros.

class vtkDataArray;

namespace vtkArrayDispatch {
namespace impl {

//------------------------------------------------------------------------------
// Implementation of the single-array dispatch mechanism.
template <typename ArrayList>
struct Dispatch;

// Terminal case:
template<>
struct Dispatch<vtkTypeList::NullType>
{
  template <typename Worker>
  static bool Execute(vtkDataArray *, Worker&)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Array dispatch failed.");
#endif
    return false;
  }
};

// Recursive case:
template <
    typename ArrayHead,
    typename ArrayTail
    >
struct Dispatch<vtkTypeList::TypeList<ArrayHead, ArrayTail> >
{
  template <typename Worker>
  static bool Execute(vtkDataArray *inArray, Worker &worker)
  {
    if (ArrayHead *array = vtkArrayDownCast<ArrayHead>(inArray))
    {
      worker(array);
      return true;
    }
    else
    {
      return Dispatch<ArrayTail>::Execute(inArray, worker);
    }
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 2 array dispatch mechanism.
template <
    typename ArrayList1,
    typename ArrayList2
    >
struct Dispatch2;

//----------------------------//
// First dispatch trampoline: //
//----------------------------//
template <
    typename Array1T,
    typename ArrayList2
    >
struct Dispatch2Trampoline;

// Dispatch2 Terminal case:
template <typename ArrayList2>
struct Dispatch2<vtkTypeList::NullType, ArrayList2>
{
  template <typename Worker>
  static bool Execute(vtkDataArray *, vtkDataArray *, Worker &)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch2 Recursive case:
template <
    typename Array1Head,
    typename Array1Tail,
    typename ArrayList2
    >
struct Dispatch2<vtkTypeList::TypeList<Array1Head, Array1Tail>, ArrayList2>
{
  typedef Dispatch2<Array1Tail, ArrayList2> NextDispatch;
  typedef Dispatch2Trampoline<Array1Head, ArrayList2> Trampoline;

  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      Worker &worker)
  {
    if (Array1Head *array = vtkArrayDownCast<Array1Head>(array1))
    {
      return Trampoline::Execute(array, array2, worker);
    }
    else
    {
      return NextDispatch::Execute(array1, array2, worker);
    }
  }
};

// Dispatch2 Trampoline terminal case:
template <typename Array1T>
struct Dispatch2Trampoline<Array1T, vtkTypeList::NullType>
{
  template <typename Worker>
  static bool Execute(Array1T *, vtkDataArray *, Worker &)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch2 Trampoline recursive case:
template <
    typename Array1T,
    typename Array2Head,
    typename Array2Tail
    >
struct Dispatch2Trampoline<
    Array1T,
    vtkTypeList::TypeList<Array2Head, Array2Tail>
    >
{
  typedef Dispatch2Trampoline<Array1T, Array2Tail> NextDispatch;

  template <typename Worker>
  static bool Execute(Array1T *array1, vtkDataArray *array2, Worker &worker)
  {
    if (Array2Head *array = vtkArrayDownCast<Array2Head>(array2))
    {
      worker(array1, array);
      return true;
    }
    else
    {
      return NextDispatch::Execute(array1, array2, worker);
    }
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 2 array same-type dispatch mechanism.
template <
    typename ArrayList1,
    typename ArrayList2
    >
struct Dispatch2Same;

// Terminal case:
template <typename ArrayList2>
struct Dispatch2Same<vtkTypeList::NullType, ArrayList2>
{
  template <typename Worker>
  static bool Execute(vtkDataArray *, vtkDataArray *, Worker &)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
    return false;
  }
};

// Recursive case:
template <
    typename ArrayHead,
    typename ArrayTail,
    typename ArrayList2
    >
struct Dispatch2Same<
    vtkTypeList::TypeList<ArrayHead, ArrayTail>,
    ArrayList2
    >
{
  typedef Dispatch2Same<ArrayTail, ArrayList2> NextDispatch;
  typedef vtkTypeList_Create_1(typename ArrayHead::ValueType) ValueType;
  typedef typename FilterArraysByValueType<ArrayList2, ValueType>::Result ValueArrayList;
  typedef Dispatch2Trampoline<ArrayHead, ValueArrayList> Trampoline;

  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      Worker &worker)
  {
    if (ArrayHead *array = vtkArrayDownCast<ArrayHead>(array1))
    {
      return Trampoline::Execute(array, array2, worker);
    }
    else
    {
      return NextDispatch::Execute(array1, array2, worker);
    }
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 3 array dispatch mechanism.
template <
    typename ArrayList1,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3;

//-----------------------------//
// First dispatch trampoline: //
//---------------------------//
template <
    typename Array1T,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3Trampoline1;

//------------------------------//
// Second dispatch trampoline: //
//----------------------------//
template <
    typename Array1T,
    typename Array2T,
    typename ArrayList3
    >
struct Dispatch3Trampoline2;

// Dispatch3 Terminal case:
template <
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3<vtkTypeList::NullType, ArrayList2, ArrayList3>
{
  template <typename Worker>
  static bool Execute(vtkDataArray *, vtkDataArray*, vtkDataArray*, Worker&)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch3 Recursive case:
template <
    typename ArrayHead,
    typename ArrayTail,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3<vtkTypeList::TypeList<ArrayHead, ArrayTail>, ArrayList2,
                 ArrayList3>
{
private:
  typedef Dispatch3Trampoline1<ArrayHead, ArrayList2, ArrayList3> Trampoline;
  typedef Dispatch3<ArrayTail, ArrayList2, ArrayList3> NextDispatch;
public:
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    if (ArrayHead *array = vtkArrayDownCast<ArrayHead>(array1))
    {
      return Trampoline::Execute(array, array2, array3, worker);
    }
    else
    {
      return NextDispatch::Execute(array1, array2, array3, worker);
    }
  }
};

// Dispatch3 Trampoline1 terminal case:
template <
    typename Array1T,
    typename ArrayList3
    >
struct Dispatch3Trampoline1<Array1T, vtkTypeList::NullType, ArrayList3>
{
  template <typename Worker>
  static bool Execute(Array1T*, vtkDataArray*, vtkDataArray*, Worker&)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch3 Trampoline1 recursive case:
template <
    typename Array1T,
    typename ArrayHead,
    typename ArrayTail,
    typename ArrayList3
    >
struct Dispatch3Trampoline1<Array1T,
                            vtkTypeList::TypeList<ArrayHead, ArrayTail>,
                            ArrayList3>
{
private:
  typedef Dispatch3Trampoline2<Array1T, ArrayHead, ArrayList3> Trampoline;
  typedef Dispatch3Trampoline1<Array1T, ArrayTail, ArrayList3> NextDispatch;
public:
  template <typename Worker>
  static bool Execute(Array1T *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    if (ArrayHead *array = vtkArrayDownCast<ArrayHead>(array2))
    {
      return Trampoline::Execute(array1, array, array3, worker);
    }
    else
    {
      return NextDispatch::Execute(array1, array2, array3, worker);
    }
  }
};


// Dispatch3 Trampoline2 terminal case:
template <
    typename Array1T,
    typename Array2T
    >
struct Dispatch3Trampoline2<Array1T, Array2T, vtkTypeList::NullType>
{
  template <typename Worker>
  static bool Execute(Array1T*, Array2T*, vtkDataArray*, Worker&)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch3 Trampoline2 recursive case:
template <
    typename Array1T,
    typename Array2T,
    typename ArrayHead,
    typename ArrayTail
    >
struct Dispatch3Trampoline2<Array1T, Array2T,
                            vtkTypeList::TypeList<ArrayHead, ArrayTail> >
{
private:
  typedef Dispatch3Trampoline2<Array1T, Array2T, ArrayTail> NextDispatch;
public:
  template <typename Worker>
  static bool Execute(Array1T *array1, Array2T *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    if (ArrayHead *array = vtkArrayDownCast<ArrayHead>(array3))
    {
      worker(array1, array2, array);
      return true;
    }
    else
    {
      return NextDispatch::Execute(array1, array2, array3, worker);
    }
  }
};

//------------------------------------------------------------------------------
// Description:
// Dispatch three arrays, enforcing that all three have the same ValueType.
// Initially, set both ArraysToTest and ArrayList to the same TypeList.
// ArraysToTest is iterated through, while ArrayList is preserved for later
// dispatches.
template <
    typename ArrayList1,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3Same;

// Dispatch3Same terminal case:
template <typename ArrayList2, typename ArrayList3>
struct Dispatch3Same<vtkTypeList::NullType, ArrayList2, ArrayList3>
{
  template <typename Worker>
  static bool Execute(vtkDataArray*, vtkDataArray*,
                      vtkDataArray*, Worker&)
  {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
    vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
    return false;
  }
};

// Dispatch3Same recursive case:
template <
    typename ArrayHead,
    typename ArrayTail,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3Same<
    vtkTypeList::TypeList<ArrayHead, ArrayTail>,
    ArrayList2,
    ArrayList3
    >
{
private:
  typedef vtkTypeList_Create_1(typename ArrayHead::ValueType) ValueType;
  typedef typename FilterArraysByValueType<ArrayList2, ValueType>::Result ValueArrays2;
  typedef typename FilterArraysByValueType<ArrayList3, ValueType>::Result ValueArrays3;
  typedef Dispatch3Trampoline1<ArrayHead, ValueArrays2, ValueArrays3> Trampoline;
  typedef Dispatch3Same<ArrayTail, ArrayList2, ArrayList3> NextDispatch;
public:
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    if (ArrayHead *array = vtkArrayDownCast<ArrayHead>(array1))
    {
      return Trampoline::Execute(array, array2, array3, worker);
    }
    else
    {
      return NextDispatch::Execute(array1, array2, array3, worker);
    }
  }
};

} // end namespace impl

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
struct FilterArraysByValueType<vtkTypeList::TypeList<ArrayHead, ArrayTail>,
                               ValueList>
{
private:
  typedef typename ArrayHead::ValueType ValueType;
  enum { ValueIsAllowed =
           vtkTypeList::IndexOf<ValueList, ValueType>::Result >= 0 };
  typedef typename FilterArraysByValueType<ArrayTail, ValueList>::Result NewTail;
public:
  typedef typename vtkTypeList::Select<ValueIsAllowed,
                                       vtkTypeList::TypeList<ArrayHead, NewTail>,
                                       NewTail>::Result Result;
};

//------------------------------------------------------------------------------
// DispatchByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch:
template <
    typename ArrayHead,
    typename ArrayTail
    >
struct DispatchByArray<vtkTypeList::TypeList<ArrayHead, ArrayTail> >
{
private:
  typedef vtkTypeList::TypeList<ArrayHead, ArrayTail> ArrayList;
  typedef typename vtkTypeList::Unique<ArrayList>::Result UniqueArrays;
  typedef typename vtkTypeList::DerivedToFront<UniqueArrays>::Result SortedUniqueArrays;
  typedef impl::Dispatch<SortedUniqueArrays> ArrayDispatcher;
public:
  DispatchByArray() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *inArray, Worker &worker)
  {
    return ArrayDispatcher::Execute(inArray, worker);
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
  Dispatch() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array, Worker &worker)
  {
    return Dispatcher::Execute(array, worker);
  }
};

//------------------------------------------------------------------------------
// DispatchByValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch
template <
    typename ValueTypeHead,
    typename ValueTypeTail
    >
struct DispatchByValueType<vtkTypeList::TypeList<ValueTypeHead, ValueTypeTail> >
{
private:
  typedef vtkTypeList::TypeList<ValueTypeHead, ValueTypeTail> ValueTypeList;
  typedef typename FilterArraysByValueType<Arrays, ValueTypeList>::Result ArrayList;
  typedef typename vtkTypeList::Unique<ArrayList>::Result UniqueArrays;
  typedef typename vtkTypeList::DerivedToFront<UniqueArrays>::Result SortedUniqueArrays;
  typedef impl::Dispatch<SortedUniqueArrays> ArrayDispatcher;
public:
  DispatchByValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *inArray, Worker &worker)
  {
    return ArrayDispatcher::Execute(inArray, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch2ByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2:
template <
    typename ArrayList1,
    typename ArrayList2
    >
struct Dispatch2ByArray
{
private:
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef impl::Dispatch2<SortedUniqueArray1, SortedUniqueArray2> ArrayDispatcher;
public:
  Dispatch2ByArray() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      Worker &worker)
  {
    return ArrayDispatcher::Execute(array1, array2, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch2 implementation:
//------------------------------------------------------------------------------
struct Dispatch2
{
private:
  typedef Dispatch2ByArray<vtkArrayDispatch::Arrays, vtkArrayDispatch::Arrays> Dispatcher;
public:
  Dispatch2() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      Worker &worker)
  {
    return Dispatcher::Execute(array1, array2, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch2ByValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2
template <
    typename ValueTypeList1,
    typename ValueTypeList2
    >
struct Dispatch2ByValueType
{
private:
  typedef typename FilterArraysByValueType<Arrays, ValueTypeList1>::Result ArrayList1;
  typedef typename FilterArraysByValueType<Arrays, ValueTypeList2>::Result ArrayList2;
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef impl::Dispatch2<SortedUniqueArray1, SortedUniqueArray2> ArrayDispatcher;
public:
  Dispatch2ByValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2, Worker &worker)
  {
    return ArrayDispatcher::Execute(array1, array2, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch2BySameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2Same
template <typename ValueTypeList>
struct Dispatch2BySameValueType
{
private:
  typedef typename FilterArraysByValueType<Arrays, ValueTypeList>::Result ArrayList;
  typedef typename vtkTypeList::Unique<ArrayList>::Result UniqueArray;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray>::Result SortedUniqueArray;
  typedef impl::Dispatch2Same<SortedUniqueArray, SortedUniqueArray> Dispatcher;
public:
  Dispatch2BySameValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      Worker &worker)
  {
    return Dispatcher::Execute(array1, array2, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch2ByArrayWithSameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2Same
template <
    typename ArrayList1,
    typename ArrayList2
    >
struct Dispatch2ByArrayWithSameValueType
{
private:
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef impl::Dispatch2Same<SortedUniqueArray1, SortedUniqueArray2> Dispatcher;
public:
  Dispatch2ByArrayWithSameValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      Worker &worker)
  {
    return Dispatcher::Execute(array1, array2, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch2SameValueType implementation:
//------------------------------------------------------------------------------
struct Dispatch2SameValueType
{
private:
  typedef Dispatch2ByArrayWithSameValueType<vtkArrayDispatch::Arrays, vtkArrayDispatch::Arrays> Dispatcher;
public:
  Dispatch2SameValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      Worker &worker)
  {
    return Dispatcher::Execute(array1, array2, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch3ByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3:
template <
    typename ArrayList1,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3ByArray
{
private:
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::Unique<ArrayList3>::Result UniqueArray3;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray3>::Result SortedUniqueArray3;
  typedef impl::Dispatch3<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3> ArrayDispatcher;
public:
  Dispatch3ByArray() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    return ArrayDispatcher::Execute(array1, array2, array3, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch3 implementation:
//------------------------------------------------------------------------------
struct Dispatch3
{
private:
  typedef Dispatch3ByArray<vtkArrayDispatch::Arrays,
                           vtkArrayDispatch::Arrays,
                           vtkArrayDispatch::Arrays> Dispatcher;
public:
  Dispatch3() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    return Dispatcher::Execute(array1, array2, array3, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch3ByValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3
template <
    typename ValueTypeList1,
    typename ValueTypeList2,
    typename ValueTypeList3
    >
struct Dispatch3ByValueType
{
private:
  typedef typename FilterArraysByValueType<Arrays, ValueTypeList1>::Result ArrayList1;
  typedef typename FilterArraysByValueType<Arrays, ValueTypeList2>::Result ArrayList2;
  typedef typename FilterArraysByValueType<Arrays, ValueTypeList3>::Result ArrayList3;
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::Unique<ArrayList3>::Result UniqueArray3;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray3>::Result SortedUniqueArray3;
  typedef impl::Dispatch3<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3> ArrayDispatcher;
public:
  Dispatch3ByValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    return ArrayDispatcher::Execute(array1, array2, array3, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch3BySameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3Same
template <typename ValueTypeList>
struct Dispatch3BySameValueType
{
private:
  typedef typename FilterArraysByValueType<Arrays, ValueTypeList>::Result ArrayList;
  typedef typename vtkTypeList::Unique<ArrayList>::Result UniqueArray;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray>::Result SortedUniqueArray;
  typedef impl::Dispatch3Same<SortedUniqueArray, SortedUniqueArray, SortedUniqueArray> Dispatcher;
public:
  Dispatch3BySameValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    return Dispatcher::Execute(array1, array2, array3, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch3BySameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3Same
template <
    typename ArrayList1,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3ByArrayWithSameValueType
{
private:
  typedef typename vtkTypeList::Unique<ArrayList1>::Result UniqueArray1;
  typedef typename vtkTypeList::Unique<ArrayList2>::Result UniqueArray2;
  typedef typename vtkTypeList::Unique<ArrayList3>::Result UniqueArray3;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray1>::Result SortedUniqueArray1;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray2>::Result SortedUniqueArray2;
  typedef typename vtkTypeList::DerivedToFront<UniqueArray3>::Result SortedUniqueArray3;
  typedef impl::Dispatch3Same<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3> Dispatcher;
public:
  Dispatch3ByArrayWithSameValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    return Dispatcher::Execute(array1, array2, array3, worker);
  }
};

//------------------------------------------------------------------------------
// Dispatch3SameValueType implementation:
//------------------------------------------------------------------------------
struct Dispatch3SameValueType
{
private:
  typedef Dispatch3ByArrayWithSameValueType<vtkArrayDispatch::Arrays,
                                            vtkArrayDispatch::Arrays,
                                            vtkArrayDispatch::Arrays> Dispatcher;
public:
  Dispatch3SameValueType() {}
  template <typename Worker>
  static bool Execute(vtkDataArray *array1, vtkDataArray *array2,
                      vtkDataArray *array3, Worker &worker)
  {
    return Dispatcher::Execute(array1, array2, array3, worker);
  }
};

} // end namespace vtkArrayDispatch

#endif // vtkArrayDispatch_txx
