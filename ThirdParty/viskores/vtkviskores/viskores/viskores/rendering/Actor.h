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
#ifndef viskores_rendering_Actor_h
#define viskores_rendering_Actor_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Mapper.h>

#include <memory>

namespace viskores
{
namespace rendering
{

/// @brief An item to be rendered.
///
/// The `Actor` holds the geometry from a `viskores::cont::DataSet` as well as other visual
/// properties that define how the geometry should look when it is rendered.
class VISKORES_RENDERING_EXPORT Actor
{
public:
  Actor(const viskores::cont::DataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName);

  Actor(const viskores::cont::DataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName,
        const viskores::cont::ColorTable& colorTable);

  Actor(const viskores::cont::DataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName,
        const viskores::rendering::Color& color);

  Actor(const viskores::cont::PartitionedDataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName);

  Actor(const viskores::cont::PartitionedDataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName,
        const viskores::cont::ColorTable& colorTable);

  Actor(const viskores::cont::PartitionedDataSet dataSet,
        const std::string coordinateName,
        const std::string fieldName,
        const viskores::rendering::Color& color);

  /// Create an `Actor` object that renders a set of cells positioned by a given coordiante
  /// system. A field to apply psudocoloring is also provided. The default colormap is applied.
  /// The cells, coordinates, and field are typically pulled from a `viskores::cont::DataSet` object.
  Actor(const viskores::cont::UnknownCellSet& cells,
        const viskores::cont::CoordinateSystem& coordinates,
        const viskores::cont::Field& scalarField);

  /// Create an `Actor` object that renders a set of cells positioned by a given coordiante
  /// system. A field to apply psudocoloring is also provided. A color table providing the map
  /// from scalar values to colors is also provided.
  /// The cells, coordinates, and field are typically pulled from a `viskores::cont::DataSet` object.
  Actor(const viskores::cont::UnknownCellSet& cells,
        const viskores::cont::CoordinateSystem& coordinates,
        const viskores::cont::Field& scalarField,
        const viskores::cont::ColorTable& colorTable);

  /// Create an `Actor` object that renders a set of cells positioned by a given coordiante
  /// system. A constant color to apply to the object is also provided.
  /// The cells and coordinates are typically pulled from a `viskores::cont::DataSet` object.
  // Why do you have to provide a `Field` if a constant color is provided?
  Actor(const viskores::cont::UnknownCellSet& cells,
        const viskores::cont::CoordinateSystem& coordinates,
        const viskores::cont::Field& scalarField,
        const viskores::rendering::Color& color);

  Actor(const Actor&);
  Actor& operator=(const Actor&);

  Actor(Actor&&) noexcept;
  Actor& operator=(Actor&&) noexcept;
  ~Actor();

  void Render(viskores::rendering::Mapper& mapper,
              viskores::rendering::Canvas& canvas,
              const viskores::rendering::Camera& camera) const;

  const viskores::cont::UnknownCellSet& GetCells() const;

  viskores::cont::CoordinateSystem GetCoordinates() const;

  const viskores::cont::Field& GetScalarField() const;

  const viskores::cont::ColorTable& GetColorTable() const;

  const viskores::Range& GetScalarRange() const;

  const viskores::Bounds& GetSpatialBounds() const;

  /// @brief Specifies the range for psudocoloring.
  ///
  /// When coloring data by mapping a scalar field to colors, this is the range used for
  /// the colors provided by the table. If a range is not provided, the range of data in the
  /// field is used.
  void SetScalarRange(const viskores::Range& scalarRange);

private:
  struct InternalsType;
  std::unique_ptr<InternalsType> Internals;

  void Init(const viskores::cont::CoordinateSystem& coordinates,
            const viskores::cont::Field& scalarField);

  void Init();
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_Actor_h
