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

#ifndef viskores_Bounds_h
#define viskores_Bounds_h

#include <viskores/Range.h>

namespace viskores
{

/// \brief Represent an axis-aligned 3D bounds in space.
///
/// \c viskores::Bounds is a helper class for representing the axis-aligned box
/// representing some region in space. The typical use of this class is to
/// express the containing box of some geometry. The box is specified as ranges
/// in the x, y, and z directions.
///
/// \c Bounds also contains several helper functions for computing and
/// maintaining the bounds.
///
struct Bounds
{
  /// The range of values in the X direction. The `viskores::Range` struct provides
  /// the minimum and maximum along that axis.
  viskores::Range X;
  /// The range of values in the Y direction. The `viskores::Range` struct provides
  /// the minimum and maximum along that axis.
  viskores::Range Y;
  /// The range of values in the Z direction. The `viskores::Range` struct provides
  /// the minimum and maximum along that axis.
  viskores::Range Z;

  /// Construct an empty bounds. The bounds will represent no space until
  /// otherwise modified.
  VISKORES_EXEC_CONT
  Bounds() {}

  Bounds(const Bounds&) = default;

  /// Construct a bounds with a given range in the x, y, and z dimensions.
  VISKORES_EXEC_CONT
  Bounds(const viskores::Range& xRange,
         const viskores::Range& yRange,
         const viskores::Range& zRange)
    : X(xRange)
    , Y(yRange)
    , Z(zRange)
  {
  }

  /// Construct a bounds with the minimum and maximum coordinates in the x, y, and z
  /// directions.
  template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
  VISKORES_EXEC_CONT Bounds(const T1& minX,
                            const T2& maxX,
                            const T3& minY,
                            const T4& maxY,
                            const T5& minZ,
                            const T6& maxZ)
    : X(viskores::Range(minX, maxX))
    , Y(viskores::Range(minY, maxY))
    , Z(viskores::Range(minZ, maxZ))
  {
  }

  /// Initialize bounds with an array of 6 values in the order xmin, xmax,
  /// ymin, ymax, zmin, zmax.
  ///
  template <typename T>
  VISKORES_EXEC_CONT explicit Bounds(const T bounds[6])
    : X(viskores::Range(bounds[0], bounds[1]))
    , Y(viskores::Range(bounds[2], bounds[3]))
    , Z(viskores::Range(bounds[4], bounds[5]))
  {
  }

  /// Initialize bounds with the minimum corner point and the maximum corner
  /// point.
  ///
  template <typename T>
  VISKORES_EXEC_CONT Bounds(const viskores::Vec<T, 3>& minPoint,
                            const viskores::Vec<T, 3>& maxPoint)
    : X(viskores::Range(minPoint[0], maxPoint[0]))
    , Y(viskores::Range(minPoint[1], maxPoint[1]))
    , Z(viskores::Range(minPoint[2], maxPoint[2]))
  {
  }

  viskores::Bounds& operator=(const viskores::Bounds& src) = default;

  /// \b Determine if the bounds are valid (i.e. has at least one valid point).
  ///
  /// \c IsNonEmpty returns true if the bounds contain some valid points. If
  /// the bounds are any real region, even if a single point or it expands to
  /// infinity, true is returned.
  ///
  VISKORES_EXEC_CONT
  bool IsNonEmpty() const
  {
    return (this->X.IsNonEmpty() && this->Y.IsNonEmpty() && this->Z.IsNonEmpty());
  }

  /// \b Determines if a point coordinate is within the bounds.
  ///
  template <typename T>
  VISKORES_EXEC_CONT bool Contains(const viskores::Vec<T, 3>& point) const
  {
    return (this->X.Contains(point[0]) && this->Y.Contains(point[1]) && this->Z.Contains(point[2]));
  }

  /// \b Returns the volume of the bounds.
  ///
  /// \c Volume computes the product of the lengths of the ranges in each dimension. If the bounds
  /// are empty, 0 is returned.
  ///
  VISKORES_EXEC_CONT
  viskores::Float64 Volume() const
  {
    if (this->IsNonEmpty())
    {
      return (this->X.Length() * this->Y.Length() * this->Z.Length());
    }
    else
    {
      return 0.0;
    }
  }

