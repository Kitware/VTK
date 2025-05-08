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
#ifndef viskores_rendering_raytracing_Cylinder_Extractor_h
#define viskores_rendering_raytracing_Cylinder_Extractor_h

#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

/**
 * \brief CylinderExtractor creates a line segments from
 *        the edges of a cell set.
 *
 */
class CylinderExtractor
{
protected:
  viskores::cont::ArrayHandle<viskores::Id3> CylIds;
  viskores::cont::ArrayHandle<viskores::Float32> Radii;

public:
  //
  // Extract all vertex shapes with constant radius
  //
  void ExtractCells(const viskores::cont::UnknownCellSet& cells, viskores::Float32 radius);

  //
  // Extract all vertex elements with radius based on scalar values
  //
  void ExtractCells(const viskores::cont::UnknownCellSet& cells,
                    const viskores::cont::Field& field,
                    const viskores::Float32 minRadius,
                    const viskores::Float32 maxRadius);


  viskores::cont::ArrayHandle<viskores::Id3> GetCylIds();

  viskores::cont::ArrayHandle<viskores::Float32> GetRadii();
  viskores::Id GetNumberOfCylinders() const;

protected:
  void SetUniformRadius(const viskores::Float32 radius);
  void SetVaryingRadius(const viskores::Float32 minRadius,
                        const viskores::Float32 maxRadius,
                        const viskores::cont::Field& field);

  //  void SetPointIdsFromCoords(const viskores::cont::CoordinateSystem& coords);
  void SetCylinderIdsFromCells(const viskores::cont::UnknownCellSet& cells);

}; // class ShapeIntersector
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Shape_Extractor_h
