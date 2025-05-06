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
#ifndef viskores_RangeId3_h
#define viskores_RangeId3_h

#include <viskores/RangeId.h>

namespace viskores
{

/// \brief Represent 3D integer range.
///
/// \c viskores::RangeId3 is a helper class for representing a 3D range of integer
/// values. The typical use of this class is to express a box of indices
/// in the x, y, and z directions.
///
/// \c RangeId3 also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId3
{
  /// The range of values in the X direction. The `viskores::RangeId` struct provides
  /// the minimum and maximum along that axis.
  viskores::RangeId X;
  /// The range of values in the Y direction. The `viskores::RangeId` struct provides
  /// the minimum and maximum along that axis.
  viskores::RangeId Y;
  /// The range of values in the Z direction. The `viskores::RangeId` struct provides
  /// the minimum and maximum along that axis.
  viskores::RangeId Z;

  /// Construct an empty 3D range.
  RangeId3() = default;

  /// Construct a range with the given x, y, and z directions.
  VISKORES_EXEC_CONT
  RangeId3(const viskores::RangeId& xrange,
           const viskores::RangeId& yrange,
           const viskores::RangeId& zrange)
    : X(xrange)
    , Y(yrange)
    , Z(zrange)
  {
  }

  /// Construct a range with the given minimum (inclusive) and maximum (exclusive) points.
  VISKORES_EXEC_CONT
  RangeId3(viskores::Id minX,
           viskores::Id maxX,
           viskores::Id minY,
           viskores::Id maxY,
           viskores::Id minZ,
           viskores::Id maxZ)
    : X(viskores::RangeId(minX, maxX))
    , Y(viskores::RangeId(minY, maxY))
    , Z(viskores::RangeId(minZ, maxZ))
  {
  }

  /// Initialize range with an array of 6 values in the order xmin, xmax,
  /// ymin, ymax, zmin, zmax.
  ///
  VISKORES_EXEC_CONT
  explicit RangeId3(const viskores::Id range[6])
    : X(viskores::RangeId(range[0], range[1]))
    , Y(viskores::RangeId(range[2], range[3]))
    , Z(viskores::RangeId(range[4], range[5]))
  {
  }

  /// Initialize range with the minimum and the maximum corners
  ///
  VISKORES_EXEC_CONT
  RangeId3(const viskores::Id3& min, const viskores::Id3& max)
    : X(viskores::RangeId(min[0], max[0]))
    , Y(viskores::RangeId(min[1], max[1]))
    , Z(viskores::RangeId(min[2], max[2]))
  {
  }

  /// \b Determine if the range is non-empty.
  ///
  /// \c IsNonEmpty returns true if the range is non-empty.
  ///
  VISKORES_EXEC_CONT
  bool IsNonEmpty() const
  {
    return (this->X.IsNonEmpty() && this->Y.IsNonEmpty() && this->Z.IsNonEmpty());
  }

  /// \b Determines if an Id3 value is within the range.
  ///
  VISKORES_EXEC_CONT
  bool Contains(const viskores::Id3& val) const
  {
    return (this->X.Contains(val[0]) && this->Y.Contains(val[1]) && this->Z.Contains(val[2]));
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle of the range.
  ///
  VISKORES_EXEC_CONT
  viskores::Id3 Center() const
  {
    return viskores::Id3(this->X.Center(), this->Y.Center(), this->Z.Center());
  }

  VISKORES_EXEC_CONT
  viskores::Id3 Dimensions() const
  {
    return viskores::Id3(this->X.Length(), this->Y.Length(), this->Z.Length());
  }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already include this value, then
  /// nothing is done.
  ///
  template <typename T>
  VISKORES_EXEC_CONT void Include(const viskores::Vec<T, 3>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
    this->Z.Include(point[2]);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands the range just enough to include
  /// the other range. Essentially it is the union of the two ranges.
  ///
  VISKORES_EXEC_CONT
  void Include(const viskores::RangeId3& range)
  {
    this->X.Include(range.X);
    this->Y.Include(range.Y);
    this->Z.Include(range.Z);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VISKORES_EXEC_CONT
  viskores::RangeId3 Union(const viskores::RangeId3& other) const
  {
    viskores::RangeId3 unionRangeId3(*this);
    unionRangeId3.Include(other);
    return unionRangeId3;
  }

  /// \b Operator for union
  ///
  VISKORES_EXEC_CONT
  viskores::RangeId3 operator+(const viskores::RangeId3& other) const { return this->Union(other); }

  VISKORES_EXEC_CONT
  bool operator==(const viskores::RangeId3& range) const
  {
    return ((this->X == range.X) && (this->Y == range.Y) && (this->Z == range.Z));
  }

  VISKORES_EXEC_CONT
  bool operator!=(const viskores::RangeId3& range) const
  {
    return ((this->X != range.X) || (this->Y != range.Y) || (this->Z != range.Z));
  }
  VISKORES_EXEC_CONT
  viskores::RangeId& operator[](IdComponent c) noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else if (c == 1)
    {
      return this->Y;
    }
    else
    {
      return this->Z;
    }
  }

  VISKORES_EXEC_CONT
  const viskores::RangeId& operator[](IdComponent c) const noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else if (c == 1)
    {
      return this->Y;
    }
    else
    {
      return this->Z;
    }
  }
};

/// Helper function for printing range during testing
///
inline VISKORES_CONT std::ostream& operator<<(std::ostream& stream, const viskores::RangeId3& range)
{
  return stream << "{ X:" << range.X << ", Y:" << range.Y << ", Z:" << range.Z << " }";
} // Declared inside of viskores namespace so that the operator work with ADL lookup
} // namespace viskores

#endif //viskores_RangeId3_h
