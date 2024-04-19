// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2001 by Andrei Alexandrescu
// SPDX-FileCopyrightText: Copyright (c) 2001. Addison-Wesley.
// SPDX-License-Identifier: BSD-3-Clause AND MIT

////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied".
////////////////////////////////////////////////////////////////////////////////

/**
 * @class   vtkTypeList
 * @brief   TypeList implementation and utilities.
 *
 *
 * vtkTypeList provides a way to collect a list of types using C++ templates.
 * In VTK, this is used heavily by the vtkArrayDispatch system to instantiate
 * templated code for specific array implementations. The book "Modern C++
 * Design: Generic Programming and Design Patterns Applied" by Andrei
 * Alexandrescu provides additional details and applications for typeLists. This
 * implementation is heavily influenced by the example code in the book.
 *
 * Note that creating a typelist in C++ is simplified greatly by using the
 * vtkTypeList::Create<T1, T2, ...> functions.
 *
 * @sa
 * vtkArrayDispatch vtkTypeListMacros
 */

#ifndef vtkTypeList_h
#define vtkTypeList_h

#include "vtkABINamespace.h"
#include "vtkTypeListMacros.h"

namespace vtkTypeList
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
/**
 * Used to terminate a TypeList.
 */
struct NullType
{
};

//------------------------------------------------------------------------------
///@{
/**
 * Generic implementation of TypeList.
 */
template <typename T, typename U>
struct TypeList
{
  typedef T Head;
  typedef U Tail;
};
///@}

//------------------------------------------------------------------------------
/**
 * Sets Result to T if Exp is true, or F if Exp is false.
 */
template <bool Exp, typename T, typename F>
struct Select;

//------------------------------------------------------------------------------
/**
 * Sets member Result to true if a conversion exists to convert type From to
 * type To. Member SameType will be true if the types are identical.
 */
template <typename From, typename To>
struct CanConvert;

//------------------------------------------------------------------------------
/**
 * Sets the enum value Result to the index of type T in the TypeList TList.
 * Result will equal -1 if the type is not found.
 */
template <typename TList, typename T>
struct IndexOf;

//------------------------------------------------------------------------------
/**
 * Erase the first element of type T from TypeList TList, storing the new list
 * in Result.
 */
template <typename TList, typename T>
struct Erase;

//------------------------------------------------------------------------------
/**
 * Erase all type T from TypeList TList, storing the new list in Result.
 */
template <typename TList, typename T>
struct EraseAll;

//------------------------------------------------------------------------------
/**
 * Remove all duplicate types from TypeList TList, storing the new list in
 * Result.
 */
template <typename TList>
struct Unique;

//------------------------------------------------------------------------------
/**
 * Replace the first instance of Bad with Good in the TypeList TList, storing
 * the new list in Result.
 */
template <typename TList, typename Bad, typename Good>
struct Replace;

//------------------------------------------------------------------------------
/**
 * Replace all instances of Bad with Good in the TypeList TList, storing the
 * new list in Result.
 */
template <typename TList, typename Bad, typename Good>
struct ReplaceAll;

//------------------------------------------------------------------------------
/**
 * Given a type T and a TypeList TList, store the most derived type of T in
 * TList as Result. If no subclasses of T exist in TList, T will be set as
 * Result, even if T itself is not in TList.
 */
template <typename TList, typename T>
struct MostDerived;

//------------------------------------------------------------------------------
/**
 * Sort the TypeList from most-derived to least-derived type, storing the
 * sorted TypeList in Result. Note that the input TypeList cannot have duplicate
 * types (see Unique).
 */
template <typename TList>
struct DerivedToFront;

//------------------------------------------------------------------------------
/**
 * Appends type T to TypeList TList and stores the result in Result.
 */
template <typename TList, typename T>
struct Append;

VTK_ABI_NAMESPACE_END
} // end namespace vtkTypeList

#include "vtkTypeList.txx"

namespace vtkTypeList
{
VTK_ABI_NAMESPACE_BEGIN

template <typename... Ts>
using Create = typename vtkTypeList::detail::CreateImpl<Ts...>::type;

VTK_ABI_NAMESPACE_END
} // end namespace vtkTypeList

#endif // vtkTypeList_h
// VTK-HeaderTest-Exclude: vtkTypeList.h
