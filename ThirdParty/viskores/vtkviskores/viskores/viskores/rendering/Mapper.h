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
#ifndef viskores_rendering_Mapper_h
#define viskores_rendering_Mapper_h

#include <viskores/cont/ColorTable.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
namespace viskores
{
namespace rendering
{

/// @brief Converts data into commands to a rendering system.
///
/// This is the base class for all mapper classes in Viskores. Different concrete
/// derived classes can provide different representations and rendering techniques.
class VISKORES_RENDERING_EXPORT Mapper
{
public:
  VISKORES_CONT
  Mapper() {}

  virtual ~Mapper();

  virtual void RenderCells(const viskores::cont::UnknownCellSet& cellset,
                           const viskores::cont::CoordinateSystem& coords,
                           const viskores::cont::Field& scalarField,
                           const viskores::cont::ColorTable& colorTable,
                           const viskores::rendering::Camera& camera,
                           const viskores::Range& scalarRange);

  void RenderCells(const viskores::cont::UnknownCellSet& cellset,
                   const viskores::cont::CoordinateSystem& coords,
                   const viskores::cont::Field& scalarField,
                   const viskores::cont::ColorTable& colorTable,
                   const viskores::rendering::Camera& camera,
                   const viskores::Range& scalarRange,
                   const viskores::cont::Field& ghostField);

  virtual void RenderCellsPartitioned(const viskores::cont::PartitionedDataSet partitionedData,
                                      const std::string fieldName,
                                      const viskores::cont::ColorTable& colorTable,
                                      const viskores::rendering::Camera& camera,
                                      const viskores::Range& scalarRange);

  virtual void SetActiveColorTable(const viskores::cont::ColorTable& ct);

  virtual void SetCanvas(viskores::rendering::Canvas* canvas) = 0;
  virtual viskores::rendering::Canvas* GetCanvas() const = 0;

  virtual viskores::rendering::Mapper* NewCopy() const = 0;

  virtual void SetLogarithmX(bool l);
  virtual void SetLogarithmY(bool l);

protected:
  viskores::cont::ArrayHandle<viskores::Vec4f_32> ColorMap;
  bool LogarithmX = false;
  bool LogarithmY = false;

  // for the volume renderer sorting back to front gives better results for transarent colors, which is the default
  // but for the raytracer front to back is better.
  bool SortBackToFront = true;

  virtual void RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                               const viskores::cont::CoordinateSystem& coords,
                               const viskores::cont::Field& scalarField,
                               const viskores::cont::ColorTable& colorTable,
                               const viskores::rendering::Camera& camera,
                               const viskores::Range& scalarRange,
                               const viskores::cont::Field& ghostField) = 0;
};
}
} //namespace viskores::rendering
#endif //viskores_rendering_Mapper_h
