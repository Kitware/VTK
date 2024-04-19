// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkRangeIterableTraits_h
#define vtkRangeIterableTraits_h

#include "vtkABINamespace.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCollection;
class vtkCompositeDataSet;
class vtkDataObjectTree;
VTK_ABI_NAMESPACE_END

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

template <typename CollectionType>
struct CollectionRange;

struct CompositeDataSetRange;
struct DataObjectTreeRange;

//------------------------------------------------------------------------------
// DeduceRangeType:
// These function signatures define a mapping from an Iterable (e.g. vtkObject)
// to a RangeType (e.g. the for-range iterable object).
// They are not implemented, as only the signatures are important. Classes used
// should only be forward declared in this header.
// Since classes are only forward declared, the argument type should be const&
// qualified, and the return type should be a reference.

// vtkCollection subclasses --> CollectionRange
template <typename CollectionType,
  typename = typename std::enable_if<std::is_base_of<vtkCollection, CollectionType>::value>::type>
CollectionRange<CollectionType>& DeduceRangeType(const CollectionType&);

// vtkCompositeDataSet --> CompositeDataSetRange
CompositeDataSetRange& DeduceRangeType(const vtkCompositeDataSet&);

// vtkDataObjectTree --> DataObjectTreeRange
DataObjectTreeRange& DeduceRangeType(const vtkDataObjectTree&);

// Traits class that defines a RangeType corresponding to the iterable range
// type most appropriate for Iterable.
template <typename Iterable>
struct IterableTraits
{
private:
  using RangeTypeInternal = decltype(vtk::detail::DeduceRangeType(std::declval<Iterable>()));

public:
  using RangeType = typename std::decay<RangeTypeInternal>::type;
};

VTK_ABI_NAMESPACE_END
}
}

#endif // vtkRangeIterableTraits_h

// VTK-HeaderTest-Exclude: vtkRangeIterableTraits.h
