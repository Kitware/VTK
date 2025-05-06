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

#ifndef viskores_worklet_contour_flyingedges_h
#define viskores_worklet_contour_flyingedges_h

#include <viskores/filter/contour/worklet/contour/FlyingEdgesHelpers.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdgesPass1.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdgesPass2.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdgesPass4.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/Invoker.h>

namespace viskores
{
namespace worklet
{
namespace flying_edges
{

namespace detail
{
template <typename T, typename S>
viskores::Id extend_by(viskores::cont::ArrayHandle<T, S>& handle, viskores::Id size)
{
  viskores::Id oldLen = handle.GetNumberOfValues();
  handle.Allocate(oldLen + size, viskores::CopyFlag::On);
  return oldLen;
}
}

//----------------------------------------------------------------------------
template <typename IVType,
          typename ValueType,
          typename CoordsType,
          typename StorageTagField,
          typename StorageTagVertices,
          typename StorageTagNormals,
          typename CoordinateType,
          typename NormalType>
viskores::cont::CellSetSingleType<> execute(
  const viskores::cont::CellSetStructured<3>& cells,
  const CoordsType coordinateSystem,
  const std::vector<IVType>& isovalues,
  const viskores::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
  viskores::cont::ArrayHandle<viskores::Vec<CoordinateType, 3>, StorageTagVertices>& points,
  viskores::cont::ArrayHandle<viskores::Vec<NormalType, 3>, StorageTagNormals>& normals,
  viskores::worklet::contour::CommonState& sharedState)
{
  viskores::cont::Invoker invoke;
  auto pdims = cells.GetPointDimensions();

  viskores::cont::ArrayHandle<viskores::UInt8> edgeCases;
  edgeCases.Allocate(coordinateSystem.GetData().GetNumberOfValues());

  viskores::cont::CellSetStructured<2> metaDataMesh2D;
  viskores::cont::ArrayHandle<viskores::Id> metaDataLinearSums; //per point of metaDataMesh
  viskores::cont::ArrayHandle<viskores::Id> metaDataMin;        //per point of metaDataMesh
  viskores::cont::ArrayHandle<viskores::Id> metaDataMax;        //per point of metaDataMesh
  viskores::cont::ArrayHandle<viskores::Int32> metaDataNumTris; //per cell of metaDataMesh

  auto metaDataSums = viskores::cont::make_ArrayHandleGroupVec<3>(metaDataLinearSums);

  // Since sharedState can be re-used between invocations of contour,
  // we need to make sure we reset the size of the Interpolation
  // arrays so we don't execute Pass5 over an array that is too large
  sharedState.InterpolationEdgeIds.ReleaseResources();
  sharedState.InterpolationWeights.ReleaseResources();
  sharedState.CellIdMap.ReleaseResources();

  viskores::cont::ArrayHandle<viskores::Id> triangle_topology;
  for (std::size_t i = 0; i < isovalues.size(); ++i)
  {
    auto multiContourCellOffset = sharedState.CellIdMap.GetNumberOfValues();
    auto multiContourPointOffset = sharedState.InterpolationWeights.GetNumberOfValues();
    IVType isoval = isovalues[i];

    //----------------------------------------------------------------------------
    // PASS 1: Process all of the voxel edges that compose each row. Determine the
    // edges case classification, count the number of edge intersections, and
    // figure out where intersections along the row begins and ends
    // (i.e., gather information for computational trimming).
    //
    {
      VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "FlyingEdges Pass1");

      // We have different logic for GPU's compared to Shared memory systems
      // since this is the first touch of lots of the arrays, and will effect
      // NUMA perf.
      //
      // Additionally GPU's does significantly better when you do an initial fill
      // and write only non-below values
      //
      ComputePass1<IVType> worklet1(isoval, pdims);
      viskores::cont::TryExecuteOnDevice(invoke.GetDevice(),
                                         launchComputePass1{},
                                         worklet1,
                                         inputField,
                                         edgeCases,
                                         metaDataMesh2D,
                                         metaDataSums,
                                         metaDataMin,
                                         metaDataMax);
    }

    //----------------------------------------------------------------------------
    // PASS 2: Process a single row of voxels/cells. Count the number of other
    // axis intersections by topological reasoning from previous edge cases.
    // Determine the number of primitives (i.e., triangles) generated from this
    // row. Use computational trimming to reduce work.
    {
      VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "FlyingEdges Pass2");
      ComputePass2 worklet2(pdims);
      invoke(worklet2,
             metaDataMesh2D,
             metaDataSums,
             metaDataMin,
             metaDataMax,
             metaDataNumTris,
             edgeCases);
    }

    //----------------------------------------------------------------------------
    // PASS 3: Compute the number of points and triangles that each edge
    // row needs to generate by using exclusive scans.
    viskores::cont::Algorithm::ScanExtended(metaDataNumTris, metaDataNumTris);
    auto sumTris =
      viskores::cont::ArrayGetValue(metaDataNumTris.GetNumberOfValues() - 1, metaDataNumTris);
    if (sumTris > 0)
    {
      detail::extend_by(triangle_topology, 3 * sumTris);
      detail::extend_by(sharedState.CellIdMap, sumTris);


      viskores::Id newPointSize =
        viskores::cont::Algorithm::ScanExclusive(metaDataLinearSums, metaDataLinearSums);
      detail::extend_by(sharedState.InterpolationEdgeIds, newPointSize);
      detail::extend_by(sharedState.InterpolationWeights, newPointSize);

      //----------------------------------------------------------------------------
      // PASS 4: Process voxel rows and generate topology, and interpolation state
      {
        VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "FlyingEdges Pass4");

        auto pass4 = launchComputePass4(pdims, multiContourCellOffset, multiContourPointOffset);

        detail::extend_by(points, newPointSize);
        if (sharedState.GenerateNormals)
        {
          detail::extend_by(normals, newPointSize);
        }

        viskores::cont::TryExecuteOnDevice(invoke.GetDevice(),
                                           pass4,
                                           newPointSize,
                                           isoval,
                                           coordinateSystem,
                                           inputField,
                                           edgeCases,
                                           metaDataMesh2D,
                                           metaDataSums,
                                           metaDataMin,
                                           metaDataMax,
                                           metaDataNumTris,
                                           sharedState,
                                           triangle_topology,
                                           points,
                                           normals);
      }
    }
  }

  viskores::cont::CellSetSingleType<> outputCells;
  outputCells.Fill(points.GetNumberOfValues(), viskores::CELL_SHAPE_TRIANGLE, 3, triangle_topology);
  return outputCells;
}

} //namespace flying_edges
}
}

#endif
