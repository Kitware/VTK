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
#ifndef viskores_rendering_raytracing_Triangle_Extractor_h
#define viskores_rendering_raytracing_Triangle_Extractor_h

#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class VISKORES_RENDERING_EXPORT TriangleExtractor
{
protected:
  viskores::cont::ArrayHandle<viskores::Id4> Triangles; // (cellid, v0, v1, v2)
public:
  void ExtractCells(const viskores::cont::UnknownCellSet& cells);

  void ExtractCells(const viskores::cont::UnknownCellSet& cells,
                    const viskores::cont::Field& ghostField);

  viskores::cont::ArrayHandle<viskores::Id4> GetTriangles();
  viskores::Id GetNumberOfTriangles() const;
}; // class TriangleExtractor
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Triangle_Extractor_h
