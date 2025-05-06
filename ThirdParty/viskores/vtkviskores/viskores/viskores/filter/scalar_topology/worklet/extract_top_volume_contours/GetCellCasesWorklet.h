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

#ifndef viskores_filter_scalar_topology_worklet_extract_top_volume_contours_get_cell_cases_worklet_h
#define viskores_filter_scalar_topology_worklet_extract_top_volume_contours_get_cell_cases_worklet_h

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
/// Worklet for getting the polarity case of a cell compared to the isovalue.
/// Only consider 2D and 3D data.
/// The output for each cell is an integer ([0, 7] if 2D, or [0, 255] if 3D)
/// indicating the polarity at each vertex of the cell compared to the isovalue.
template <typename ValueType>
class GetCellCasesWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn localIdx,          // (input) local point index
    WholeArrayIn dataValues,   // (array input) data values within block
    WholeArrayIn globalIds,    // (array input) global regular IDs within block
    WholeArrayIn vertexOffset, // (array input) vertex offset look-up table
    FieldOut caseCell          // (output) the polarity case (in binary) of the cell
  );
  using ExecutionSignature = _5(_1, _2, _3, _4);
  using InputDomain = _1;

  using IdArrayPortalType = typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;
  using ValueArrayPortalType = typename viskores::cont::ArrayHandle<ValueType>::ReadPortalType;

  /// <summary>
  /// Constructor
  /// </summary>
  /// <param name="ptDimensions">dimension of points in the grid</param>
  /// <param name="branchSaddleEpsilon">the direction for tiebreaking when comparing values</param>
  /// <param name="isoValue">isovalue for the isosurface to extract</param>
  /// <returns></returns>
  VISKORES_EXEC_CONT
  GetCellCasesWorklet(const viskores::Id3 ptDimensions,
                      const viskores::Id branchSaddleEpsilon,
                      const ValueType isoValue,
                      const bool shiftIsovalueByEps,
                      const viskores::Id globalRegularId)
    : PointDimensions(ptDimensions)
    , BranchSaddleEpsilon(branchSaddleEpsilon)
    , IsoValue(isoValue)
    , ShiftIsovalueByEpsilon(shiftIsovalueByEps)
    , GlobalRegularId(globalRegularId)
  {
    CellDimensions[0] = ptDimensions[0] - 1;
    CellDimensions[1] = ptDimensions[1] - 1;
    CellDimensions[2] = ptDimensions[2] - 1;
  }

  /// <summary>
  /// Computes the polarity case of cells
  /// </summary>
  /// <param name="localIndex">the local index of the point in the local grid</param>
  /// <param name="dataValuesPortal">all data values on the local grid points</param>
  /// <returns>integer indicating the polarity case of the cell originating at the input point</returns>
  VISKORES_EXEC viskores::Id operator()(const viskores::Id localIndex,
                                        const ValueArrayPortalType& dataValuesPortal,
                                        const IdArrayPortalType& globalIdsPortal,
                                        const IdArrayPortalType& vertexOffset) const
  {
    const viskores::Id nPoints = PointDimensions[0] * PointDimensions[1] * PointDimensions[2];
    VISKORES_ASSERT(dataValuesPortal.GetNumberOfValues() == nPoints);
    if (CellDimensions[2] <= 0)
    {
      // the 2D local coordinate of the input point
      viskores::Id2 localPt(localIndex % CellDimensions[0], localIndex / CellDimensions[0]);
      viskores::Id caseCell = 0;
      // iterate over all points of the cell
      for (viskores::Id i = 0; i < nVertices2d; i++)
      {
        viskores::Id currPtIdx = i * 2;
        viskores::Id currPt = vertexOffset.Get(currPtIdx) + localPt[0] +
          (vertexOffset.Get(currPtIdx + 1) + localPt[1]) * PointDimensions[0];
        VISKORES_ASSERT(currPt < nPoints);
        // when point value == IsoValue
        // the index is 1 only if the branch is lower-end
        // Update 01/09/2025:
        // We need to bring in the global regular ID of vertices for simulation of simplicity.
        // When BranchSaddleEpsilon == -1 (lower end is leaf), the extracted contour has the isovalue of value.(GlobalRegularId - 0.5);
        // When BranchSaddleEpsilon == 1 (upper end is leaf), the extracted contour has the isovalue of value.(GlobalRegularId + 0.5);
        // We only mark the polarity of the vertex as positive when the value and global regular id is strictly larger than the extracted contour.
        if (dataValuesPortal.Get(currPt) > IsoValue ||
            (dataValuesPortal.Get(currPt) == IsoValue &&
             ((ShiftIsovalueByEpsilon && BranchSaddleEpsilon < 0) ||
              (!ShiftIsovalueByEpsilon &&
               (BranchSaddleEpsilon > 0 ? globalIdsPortal.Get(currPt) > GlobalRegularId
                                        : globalIdsPortal.Get(currPt) >= GlobalRegularId)))))
          caseCell |= viskores::Id(1) << i;
      }
      return caseCell;
    }
    else
    {
      // the 3D local coordinate of the input point
      viskores::Id3 localPt(localIndex % CellDimensions[0],
                            (localIndex / CellDimensions[0]) % CellDimensions[1],
                            localIndex / (CellDimensions[0] * CellDimensions[1]));
      viskores::Id caseCell = 0;
      // iterate over all points of the cell
      for (viskores::Id i = 0; i < nVertices3d; i++)
      {
        viskores::Id currPtIdx = i * 3;
        viskores::Id currPt = vertexOffset.Get(currPtIdx) + localPt[0] +
          (vertexOffset.Get(currPtIdx + 1) + localPt[1]) * PointDimensions[0] +
          (vertexOffset.Get(currPtIdx + 2) + localPt[2]) *
            (PointDimensions[0] * PointDimensions[1]);
        VISKORES_ASSERT(currPt < nPoints);
        // when point value == IsoValue
        // the index is 1 only if the branch is lower-end
        // Update 01/09/2025:
        // We need to bring in the global regular ID of vertices for simulation of simplicity.
        // When BranchSaddleEpsilon == -1 (lower end is leaf), the extracted contour has the isovalue of value.(GlobalRegularId - 0.5);
        // When BranchSaddleEpsilon == 1 (upper end is leaf), the extracted contour has the isovalue of value.(GlobalRegularId + 0.5);
        // We only mark the polarity of the vertex as positive when the value and global regular id is strictly larger than the extracted contour.
        if (dataValuesPortal.Get(currPt) > IsoValue ||
            (dataValuesPortal.Get(currPt) == IsoValue &&
             ((ShiftIsovalueByEpsilon && BranchSaddleEpsilon < 0) ||
              (!ShiftIsovalueByEpsilon &&
               (BranchSaddleEpsilon > 0 ? globalIdsPortal.Get(currPt) > GlobalRegularId
                                        : globalIdsPortal.Get(currPt) >= GlobalRegularId)))))
          caseCell |= viskores::Id(1) << i;
      }
      return caseCell;
    }
  }

private:
  viskores::Id3 PointDimensions;
  viskores::Id BranchSaddleEpsilon;
  ValueType IsoValue;
  bool ShiftIsovalueByEpsilon;
  viskores::Id GlobalRegularId;
  viskores::Id3 CellDimensions;

}; // GetCellCasesWorklet

} // namespace extract_top_volume_contours
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
