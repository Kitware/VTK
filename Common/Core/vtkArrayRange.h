// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkArrayRange
 * @brief   Stores a half-open range of array coordinates.
 *
 *
 * vtkArrayRange stores a half-open range of array coordinates along a
 * single dimension of a vtkArraySlice object.
 *
 * @sa
 * vtkArray, vtkArrayRange
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
 * Laboratories.
 */

#ifndef vtkArrayRange_h
#define vtkArrayRange_h

#include "vtkArrayCoordinates.h"
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkArrayRange
{
public:
  typedef vtkArrayCoordinates::CoordinateT CoordinateT;

  /**
   * Creates an empty range.
   */
  vtkArrayRange();

  /**
   * Creates a half-open range [begin, end).
   * Note that begin must be <= end,
   * if not, creates the empty range [begin, begin).
   */
  vtkArrayRange(CoordinateT begin, CoordinateT end);

  /**
   * Returns the beginning of the range
   */
  CoordinateT GetBegin() const;

  /**
   * Returns one-past-the-end of the range
   */
  CoordinateT GetEnd() const;

  /**
   * Returns the size of the range (the distance End - Begin).
   */
  CoordinateT GetSize() const;

  /**
   * Returns true iff the given range is a non-overlapping subset of this
   * range.
   */
  bool Contains(const vtkArrayRange& range) const;

  /**
   * Returns true iff the given coordinate falls within this range.
   */
  bool Contains(CoordinateT coordinate) const;

  ///@{
  /**
   * Equality comparisons.
   */
  VTKCOMMONCORE_EXPORT friend bool operator==(const vtkArrayRange& lhs, const vtkArrayRange& rhs);
  VTKCOMMONCORE_EXPORT friend bool operator!=(const vtkArrayRange& lhs, const vtkArrayRange& rhs);
  ///@}

  /**
   * Serialization.
   */
  VTKCOMMONCORE_EXPORT friend ostream& operator<<(ostream& stream, const vtkArrayRange& rhs);

private:
  /**
   * Stores the beginning of the range.
   */
  CoordinateT Begin;

  ///@{
  /**
   * Stores one-past-the-end of the range.
   */
  CoordinateT End;
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkArrayRange.h
