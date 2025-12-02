// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkShellBinIterator
 * @brief   A fast, lightweight class for iterating over the bins of
 *          a 3D vtkStaticPointLocator
 *
 * vtkShellBinIterator iterates over the bins of a (regular binning)
 * static point locator.  Given an initial starting position x[3], it visits
 * the surrounding bins in nested shells, akin to a "spherical" traversal
 * (i.e., in nested, hollow blocks of bins).
 *
 * @sa
 * Note that the vtkStaticPointLocator class is internally templated, but
 * provides a non-templated API - this is done to ensure that the class can
 * be easily used by interpreted, wrapped languages (e.g., Python).  This
 * iterator class follows this pattern.
 *
 * @sa
 * vtkStaticPointLocator vtkVoronoiCore3D
 */
#ifndef vtkShellBinIterator_h
#define vtkShellBinIterator_h

#include "vtkFiltersMeshingModule.h" // For export macro
#include "vtkLocatorInterface.h"     // for array of (pt,d**2) tuples
#include "vtkSystemIncludes.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkStaticPointLocator;
class vtkDoubleArray;
struct InternalShellBinIterator;

// A simple dispatch mechanism to internal, templated classes.
VTK_WRAPEXCLUDE struct VTKFILTERSMESHING_EXPORT vtkShellBinIteratorDispatch
{
  bool LargeIds;
  InternalShellBinIterator* Iterator;

  vtkShellBinIteratorDispatch()
    : LargeIds(false)
    , Iterator(nullptr)
  {
  }
  vtkShellBinIteratorDispatch(vtkStaticPointLocator* loc) { this->Initialize(loc); }
  void Initialize(vtkStaticPointLocator* locator);
  bool Begin(vtkIdType pid, double x[3], vtkDist2TupleArray& results);
  bool Next(double radius2, vtkDoubleArray* spheres, vtkDist2TupleArray& results);
  vtkIdType GetBinId();
  void GetBin(int IJK[3]);
  double GetMinD2();
}; // vtkShellBinIteratorDispatch

// VTK class proper
class VTKFILTERSMESHING_EXPORT vtkShellBinIterator
{
public:
  /**
   * Construct a default iterator.
   */
  vtkShellBinIterator() = default;

  /**
   * Construct the iterator with a vtkStaticPointLocator. The
   * vtkStaticPointLocator must have invoked BuildLocator() prior to
   * construction of this iterator.
   */
  vtkShellBinIterator(vtkStaticPointLocator* loc)
    : Dispatch(loc)
  {
  }

  /**
   * Initialize an iterator with the associated vtkStaticPointLocator over
   * which to iterate. The vtkStaticPointLocator must have invoked
   * BuildLocator() prior to initialization of this iterator.
   */
  void Initialize(vtkStaticPointLocator* locator) { this->Dispatch.Initialize(locator); }

  /**
   * Begin iterating over the bins, starting with point ptId at
   * position x[3]. Any points in this initial bin are returned
   * in the results array.
   */
  bool Begin(vtkIdType ptId, double x[3], vtkDist2TupleArray& results)
  {
    return this->Dispatch.Begin(ptId, x, results);
  }

  /**
   * Move to the next bin, returning all points that are inside the sphere
   * given by x[3] (specified in Begin()) and associated radius**2, and
   * an optional array of spheres.
   */
  bool Next(double radius2, vtkDoubleArray* spheres, vtkDist2TupleArray& results)
  {
    return this->Dispatch.Next(radius2, spheres, results);
  }

  /**
   * Return the current bin/bucket id of traversal.
   */
  vtkIdType GetBinId() { return this->Dispatch.GetBinId(); }

  /**
   * Return the current bin/bucket index IJK of traversal.
   */
  void GetBin(int IJK[3]) { this->Dispatch.GetBin(IJK); }

  /**
   * Return the minimum distance of the current shell of bins to the initial
   * starting point x[3].
   */
  double GetMinD2() { return this->Dispatch.GetMinD2(); }

private:
  // Used to dispatch to the internally instantiated templated classes.
  vtkShellBinIteratorDispatch Dispatch;
};

VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkShellBinIterator.h
