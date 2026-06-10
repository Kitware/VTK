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

#include <viskores/rendering/raytracing/RunTriangulator.h>

#include <viskores/cont/TryExecute.h>
#include <viskores/rendering/raytracing/Triangulator.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

void RunTriangulator(const viskores::cont::UnknownCellSet& cellSet,
                     viskores::cont::ArrayHandle<viskores::Id4>& indices,
                     viskores::Id& numberOfTriangles,
                     const viskores::cont::Field& ghostField)
{
  viskores::rendering::raytracing::Triangulator triangulator;
  triangulator.Run(cellSet, indices, numberOfTriangles, ghostField);
}

}
}
} // namespace viskores::rendering::raytracing
