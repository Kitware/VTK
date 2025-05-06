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
#ifndef viskores_rendering_raytracing_Quad_Extractor_h
#define viskores_rendering_raytracing_Quad_Extractor_h

#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class QuadExtractor
{
protected:
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>> QuadIds;
  viskores::cont::ArrayHandle<viskores::Float32> Radii;

public:
  void ExtractCells(const viskores::cont::UnknownCellSet& cells);

  viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>> GetQuadIds();

  viskores::Id GetNumberOfQuads() const;

protected:
  void SetQuadIdsFromCells(const viskores::cont::UnknownCellSet& cells);

}; // class ShapeIntersector
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Shape_Extractor_h
