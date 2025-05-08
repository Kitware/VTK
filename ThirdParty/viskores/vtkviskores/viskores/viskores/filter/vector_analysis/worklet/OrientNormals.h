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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_m_worklet_OrientNormals_h
#define viskores_m_worklet_OrientNormals_h

#include <viskores/Types.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleTransform.h>

#include <viskores/filter/vector_analysis/worklet/OrientCellNormals.h>
#include <viskores/filter/vector_analysis/worklet/OrientPointAndCellNormals.h>
#include <viskores/filter/vector_analysis/worklet/OrientPointNormals.h>

namespace viskores
{
namespace worklet
{

///
/// Orients normals to point outside of the dataset. This requires a closed
/// manifold surface or else the behavior is undefined. This requires an
/// unstructured cellset as input.
///
class OrientNormals
{
public:
  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  VISKORES_CONT static void RunCellNormals(
    const CellSetType& cells,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    viskores::cont::ArrayHandle<viskores::Vec<CellNormalCompType, 3>, CellNormalStorageType>&
      cellNormals)
  {
    OrientCellNormals::Run(cells, coords, cellNormals);
  }

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType>
  VISKORES_CONT static void RunPointNormals(
    const CellSetType& cells,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    viskores::cont::ArrayHandle<viskores::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals)
  {
    OrientPointNormals::Run(cells, coords, pointNormals);
  }

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  VISKORES_CONT static void RunPointAndCellNormals(
    const CellSetType& cells,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    viskores::cont::ArrayHandle<viskores::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals,
    viskores::cont::ArrayHandle<viskores::Vec<CellNormalCompType, 3>, CellNormalStorageType>&
      cellNormals)
  {
    OrientPointAndCellNormals::Run(cells, coords, pointNormals, cellNormals);
  }

  struct NegateFunctor
  {
    template <typename T>
    VISKORES_EXEC_CONT T operator()(const T& val) const
    {
      return -val;
    }
  };

  ///
  /// Reverse the normals to point in the opposite direction.
  ///
  template <typename NormalCompType, typename NormalStorageType>
  VISKORES_CONT static void RunFlipNormals(
    viskores::cont::ArrayHandle<viskores::Vec<NormalCompType, 3>, NormalStorageType>& normals)
  {
    const auto flippedAlias = viskores::cont::make_ArrayHandleTransform(normals, NegateFunctor{});
    viskores::cont::Algorithm::Copy(flippedAlias, normals);
  }
};
}
} // end namespace viskores::worklet


#endif // viskores_m_worklet_OrientNormals_h
