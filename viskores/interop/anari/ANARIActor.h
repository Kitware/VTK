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

#ifndef viskores_interop_anari_ANARIActor_h
#define viskores_interop_anari_ANARIActor_h

// viskores
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/interop/anari/ViskoresANARITypes.h>
// std
#include <array>
#include <memory>

#include <viskores/interop/anari/viskores_anari_export.h>

namespace viskores
{
namespace interop
{
namespace anari
{

/// \brief Convenience type used to represent all the fields in an `ANARIActor`.
///
using FieldSet = std::array<viskores::cont::Field, 4>;

/// \brief Returns the appropriate ANARI attribute string based on field index.
///
const char* AnariMaterialInputString(viskores::IdComponent p);

/// \brief Collects cells, coords, and 0-4 fields for ANARI mappers to consume.
///
/// `ANARIActor` represents a selected set of cells, coordinates, and fields for
/// `ANARIMapper` based mappers to map onto ANARI objects. This class also
/// maintains which field is the "main" field, which almost always is the field
/// which is used to color the geometry or volume.
///
/// Mappers creating geometry will generally add all fields as attribute arrays
/// if possible, letting applications use more than one field as material inputs
/// or data to be color mapped by samplers.
///
struct VISKORES_ANARI_EXPORT ANARIActor
{
  ANARIActor() = default;

  /// @brief Main constructor taking cells, coordinates, and up to 4 fields.
  ///
  ANARIActor(const viskores::cont::UnknownCellSet& cells,
             const viskores::cont::CoordinateSystem& coordinates,
             const viskores::cont::Field& field0 = {},
             const viskores::cont::Field& field1 = {},
             const viskores::cont::Field& field2 = {},
             const viskores::cont::Field& field3 = {});

  /// @brief Convenience constructor when an entire FieldSet already exists.
  ///
  ANARIActor(const viskores::cont::UnknownCellSet& cells,
             const viskores::cont::CoordinateSystem& coordinates,
             const FieldSet& fieldset);

  /// @brief Convenience constructor using a dataset + named fields.
  ///
  ANARIActor(const viskores::cont::DataSet& dataset,
             const std::string& field0 = "",
             const std::string& field1 = "",
             const std::string& field2 = "",
             const std::string& field3 = "");

  const viskores::cont::UnknownCellSet& GetCellSet() const;
  const viskores::cont::CoordinateSystem& GetCoordinateSystem() const;
  const viskores::cont::Field& GetField(viskores::IdComponent idx = -1) const;

  FieldSet GetFieldSet() const;

  void SetPrimaryFieldIndex(viskores::IdComponent idx);
  viskores::IdComponent GetPrimaryFieldIndex() const;

  /// @brief Utility to reconstitute a DataSet from the items in the actor.
  ///
  viskores::cont::DataSet MakeDataSet(bool includeFields = false) const;

private:
  struct ActorData
  {
    viskores::cont::UnknownCellSet Cells;
    viskores::cont::CoordinateSystem Coordinates;
    FieldSet Fields;
    viskores::IdComponent PrimaryField{ 0 };
  };

  std::shared_ptr<ActorData> Data = std::make_shared<ActorData>();
};

} // namespace anari
} // namespace interop
} // namespace viskores

#endif
