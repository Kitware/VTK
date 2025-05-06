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
#ifndef viskores_rendering_MapperPoint_h
#define viskores_rendering_MapperPoint_h

#include <viskores/Deprecated.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Mapper.h>

#include <memory>

namespace viskores
{
namespace rendering
{

/// @brief This mapper renders points from a cell set.
///
/// This mapper can natively create points from
/// vertex cell shapes as well as use the points
/// defined by a coordinate system.
class VISKORES_RENDERING_EXPORT MapperPoint : public Mapper
{
public:
  MapperPoint();

  ~MapperPoint();

  void SetCanvas(viskores::rendering::Canvas* canvas) override;
  virtual viskores::rendering::Canvas* GetCanvas() const override;

  /// @brief Specify the elements the points will be associated with.
  ///
  /// The point mapper will place visible points over locations specified by either the points
  /// or the cells of a mesh.
  virtual viskores::cont::Field::Association GetAssociation() const;
  /// @copydoc GetAssociation
  virtual void SetAssociation(viskores::cont::Field::Association association);
  /// @copydoc GetAssociation
  virtual bool GetUseCells() const;
  /// @copydoc GetAssociation
  virtual void SetUseCells();
  /// @copydoc GetAssociation
  virtual bool GetUsePoints() const;
  /// @copydoc GetAssociation
  virtual void SetUsePoints();
  VISKORES_DEPRECATED(2.2, "Use SetUseCells or SetAssociation.")
  void UseCells();
  VISKORES_DEPRECATED(2.2, "Use SetUsePoints or SetAssociation.")
  void UseNodes();

  ///
  /// @brief Render points using a variable radius based on the scalar field.
  ///
  /// The default is false.
  void UseVariableRadius(bool useVariableRadius);

  /// @brief Set a base raidus for all points.
  ///
  /// If a radius is never specified the default heuristic is used.
  void SetRadius(const viskores::Float32& radius);

  /// When using a variable raidus for all points, the
  /// radius delta controls how much larger and smaller
  /// radii become based on the scalar field. If the delta
  /// is 0 all points will have the same radius. If the delta
  /// is 0.5 then the max/min scalar values would have a radii
  /// of base +/- base * 0.5.
  void SetRadiusDelta(const viskores::Float32& delta);

  void SetCompositeBackground(bool on);
  viskores::rendering::Mapper* NewCopy() const override;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;

  struct RenderFunctor;

  void RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                       const viskores::cont::CoordinateSystem& coords,
                       const viskores::cont::Field& scalarField,
                       const viskores::cont::ColorTable& colorTable,
                       const viskores::rendering::Camera& camera,
                       const viskores::Range& scalarRange,
                       const viskores::cont::Field& ghostField) override;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_MapperPoint_h
