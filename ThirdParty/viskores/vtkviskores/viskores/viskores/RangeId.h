//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_RangeId_h
#define viskores_RangeId_h

#include <viskores/Math.h>
#include <viskores/Types.h>

namespace viskores
{

/// \brief Represent a range of viskores::Id values.
///
/// \c viskores::RangeId is a helper class for representing a range of viskores::Id
/// values. This is specified simply with a \c Min and \c Max value, where
/// \c Max is exclusive.
///
/// \c RangeId also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId
{
  /// The minimum index of the range (inclusive).
  viskores::Id Min;
  /// The maximum index of the range (exclusive).
  viskores::Id Max;

  /// Construct a range with no indices.
  VISKORES_EXEC_CONT
  RangeId()
    : Min(0)
    , Max(0)
  {
  }

  /// Construct a range with the given minimum (inclusive) and maximum (exclusive)
  /// indices.
  VISKORES_EXEC_CONT
  RangeId(viskores::Id min, viskores::Id max)
    : Min(min)
    , Max(max)
  {
  }

  /// \b Determine if the range is valid.
  ///
  /// \c IsNonEmpty return true if the range contains some valid values between
  /// \c Min and \c Max. If \c Max <= \c Min, then no values satisfy
  /// the range and \c IsNonEmpty returns false. Otherwise, return true.
  ///
  VISKORES_EXEC_CONT
  bool IsNonEmpty() const { return (this->Min < this->Max); }

  /// \b Determines if a value is within the range.
  ///
  /// \c Contains returns true if the give value is within the range, false
  /// otherwise.
  ///
  VISKORES_EXEC_CONT
  bool Contains(viskores::Id value) const { return ((this->Min <= value) && (this->Max > value)); }

  /// \b Returns the length of the range.
  ///
  /// \c Length computes the distance between the min and max. If the range
  /// is empty, 0 is returned.
  ///
  VISKORES_EXEC_CONT
  viskores::Id Length() const { return this->Max - this->Min; }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle value of the range.
  ///
  VISKORES_EXEC_CONT
  viskores::Id Center() const { return (this->Min + this->Max) / 2; }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already includes this value, then nothing is
  /// done.
  ///
  VISKORES_EXEC_CONT
  void Include(viskores::Id value)
  {
    this->Min = viskores::Min(this->Min, value);
    this->Max = viskores::Max(this->Max, value + 1);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands this range just enough to include that
  /// of another range. Essentially it is the union of the two ranges.
  ///
  VISKORES_EXEC_CONT
  void Include(const viskores::RangeId& range)
  {
    this->Min = viskores::Min(this->Min, range.Min);
    this->Max = viskores::Max(this->Max, range.Max);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VISKORES_EXEC_CONT
  viskores::RangeId Union(const viskores::RangeId& other) const
  {
    viskores::RangeId unionRange(*this);
    unionRange.Include(other);
    return unionRange;
  }

  /// \b Operator for union
  ///
  VISKORES_EXEC_CONT
  viskores::RangeId operator+(const viskores::RangeId& other) const { return this->Union(other); }

  VISKORES_EXEC_CONT
  bool operator==(const viskores::RangeId& other) const
  {
    return ((this->Min == other.Min) && (this->Max == other.Max));
  }

  VISKORES_EXEC_CONT
  bool operator!=(const viskores::RangeId& other) const
  {
    return ((this->Min != other.Min) || (this->Max != other.Max));
  }
};

/// Helper function for printing ranges during testing
///
static inline VISKORES_CONT std::ostream& operator<<(std::ostream& stream,
                                                     const viskores::RangeId& range)
{
  return stream << "[" << range.Min << ".." << range.Max << ")";
} // Declared inside of viskores namespace so that the operator work with ADL lookup
} // namespace viskores

#endif // viskores_RangeId_h
