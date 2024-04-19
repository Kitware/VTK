// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkTypeList_txx
#define vtkTypeList_txx

#include "vtkTypeList.h"

namespace vtkTypeList
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

template <typename... Ts>
struct CreateImpl;

// Unroll small cases to help out compiler:
template <typename T1, typename T2, typename T3, typename T4>
struct CreateImpl<T1, T2, T3, T4>
{
  using type = vtkTypeList::TypeList<T1,
    vtkTypeList::TypeList<T2,
      vtkTypeList::TypeList<T3, vtkTypeList::TypeList<T4, vtkTypeList::NullType>>>>;
};

template <typename T1, typename T2, typename T3>
struct CreateImpl<T1, T2, T3>
{
  using type = vtkTypeList::TypeList<T1,
    vtkTypeList::TypeList<T2, vtkTypeList::TypeList<T3, vtkTypeList::NullType>>>;
};

template <typename T1, typename T2>
struct CreateImpl<T1, T2>
{
  using type = vtkTypeList::TypeList<T1, vtkTypeList::TypeList<T2, vtkTypeList::NullType>>;
};

template <typename T1>
struct CreateImpl<T1>
{
  using type = vtkTypeList::TypeList<T1, vtkTypeList::NullType>;
};

template <>
struct CreateImpl<>
{
  using type = vtkTypeList::NullType;
};

template <typename T1, typename T2, typename T3, typename T4, typename... Tail>
struct CreateImpl<T1, T2, T3, T4, Tail...>
{
  using type = vtkTypeList::TypeList<T1,
    vtkTypeList::TypeList<T2,
      vtkTypeList::TypeList<T3,
        vtkTypeList::TypeList<T4, typename vtkTypeList::detail::CreateImpl<Tail...>::type>>>>;
};

VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Description:
// Sets Result to T if Exp is true, or F if Exp is false.
template <bool Exp, typename T, typename F>
struct Select // True case:
{
  typedef T Result;
};

// False case:
template <typename T, typename F>
struct Select<false, T, F>
{
  typedef F Result;
};

//------------------------------------------------------------------------------
// Description:
// Sets member Result to true if a conversion exists to convert type From to
// type To. Member SameType will be true if the types are identical.
template <typename From, typename To>
struct CanConvert
{
private:
  typedef char Small;
  class Big
  {
    char dummy[2];
  };
  static Small DoTest(const To&); // Not implemented
  static Big DoTest(...);         // Not implemented
  static From* MakeFrom();        // Not implemented
public:
  enum
  {
    Result = (sizeof(DoTest(*MakeFrom())) == sizeof(Small)),
    SameType = false
  };
};

// Specialize for SameType:
template <typename T>
struct CanConvert<T, T>
{
  enum
  {
    Result = true,
    SameType = true
  };
};

//------------------------------------------------------------------------------
// Description:
// Sets the enum value Result to the index of type T in the TypeList TList.
// Result will equal -1 if the type is not found.

// Terminal case:
template <typename T>
struct IndexOf<NullType, T>
{
  enum
  {
    Result = -1
  };
};

// Matching case:
template <typename T, typename Tail>
struct IndexOf<TypeList<T, Tail>, T>
{
  enum
  {
    Result = 0
  };
};

// Recursive case:
template <typename T, typename U, typename Tail>
struct IndexOf<TypeList<U, Tail>, T>
{
private:
  enum
  {
    TailResult = IndexOf<Tail, T>::Result
  };

public:
  enum
  {
    Result = TailResult == -1 ? -1 : 1 + TailResult
  };
};

//------------------------------------------------------------------------------
// Description:
// Erase the first element of type T from TypeList TList.

// Terminal case:
template <typename T>
struct Erase<NullType, T>
{
  typedef NullType Result;
};

// Match:
template <typename Tail, typename T>
struct Erase<TypeList<T, Tail>, T>
{
  typedef Tail Result;
};

// No match:
template <typename Head, typename Tail, typename T>
struct Erase<TypeList<Head, Tail>, T>
{
  typedef TypeList<Head, typename Erase<Tail, T>::Result> Result;
};

//------------------------------------------------------------------------------
// Description:
// Erase all type T from TypeList TList.

// Terminal case:
template <typename T>
struct EraseAll<NullType, T>
{
  typedef NullType Result;
};

// Match:
template <typename Tail, typename T>
struct EraseAll<TypeList<T, Tail>, T>
{
  typedef typename EraseAll<T, Tail>::Result Result;
};

// No match:
template <typename Head, typename Tail, typename T>
struct EraseAll<TypeList<Head, Tail>, T>
{
  typedef TypeList<Head, typename EraseAll<Tail, T>::Result> Result;
};

//------------------------------------------------------------------------------
// Description:
// Remove all duplicate types from TypeList TList

