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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_filter_scalar_topology_worklet_extract_top_volume_contours_get_edges_in_cell_worklet_h
#define viskores_filter_scalar_topology_worklet_extract_top_volume_contours_get_edges_in_cell_worklet_h

#include <viskores/filter/scalar_topology/worklet/extract_top_volume_contours/CopyConstArraysWorklet.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace scalar_topology
{
namespace extract_top_volume_contours
{

constexpr viskores::IdComponent MAX_MARCHING_CUBE_TRIANGLES = static_cast<viskores::IdComponent>(5);
constexpr viskores::IdComponent MAX_LINEAR_INTERPOLATION_TRIANGLES =
  static_cast<viskores::IdComponent>(12);

/// Worklet for calculating the edges to be drawn in the cell
/// NOTE: this worklet can only work on 2D and 3D data
template <typename ValueType>
class GetEdgesInCellWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn edgeOffset,          // (input) offset of output edge in the output array
    FieldIn caseCell,            // (input) the marching cube case of the cell
    WholeArrayIn localIds,       // (array input) local ids of points
    WholeArrayIn dataValues,     // (array input) data values within block
    WholeArrayIn globalIds,      // (array input) global regular ids within block
    WholeArrayIn vertexOffset,   // (array input) vertex offset look-up table
    WholeArrayIn edgeTable,      // (array input) edge-in-cell look-up table
    WholeArrayIn numBoundTable,  // (array input) number of boundaries look-up table
    WholeArrayIn boundaryTable,  // (array input) edge-of-boundary look-up table
    WholeArrayIn labelEdgeTable, // (array input) label edge (only for 3D) look-up table
    WholeArrayOut edgesFrom,     // (array output) array of start-points of edges on the isosurface
    WholeArrayOut edgesTo,       // (array output) array of end-points of edges on the isosurface
    WholeArrayOut
      isValidEdges, // (array output) whether the edge plan to draw belongs to the branch
    ExecObject
      findSuperarcForNode // (execution object) detector for the superarc of interpolated nodes
  );
  using ExecutionSignature =
    void(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14);
  using InputDomain = _1;

  using IdArrayReadPortalType = typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;
  using IdArrayWritePortalType =
    typename viskores::cont::ArrayHandle<viskores::Id>::WritePortalType;
  using ValueArrayPortalType = typename viskores::cont::ArrayHandle<ValueType>::ReadPortalType;
  using EdgePointArrayPortalType =
    typename viskores::cont::ArrayHandle<viskores::Vec3f_64>::WritePortalType;

  /// Constructor
  /// ptDimensions: dimension of points in the grid
  /// branchSuperarc: the superarc on the given branch intersecting the isosurface
  /// isoValue: isovalue for the isosurface to extract
  VISKORES_EXEC_CONT
  GetEdgesInCellWorklet(const viskores::Id3 ptDimensions,
                        const viskores::Id3 globalPointIndexStart,
                        const ValueType isoValue,
                        const viskores::Id globalRegularId,
                        const viskores::Id branchSuperarc,
                        const viskores::Id branchSaddleEpsilon,
                        const viskores::Id totNumPoints,
                        const bool marchingCubes,
                        const bool contourByValue)
    : PointDimensions(ptDimensions)
    , GlobalPointIndexStart(globalPointIndexStart)
    , IsoValue(isoValue)
    , GlobalRegularId(globalRegularId)
    , BranchSuperarc(branchSuperarc)
    , BranchSaddleEpsilon(branchSaddleEpsilon)
    , TotalNumPoints(totNumPoints)
    , IsMarchingCubes(marchingCubes)
    , IsContourByValue(contourByValue)
  {
    CellDimensions[0] = ptDimensions[0] - 1;
    CellDimensions[1] = ptDimensions[1] - 1;
    CellDimensions[2] = ptDimensions[2] - 1;
  }

  // cell index: the point index within the local cell
  VISKORES_EXEC viskores::Id CellIndexToNodeIndex2D(const viskores::Id2& localPt,
                                                    const viskores::Id cellIndex,
                                                    const IdArrayReadPortalType& vertOffset) const
  {
    return vertOffset.Get(cellIndex * 2) + localPt[0] +
      (vertOffset.Get(cellIndex * 2 + 1) + localPt[1]) * PointDimensions[0];
  }

  // cell index: the point index within the local cell
  VISKORES_EXEC viskores::Vec3f_64 CellIndexToNodeCoord2D(
    const viskores::Id2& localPt,
    const viskores::Id cellIndex,
    const IdArrayReadPortalType& vertOffset) const
  {
    return viskores::Vec3f_64(
      static_cast<viskores::Float64>(vertOffset.Get(cellIndex * 2) + localPt[0]),
      static_cast<viskores::Float64>(vertOffset.Get(cellIndex * 2 + 1) + localPt[1]),
      static_cast<viskores::Float64>(0));
  }

  // cell index: the point index within the local cell
  VISKORES_EXEC viskores::Id CellIndexToNodeIndex3D(const viskores::Id3& localPt,
                                                    const viskores::Id cellIndex,
                                                    const IdArrayReadPortalType& vertOffset) const
  {
    return vertOffset.Get(cellIndex * 3) + localPt[0] +
      (vertOffset.Get(cellIndex * 3 + 1) + localPt[1]) * PointDimensions[0] +
      (vertOffset.Get(cellIndex * 3 + 2) + localPt[2]) * (PointDimensions[0] * PointDimensions[1]);
  }

  // cell index: the point index within the local cell
  VISKORES_EXEC viskores::Vec3f_64 CellIndexToNodeCoord3D(
    const viskores::Id3& localPt,
    const viskores::Id cellIndex,
    const IdArrayReadPortalType& vertOffset) const
  {
    return viskores::Vec3f_64(
      static_cast<viskores::Float64>(vertOffset.Get(cellIndex * 3) + localPt[0]),
      static_cast<viskores::Float64>(vertOffset.Get(cellIndex * 3 + 1) + localPt[1]),
      static_cast<viskores::Float64>(vertOffset.Get(cellIndex * 3 + 2) + localPt[2]));
  }

  // Implementation to draw isosurface edges
  // all hard-coded numbers in this function depends on the dimension of the data
  // The number of vertices/lines/faces of a cell is fixed for a certain dimension
  // The number of cases for the marching cube algorithm are also hard-coded
  // Check MarchingCubesDataTables.h for more details
  template <typename FindSuperarcExecType>
  VISKORES_EXEC void operator()(
    const viskores::Id localIndex, // refers to the index in the grid
    const viskores::Id edgeOffset,
    const viskores::Id caseCell,
    const IdArrayReadPortalType& localIdsPortal, // refers to the index in (superarc etc.) arrays
    const ValueArrayPortalType& dataValuesPortal,
    const IdArrayReadPortalType& globalIdsPortal,
    const IdArrayReadPortalType& vertexOffset,
    const IdArrayReadPortalType& edgeTable,
    const IdArrayReadPortalType& numBoundTable,
    const IdArrayReadPortalType& boundaryTable,
    const IdArrayReadPortalType& labelEdgeTable,
    EdgePointArrayPortalType& edgesFromPortal,
    EdgePointArrayPortalType& edgesToPortal,
    IdArrayWritePortalType& isValidEdgesPortal,
    const FindSuperarcExecType& findSuperarcForNode) const
  {
    const viskores::Id nPoints = PointDimensions[0] * PointDimensions[1] * PointDimensions[2];
    // 2D
    if (CellDimensions[2] <= 0)
    {
      const viskores::Id2 localPt(localIndex % CellDimensions[0], localIndex / CellDimensions[0]);

      VISKORES_ASSERT(localIdsPortal.GetNumberOfValues() == nPoints);
      VISKORES_ASSERT(dataValuesPortal.GetNumberOfValues() == nPoints);

      const viskores::Id numEdges = numBoundTable.Get(caseCell);
      if (numEdges < 1)
        return;
      for (viskores::Id edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
      {
        const viskores::Id lineForCaseOffset = caseCell * nLineTableElemSize2d; // 8;
        const viskores::Id lineOffset = lineForCaseOffset + edgeIndex * 2;
        // lineFrom and lineTo are two edges where the isosurface edge intersects
        const viskores::Id lineFrom = boundaryTable.Get(lineOffset);
        const viskores::Id lineTo = boundaryTable.Get(lineOffset + 1);

        // We need to assure that both lineFrom and lineTo belong to the branch
        // all 0 and 1 in the variables below refer to the two vertices of the line
        const viskores::Id lineFromVert0 =
          CellIndexToNodeIndex2D(localPt, edgeTable.Get(lineFrom * 2), vertexOffset);
        const viskores::Id lineFromVert1 =
          CellIndexToNodeIndex2D(localPt, edgeTable.Get(lineFrom * 2 + 1), vertexOffset);
        VISKORES_ASSERT(lineFromVert0 < nPoints);
        VISKORES_ASSERT(lineFromVert1 < nPoints);
        const viskores::Id lineFromVert0LocalId = localIdsPortal.Get(lineFromVert0);
        const viskores::Id lineFromVert1LocalId = localIdsPortal.Get(lineFromVert1);
        const ValueType lineFromVert0Value = dataValuesPortal.Get(lineFromVert0);
        const ValueType lineFromVert1Value = dataValuesPortal.Get(lineFromVert1);
        const viskores::Id lineFromVert0GlobalId = globalIdsPortal.Get(lineFromVert0);
        const viskores::Id lineFromVert1GlobalId = globalIdsPortal.Get(lineFromVert1);
        // due to simulation of simplicity
        // vert0 < vert1 if their values are equal
        const viskores::Id lowVertFrom =
          lineFromVert0Value <= lineFromVert1Value ? lineFromVert0LocalId : lineFromVert1LocalId;
        const viskores::Id highVertFrom =
          lineFromVert0Value > lineFromVert1Value ? lineFromVert0LocalId : lineFromVert1LocalId;

        const viskores::Id lineToVert0 =
          CellIndexToNodeIndex2D(localPt, edgeTable.Get(lineTo * 2), vertexOffset);
        const viskores::Id lineToVert1 =
          CellIndexToNodeIndex2D(localPt, edgeTable.Get(lineTo * 2 + 1), vertexOffset);
        VISKORES_ASSERT(lineToVert0 < nPoints);
        VISKORES_ASSERT(lineToVert1 < nPoints);
        const viskores::Id lineToVert0LocalId = localIdsPortal.Get(lineToVert0);
        const viskores::Id lineToVert1LocalId = localIdsPortal.Get(lineToVert1);
        const ValueType lineToVert0Value = dataValuesPortal.Get(lineToVert0);
        const ValueType lineToVert1Value = dataValuesPortal.Get(lineToVert1);
        const viskores::Id lineToVert0GlobalId = globalIdsPortal.Get(lineToVert0);
        const viskores::Id lineToVert1GlobalId = globalIdsPortal.Get(lineToVert1);
        // due to simulation of simplicity
        // vert0 < vert1 if their values are equal
        const viskores::Id lowVertTo =
          lineToVert0Value <= lineToVert1Value ? lineToVert0LocalId : lineToVert1LocalId;
        const viskores::Id highVertTo =
          lineToVert0Value > lineToVert1Value ? lineToVert0LocalId : lineToVert1LocalId;

        viskores::Id lineFromSuperarc = -1;
        viskores::Id lineToSuperarc = -1;

        // We always extract the isosurface above/below the isovalue by 0+
        // If we extract contours by value (i.e., ignore simulation of simplicity),
        // the global regular ID of the contour should be inf small or large;
        // otherwise, it is +/-1 by the global regular id of the saddle end of the branch.
        VISKORES_ASSERT(BranchSaddleEpsilon != 0);
        viskores::Id contourGRId = IsContourByValue
          ? (BranchSaddleEpsilon > 0 ? TotalNumPoints : -1)
          : GlobalRegularId + BranchSaddleEpsilon;
        lineFromSuperarc = findSuperarcForNode.FindSuperArcForUnknownNode(
          contourGRId, IsoValue, highVertFrom, lowVertFrom);
        lineToSuperarc = findSuperarcForNode.FindSuperArcForUnknownNode(
          contourGRId, IsoValue, highVertTo, lowVertTo);

        // we only draw the line if both lineFrom and lineTo belongs to the branch of query
        if (lineFromSuperarc != BranchSuperarc || lineToSuperarc != BranchSuperarc)
        {
          isValidEdgesPortal.Set(edgeOffset + edgeIndex, 0);
          continue;
        }
        isValidEdgesPortal.Set(edgeOffset + edgeIndex, 1);

        // Now let's draw the line
        viskores::Vec3f_64 lineFromVert0Coord =
          CellIndexToNodeCoord2D(localPt, edgeTable.Get(lineFrom * 2), vertexOffset);
        viskores::Vec3f_64 lineFromVert1Coord =
          CellIndexToNodeCoord2D(localPt, edgeTable.Get(lineFrom * 2 + 1), vertexOffset);
        viskores::Vec3f_64 lineToVert0Coord =
          CellIndexToNodeCoord2D(localPt, edgeTable.Get(lineTo * 2), vertexOffset);
        viskores::Vec3f_64 lineToVert1Coord =
          CellIndexToNodeCoord2D(localPt, edgeTable.Get(lineTo * 2 + 1), vertexOffset);

        viskores::Vec3f_64 fromPt(lineFromVert0Coord);
        viskores::Vec3f_64 toPt(lineToVert0Coord);

        // when values of two vertices in the cell are equal, we rely on the simulation of simplicity
        viskores::Float64 fromRatio = lineFromVert1Value == lineFromVert0Value
          ? viskores::Float64(GlobalRegularId - lineFromVert0GlobalId) /
            (lineFromVert1GlobalId - lineFromVert0GlobalId)
          : viskores::Float64(IsoValue - lineFromVert0Value) /
            (lineFromVert1Value - lineFromVert0Value);
        viskores::Float64 toRatio = lineToVert1Value == lineToVert0Value
          ? viskores::Float64(GlobalRegularId - lineToVert0GlobalId) /
            (lineToVert1GlobalId - lineToVert0GlobalId)
          : viskores::Float64(IsoValue - lineToVert0Value) / (lineToVert1Value - lineToVert0Value);

        VISKORES_ASSERT(fromRatio >= 0.0 && fromRatio <= 1.0);
        VISKORES_ASSERT(toRatio >= 0.0 && toRatio <= 1.0);

        fromPt += (lineFromVert1Coord - lineFromVert0Coord) * fromRatio;
        toPt += (lineToVert1Coord - lineToVert0Coord) * toRatio;

        edgesFromPortal.Set(edgeOffset + edgeIndex, fromPt + GlobalPointIndexStart);
        edgesToPortal.Set(edgeOffset + edgeIndex, toPt + GlobalPointIndexStart);
      }
    }
    else // 3D
    {
      viskores::Id3 localPt(localIndex % CellDimensions[0],
                            (localIndex / CellDimensions[0]) % CellDimensions[1],
                            localIndex / (CellDimensions[0] * CellDimensions[1]));

      const viskores::Id numTriangles = numBoundTable.Get(caseCell);
      if (numTriangles < 1)
        return;

      // we check a specific edge to know the superarc of the triangle
      // the edge label of the triangle is stored in labelEdgeTable in MarchingCubesDataTables.h
      // there are at most 5 triangles to draw in each 3D cell (for marching cubes)
      // for linear interpolation, there are at most 12 triangles
      if (IsMarchingCubes)
        VISKORES_ASSERT(numTriangles <= MAX_MARCHING_CUBE_TRIANGLES);
      else
        VISKORES_ASSERT(numTriangles <= MAX_LINEAR_INTERPOLATION_TRIANGLES);
      viskores::Id triangleSuperarc[MAX_LINEAR_INTERPOLATION_TRIANGLES + 1];

      viskores::Id triangleLabelIdx = 0;
      const viskores::Id nLabelEdgeElemSize =
        IsMarchingCubes ? nLabelEdgeTableMC3dElemSize : nLabelEdgeTableLT3dElemSize;
      viskores::Id labelPtr = caseCell * nLabelEdgeElemSize;
      while (labelEdgeTable.Get(labelPtr) != -1)
      {
        viskores::Id labelCount = labelEdgeTable.Get(labelPtr++);
        viskores::Id labelEdge = labelEdgeTable.Get(labelPtr++);

        // compute the superarc of the labelEdge belong to the branch
        const viskores::Id labelEdgeVert0 =
          CellIndexToNodeIndex3D(localPt, edgeTable.Get(labelEdge * 2), vertexOffset);
        const viskores::Id labelEdgeVert1 =
          CellIndexToNodeIndex3D(localPt, edgeTable.Get(labelEdge * 2 + 1), vertexOffset);
        VISKORES_ASSERT(labelEdgeVert0 < nPoints);
        VISKORES_ASSERT(labelEdgeVert1 < nPoints);

        const viskores::Id labelEdgeVert0LocalId = localIdsPortal.Get(labelEdgeVert0);
        const viskores::Id labelEdgeVert1LocalId = localIdsPortal.Get(labelEdgeVert1);
        const ValueType labelEdgeVert0Value = dataValuesPortal.Get(labelEdgeVert0);
        const ValueType labelEdgeVert1Value = dataValuesPortal.Get(labelEdgeVert1);
        // due to simulation of simplicity
        // vert0 < vert1 if their values are equal
        const viskores::Id lowVert = labelEdgeVert0Value <= labelEdgeVert1Value
          ? labelEdgeVert0LocalId
          : labelEdgeVert1LocalId;
        const viskores::Id highVert =
          labelEdgeVert0Value > labelEdgeVert1Value ? labelEdgeVert0LocalId : labelEdgeVert1LocalId;

        viskores::Id labelEdgeSuperarc = -1;

        // We always extract the isosurface above/below the isovalue.globalRegularId by 0+
        // If we extract contours by value (i.e., ignore simulation of simplicity),
        // the global regular ID of the contour should be inf small or large;
        // otherwise, it is +/-1 by the global regular id of the saddle end of the branch.
        VISKORES_ASSERT(BranchSaddleEpsilon != 0);
        viskores::Id contourGRId = IsContourByValue
          ? (BranchSaddleEpsilon > 0 ? TotalNumPoints : -1)
          : GlobalRegularId + BranchSaddleEpsilon;

        labelEdgeSuperarc =
          findSuperarcForNode.FindSuperArcForUnknownNode(contourGRId, IsoValue, highVert, lowVert);
        for (viskores::Id i = 0; i < labelCount; i++)
          triangleSuperarc[triangleLabelIdx++] = labelEdgeSuperarc;
      }

      VISKORES_ASSERT(triangleLabelIdx == numTriangles);

      const viskores::Id nTriTableElemSize =
        IsMarchingCubes ? nTriTableMC3dElemSize : nTriTableLT3dElemSize;
      for (viskores::Id triIndex = 0; triIndex < numTriangles; triIndex++)
      {
        const viskores::Id lineFroms[3] = {
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3),
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3 + 1),
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3 + 2)
        };
        const viskores::Id lineTos[3] = {
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3 + 1),
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3 + 2),
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3)
        };

        const viskores::Id labelEdgeSuperarc = triangleSuperarc[triIndex];
        // we only draw the triangle if the triangle lies on the branch of query
        if (labelEdgeSuperarc != BranchSuperarc)
        {
          isValidEdgesPortal.Set(edgeOffset + triIndex * 3, 0);
          isValidEdgesPortal.Set(edgeOffset + triIndex * 3 + 1, 0);
          isValidEdgesPortal.Set(edgeOffset + triIndex * 3 + 2, 0);
          continue;
        }
        isValidEdgesPortal.Set(edgeOffset + triIndex * 3, 1);
        isValidEdgesPortal.Set(edgeOffset + triIndex * 3 + 1, 1);
        isValidEdgesPortal.Set(edgeOffset + triIndex * 3 + 2, 1);

        for (viskores::Id edgeIndex = 0; edgeIndex < 3; edgeIndex++)
        {
          // lineFrom and lineTo are two edges where the edge of the triangle intersects
          const viskores::Id lineFrom = lineFroms[edgeIndex];
          const viskores::Id lineTo = lineTos[edgeIndex];

          // Now let's draw the line
          viskores::Vec3f_64 lineFromVert0Coord =
            CellIndexToNodeCoord3D(localPt, edgeTable.Get(lineFrom * 2), vertexOffset);
          viskores::Vec3f_64 lineFromVert1Coord =
            CellIndexToNodeCoord3D(localPt, edgeTable.Get(lineFrom * 2 + 1), vertexOffset);
          viskores::Vec3f_64 lineToVert0Coord =
            CellIndexToNodeCoord3D(localPt, edgeTable.Get(lineTo * 2), vertexOffset);
          viskores::Vec3f_64 lineToVert1Coord =
            CellIndexToNodeCoord3D(localPt, edgeTable.Get(lineTo * 2 + 1), vertexOffset);

          viskores::Vec3f_64 fromPt(lineFromVert0Coord);
          viskores::Vec3f_64 toPt(lineToVert0Coord);

          const viskores::Id lineFromVert0 =
            CellIndexToNodeIndex3D(localPt, edgeTable.Get(lineFrom * 2), vertexOffset);
          const viskores::Id lineFromVert1 =
            CellIndexToNodeIndex3D(localPt, edgeTable.Get(lineFrom * 2 + 1), vertexOffset);
          VISKORES_ASSERT(lineFromVert0 < nPoints);
          VISKORES_ASSERT(lineFromVert1 < nPoints);
          const ValueType lineFromVert0Value = dataValuesPortal.Get(lineFromVert0);
          const ValueType lineFromVert1Value = dataValuesPortal.Get(lineFromVert1);
          const viskores::Id lineFromVert0GlobalId = globalIdsPortal.Get(lineFromVert0);
          const viskores::Id lineFromVert1GlobalId = globalIdsPortal.Get(lineFromVert1);

          const viskores::Id lineToVert0 =
            CellIndexToNodeIndex3D(localPt, edgeTable.Get(lineTo * 2), vertexOffset);
          const viskores::Id lineToVert1 =
            CellIndexToNodeIndex3D(localPt, edgeTable.Get(lineTo * 2 + 1), vertexOffset);
          VISKORES_ASSERT(lineToVert0 < nPoints);
          VISKORES_ASSERT(lineToVert1 < nPoints);
          const ValueType lineToVert0Value = dataValuesPortal.Get(lineToVert0);
          const ValueType lineToVert1Value = dataValuesPortal.Get(lineToVert1);
          const viskores::Id lineToVert0GlobalId = globalIdsPortal.Get(lineToVert0);
          const viskores::Id lineToVert1GlobalId = globalIdsPortal.Get(lineToVert1);

          viskores::Float64 fromRatio = lineFromVert1Value == lineFromVert0Value
            ? viskores::Float64(GlobalRegularId - lineFromVert0GlobalId) /
              (lineFromVert1GlobalId - lineFromVert0GlobalId)
            : viskores::Float64(IsoValue - lineFromVert0Value) /
              (lineFromVert1Value - lineFromVert0Value);
          viskores::Float64 toRatio = lineToVert1Value == lineToVert0Value
            ? viskores::Float64(GlobalRegularId - lineToVert0GlobalId) /
              (lineToVert1GlobalId - lineToVert0GlobalId)
            : viskores::Float64(IsoValue - lineToVert0Value) /
              (lineToVert1Value - lineToVert0Value);
          VISKORES_ASSERT(fromRatio >= 0.0 && fromRatio <= 1.0);
          VISKORES_ASSERT(toRatio >= 0.0 && toRatio <= 1.0);

          fromPt += (lineFromVert1Coord - lineFromVert0Coord) * fromRatio;
          toPt += (lineToVert1Coord - lineToVert0Coord) * toRatio;

          edgesFromPortal.Set(edgeOffset + triIndex * 3 + edgeIndex,
                              fromPt + GlobalPointIndexStart);
          edgesToPortal.Set(edgeOffset + triIndex * 3 + edgeIndex, toPt + GlobalPointIndexStart);
        }
      }
    }
  }

private:
  viskores::Id3 PointDimensions;
  viskores::Id3 GlobalPointIndexStart;
  ValueType IsoValue;
  viskores::Id GlobalRegularId;
  viskores::Id BranchSuperarc;
  viskores::Id BranchSaddleEpsilon;
  viskores::Id TotalNumPoints;
  bool IsMarchingCubes;
  bool IsContourByValue;
  viskores::Id3 CellDimensions;

}; // GetEdgesInCellWorklet

} // namespace extract_top_volume_contours
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
