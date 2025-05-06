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

#ifndef viskores_VecAxisAlignedPointCoordinates_h
#define viskores_VecAxisAlignedPointCoordinates_h

#include <viskores/Math.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{

namespace detail
{

/// Specifies the size of VecAxisAlignedPointCoordinates for the given
/// dimension.
///
template <viskores::IdComponent NumDimensions>
struct VecAxisAlignedPointCoordinatesNumComponents;

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<1>
{
  static constexpr viskores::IdComponent NUM_COMPONENTS = 2;
};

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<2>
{
  static constexpr viskores::IdComponent NUM_COMPONENTS = 4;
};

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<3>
{
  static constexpr viskores::IdComponent NUM_COMPONENTS = 8;
};

struct VecAxisAlignedPointCoordinatesOffsetTable
{
  VISKORES_EXEC_CONT viskores::FloatDefault Get(viskores::Int32 i, viskores::Int32 j) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::FloatDefault offsetTable[8][3] = {
      { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }
    };
    return offsetTable[i][j];
  }
};

} // namespace detail

/// \brief An implicit vector for point coordinates in axis aligned cells. For
/// internal use only.
///
/// The \c VecAxisAlignedPointCoordinates class is a Vec-like class that holds
/// the point coordinates for a axis aligned cell. The class is templated on the
/// dimensions of the cell, which can be 1 (for a line).
///
/// This is an internal class used to represent coordinates for uniform datasets
/// in an execution environment when executing a WorkletMapPointToCell. Users
/// should not directly construct this class under any circumstances. Use the
/// related ArrayPortalUniformPointCoordinates and
/// ArrayHandleUniformPointCoordinates classes instead.
///
template <viskores::IdComponent NumDimensions>
class VecAxisAlignedPointCoordinates
{
public:
  using ComponentType = viskores::Vec3f;

  static constexpr viskores::IdComponent NUM_COMPONENTS =
    detail::VecAxisAlignedPointCoordinatesNumComponents<NumDimensions>::NUM_COMPONENTS;

  VISKORES_EXEC_CONT
  VecAxisAlignedPointCoordinates(ComponentType origin = ComponentType(0, 0, 0),
                                 ComponentType spacing = ComponentType(1, 1, 1))
    : Origin(origin)
    , Spacing(spacing)
  {
  }

  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfComponents() const { return NUM_COMPONENTS; }

  template <viskores::IdComponent DestSize>
  VISKORES_EXEC_CONT void CopyInto(viskores::Vec<ComponentType, DestSize>& dest) const
  {
    viskores::IdComponent numComponents = viskores::Min(DestSize, this->GetNumberOfComponents());
    for (viskores::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VISKORES_EXEC_CONT
  ComponentType operator[](viskores::IdComponent index) const
  {
    detail::VecAxisAlignedPointCoordinatesOffsetTable table;
    return ComponentType(this->Origin[0] + table.Get(index, 0) * this->Spacing[0],
                         this->Origin[1] + table.Get(index, 1) * this->Spacing[1],
                         this->Origin[2] + table.Get(index, 2) * this->Spacing[2]);
  }

  VISKORES_EXEC_CONT
  const ComponentType& GetOrigin() const { return this->Origin; }

  VISKORES_EXEC_CONT
  const ComponentType& GetSpacing() const { return this->Spacing; }

private:
  // Position of lower left point.
  ComponentType Origin;

  // Spacing in the x, y, and z directions.
  ComponentType Spacing;
};

template <viskores::IdComponent NumDimensions>
struct TypeTraits<viskores::VecAxisAlignedPointCoordinates<NumDimensions>>
{
  using NumericTag = viskores::TypeTraitsRealTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VISKORES_EXEC_CONT
  static viskores::VecAxisAlignedPointCoordinates<NumDimensions> ZeroInitialization()
  {
    return viskores::VecAxisAlignedPointCoordinates<NumDimensions>(viskores::Vec3f(0, 0, 0),
                                                                   viskores::Vec3f(0, 0, 0));
  }
};

template <viskores::IdComponent NumDimensions>
struct VecTraits<viskores::VecAxisAlignedPointCoordinates<NumDimensions>>
{
  using VecType = viskores::VecAxisAlignedPointCoordinates<NumDimensions>;

  using ComponentType = viskores::Vec3f;
  using BaseComponentType = viskores::FloatDefault;
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;
  using IsSizeStatic = viskores::VecTraitsTagSizeStatic;

  static constexpr viskores::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const VecType&) { return NUM_COMPONENTS; }

  VISKORES_EXEC_CONT
  static ComponentType GetComponent(const VecType& vector, viskores::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  // These are a bit of a hack since VecAxisAlignedPointCoordinates only supports one component
  // type. Using these might not work as expected.
  template <typename NewComponentType>
  using ReplaceComponentType = viskores::Vec<NewComponentType, NUM_COMPONENTS>;
  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    viskores::Vec<viskores::Vec<NewComponentType, 3>, NUM_COMPONENTS>;

  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const VecType& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

/// Helper function for printing out vectors during testing.
///
template <viskores::IdComponent NumDimensions>
inline VISKORES_CONT std::ostream& operator<<(
  std::ostream& stream,
  const viskores::VecAxisAlignedPointCoordinates<NumDimensions>& vec)
{
  stream << "[";
  for (viskores::IdComponent component = 0; component < vec.NUM_COMPONENTS - 1; component++)
  {
    stream << vec[component] << ",";
  }
  return stream << vec[vec.NUM_COMPONENTS - 1] << "]";
}

} // namespace viskores

#endif //viskores_VecAxisAlignedPointCoordinates_h
