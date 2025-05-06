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

#ifndef viskores_Range_h
#define viskores_Range_h

#include <viskores/Assert.h>
#include <viskores/Math.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{

/// \brief Represent a continuous scalar range of values.
///
/// \c viskores::Range is a helper class for representing a range of floating point
/// values from a minimum value to a maximum value. This is specified simply
/// enough with a \c Min and \c Max value.
///
/// \c Range also contains several helper functions for computing and
/// maintaining the range.
///
struct Range
{
  /// The minumum value of the range (inclusive).
  viskores::Float64 Min;
  /// Tha maximum value of the range (inclusive).
  viskores::Float64 Max;

  /// Construct a range with a given minimum and maximum. If no minimum or maximum is
  /// given, the range will be empty.
  VISKORES_EXEC_CONT
  Range()
    : Min(viskores::Infinity64())
    , Max(viskores::NegativeInfinity64())
  {
  }

  Range(const Range&) = default;
  Range(Range&&) = default;

  template <typename T1, typename T2>
  VISKORES_EXEC_CONT Range(const T1& min, const T2& max)
    : Min(static_cast<viskores::Float64>(min))
    , Max(static_cast<viskores::Float64>(max))
  {
  }

  viskores::Range& operator=(const viskores::Range& src) = default;
  viskores::Range& operator=(viskores::Range&& src) = default;

  /// \b Determine if the range is valid (i.e. has at least one valid point).
  ///
  /// \c IsNonEmpty return true if the range contains some valid values between
  /// \c Min and \c Max. If \c Max is less than \c Min, then no values satisfy
  /// the range and \c IsNonEmpty returns false. Otherwise, return true.
  ///
  /// \c IsNonEmpty assumes \c Min and \c Max are inclusive. That is, if they
  /// are equal then true is returned.
  ///
  VISKORES_EXEC_CONT
  bool IsNonEmpty() const { return (this->Min <= this->Max); }

  /// \b Determines if a value is within the range.
  ///
  /// \c Contains returns true if the give value is within the range, false
  /// otherwise. \c Contains treats the min and max as inclusive. That is, if
  /// the value is exactly the min or max, true is returned.
  ///
  template <typename T>
  VISKORES_EXEC_CONT bool Contains(const T& value) const
  {
    return ((this->Min <= static_cast<viskores::Float64>(value)) &&
            (this->Max >= static_cast<viskores::Float64>(value)));
  }

  /// \b Returns the length of the range.
  ///
  /// \c Length computes the distance between the min and max. If the range
  /// is empty, 0 is returned.
  ///
  VISKORES_EXEC_CONT
  viskores::Float64 Length() const
  {
    if (this->IsNonEmpty())
    {
      return (this->Max - this->Min);
    }
    else
    {
      return 0.0;
    }
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle value of the range. If the range is empty,
  /// NaN is returned.
  ///
  VISKORES_EXEC_CONT
  viskores::Float64 Center() const
  {
    if (this->IsNonEmpty())
    {
      return 0.5 * (this->Max + this->Min);
    }
    else
    {
      return viskores::Nan64();
    }
  }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already includes this value, then nothing is
  /// done.
  ///
  template <typename T>
  VISKORES_EXEC_CONT void Include(const T& value)
  {
    this->Min = viskores::Min(this->Min, static_cast<viskores::Float64>(value));
    this->Max = viskores::Max(this->Max, static_cast<viskores::Float64>(value));
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands this range just enough to include that
  /// of another range. Essentially it is the union of the two ranges.
  ///
  VISKORES_EXEC_CONT
  void Include(const viskores::Range& range)
  {
    if (range.IsNonEmpty())
    {
      this->Min = viskores::Min(this->Min, range.Min);
      this->Max = viskores::Max(this->Max, range.Max);
    }
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VISKORES_EXEC_CONT
  viskores::Range Union(const viskores::Range& otherRange) const
  {
    viskores::Range unionRange(*this);
    unionRange.Include(otherRange);
    return unionRange;
  }

  /// \b Return the intersection of this and another range.
  ///
  VISKORES_EXEC_CONT
  viskores::Range Intersection(const viskores::Range& otherRange) const
  {
    return viskores::Range(viskores::Max(this->Min, otherRange.Min),
                           viskores::Min(this->Max, otherRange.Max));
  }

  /// \b Operator for union
  ///
  VISKORES_EXEC_CONT
  viskores::Range operator+(const viskores::Range& otherRange) const
  {
    return this->Union(otherRange);
  }

  VISKORES_EXEC_CONT
  bool operator==(const viskores::Range& otherRange) const
  {
    return ((this->Min == otherRange.Min) && (this->Max == otherRange.Max));
  }

  VISKORES_EXEC_CONT
  bool operator!=(const viskores::Range& otherRange) const
  {
    return ((this->Min != otherRange.Min) || (this->Max != otherRange.Max));
  }
};

/// Helper function for printing ranges during testing
///
inline VISKORES_CONT std::ostream& operator<<(std::ostream& stream, const viskores::Range& range)
{
  return stream << "[" << range.Min << ".." << range.Max << "]";
} // Declared inside of viskores namespace so that the operator work with ADL lookup

template <>
struct VISKORES_NEVER_EXPORT VecTraits<viskores::Range>
{
  using ComponentType = viskores::Float64;
  using BaseComponentType = viskores::Float64;

  static constexpr viskores::IdComponent NUM_COMPONENTS = 2;
  static constexpr viskores::IdComponent GetNumberOfComponents(const viskores::Range&)
  {
    return NUM_COMPONENTS;
  }
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;
  using IsSizeStatic = viskores::VecTraitsTagSizeStatic;

  VISKORES_EXEC_CONT
  static const ComponentType& GetComponent(const viskores::Range& range,
                                           viskores::IdComponent component)
  {
    VISKORES_ASSERT((component == 0) || (component == 1));
    return (component == 0) ? range.Min : range.Max;
  }
  VISKORES_EXEC_CONT
  static ComponentType& GetComponent(viskores::Range& range, viskores::IdComponent component)
  {
    VISKORES_ASSERT((component == 0) || (component == 1));
    return (component == 0) ? range.Min : range.Max;
  }

  VISKORES_EXEC_CONT
  static void SetComponent(viskores::Range& range,
                           viskores::IdComponent component,
                           ComponentType value)
  {
    VISKORES_ASSERT((component == 0) || (component == 1));
    if (component == 0)
    {
      range.Min = value;
    }
    else
    {
      range.Max = value;
    }
  }

  template <typename NewComponentType>
  using ReplaceComponentType = viskores::Vec<NewComponentType, NUM_COMPONENTS>;
  template <typename NewComponentType>
  using ReplaceBaseComponentType = viskores::Vec<NewComponentType, NUM_COMPONENTS>;

  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const viskores::Range& src,
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


#endif //viskores_Range_h
