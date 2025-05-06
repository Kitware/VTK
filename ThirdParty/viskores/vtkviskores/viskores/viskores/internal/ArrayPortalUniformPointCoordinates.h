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
#ifndef viskores_internal_ArrayPortalUniformPointCoordinates_h
#define viskores_internal_ArrayPortalUniformPointCoordinates_h

#include <viskores/Assert.h>
#include <viskores/Types.h>

namespace viskores
{
namespace internal
{

/// \brief An implicit array port that computes point coordinates for a uniform grid.
///
class VISKORES_ALWAYS_EXPORT ArrayPortalUniformPointCoordinates
{
public:
  using ValueType = viskores::Vec3f;

  VISKORES_EXEC_CONT
  ArrayPortalUniformPointCoordinates()
    : Dimensions(0)
    , NumberOfValues(0)
    , Origin(0, 0, 0)
    , Spacing(1, 1, 1)
  {
  }

  VISKORES_EXEC_CONT
  ArrayPortalUniformPointCoordinates(viskores::Id3 dimensions, ValueType origin, ValueType spacing)
    : Dimensions(dimensions)
    , NumberOfValues(dimensions[0] * dimensions[1] * dimensions[2])
    , Origin(origin)
    , Spacing(spacing)
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->GetNumberOfValues());
    return this->Get(viskores::Id3(index % this->Dimensions[0],
                                   (index / this->Dimensions[0]) % this->Dimensions[1],
                                   index / (this->Dimensions[0] * this->Dimensions[1])));
  }

  VISKORES_EXEC_CONT
  viskores::Id3 GetRange3() const { return this->Dimensions; }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id3 index) const
  {
    VISKORES_ASSERT((index[0] >= 0) && (index[1] >= 0) && (index[2] >= 0));
    VISKORES_ASSERT((index[0] < this->Dimensions[0]) && (index[1] < this->Dimensions[1]) &&
                    (index[2] < this->Dimensions[2]));
    return ValueType(
      this->Origin[0] + this->Spacing[0] * static_cast<viskores::FloatDefault>(index[0]),
      this->Origin[1] + this->Spacing[1] * static_cast<viskores::FloatDefault>(index[1]),
      this->Origin[2] + this->Spacing[2] * static_cast<viskores::FloatDefault>(index[2]));
  }

  VISKORES_EXEC_CONT
  const viskores::Id3& GetDimensions() const { return this->Dimensions; }

  VISKORES_EXEC_CONT
  const ValueType& GetOrigin() const { return this->Origin; }

  VISKORES_EXEC_CONT
  const ValueType& GetSpacing() const { return this->Spacing; }

private:
  viskores::Id3 Dimensions = { 0, 0, 0 };
  viskores::Id NumberOfValues = 0;
  ValueType Origin = { 0.0f, 0.0f, 0.0f };
  ValueType Spacing = { 0.0f, 0.0f, 0.0f };
};
}
} // namespace viskores::internal

#endif //viskores_internal_ArrayPortalUniformPointCoordinates_h
