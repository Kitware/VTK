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
#ifndef viskores_RangeId2_h
#define viskores_RangeId2_h

#include <viskores/RangeId.h>

namespace viskores
{

/// @brief Represent 2D integer range.
///
/// `viskores::RangeId2` is a helper class for representing a 2D range of integer
/// values. The typical use of this class is to express a box of indices
/// in the x and y directions.
///
/// `RangeId2` also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId2
{
  /// The range of values in the X direction. The `viskores::RangeId` struct provides
  /// the minimum and maximum along that axis.
  viskores::RangeId X;
  /// The range of values in the Y direction. The `viskores::RangeId` struct provides
  /// the minimum and maximum along that axis.
  viskores::RangeId Y;

  /// Construct an empty 2D range.
  RangeId2() = default;

  /// Construct a range with the given x and y directions.
  VISKORES_EXEC_CONT
  RangeId2(const viskores::RangeId& xrange, const viskores::RangeId& yrange)
    : X(xrange)
    , Y(yrange)
  {
  }

  /// Construct a range with the given minimum (inclusive) and maximum (exclusive) points.
  VISKORES_EXEC_CONT
  RangeId2(viskores::Id minX, viskores::Id maxX, viskores::Id minY, viskores::Id maxY)
    : X(viskores::RangeId(minX, maxX))
    , Y(viskores::RangeId(minY, maxY))
  {
  }

  /// Initialize range with an array of 4 values in the order xmin, xmax,
  /// ymin, ymax.
  ///
  VISKORES_EXEC_CONT
  explicit RangeId2(const viskores::Id range[4])
    : X(viskores::RangeId(range[0], range[1]))
    , Y(viskores::RangeId(range[2], range[3]))
  {
  }

  /// Initialize range with the minimum and the maximum corners
  ///
  VISKORES_EXEC_CONT
  RangeId2(const viskores::Id2& min, const viskores::Id2& max)
    : X(viskores::RangeId(min[0], max[0]))
    , Y(viskores::RangeId(min[1], max[1]))
  {
  }

  /// \b Determine if the range is non-empty.
  ///
  /// \c IsNonEmpty returns true if the range is non-empty.
  ///
  VISKORES_EXEC_CONT
  bool IsNonEmpty() const { return (this->X.IsNonEmpty() && this->Y.IsNonEmpty()); }

  /// \b Determines if an Id2 value is within the range.
  ///
  VISKORES_EXEC_CONT
  bool Contains(const viskores::Id2& val) const
  {
    return (this->X.Contains(val[0]) && this->Y.Contains(val[1]));
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle of the range.
  ///
  VISKORES_EXEC_CONT
  viskores::Id2 Center() const { return viskores::Id2(this->X.Center(), this->Y.Center()); }

  VISKORES_EXEC_CONT
  viskores::Id2 Dimensions() const { return viskores::Id2(this->X.Length(), this->Y.Length()); }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already include this value, then
  /// nothing is done.
  ///
  template <typename T>
  VISKORES_EXEC_CONT void Include(const viskores::Vec<T, 2>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands the range just enough to include
  /// the other range. Essentially it is the union of the two ranges.
  ///
  VISKORES_EXEC_CONT
  void Include(const viskores::RangeId2& range)
  {
    this->X.Include(range.X);
    this->Y.Include(range.Y);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VISKORES_EXEC_CONT
  viskores::RangeId2 Union(const viskores::RangeId2& other) const
  {
    viskores::RangeId2 unionRangeId2(*this);
    unionRangeId2.Include(other);
    return unionRangeId2;
  }

  /// \b Operator for union
  ///
  VISKORES_EXEC_CONT
  viskores::RangeId2 operator+(const viskores::RangeId2& other) const { return this->Union(other); }

  VISKORES_EXEC_CONT
  bool operator==(const viskores::RangeId2& range) const
  {
    return ((this->X == range.X) && (this->Y == range.Y));
  }

  VISKORES_EXEC_CONT
  bool operator!=(const viskores::RangeId2& range) const
  {
    return ((this->X != range.X) || (this->Y != range.Y));
  }

  VISKORES_EXEC_CONT
  viskores::RangeId& operator[](IdComponent c) noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else
    {
      return this->Y;
    }
  }

  VISKORES_EXEC_CONT
  const viskores::RangeId& operator[](IdComponent c) const noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else
    {
      return this->Y;
    }
  }
};

} // namespace viskores

/// Helper function for printing range during testing
///
static inline VISKORES_CONT std::ostream& operator<<(std::ostream& stream,
                                                     const viskores::RangeId2& range)
{
  return stream << "{ X:" << range.X << ", Y:" << range.Y << " }";
}

#endif //viskores_RangeId2_h