// Terminal case:
template <>
struct Unique<NullType>
{
  typedef NullType Result;
};

template <typename Head, typename Tail>
struct Unique<TypeList<Head, Tail>>
{
private:
  typedef typename Unique<Tail>::Result UniqueTail;
  typedef typename Erase<UniqueTail, Head>::Result NewTail;

public:
  typedef TypeList<Head, NewTail> Result;
};

//------------------------------------------------------------------------------
// Description:
// Replace the first instance of Bad with Good in the TypeList TList.

// Terminal case:
template <typename Bad, typename Good>
struct Replace<NullType, Bad, Good>
{
  typedef NullType Result;
};

// Match:
template <typename Tail, typename Bad, typename Good>
struct Replace<TypeList<Bad, Tail>, Bad, Good>
{
  typedef TypeList<Good, Tail> Result;
};

// No match:
template <typename Head, typename Tail, typename Bad, typename Good>
struct Replace<TypeList<Head, Tail>, Bad, Good>
{
  typedef TypeList<Head, typename Replace<Tail, Bad, Good>::Result> Result;
};

// Trivial case:
template <typename Head, typename Tail, typename T>
struct Replace<TypeList<Head, Tail>, T, T>
{
  typedef TypeList<Head, Tail> Result;
};

//------------------------------------------------------------------------------
// Description:
// Replace all instances of Bad with Good in the TypeList TList.

// Terminal case:
template <typename Bad, typename Good>
struct ReplaceAll<NullType, Bad, Good>
{
  typedef NullType Result;
};

// Match:
template <typename Tail, typename Bad, typename Good>
struct ReplaceAll<TypeList<Bad, Tail>, Bad, Good>
{
  typedef TypeList<Good, typename ReplaceAll<Tail, Bad, Good>::Result> Result;
};

// No match:
template <typename Head, typename Tail, typename Bad, typename Good>
struct ReplaceAll<TypeList<Head, Tail>, Bad, Good>
{
  typedef TypeList<Head, typename ReplaceAll<Tail, Bad, Good>::Result> Result;
};

// Trivial case:
template <typename Head, typename Tail, typename T>
struct ReplaceAll<TypeList<Head, Tail>, T, T>
{
  typedef TypeList<Head, Tail> Result;
};

//------------------------------------------------------------------------------
// Description:
// Given a type T and a TypeList TList, store the most derived type of T in
// TList as Result. If no subclasses of T exist in TList, T will be set as
// Result, even if T itself is not in TList.

// Terminal case:
template <typename T>
struct MostDerived<NullType, T>
{
  typedef T Result;
};

// Recursive case:
template <typename Head, typename Tail, typename T>
struct MostDerived<TypeList<Head, Tail>, T>
{
private:
  typedef typename MostDerived<Tail, T>::Result TailResult;

public:
  typedef typename Select<CanConvert<TailResult, Head>::Result, Head, TailResult>::Result Result;
};

//------------------------------------------------------------------------------
// Description:
// Sort the TypeList from most-derived to least-derived type, storing the
// sorted TypeList in Result. Note that the input TypeList cannot have duplicate
// types (see Unique).

// Terminal case:
template <>
struct DerivedToFront<NullType>
{
  typedef NullType Result;
};

// Recursive case:
template <typename Head, typename Tail>
struct DerivedToFront<TypeList<Head, Tail>>
{
private:
  typedef typename MostDerived<Tail, Head>::Result Derived;
  typedef typename Replace<Tail, Derived, Head>::Result Replaced;
  typedef typename DerivedToFront<Replaced>::Result NewTail;

public:
  typedef TypeList<Derived, NewTail> Result;
};

//------------------------------------------------------------------------------
// Description:
// Appends type T to TypeList TList and stores the result in Result.

// Corner case, both are NullType:
template <>
struct Append<vtkTypeList::NullType, vtkTypeList::NullType>
{
  typedef vtkTypeList::NullType Result;
};

// Terminal case (Single type):
template <typename T>
struct Append<vtkTypeList::NullType, T>
{
  typedef vtkTypeList::TypeList<T, vtkTypeList::NullType> Result;
};

// Terminal case (TypeList):
template <typename Head, typename Tail>
struct Append<vtkTypeList::NullType, vtkTypeList::TypeList<Head, Tail>>
{
  typedef vtkTypeList::TypeList<Head, Tail> Result;
};

// Recursive case:
template <typename Head, typename Tail, typename T>
struct Append<vtkTypeList::TypeList<Head, Tail>, T>
{
  typedef vtkTypeList::TypeList<Head, typename Append<Tail, T>::Result> Result;
};

VTK_ABI_NAMESPACE_END
}

#endif // vtkTypeList_txx
