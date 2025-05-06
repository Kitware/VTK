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

#include <viskores/cont/Field.h>
#include <viskores/rendering/internal/RunTriangulator.h>
#include <viskores/rendering/raytracing/TriangleExtractor.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

void TriangleExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells)
{
  ExtractCells(cells,
               make_FieldCell(viskores::cont::GetGlobalGhostCellFieldName(),
                              viskores::cont::ArrayHandleConstant<viskores::UInt8>(
                                0, cells.GetNumberOfCells())));
}

void TriangleExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                     const viskores::cont::Field& ghostField)
{
  viskores::Id numberOfTriangles;
  viskores::rendering::internal::RunTriangulator(
    cells, this->Triangles, numberOfTriangles, ghostField);
}

viskores::cont::ArrayHandle<viskores::Id4> TriangleExtractor::GetTriangles()
{
  return this->Triangles;
}

viskores::Id TriangleExtractor::GetNumberOfTriangles() const
{
  return this->Triangles.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
