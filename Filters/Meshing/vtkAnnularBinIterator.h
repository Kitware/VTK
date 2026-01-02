// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnnularBinIterator
 * @brief   A fast, lightweight class for iterating over the bins of
 *          a 2D vtkStaticPointLocator2D
 *
 * vtkAnnularBinIterator iterates over the bins of a (regular binning) 2D
 * static point locator.  Given an initial starting position x[2], it visits
 * the surrounding bins in nested annular, shells, akin to a "annular"
 * traversal (i.e., in nested, hollow squares of bins).
 *
 * @sa
 * Note that the vtkStaticPointLocator2D class is internally templated, but
 * provides a non-templated API - this is done to ensure that the class can
 * be easily used by interpreted, wrapped languages (e.g., Python).  This
 * iterator class follows this pattern.
 *
 * @sa
 * vtkStaticPointLocator2D vtkVoronoiCore2D
 */
#ifndef vtkAnnularBinIterator_h
#define vtkAnnularBinIterator_h

#include "vtkFiltersMeshingModule.h" // For export macro
#include "vtkLocatorInterface.h"     // for array of (pt,d**2) tuples
#include "vtkSystemIncludes.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkStaticPointLocator2D;
class vtkDoubleArray;
struct InternalAnnularBinIterator;

// A simple dispatch mechanism to internal, templated classes.
VTK_WRAPEXCLUDE struct VTKFILTERSMESHING_EXPORT vtkAnnularBinIteratorDispatch
{
  bool LargeIds;
  InternalAnnularBinIterator* Iterator;

  vtkAnnularBinIteratorDispatch()
    : LargeIds(false)
    , Iterator(nullptr)
  {
  }
  vtkAnnularBinIteratorDispatch(vtkStaticPointLocator2D* loc) { this->Initialize(loc); }
  void Initialize(vtkStaticPointLocator2D* locator);
  bool Begin(vtkIdType pid, double x[3], vtkDist2TupleArray& results);
  bool Next(double radius2, vtkDoubleArray* spheres, vtkDist2TupleArray& results);
  vtkIdType GetBinId();
  void GetBin(int IJ[2]);
  double GetMinD2();
}; // vtkAnnularBinIteratorDispatch

// VTK class proper
class VTKFILTERSMESHING_EXPORT vtkAnnularBinIterator
{
public:
  /**
   * Construct default iterator.
   */
  vtkAnnularBinIterator() = default;

  /**
   * Construct the iterator with a vtkStaticPointLocator2D. The
   * vtkStaticPointLocator2D must have invoked BuildLocator() prior to
   * construction of this iterator.
   */
  vtkAnnularBinIterator(vtkStaticPointLocator2D* loc)
    : Dispatch(loc)
  {
  }

  /**
   * Initialize an iterator with the associated vtkStaticPointLocator2D over
   * which to iterate. The vtkStaticPointLocator2D must have invoked
   * BuildLocator() prior to initialization of this iterator.
   */
  void Initialize(vtkStaticPointLocator2D* locator) { this->Dispatch.Initialize(locator); }

  /**
   * Begin iterating over the bins, starting with point ptId at position
   * x[3]. Any points contained in this initial bin are returned in the
   * results array.
   */
  bool Begin(vtkIdType ptId, double x[3], vtkDist2TupleArray& results)
  {
    return this->Dispatch.Begin(ptId, x, results);
  }

  /**
   * Move to the next bin, returning all points that are inside the circle
   * given by x[2] (specified in Begin()) and associated radius**2, and
   * an optional array of circles.
   */
  bool Next(double radius2, vtkDoubleArray* circles, vtkDist2TupleArray& results)
  {
    return this->Dispatch.Next(radius2, circles, results);
  }

  /**
   * Return the current bin/bucket id of traversal.
   */
  vtkIdType GetBinId() { return this->Dispatch.GetBinId(); }

  /**
   * Return the current bin/bucket index IJ of traversal.
   */
  void GetBin(int IJ[2]) { this->Dispatch.GetBin(IJ); }

  /**
   * Return the minimum distance of the current annulus of bins to the
   * initial starting point x[2].
   */
  double GetMinD2() { return this->Dispatch.GetMinD2(); }

private:
  // Used to dispatch to the internally instantiated templated classes.
  vtkAnnularBinIteratorDispatch Dispatch;
};

VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkAnnularBinIterator.h
