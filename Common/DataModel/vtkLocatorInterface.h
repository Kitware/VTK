// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLocatorInterface
 * @brief   A variety of classes used for high-performance interface
 *          to spatial locator classes.
 *
 * vtkLocatorInterface provides a set of light-weight classes and structs for
 * interface to various spatial locator classes. These are used so that
 * information can be efficiently transferred between the locator and calling
 * methods.
 *
 * @sa
 * vtkStaticPointLocator vtkStaticPointLocator2D
 */

#ifndef vtkLocatorInterface_h
#define vtkLocatorInterface_h

#include "vtkABINamespace.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSystemIncludes.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class   vtkDist2Tuple
 * @brief   Represent a 2-tuple consisting of a VTK Id, and a distance**2 value.
 *
 * vtkDist2Tuple is used to represent a distance of some entity specified by VTK Id
 * (e.g., typically a point id) to another entity.
 */
struct vtkDist2Tuple
{
  vtkIdType Id;
  double Dist2;
  vtkDist2Tuple()
    : Id(-1)
    , Dist2(0)
  {
  }
  vtkDist2Tuple(vtkIdType id, double d2)
    : Id(id)
    , Dist2(d2)
  {
  }

  // Support sort operation from smallest to largest.
  bool operator<(const vtkDist2Tuple& tuple) const { return Dist2 < tuple.Dist2; }
};

/**
 * @class  vtkDist2TupleArray
 * @brief   Represent an array of vtkDist2Tuples. It derives from
 *          std::vector<vtkDist2Tuple>.
 *
 * vtkDist2TupleArray is shorthand for std::vector<vtkDist2Tuple>. It can be
 * readily used in wrapped VTK interfaces.
 */
struct VTKCOMMONDATAMODEL_EXPORT vtkDist2TupleArray : public std::vector<vtkDist2Tuple>
{
};
typedef std::vector<vtkDist2Tuple> vtkDist2TupleType;
typedef std::vector<vtkDist2Tuple>::iterator vtkDist2TupleIterator;

//------------------------------------------------------------------------------
// The following tuple is what is sorted in the locator maps (i.e., a map of
// bucket/bins to point ids). Note that it is templated because depending on
// the number of points / buckets to process we may want to use vtkIdType to
// represent the tuple. Otherwise for performance reasons it's best to use an
// int (or other integral type). Typically sort() is 25-30% faster on smaller
// integral types, plus it takes a heck less memory (when vtkIdType is 64-bit
// and int is 32-bit).
template <typename TTuple>
struct vtkLocatorTuple
{
  TTuple PtId;   // originating point id
  TTuple Bucket; // i-j-k index into bucket space

  //  Operator< used to support the subsequent sort operation. There are two
  //  implementations, one gives a stable sort (points ordered by id within
  //  each bucket) and the other a little faster but less stable (in parallel
  //  sorting the order of sorted points in a bucket may vary).
  bool operator<(const vtkLocatorTuple& tuple) const
  {
    if (Bucket < tuple.Bucket)
      return true;
    if (tuple.Bucket < Bucket)
      return false;
    if (PtId < tuple.PtId)
      return true;
    return false;
  }
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkLocatorInterface.h