  /// \b Returns the area of the bounds in the X-Y-plane.
  ///
  /// \c Area computes the product of the lengths of the ranges in dimensions X and Y. If the bounds
  /// are empty, 0 is returned.
  ///
  VISKORES_EXEC_CONT
  viskores::Float64 Area() const
  {
    if (this->IsNonEmpty())
    {
      return (this->X.Length() * this->Y.Length());
    }
    else
    {
      return 0.0;
    }
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the point at the middle of the bounds. If the bounds
  /// are empty, the results are undefined.
  ///
  VISKORES_EXEC_CONT
  viskores::Vec3f_64 Center() const
  {
    return viskores::Vec3f_64(this->X.Center(), this->Y.Center(), this->Z.Center());
  }

  /// \b Returns the min point of the bounds
  ///
  /// \c MinCorder returns the minium point of the bounds.If the bounds
  /// are empty, the results are undefined.
  ///
  VISKORES_EXEC_CONT
  viskores::Vec3f_64 MinCorner() const
  {
    return viskores::Vec3f_64(this->X.Min, this->Y.Min, this->Z.Min);
  }

  /// \b Returns the max point of the bounds
  ///
  /// \c MaxCorder returns the minium point of the bounds.If the bounds
  /// are empty, the results are undefined.
  ///
  VISKORES_EXEC_CONT
  viskores::Vec3f_64 MaxCorner() const
  {
    return viskores::Vec3f_64(this->X.Max, this->Y.Max, this->Z.Max);
  }

  /// \b Expand bounds to include a point.
  ///
  /// This version of \c Include expands the bounds just enough to include the
  /// given point coordinates. If the bounds already include this point, then
  /// nothing is done.
  ///
  template <typename T>
  VISKORES_EXEC_CONT void Include(const viskores::Vec<T, 3>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
    this->Z.Include(point[2]);
  }

  /// \b Expand bounds to include other bounds.
  ///
  /// This version of \c Include expands these bounds just enough to include
  /// that of another bounds. Essentially it is the union of the two bounds.
  ///
  VISKORES_EXEC_CONT
  void Include(const viskores::Bounds& bounds)
  {
    this->X.Include(bounds.X);
    this->Y.Include(bounds.Y);
    this->Z.Include(bounds.Z);
  }

  /// \b Return the union of this and another bounds.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VISKORES_EXEC_CONT
  viskores::Bounds Union(const viskores::Bounds& otherBounds) const
  {
    viskores::Bounds unionBounds(*this);
    unionBounds.Include(otherBounds);
    return unionBounds;
  }

  /// \b Return the intersection of this and another range.
  ///
  VISKORES_EXEC_CONT
  viskores::Bounds Intersection(const viskores::Bounds& otherBounds) const
  {
    return viskores::Bounds(this->X.Intersection(otherBounds.X),
                            this->Y.Intersection(otherBounds.Y),
                            this->Z.Intersection(otherBounds.Z));
  }

  /// \b Operator for union
  ///
  VISKORES_EXEC_CONT
  viskores::Bounds operator+(const viskores::Bounds& otherBounds) const
  {
    return this->Union(otherBounds);
  }

  VISKORES_EXEC_CONT
  bool operator==(const viskores::Bounds& bounds) const
  {
    return ((this->X == bounds.X) && (this->Y == bounds.Y) && (this->Z == bounds.Z));
  }

  VISKORES_EXEC_CONT
  bool operator!=(const viskores::Bounds& bounds) const
  {
    return ((this->X != bounds.X) || (this->Y != bounds.Y) || (this->Z != bounds.Z));
  }
};

/// Helper function for printing bounds during testing
///
inline VISKORES_CONT std::ostream& operator<<(std::ostream& stream, const viskores::Bounds& bounds)
{
  return stream << "{ X:" << bounds.X << ", Y:" << bounds.Y << ", Z:" << bounds.Z << " }";
}

template <>
struct VISKORES_NEVER_EXPORT VecTraits<viskores::Bounds>
{
  using ComponentType = viskores::Range;
  using BaseComponentType = viskores::VecTraits<viskores::Range>::BaseComponentType;

  static constexpr viskores::IdComponent NUM_COMPONENTS = 3;
  static constexpr viskores::IdComponent GetNumberOfComponents(const viskores::Bounds&)
  {
    return NUM_COMPONENTS;
  }
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;
  using IsSizeStatic = viskores::VecTraitsTagSizeStatic;

  VISKORES_EXEC_CONT
  static const ComponentType& GetComponent(const viskores::Bounds& bounds,
                                           viskores::IdComponent component)
  {
    VISKORES_ASSERT((component >= 0) || (component < 3));
    switch (component)
    {
      case 0:
        return bounds.X;
      case 1:
        return bounds.Y;
      case 2:
        return bounds.Z;
      default:
        // Should never reach here
        return bounds.X;
    }
  }
  VISKORES_EXEC_CONT
  static ComponentType& GetComponent(viskores::Bounds& bounds, viskores::IdComponent component)
  {
    VISKORES_ASSERT((component >= 0) || (component < 3));
    switch (component)
    {
      case 0:
        return bounds.X;
      case 1:
        return bounds.Y;
      case 2:
        return bounds.Z;
      default:
        // Should never reach here
        return bounds.X;
    }
  }

  VISKORES_EXEC_CONT
  static void SetComponent(viskores::Bounds& bounds,
                           viskores::IdComponent component,
                           const ComponentType& value)
  {
    VISKORES_ASSERT((component >= 0) || (component < 3));
    switch (component)
    {
      case 0:
        bounds.X = value;
        break;
      case 1:
        bounds.Y = value;
        break;
      case 2:
        bounds.Z = value;
        break;
    }
  }

  template <typename NewComponentType>
  using ReplaceComponentType = viskores::Vec<NewComponentType, NUM_COMPONENTS>;
  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    viskores::Vec<NewComponentType,
                  NUM_COMPONENTS * viskores::VecTraits<viskores::Range>::NUM_COMPONENTS>;

  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const viskores::Bounds& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    const viskores::IdComponent maxComponent =
      (destSize < NUM_COMPONENTS) ? destSize : NUM_COMPONENTS;
    for (viskores::IdComponent component = 0; component < maxComponent; ++component)
    {
      dest[component] = GetComponent(src, component);
    }
  }
};

} // namespace viskores

#endif //viskores_Bounds_h
