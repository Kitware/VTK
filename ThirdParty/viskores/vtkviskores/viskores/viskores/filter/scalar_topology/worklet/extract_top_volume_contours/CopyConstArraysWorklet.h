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

#ifndef viskores_filter_scalar_topology_worklet_extract_top_volume_contours_copy_const_arrays_worklet_h
#define viskores_filter_scalar_topology_worklet_extract_top_volume_contours_copy_const_arrays_worklet_h

#include <viskores/Assert.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace scalar_topology
{
namespace extract_top_volume_contours
{

constexpr viskores::IdComponent COPY_VERTEXOFFSET = 1;
constexpr viskores::IdComponent COPY_EDGETABLE = 2;
constexpr viskores::IdComponent COPY_NUMBOUNDTABLE = 3;
constexpr viskores::IdComponent COPY_BOUNDARYTABLE = 4;
constexpr viskores::IdComponent COPY_LABELEDGETABLE = 5;

constexpr viskores::Id nVertices2d = 4;
constexpr viskores::Id nEdges2d = 5;
constexpr viskores::Id nCases2d = 16;
constexpr viskores::Id nLineTableElemSize2d = 8;

constexpr viskores::Id nVertices3d = 8;
constexpr viskores::Id nEdgesMC3d = 12;
constexpr viskores::Id nEdgesLT3d = 19;
constexpr viskores::Id nCasesMC3d = 256;
constexpr viskores::Id nCasesLT3d = 256;

constexpr viskores::Id nTriTableMC3dElemSize = 16; // (at most 5 triangles, each with 3 vertices)
constexpr viskores::Id nTriTableLT3dElemSize = 37; // (at most 12 triangles, each with 3 vertices)

constexpr viskores::Id nLabelEdgeTableMC3dElemSize = 9;  // (at most 4 label edges)
constexpr viskores::Id nLabelEdgeTableLT3dElemSize = 13; // (at most 6 tetrahedra)

/// Worklet for copying the const array in the MarchingCubesDataTables.h
class CopyConstArraysForMarchingCubesDataTablesWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn arrayIndex,    // (input) index of the array (need length specification outside)
    FieldOut constArrayOut // (output) the vertex offset array based on data type / specification
  );
  using ExecutionSignature = _2(_1);
  using InputDomain = _1;

  /// <summary>
  /// Constructor
  /// </summary>
  VISKORES_EXEC_CONT
  CopyConstArraysForMarchingCubesDataTablesWorklet(const bool isData2D,
                                                   const bool isMarchingCube,
                                                   const viskores::IdComponent copyType)
    : IsData2D(isData2D)
    , IsMarchingCube(isMarchingCube)
    , CopyType(copyType)
  {
  }

  /// <summary>
  /// Implementation of CopyConstArraysForMarchingCubesDataTablesWorklet.
  /// Based on the data dimension, whether or not using marching cubes, and the copy type,
  /// call the corresponding function to return the const array.
  /// </summary>
  /// <param name="arrayIndex">array index. Length is determined by worklet calls outside.</param>
  /// <param name="constArrayOut">output, the const array value at the given index.</param>
  /// <returns></returns>
  VISKORES_EXEC viskores::Id operator()(const viskores::Id arrayIndex) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id vertexOffset2d[nVertices2d * 2] = { 0, 0, 1, 0,
                                                                                     1, 1, 0, 1 };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id edgeTable2d[nEdges2d * 2] = { 0, 1, 1, 2, 3,
                                                                               2, 0, 3, 0, 2 };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id numLinesTable2d[nCases2d] = { 0, 2, 1, 2, 2, 2,
                                                                               2, 1, 1, 2, 2, 2,
                                                                               2, 1, 2, 0 };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id lineTable2d[nCases2d * nLineTableElemSize2d] = {
#define X -1
      X, X, X, X, X, X, X, X, 3, 4, 4, 0, X, X, X, X, 0, 1, X, X, X, X, X, X, 3, 4,
      4, 1, X, X, X, X, 2, 4, 4, 1, X, X, X, X, 3, 2, 0, 1, X, X, X, X, 2, 4, 4, 0,
      X, X, X, X, 3, 2, X, X, X, X, X, X, 3, 2, X, X, X, X, X, X, 2, 4, 4, 0, X, X,
      X, X, 3, 2, 0, 1, X, X, X, X, 2, 4, 4, 1, X, X, X, X, 3, 4, 4, 1, X, X, X, X,
      0, 1, X, X, X, X, X, X, 3, 4, 4, 0, X, X, X, X, X, X, X, X, X, X, X, X
#undef X
    };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id vertexOffset3d[nVertices3d * 3] = {
      0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1
    };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id edgeTableMC3d[nEdgesMC3d * 2] = {
      0, 1, 1, 2, 3, 2, 0, 3, 4, 5, 5, 6, 7, 6, 4, 7, 0, 4, 1, 5, 2, 6, 3, 7
    };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id edgeTableLT3d[nEdgesLT3d * 2] = {
      0, 1, 1, 2, 3, 2, 0, 3, 4, 5, 5, 6, 7, 6, 4, 7, 0, 4, 1,
      5, 2, 6, 3, 7, 0, 2, 4, 6, 0, 5, 3, 6, 0, 7, 1, 6, 0, 6
    };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id numTrianglesTableMC3d[nCasesMC3d] = {
      0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 2, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3,
      4, 4, 3, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 2, 3, 3, 2, 3, 4, 4, 3, 3, 4,
      4, 3, 4, 5, 5, 2, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 2, 3, 3, 4, 3, 4, 4,
      5, 3, 4, 4, 5, 4, 5, 5, 4, 2, 3, 3, 4, 3, 4, 2, 3, 3, 4, 4, 5, 4, 5, 3, 2, 3, 4, 4, 3,
      4, 5, 3, 2, 4, 5, 5, 4, 5, 2, 4, 1, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 2,
      3, 3, 4, 3, 4, 4, 5, 3, 2, 4, 3, 4, 3, 5, 2, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5,
      5, 4, 3, 4, 4, 3, 4, 5, 5, 4, 4, 3, 5, 2, 5, 4, 2, 1, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4,
      5, 2, 3, 3, 2, 3, 4, 4, 5, 4, 5, 5, 2, 4, 3, 5, 4, 3, 2, 4, 1, 3, 4, 4, 5, 4, 5, 3, 4,
      4, 5, 5, 2, 3, 4, 2, 1, 2, 3, 3, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 1, 1, 0,
    };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id numTrianglesTableLT3d[nCasesLT3d] = {
      0,  6,  2,  8,  2,  8,  4,  8,  2,  8,  4,  10, 4,  8,  6,  8,  2,  8,  4,  10, 4,  10,
      6,  10, 4,  10, 6,  12, 6,  10, 8,  10, 2,  8,  4,  8,  4,  10, 6,  8,  4,  10, 6,  10,
      6,  10, 8,  8,  4,  8,  6,  8,  6,  10, 8,  8,  6,  10, 8,  10, 8,  10, 10, 8,  6,  12,
      8,  10, 8,  10, 8,  8,  8,  10, 10, 8,  8,  8,  8,  6,  8,  10, 10, 8,  10, 8,  10, 6,
      10, 8,  12, 6,  10, 6,  10, 4,  8,  10, 8,  8,  10, 8,  8,  6,  10, 8,  10, 6,  10, 6,
      8,  4,  8,  8,  8,  6,  10, 6,  8,  4,  10, 6,  10, 4,  10, 4,  8,  2,  2,  8,  4,  10,
      4,  10, 6,  10, 4,  8,  6,  10, 6,  8,  8,  8,  4,  8,  6,  10, 6,  10, 8,  10, 6,  8,
      8,  10, 8,  8,  10, 8,  4,  10, 6,  10, 6,  12, 8,  10, 6,  10, 8,  10, 8,  10, 10, 8,
      6,  8,  8,  8,  8,  10, 10, 8,  8,  8,  10, 8,  10, 8,  12, 6,  8,  10, 10, 8,  10, 8,
      10, 6,  8,  8,  10, 6,  8,  6,  8,  4,  8,  8,  10, 6,  10, 6,  10, 4,  8,  6,  10, 4,
      8,  4,  8,  2,  10, 8,  10, 6,  12, 6,  10, 4,  10, 6,  10, 4,  10, 4,  8,  2,  8,  6,
      8,  4,  10, 4,  8,  2,  8,  4,  8,  2,  8,  2,  6,  0
    };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id
      triTableMC3d[nCasesMC3d * nTriTableMC3dElemSize] = {
#define X -1
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  8,  3,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  9,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  1,  8,  3,  9,  8,  1,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  2,  10, X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  8,  3,  1,  2,  10, X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  9,  2,  10, 0,  2,  9,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  2,  8,  3,
        2,  10, 8,  10, 9,  8,  X,  X,  X,  X,  X,  X,  X,  3,  11, 2,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  0,  11, 2,  8,  11, 0,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,
        9,  0,  2,  3,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  11, 2,  1,  9,  11, 9,  8,
        11, X,  X,  X,  X,  X,  X,  X,  3,  10, 1,  11, 10, 3,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  0,  10, 1,  0,  8,  10, 8,  11, 10, X,  X,  X,  X,  X,  X,  X,  3,  9,  0,  3,  11, 9,
        11, 10, 9,  X,  X,  X,  X,  X,  X,  X,  9,  8,  10, 10, 8,  11, X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  4,  7,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  4,  3,  0,  7,
        3,  4,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  9,  8,  4,  7,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  4,  1,  9,  4,  7,  1,  7,  3,  1,  X,  X,  X,  X,  X,  X,  X,  1,  2,
        10, 8,  4,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  4,  7,  3,  0,  4,  1,  2,  10,
        X,  X,  X,  X,  X,  X,  X,  9,  2,  10, 9,  0,  2,  8,  4,  7,  X,  X,  X,  X,  X,  X,  X,
        2,  10, 9,  2,  9,  7,  2,  7,  3,  7,  9,  4,  X,  X,  X,  X,  8,  4,  7,  3,  11, 2,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  11, 4,  7,  11, 2,  4,  2,  0,  4,  X,  X,  X,  X,  X,
        X,  X,  9,  0,  1,  8,  4,  7,  2,  3,  11, X,  X,  X,  X,  X,  X,  X,  4,  7,  11, 9,  4,
        11, 9,  11, 2,  9,  2,  1,  X,  X,  X,  X,  3,  10, 1,  3,  11, 10, 7,  8,  4,  X,  X,  X,
        X,  X,  X,  X,  1,  11, 10, 1,  4,  11, 1,  0,  4,  7,  11, 4,  X,  X,  X,  X,  4,  7,  8,
        9,  0,  11, 9,  11, 10, 11, 0,  3,  X,  X,  X,  X,  4,  7,  11, 4,  11, 9,  9,  11, 10, X,
        X,  X,  X,  X,  X,  X,  9,  5,  4,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  9,
        5,  4,  0,  8,  3,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  5,  4,  1,  5,  0,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  8,  5,  4,  8,  3,  5,  3,  1,  5,  X,  X,  X,  X,  X,  X,
        X,  1,  2,  10, 9,  5,  4,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  0,  8,  1,  2,  10,
        4,  9,  5,  X,  X,  X,  X,  X,  X,  X,  5,  2,  10, 5,  4,  2,  4,  0,  2,  X,  X,  X,  X,
        X,  X,  X,  2,  10, 5,  3,  2,  5,  3,  5,  4,  3,  4,  8,  X,  X,  X,  X,  9,  5,  4,  2,
        3,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  11, 2,  0,  8,  11, 4,  9,  5,  X,  X,
        X,  X,  X,  X,  X,  0,  5,  4,  0,  1,  5,  2,  3,  11, X,  X,  X,  X,  X,  X,  X,  2,  1,
        5,  2,  5,  8,  2,  8,  11, 4,  8,  5,  X,  X,  X,  X,  10, 3,  11, 10, 1,  3,  9,  5,  4,
        X,  X,  X,  X,  X,  X,  X,  4,  9,  5,  0,  8,  1,  8,  10, 1,  8,  11, 10, X,  X,  X,  X,
        5,  4,  0,  5,  0,  11, 5,  11, 10, 11, 0,  3,  X,  X,  X,  X,  5,  4,  8,  5,  8,  10, 10,
        8,  11, X,  X,  X,  X,  X,  X,  X,  9,  7,  8,  5,  7,  9,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  9,  3,  0,  9,  5,  3,  5,  7,  3,  X,  X,  X,  X,  X,  X,  X,  0,  7,  8,  0,  1,
        7,  1,  5,  7,  X,  X,  X,  X,  X,  X,  X,  1,  5,  3,  3,  5,  7,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  9,  7,  8,  9,  5,  7,  10, 1,  2,  X,  X,  X,  X,  X,  X,  X,  10, 1,  2,
        9,  5,  0,  5,  3,  0,  5,  7,  3,  X,  X,  X,  X,  8,  0,  2,  8,  2,  5,  8,  5,  7,  10,
        5,  2,  X,  X,  X,  X,  2,  10, 5,  2,  5,  3,  3,  5,  7,  X,  X,  X,  X,  X,  X,  X,  7,
        9,  5,  7,  8,  9,  3,  11, 2,  X,  X,  X,  X,  X,  X,  X,  9,  5,  7,  9,  7,  2,  9,  2,
        0,  2,  7,  11, X,  X,  X,  X,  2,  3,  11, 0,  1,  8,  1,  7,  8,  1,  5,  7,  X,  X,  X,
        X,  11, 2,  1,  11, 1,  7,  7,  1,  5,  X,  X,  X,  X,  X,  X,  X,  9,  5,  8,  8,  5,  7,
        10, 1,  3,  10, 3,  11, X,  X,  X,  X,  5,  7,  0,  5,  0,  9,  7,  11, 0,  1,  0,  10, 11,
        10, 0,  X,  11, 10, 0,  11, 0,  3,  10, 5,  0,  8,  0,  7,  5,  7,  0,  X,  11, 10, 5,  7,
        11, 5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  10, 6,  5,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  0,  8,  3,  5,  10, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  9,  0,
        1,  5,  10, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  8,  3,  1,  9,  8,  5,  10, 6,
        X,  X,  X,  X,  X,  X,  X,  1,  6,  5,  2,  6,  1,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        1,  6,  5,  1,  2,  6,  3,  0,  8,  X,  X,  X,  X,  X,  X,  X,  9,  6,  5,  9,  0,  6,  0,
        2,  6,  X,  X,  X,  X,  X,  X,  X,  5,  9,  8,  5,  8,  2,  5,  2,  6,  3,  2,  8,  X,  X,
        X,  X,  2,  3,  11, 10, 6,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  11, 0,  8,  11, 2,
        0,  10, 6,  5,  X,  X,  X,  X,  X,  X,  X,  0,  1,  9,  2,  3,  11, 5,  10, 6,  X,  X,  X,
        X,  X,  X,  X,  5,  10, 6,  1,  9,  2,  9,  11, 2,  9,  8,  11, X,  X,  X,  X,  6,  3,  11,
        6,  5,  3,  5,  1,  3,  X,  X,  X,  X,  X,  X,  X,  0,  8,  11, 0,  11, 5,  0,  5,  1,  5,
        11, 6,  X,  X,  X,  X,  3,  11, 6,  0,  3,  6,  0,  6,  5,  0,  5,  9,  X,  X,  X,  X,  6,
        5,  9,  6,  9,  11, 11, 9,  8,  X,  X,  X,  X,  X,  X,  X,  5,  10, 6,  4,  7,  8,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  4,  3,  0,  4,  7,  3,  6,  5,  10, X,  X,  X,  X,  X,  X,
        X,  1,  9,  0,  5,  10, 6,  8,  4,  7,  X,  X,  X,  X,  X,  X,  X,  10, 6,  5,  1,  9,  7,
        1,  7,  3,  7,  9,  4,  X,  X,  X,  X,  6,  1,  2,  6,  5,  1,  4,  7,  8,  X,  X,  X,  X,
        X,  X,  X,  1,  2,  5,  5,  2,  6,  3,  0,  4,  3,  4,  7,  X,  X,  X,  X,  8,  4,  7,  9,
        0,  5,  0,  6,  5,  0,  2,  6,  X,  X,  X,  X,  7,  3,  9,  7,  9,  4,  3,  2,  9,  5,  9,
        6,  2,  6,  9,  X,  3,  11, 2,  7,  8,  4,  10, 6,  5,  X,  X,  X,  X,  X,  X,  X,  5,  10,
        6,  4,  7,  2,  4,  2,  0,  2,  7,  11, X,  X,  X,  X,  0,  1,  9,  4,  7,  8,  2,  3,  11,
        5,  10, 6,  X,  X,  X,  X,  9,  2,  1,  9,  11, 2,  9,  4,  11, 7,  11, 4,  5,  10, 6,  X,
        8,  4,  7,  3,  11, 5,  3,  5,  1,  5,  11, 6,  X,  X,  X,  X,  5,  1,  11, 5,  11, 6,  1,
        0,  11, 7,  11, 4,  0,  4,  11, X,  0,  5,  9,  0,  6,  5,  0,  3,  6,  11, 6,  3,  8,  4,
        7,  X,  6,  5,  9,  6,  9,  11, 4,  7,  9,  7,  11, 9,  X,  X,  X,  X,  10, 4,  9,  6,  4,
        10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  4,  10, 6,  4,  9,  10, 0,  8,  3,  X,  X,  X,
        X,  X,  X,  X,  10, 0,  1,  10, 6,  0,  6,  4,  0,  X,  X,  X,  X,  X,  X,  X,  8,  3,  1,
        8,  1,  6,  8,  6,  4,  6,  1,  10, X,  X,  X,  X,  1,  4,  9,  1,  2,  4,  2,  6,  4,  X,
        X,  X,  X,  X,  X,  X,  3,  0,  8,  1,  2,  9,  2,  4,  9,  2,  6,  4,  X,  X,  X,  X,  0,
        2,  4,  4,  2,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  8,  3,  2,  8,  2,  4,  4,  2,
        6,  X,  X,  X,  X,  X,  X,  X,  10, 4,  9,  10, 6,  4,  11, 2,  3,  X,  X,  X,  X,  X,  X,
        X,  0,  8,  2,  2,  8,  11, 4,  9,  10, 4,  10, 6,  X,  X,  X,  X,  3,  11, 2,  0,  1,  6,
        0,  6,  4,  6,  1,  10, X,  X,  X,  X,  6,  4,  1,  6,  1,  10, 4,  8,  1,  2,  1,  11, 8,
        11, 1,  X,  9,  6,  4,  9,  3,  6,  9,  1,  3,  11, 6,  3,  X,  X,  X,  X,  8,  11, 1,  8,
        1,  0,  11, 6,  1,  9,  1,  4,  6,  4,  1,  X,  3,  11, 6,  3,  6,  0,  0,  6,  4,  X,  X,
        X,  X,  X,  X,  X,  6,  4,  8,  11, 6,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  7,  10,
        6,  7,  8,  10, 8,  9,  10, X,  X,  X,  X,  X,  X,  X,  0,  7,  3,  0,  10, 7,  0,  9,  10,
        6,  7,  10, X,  X,  X,  X,  10, 6,  7,  1,  10, 7,  1,  7,  8,  1,  8,  0,  X,  X,  X,  X,
        10, 6,  7,  10, 7,  1,  1,  7,  3,  X,  X,  X,  X,  X,  X,  X,  1,  2,  6,  1,  6,  8,  1,
        8,  9,  8,  6,  7,  X,  X,  X,  X,  2,  6,  9,  2,  9,  1,  6,  7,  9,  0,  9,  3,  7,  3,
        9,  X,  7,  8,  0,  7,  0,  6,  6,  0,  2,  X,  X,  X,  X,  X,  X,  X,  7,  3,  2,  6,  7,
        2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  2,  3,  11, 10, 6,  8,  10, 8,  9,  8,  6,  7,
        X,  X,  X,  X,  2,  0,  7,  2,  7,  11, 0,  9,  7,  6,  7,  10, 9,  10, 7,  X,  1,  8,  0,
        1,  7,  8,  1,  10, 7,  6,  7,  10, 2,  3,  11, X,  11, 2,  1,  11, 1,  7,  10, 6,  1,  6,
        7,  1,  X,  X,  X,  X,  8,  9,  6,  8,  6,  7,  9,  1,  6,  11, 6,  3,  1,  3,  6,  X,  0,
        9,  1,  11, 6,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  7,  8,  0,  7,  0,  6,  3,  11,
        0,  11, 6,  0,  X,  X,  X,  X,  7,  11, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  7,  6,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  0,  8,  11, 7,  6,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  9,  11, 7,  6,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  8,  1,  9,  8,  3,  1,  11, 7,  6,  X,  X,  X,  X,  X,  X,  X,  10, 1,  2,  6,
        11, 7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  2,  10, 3,  0,  8,  6,  11, 7,  X,  X,
        X,  X,  X,  X,  X,  2,  9,  0,  2,  10, 9,  6,  11, 7,  X,  X,  X,  X,  X,  X,  X,  6,  11,
        7,  2,  10, 3,  10, 8,  3,  10, 9,  8,  X,  X,  X,  X,  7,  2,  3,  6,  2,  7,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  7,  0,  8,  7,  6,  0,  6,  2,  0,  X,  X,  X,  X,  X,  X,  X,
        2,  7,  6,  2,  3,  7,  0,  1,  9,  X,  X,  X,  X,  X,  X,  X,  1,  6,  2,  1,  8,  6,  1,
        9,  8,  8,  7,  6,  X,  X,  X,  X,  10, 7,  6,  10, 1,  7,  1,  3,  7,  X,  X,  X,  X,  X,
        X,  X,  10, 7,  6,  1,  7,  10, 1,  8,  7,  1,  0,  8,  X,  X,  X,  X,  0,  3,  7,  0,  7,
        10, 0,  10, 9,  6,  10, 7,  X,  X,  X,  X,  7,  6,  10, 7,  10, 8,  8,  10, 9,  X,  X,  X,
        X,  X,  X,  X,  6,  8,  4,  11, 8,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  6,  11,
        3,  0,  6,  0,  4,  6,  X,  X,  X,  X,  X,  X,  X,  8,  6,  11, 8,  4,  6,  9,  0,  1,  X,
        X,  X,  X,  X,  X,  X,  9,  4,  6,  9,  6,  3,  9,  3,  1,  11, 3,  6,  X,  X,  X,  X,  6,
        8,  4,  6,  11, 8,  2,  10, 1,  X,  X,  X,  X,  X,  X,  X,  1,  2,  10, 3,  0,  11, 0,  6,
        11, 0,  4,  6,  X,  X,  X,  X,  4,  11, 8,  4,  6,  11, 0,  2,  9,  2,  10, 9,  X,  X,  X,
        X,  10, 9,  3,  10, 3,  2,  9,  4,  3,  11, 3,  6,  4,  6,  3,  X,  8,  2,  3,  8,  4,  2,
        4,  6,  2,  X,  X,  X,  X,  X,  X,  X,  0,  4,  2,  4,  6,  2,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  1,  9,  0,  2,  3,  4,  2,  4,  6,  4,  3,  8,  X,  X,  X,  X,  1,  9,  4,  1,
        4,  2,  2,  4,  6,  X,  X,  X,  X,  X,  X,  X,  8,  1,  3,  8,  6,  1,  8,  4,  6,  6,  10,
        1,  X,  X,  X,  X,  10, 1,  0,  10, 0,  6,  6,  0,  4,  X,  X,  X,  X,  X,  X,  X,  4,  6,
        3,  4,  3,  8,  6,  10, 3,  0,  3,  9,  10, 9,  3,  X,  10, 9,  4,  6,  10, 4,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  4,  9,  5,  7,  6,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        0,  8,  3,  4,  9,  5,  11, 7,  6,  X,  X,  X,  X,  X,  X,  X,  5,  0,  1,  5,  4,  0,  7,
        6,  11, X,  X,  X,  X,  X,  X,  X,  11, 7,  6,  8,  3,  4,  3,  5,  4,  3,  1,  5,  X,  X,
        X,  X,  9,  5,  4,  10, 1,  2,  7,  6,  11, X,  X,  X,  X,  X,  X,  X,  6,  11, 7,  1,  2,
        10, 0,  8,  3,  4,  9,  5,  X,  X,  X,  X,  7,  6,  11, 5,  4,  10, 4,  2,  10, 4,  0,  2,
        X,  X,  X,  X,  3,  4,  8,  3,  5,  4,  3,  2,  5,  10, 5,  2,  11, 7,  6,  X,  7,  2,  3,
        7,  6,  2,  5,  4,  9,  X,  X,  X,  X,  X,  X,  X,  9,  5,  4,  0,  8,  6,  0,  6,  2,  6,
        8,  7,  X,  X,  X,  X,  3,  6,  2,  3,  7,  6,  1,  5,  0,  5,  4,  0,  X,  X,  X,  X,  6,
        2,  8,  6,  8,  7,  2,  1,  8,  4,  8,  5,  1,  5,  8,  X,  9,  5,  4,  10, 1,  6,  1,  7,
        6,  1,  3,  7,  X,  X,  X,  X,  1,  6,  10, 1,  7,  6,  1,  0,  7,  8,  7,  0,  9,  5,  4,
        X,  4,  0,  10, 4,  10, 5,  0,  3,  10, 6,  10, 7,  3,  7,  10, X,  7,  6,  10, 7,  10, 8,
        5,  4,  10, 4,  8,  10, X,  X,  X,  X,  6,  9,  5,  6,  11, 9,  11, 8,  9,  X,  X,  X,  X,
        X,  X,  X,  3,  6,  11, 0,  6,  3,  0,  5,  6,  0,  9,  5,  X,  X,  X,  X,  0,  11, 8,  0,
        5,  11, 0,  1,  5,  5,  6,  11, X,  X,  X,  X,  6,  11, 3,  6,  3,  5,  5,  3,  1,  X,  X,
        X,  X,  X,  X,  X,  1,  2,  10, 9,  5,  11, 9,  11, 8,  11, 5,  6,  X,  X,  X,  X,  0,  11,
        3,  0,  6,  11, 0,  9,  6,  5,  6,  9,  1,  2,  10, X,  11, 8,  5,  11, 5,  6,  8,  0,  5,
        10, 5,  2,  0,  2,  5,  X,  6,  11, 3,  6,  3,  5,  2,  10, 3,  10, 5,  3,  X,  X,  X,  X,
        5,  8,  9,  5,  2,  8,  5,  6,  2,  3,  8,  2,  X,  X,  X,  X,  9,  5,  6,  9,  6,  0,  0,
        6,  2,  X,  X,  X,  X,  X,  X,  X,  1,  5,  8,  1,  8,  0,  5,  6,  8,  3,  8,  2,  6,  2,
        8,  X,  1,  5,  6,  2,  1,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  3,  6,  1,  6,
        10, 3,  8,  6,  5,  6,  9,  8,  9,  6,  X,  10, 1,  0,  10, 0,  6,  9,  5,  0,  5,  6,  0,
        X,  X,  X,  X,  0,  3,  8,  5,  6,  10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  10, 5,  6,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  11, 5,  10, 7,  5,  11, X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  11, 5,  10, 11, 7,  5,  8,  3,  0,  X,  X,  X,  X,  X,  X,  X,  5,
        11, 7,  5,  10, 11, 1,  9,  0,  X,  X,  X,  X,  X,  X,  X,  10, 7,  5,  10, 11, 7,  9,  8,
        1,  8,  3,  1,  X,  X,  X,  X,  11, 1,  2,  11, 7,  1,  7,  5,  1,  X,  X,  X,  X,  X,  X,
        X,  0,  8,  3,  1,  2,  7,  1,  7,  5,  7,  2,  11, X,  X,  X,  X,  9,  7,  5,  9,  2,  7,
        9,  0,  2,  2,  11, 7,  X,  X,  X,  X,  7,  5,  2,  7,  2,  11, 5,  9,  2,  3,  2,  8,  9,
        8,  2,  X,  2,  5,  10, 2,  3,  5,  3,  7,  5,  X,  X,  X,  X,  X,  X,  X,  8,  2,  0,  8,
        5,  2,  8,  7,  5,  10, 2,  5,  X,  X,  X,  X,  9,  0,  1,  5,  10, 3,  5,  3,  7,  3,  10,
        2,  X,  X,  X,  X,  9,  8,  2,  9,  2,  1,  8,  7,  2,  10, 2,  5,  7,  5,  2,  X,  1,  3,
        5,  3,  7,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  8,  7,  0,  7,  1,  1,  7,  5,
        X,  X,  X,  X,  X,  X,  X,  9,  0,  3,  9,  3,  5,  5,  3,  7,  X,  X,  X,  X,  X,  X,  X,
        9,  8,  7,  5,  9,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  5,  8,  4,  5,  10, 8,  10,
        11, 8,  X,  X,  X,  X,  X,  X,  X,  5,  0,  4,  5,  11, 0,  5,  10, 11, 11, 3,  0,  X,  X,
        X,  X,  0,  1,  9,  8,  4,  10, 8,  10, 11, 10, 4,  5,  X,  X,  X,  X,  10, 11, 4,  10, 4,
        5,  11, 3,  4,  9,  4,  1,  3,  1,  4,  X,  2,  5,  1,  2,  8,  5,  2,  11, 8,  4,  5,  8,
        X,  X,  X,  X,  0,  4,  11, 0,  11, 3,  4,  5,  11, 2,  11, 1,  5,  1,  11, X,  0,  2,  5,
        0,  5,  9,  2,  11, 5,  4,  5,  8,  11, 8,  5,  X,  9,  4,  5,  2,  11, 3,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  2,  5,  10, 3,  5,  2,  3,  4,  5,  3,  8,  4,  X,  X,  X,  X,  5,
        10, 2,  5,  2,  4,  4,  2,  0,  X,  X,  X,  X,  X,  X,  X,  3,  10, 2,  3,  5,  10, 3,  8,
        5,  4,  5,  8,  0,  1,  9,  X,  5,  10, 2,  5,  2,  4,  1,  9,  2,  9,  4,  2,  X,  X,  X,
        X,  8,  4,  5,  8,  5,  3,  3,  5,  1,  X,  X,  X,  X,  X,  X,  X,  0,  4,  5,  1,  0,  5,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  8,  4,  5,  8,  5,  3,  9,  0,  5,  0,  3,  5,  X,
        X,  X,  X,  9,  4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  4,  11, 7,  4,
        9,  11, 9,  10, 11, X,  X,  X,  X,  X,  X,  X,  0,  8,  3,  4,  9,  7,  9,  11, 7,  9,  10,
        11, X,  X,  X,  X,  1,  10, 11, 1,  11, 4,  1,  4,  0,  7,  4,  11, X,  X,  X,  X,  3,  1,
        4,  3,  4,  8,  1,  10, 4,  7,  4,  11, 10, 11, 4,  X,  4,  11, 7,  9,  11, 4,  9,  2,  11,
        9,  1,  2,  X,  X,  X,  X,  9,  7,  4,  9,  11, 7,  9,  1,  11, 2,  11, 1,  0,  8,  3,  X,
        11, 7,  4,  11, 4,  2,  2,  4,  0,  X,  X,  X,  X,  X,  X,  X,  11, 7,  4,  11, 4,  2,  8,
        3,  4,  3,  2,  4,  X,  X,  X,  X,  2,  9,  10, 2,  7,  9,  2,  3,  7,  7,  4,  9,  X,  X,
        X,  X,  9,  10, 7,  9,  7,  4,  10, 2,  7,  8,  7,  0,  2,  0,  7,  X,  3,  7,  10, 3,  10,
        2,  7,  4,  10, 1,  10, 0,  4,  0,  10, X,  1,  10, 2,  8,  7,  4,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  4,  9,  1,  4,  1,  7,  7,  1,  3,  X,  X,  X,  X,  X,  X,  X,  4,  9,  1,
        4,  1,  7,  0,  8,  1,  8,  7,  1,  X,  X,  X,  X,  4,  0,  3,  7,  4,  3,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  4,  8,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  9,
        10, 8,  10, 11, 8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  0,  9,  3,  9,  11, 11, 9,
        10, X,  X,  X,  X,  X,  X,  X,  0,  1,  10, 0,  10, 8,  8,  10, 11, X,  X,  X,  X,  X,  X,
        X,  3,  1,  10, 11, 3,  10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  2,  11, 1,  11, 9,
        9,  11, 8,  X,  X,  X,  X,  X,  X,  X,  3,  0,  9,  3,  9,  11, 1,  2,  9,  2,  11, 9,  X,
        X,  X,  X,  0,  2,  11, 8,  0,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  2,  11, X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  2,  3,  8,  2,  8,  10, 10, 8,  9,  X,  X,
        X,  X,  X,  X,  X,  9,  10, 2,  0,  9,  2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  2,  3,
        8,  2,  8,  10, 0,  1,  8,  1,  10, 8,  X,  X,  X,  X,  1,  10, 2,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  1,  3,  8,  9,  1,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        0,  9,  1,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  3,  8,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X
#undef X
      };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id
      triTableLT3d[nCasesLT3d * nTriTableLT3dElemSize] = {
#define X -1
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 3,  12, 18,
        3,  16, 18, 8,  14, 18, 8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  0,  1,  17, 0,  9,  17, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12,
        1,  17, 14, 18, 17, 14, 9,  17, 3,  12, 18, 3,  16, 18, 8,  14, 18, 8,  16, 18, X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 12, 2,  10, X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 3,  18, 10, 3,  2,  10, 3,  16, 18, 8,  14, 18, 8,
        16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,
        17, 12, 2,  10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 3,  18, 10, 3,  2,  10, 3,  16,
        18, 8,  14, 18, 8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  2,  15,
        3,  11, 15, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 12, 18, 15, 12, 2,  15,
        16, 18, 15, 16, 11, 15, 8,  14, 18, 8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  0,  1,  17, 0,  9,  17, 3,  2,  15, 3,  11, 15, X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14,
        18, 17, 14, 9,  17, 12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 8,  14, 18, 8,  16, 18,
        X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 3,  12, 10, 3,  15, 10, 3,  11, 15, X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18,
        10, 0,  1,  10, 0,  14, 18, 18, 15, 10, 16, 18, 15, 16, 11, 15, 8,  14, 18, 8,  16, 18, X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 3,  12,
        10, 3,  15, 10, 3,  11, 15, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 18, 15, 10, 16, 18, 15, 16, 11, 15, 8,  14,
        18, 8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  8,  4,  13, 8,  7,  13,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 3,  12, 18, 3,  16, 18, 14, 18, 13,
        14, 4,  13, 16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,
        1,  17, 0,  9,  17, 8,  4,  13, 8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17, 14,
        9,  17, 3,  12, 18, 3,  16, 18, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,  X,  X,
        X,  X,  X,  X,  12, 1,  10, 12, 2,  10, 8,  4,  13, 8,  7,  13, X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,
        10, 0,  14, 18, 3,  18, 10, 3,  2,  10, 3,  16, 18, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16,
        7,  13, X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 12, 2,  10, 8,  4,
        13, 8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        18, 17, 10, 14, 18, 17, 14, 9,  17, 3,  18, 10, 3,  2,  10, 3,  16, 18, 14, 18, 13, 14, 4,
        13, 16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  3,  2,  15, 3,  11, 15, 8,  4,  13,
        8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15,
        14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,
        9,  17, 3,  2,  15, 3,  11, 15, 8,  4,  13, 8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17, 14, 9,  17, 12,
        18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13,
        X,  12, 1,  10, 3,  12, 10, 3,  15, 10, 3,  11, 15, 8,  4,  13, 8,  7,  13, X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14,
        18, 18, 15, 10, 16, 18, 15, 16, 11, 15, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,
        X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 3,  12, 10, 3,  15, 10, 3,  11,
        15, 8,  4,  13, 8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10,
        14, 18, 17, 14, 9,  17, 18, 15, 10, 16, 18, 15, 16, 11, 15, 14, 18, 13, 14, 4,  13, 16, 18,
        13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  14, 9,  5,  14, 4,  5,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,  3,  12, 18, 3,  16, 18, 8,  18, 5,  8,  4,  5,
        8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,
        17, 5,  14, 4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  3,  12, 18, 3,  16, 18, 8,
        18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 1,
        10, 14, 9,  5,  12, 2,  10, 14, 4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,
        5,  3,  18, 10, 3,  2,  10, 3,  16, 18, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,
        X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  12, 2,  10, 14, 4,  5,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,
        3,  18, 10, 3,  2,  10, 3,  16, 18, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  14, 9,  5,  3,  2,  15, 3,  11, 15, 14, 4,  5,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,
        12, 18, 0,  18, 5,  0,  9,  5,  12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 8,  18, 5,
        8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,  3,
        2,  15, 3,  11, 15, 14, 4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  12, 18, 15, 12, 2,  15, 16, 18, 15, 16,
        11, 15, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,
        5,  3,  12, 10, 3,  15, 10, 3,  11, 15, 14, 4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  18, 15,
        10, 16, 18, 15, 16, 11, 15, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,
        0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  3,  12, 10, 3,  15, 10, 3,  11, 15, 14, 4,
        5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  18, 15, 10,
        16, 18, 15, 16, 11, 15, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  14, 9,  5,  8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,
        18, 5,  0,  9,  5,  3,  12, 18, 3,  16, 18, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,  8,  14, 5,  8,
        13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  3,  12, 18, 3,  16, 18, 18, 13, 5,  16, 18, 13, 16,
        7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  12, 2,
        10, 8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  3,  18, 10, 3,  2,
        10, 3,  16, 18, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  0,  12, 10,
        0,  17, 10, 0,  14, 5,  0,  17, 5,  12, 2,  10, 8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  3,  18, 10, 3,  2,  10,
        3,  16, 18, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  14, 9,  5,  3,  2,  15, 3,  11, 15, 8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,
        9,  5,  12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 18, 13, 5,  16, 18, 13, 16, 7,  13,
        X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,  3,  2,  15, 3,  11, 15, 8,
        14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18,
        17, 12, 1,  17, 18, 17, 5,  12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 18, 13, 5,  16,
        18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  3,  12, 10, 3,  15,
        10, 3,  11, 15, 8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  18, 15, 10, 16, 18, 15, 16, 11,
        15, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10,
        0,  14, 5,  0,  17, 5,  3,  12, 10, 3,  15, 10, 3,  11, 15, 8,  14, 5,  8,  13, 5,  8,  7,
        13, X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  18, 15, 10, 16, 18, 15, 16, 11, 15,
        18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18,
        17, 10, 18, 17, 5,  18, 15, 10, 18, 15, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,
        17, 5,  3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,
        8,  13, 6,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  18, 15, 10, 18, 15, 6,  18,
        13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,
        5,  3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,
        13, 6,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  12, 18, 15, 12, 2,
        15, 18, 15, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        0,  1,  17, 0,  14, 5,  0,  17, 5,  3,  2,  15, 3,  16, 6,  3,  15, 6,  8,  14, 5,  8,  13,
        5,  8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,
        12, 18, 15, 12, 2,  15, 18, 15, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  14, 9,  5,  3,  2,  15, 3,  16, 6,  3,  15, 6,  8,  14, 5,  8,  13, 5,
        8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18,
        17, 5,  3,  18, 10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  12,
        2,  10, 16, 11, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,
        X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  3,  18, 10, 3,  2,  10, 3,  18, 6,  3,
        11, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  12, 2,
        10, 16, 11, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  3,  12, 18, 3,  18, 6,  3,  11,
        6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17,
        0,  14, 5,  0,  17, 5,  16, 11, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,  3,  12, 18,
        3,  18, 6,  3,  11, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  14, 9,  5,  16, 11, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  18,
        15, 10, 18, 15, 6,  8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  3,  12, 10, 3,
        15, 10, 3,  16, 6,  3,  15, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  0,  18,
        10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  18, 15, 10, 18, 15, 6,  8,  18, 5,  8,  4,  5,  8,
        18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  3,  12, 10, 3,  15,
        10, 3,  16, 6,  3,  15, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  12, 18, 15, 12, 2,  15, 18, 15, 6,  8,  18,
        5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,
        0,  17, 5,  3,  2,  15, 3,  16, 6,  3,  15, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,  12, 18, 15, 12, 2,  15,
        18, 15, 6,  8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  14,
        9,  5,  3,  2,  15, 3,  16, 6,  3,  15, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  3,  18, 10, 3,
        2,  10, 3,  18, 6,  3,  11, 6,  8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,
        X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  12, 2,  10, 16, 11, 6,  14,
        4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,
        10, 0,  18, 5,  0,  9,  5,  3,  18, 10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  8,  18, 5,  8,
        4,  5,  8,  18, 6,  8,  7,  6,  X,  12, 1,  10, 14, 9,  5,  12, 2,  10, 16, 11, 6,  14, 4,
        5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        12, 18, 17, 12, 1,  17, 18, 17, 5,  3,  12, 18, 3,  18, 6,  3,  11, 6,  8,  18, 5,  8,  4,
        5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,
        16, 11, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,  3,  12, 18, 3,  18, 6,  3,  11, 6,
        8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  14, 9,  5,  16,
        11, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 18, 15, 10, 18,
        15, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  8,
        4,  13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14,
        18, 18, 15, 10, 18, 15, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  12, 1,  10, 3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  8,  4,
        13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17,
        12, 1,  17, 14, 18, 17, 14, 9,  17, 12, 18, 15, 12, 2,  15, 18, 15, 6,  14, 18, 13, 14, 4,
        13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  9,  17, 3,  2,  15, 3,  16, 6,
        3,  15, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  0,  12, 18, 0,  14, 18, 12, 18, 15, 12, 2,  15, 18, 15, 6,  14, 18, 13, 14, 4,  13,
        18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  2,  15, 3,  16, 6,  3,
        15, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 3,  18, 10, 3,  2,  10, 3,
        18, 6,  3,  11, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  0,  12,
        10, 0,  17, 10, 0,  9,  17, 12, 2,  10, 16, 11, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 3,  18,
        10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,
        X,  X,  X,  12, 1,  10, 12, 2,  10, 16, 11, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17,
        14, 18, 17, 14, 9,  17, 3,  12, 18, 3,  18, 6,  3,  11, 6,  14, 18, 13, 14, 4,  13, 18, 13,
        6,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  9,  17, 16, 11, 6,  8,  4,  13, 8,  16, 6,
        8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,
        12, 18, 0,  14, 18, 3,  12, 18, 3,  18, 6,  3,  11, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  16, 11, 6,  8,  4,  13, 8,  16, 6,  8,
        13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 18, 15, 10, 18, 15, 6,  8,  14, 18, 8,
        18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17,
        10, 0,  9,  17, 3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  16, 7,  6,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 18, 15, 10, 18, 15,
        6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        12, 1,  10, 3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  16, 7,  6,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17,
        14, 9,  17, 12, 18, 15, 12, 2,  15, 18, 15, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,
        X,  X,  X,  X,  X,  0,  1,  17, 0,  9,  17, 3,  2,  15, 3,  16, 6,  3,  15, 6,  16, 7,  6,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,
        14, 18, 12, 18, 15, 12, 2,  15, 18, 15, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  2,  15, 3,  16, 6,  3,  15, 6,  16, 7,  6,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 3,  18, 10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  8,
        14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,
        17, 12, 2,  10, 16, 11, 6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 3,  18, 10, 3,  2,  10, 3,  18,
        6,  3,  11, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10,
        12, 2,  10, 16, 11, 6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17, 14, 9,  17,
        3,  12, 18, 3,  18, 6,  3,  11, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,
        X,  X,  0,  1,  17, 0,  9,  17, 16, 11, 6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 3,
        12, 18, 3,  18, 6,  3,  11, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  16, 11, 6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  16, 11,
        6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 3,  12, 18, 3,  18,
        6,  3,  11, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  0,  1,  17, 0,  9,  17, 16, 11, 6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17,
        14, 18, 17, 14, 9,  17, 3,  12, 18, 3,  18, 6,  3,  11, 6,  8,  14, 18, 8,  18, 6,  8,  7,
        6,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 12, 2,  10, 16, 11, 6,  16, 7,  6,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,
        18, 10, 0,  1,  10, 0,  14, 18, 3,  18, 10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  8,  14, 18,
        8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 12,
        2,  10, 16, 11, 6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 3,  18, 10, 3,  2,  10, 3,  18, 6,  3,
        11, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  3,  2,  15, 3,  16,
        6,  3,  15, 6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 12, 18, 15, 12, 2,  15, 18, 15,
        6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        0,  1,  17, 0,  9,  17, 3,  2,  15, 3,  16, 6,  3,  15, 6,  16, 7,  6,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17,
        14, 9,  17, 12, 18, 15, 12, 2,  15, 18, 15, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,
        X,  X,  X,  X,  X,  12, 1,  10, 3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  16, 7,  6,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,
        1,  10, 0,  14, 18, 18, 15, 10, 18, 15, 6,  8,  14, 18, 8,  18, 6,  8,  7,  6,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 3,  12, 10, 3,
        15, 10, 3,  16, 6,  3,  15, 6,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 18, 15, 10, 18, 15, 6,  8,  14, 18, 8,  18, 6,  8,
        7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  16, 11, 6,  8,  4,  13, 8,  16,
        6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 3,  12, 18, 3,  18, 6,  3,  11, 6,  14, 18,
        13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17,
        0,  9,  17, 16, 11, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17, 14, 9,  17,
        3,  12, 18, 3,  18, 6,  3,  11, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,  X,
        X,  X,  12, 1,  10, 12, 2,  10, 16, 11, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,
        14, 18, 3,  18, 10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,
        X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 12, 2,  10, 16, 11, 6,  8,
        4,  13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17,
        10, 14, 18, 17, 14, 9,  17, 3,  18, 10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  14, 18, 13, 14,
        4,  13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  3,  2,  15, 3,  16, 6,  3,  15, 6,  8,  4,
        13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  0,  12, 18, 0,  14, 18, 12, 18, 15, 12, 2,  15, 18, 15, 6,  14, 18, 13, 14, 4,
        13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  9,  17,
        3,  2,  15, 3,  16, 6,  3,  15, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17, 14, 9,  17, 12, 18, 15,
        12, 2,  15, 18, 15, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  12,
        1,  10, 3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 18,
        15, 10, 18, 15, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 3,  12, 10, 3,  15, 10, 3,  16, 6,  3,
        15, 6,  8,  4,  13, 8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 14, 18,
        17, 14, 9,  17, 18, 15, 10, 18, 15, 6,  14, 18, 13, 14, 4,  13, 18, 13, 6,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  14, 9,  5,  16, 11, 6,  14, 4,  5,  16, 7,  6,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        0,  12, 18, 0,  18, 5,  0,  9,  5,  3,  12, 18, 3,  18, 6,  3,  11, 6,  8,  18, 5,  8,  4,
        5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,
        16, 11, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  3,  12, 18, 3,  18, 6,  3,  11, 6,
        8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14,
        9,  5,  12, 2,  10, 16, 11, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  3,
        18, 10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,
        X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  12, 2,  10, 16, 11, 6,  14, 4,  5,  16,
        7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  3,  18,
        10, 3,  2,  10, 3,  18, 6,  3,  11, 6,  8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,
        X,  X,  X,  X,  X,  X,  14, 9,  5,  3,  2,  15, 3,  16, 6,  3,  15, 6,  14, 4,  5,  16, 7,
        6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18,
        0,  18, 5,  0,  9,  5,  12, 18, 15, 12, 2,  15, 18, 15, 6,  8,  18, 5,  8,  4,  5,  8,  18,
        6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,  3,  2,  15,
        3,  16, 6,  3,  15, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  12, 18, 15, 12, 2,  15, 18, 15, 6,  8,  18, 5,
        8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  3,
        12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  14, 4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  18, 15, 10, 18,
        15, 6,  8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  0,  12,
        10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  14,
        4,  5,  16, 7,  6,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  18, 15, 10, 18, 15,
        6,  8,  18, 5,  8,  4,  5,  8,  18, 6,  8,  7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  14, 9,  5,  16, 11, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,
        0,  9,  5,  3,  12, 18, 3,  18, 6,  3,  11, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,  16, 11, 6,  8,  14, 5,
        8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12,
        18, 17, 12, 1,  17, 18, 17, 5,  3,  12, 18, 3,  18, 6,  3,  11, 6,  18, 13, 5,  18, 13, 6,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  12, 2,  10, 16,
        11, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  3,  18, 10, 3,  2,  10, 3,
        18, 6,  3,  11, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17,
        10, 0,  14, 5,  0,  17, 5,  12, 2,  10, 16, 11, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,
        13, 6,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  3,  18, 10, 3,  2,  10, 3,  18,
        6,  3,  11, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        14, 9,  5,  3,  2,  15, 3,  16, 6,  3,  15, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13,
        6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,
        12, 18, 15, 12, 2,  15, 18, 15, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,  3,  2,  15, 3,  16, 6,  3,  15, 6,
        8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12,
        1,  17, 18, 17, 5,  12, 18, 15, 12, 2,  15, 18, 15, 6,  18, 13, 5,  18, 13, 6,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  3,  12, 10, 3,  15, 10, 3,
        16, 6,  3,  15, 6,  8,  14, 5,  8,  13, 5,  8,  16, 6,  8,  13, 6,  X,  X,  X,  X,  X,  X,
        X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  18, 15, 10, 18, 15, 6,  18, 13, 5,  18,
        13, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14,
        5,  0,  17, 5,  3,  12, 10, 3,  15, 10, 3,  16, 6,  3,  15, 6,  8,  14, 5,  8,  13, 5,  8,
        16, 6,  8,  13, 6,  X,  18, 17, 10, 18, 17, 5,  18, 15, 10, 18, 15, 6,  18, 13, 5,  18, 13,
        6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10,
        18, 17, 5,  18, 15, 10, 16, 18, 15, 16, 11, 15, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,
        3,  12, 10, 3,  15, 10, 3,  11, 15, 8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,
        X,  X,  0,  18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  18, 15, 10, 16, 18, 15, 16, 11, 15,
        18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  3,
        12, 10, 3,  15, 10, 3,  11, 15, 8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  12, 18, 15, 12, 2,  15, 16,
        18, 15, 16, 11, 15, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  0,  1,
        17, 0,  14, 5,  0,  17, 5,  3,  2,  15, 3,  11, 15, 8,  14, 5,  8,  13, 5,  8,  7,  13, X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,  12, 18,
        15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,
        X,  X,  X,  14, 9,  5,  3,  2,  15, 3,  11, 15, 8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,
        3,  18, 10, 3,  2,  10, 3,  16, 18, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  12, 2,  10,
        8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,
        18, 10, 0,  1,  10, 0,  18, 5,  0,  9,  5,  3,  18, 10, 3,  2,  10, 3,  16, 18, 18, 13, 5,
        16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  12, 2,  10, 8,
        14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  3,  12, 18, 3,  16, 18, 18, 13, 5,  16,
        18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14,
        5,  0,  17, 5,  8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,  3,  12, 18, 3,  16,
        18, 18, 13, 5,  16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        14, 9,  5,  8,  14, 5,  8,  13, 5,  8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  18, 15, 10,
        16, 18, 15, 16, 11, 15, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  3,  12, 10, 3,  15, 10,
        3,  11, 15, 14, 4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,
        1,  10, 0,  18, 5,  0,  9,  5,  18, 15, 10, 16, 18, 15, 16, 11, 15, 8,  18, 5,  8,  4,  5,
        8,  16, 18, X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  3,  12, 10, 3,  15, 10, 3,
        11, 15, 14, 4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  12, 18, 17, 12, 1,  17, 18, 17, 5,  12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 8,
        18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17,
        5,  3,  2,  15, 3,  11, 15, 14, 4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,  12, 18, 15, 12, 2,  15, 16, 18,
        15, 16, 11, 15, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  14, 9,  5,
        3,  2,  15, 3,  11, 15, 14, 4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 18, 17, 5,  3,  18, 10, 3,  2,  10,
        3,  16, 18, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  0,  12, 10, 0,  17, 10, 0,  14, 5,  0,  17, 5,  12, 2,  10, 14, 4,  5,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,
        18, 5,  0,  9,  5,  3,  18, 10, 3,  2,  10, 3,  16, 18, 8,  18, 5,  8,  4,  5,  8,  16, 18,
        X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 14, 9,  5,  12, 2,  10, 14, 4,  5,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18,
        17, 12, 1,  17, 18, 17, 5,  3,  12, 18, 3,  16, 18, 8,  18, 5,  8,  4,  5,  8,  16, 18, X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  14, 5,  0,  17, 5,  14, 4,
        5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  0,  12, 18, 0,  18, 5,  0,  9,  5,  3,  12, 18, 3,  16, 18, 8,  18, 5,  8,  4,
        5,  8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  14, 9,  5,  14, 4,  5,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 18, 15, 10, 16, 18, 15,
        16, 11, 15, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  0,
        12, 10, 0,  17, 10, 0,  9,  17, 3,  12, 10, 3,  15, 10, 3,  11, 15, 8,  4,  13, 8,  7,  13,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 18,
        15, 10, 16, 18, 15, 16, 11, 15, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,  X,  X,
        X,  X,  X,  X,  12, 1,  10, 3,  12, 10, 3,  15, 10, 3,  11, 15, 8,  4,  13, 8,  7,  13, X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,
        17, 14, 18, 17, 14, 9,  17, 12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 14, 18, 13, 14,
        4,  13, 16, 18, 13, 16, 7,  13, X,  0,  1,  17, 0,  9,  17, 3,  2,  15, 3,  11, 15, 8,  4,
        13, 8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        0,  12, 18, 0,  14, 18, 12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 14, 18, 13, 14, 4,
        13, 16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  3,  2,  15, 3,  11, 15, 8,  4,  13,
        8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 3,  18, 10, 3,  2,  10, 3,  16, 18,
        14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,
        17, 10, 0,  9,  17, 12, 2,  10, 8,  4,  13, 8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 3,  18, 10, 3,
        2,  10, 3,  16, 18, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,  X,  X,  X,  X,  X,
        X,  12, 1,  10, 12, 2,  10, 8,  4,  13, 8,  7,  13, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18,
        17, 14, 9,  17, 3,  12, 18, 3,  16, 18, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,
        X,  X,  X,  X,  X,  X,  0,  1,  17, 0,  9,  17, 8,  4,  13, 8,  7,  13, X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18,
        0,  14, 18, 3,  12, 18, 3,  16, 18, 14, 18, 13, 14, 4,  13, 16, 18, 13, 16, 7,  13, X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  8,  4,  13, 8,  7,  13, X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  18, 17, 10, 14, 18, 17, 14, 9,  17, 18, 15, 10, 16, 18, 15, 16, 11, 15, 8,  14, 18,
        8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,
        9,  17, 3,  12, 10, 3,  15, 10, 3,  11, 15, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 18, 15, 10, 16, 18, 15, 16,
        11, 15, 8,  14, 18, 8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 1,
        10, 3,  12, 10, 3,  15, 10, 3,  11, 15, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17, 14, 9,
        17, 12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 8,  14, 18, 8,  16, 18, X,  X,  X,  X,
        X,  X,  X,  0,  1,  17, 0,  9,  17, 3,  2,  15, 3,  11, 15, X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18,
        12, 18, 15, 12, 2,  15, 16, 18, 15, 16, 11, 15, 8,  14, 18, 8,  16, 18, X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  3,  2,  15, 3,  11, 15, X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  18,
        17, 10, 14, 18, 17, 14, 9,  17, 3,  18, 10, 3,  2,  10, 3,  16, 18, 8,  14, 18, 8,  16, 18,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 10, 0,  17, 10, 0,  9,  17, 12,
        2,  10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  0,  18, 10, 0,  1,  10, 0,  14, 18, 3,  18, 10, 3,  2,  10, 3,  16, 18, 8,
        14, 18, 8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 1,  10, 12, 2,
        10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  12, 18, 17, 12, 1,  17, 14, 18, 17, 14, 9,  17, 3,  12,
        18, 3,  16, 18, 8,  14, 18, 8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        0,  1,  17, 0,  9,  17, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  12, 18, 0,  14, 18, 3,  12, 18,
        3,  16, 18, 8,  14, 18, 8,  16, 18, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
        X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X
#undef X
      };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id
      labelEdgeTableMC3d[nCasesMC3d * nLabelEdgeTableMC3dElemSize] = {
#define X -1
        X, X, X, X, X, X, X, X, X, 1, 0, X, X, X, X, X, X, X, 1, 0, X, X, X, X, X, X, X, 2, 1, X, X,
        X, X, X, X, X, 1, 1, X, X, X, X, X, X, X, 1, 0, 1, 1, X, X, X, X, X, 2, 0, X, X, X, X, X, X,
        X, 3, 2, X, X, X, X, X, X, X, 1, 2, X, X, X, X, X, X, X, 2, 0, X, X, X, X, X, X, X, 1, 0, 1,
        2, X, X, X, X, X, 3, 1, X, X, X, X, X, X, X, 2, 1, X, X, X, X, X, X, X, 3, 0, X, X, X, X, X,
        X, X, 3, 0, X, X, X, X, X, X, X, 2, 8, X, X, X, X, X, X, X, 1, 4, X, X, X, X, X, X, X, 2, 0,
        X, X, X, X, X, X, X, 1, 0, 1, 4, X, X, X, X, X, 3, 1, X, X, X, X, X, X, X, 1, 1, 1, 4, X, X,
        X, X, X, 2, 0, 1, 1, X, X, X, X, X, 2, 0, 1, 4, X, X, X, X, X, 4, 2, X, X, X, X, X, X, X, 1,
        4, 1, 2, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 1, 0, 1, 4, 1, 2, X, X, X, 4, 1, X, X, X,
        X, X, X, X, 2, 1, 1, 4, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 1, 4, 3, 0, X, X, X, X, X,
        3, 4, X, X, X, X, X, X, X, 1, 4, X, X, X, X, X, X, X, 1, 4, 1, 0, X, X, X, X, X, 2, 0, X, X,
        X, X, X, X, X, 3, 1, X, X, X, X, X, X, X, 1, 1, 1, 4, X, X, X, X, X, 1, 0, 1, 1, 1, 4, X, X,
        X, 3, 0, X, X, X, X, X, X, X, 4, 2, X, X, X, X, X, X, X, 1, 4, 1, 2, X, X, X, X, X, 2, 0, 1,
        4, X, X, X, X, X, 2, 0, 1, 2, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 2, 1, 1, 4, X, X, X,
        X, X, 1, 4, 3, 0, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 3, 4, X, X, X, X, X, X, X, 2, 5,
        X, X, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 2, 1, X, X, X, X,
        X, X, X, 2, 5, 1, 1, X, X, X, X, X, 1, 1, 3, 0, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 3,
        2, X, X, X, X, X, X, X, 2, 5, 1, 2, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 1, 2, 3, 0, X,
        X, X, X, X, 3, 1, X, X, X, X, X, X, X, 2, 5, 2, 1, X, X, X, X, X, 5, 0, X, X, X, X, X, X, X,
        5, 0, X, X, X, X, X, X, X, 2, 5, X, X, X, X, X, X, X, 1, 5, X, X, X, X, X, X, X, 1, 0, 1, 5,
        X, X, X, X, X, 1, 0, 1, 5, X, X, X, X, X, 2, 1, 1, 5, X, X, X, X, X, 2, 1, X, X, X, X, X, X,
        X, 2, 1, 1, 0, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 4, 2, X, X, X, X, X, X, X, 1, 2, 1,
        5, X, X, X, X, X, 2, 0, 1, 5, X, X, X, X, X, 1, 0, 1, 2, 1, 5, X, X, X, 1, 5, 3, 1, X, X, X,
        X, X, 3, 1, X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 3, 5,
        X, X, X, X, X, X, X, 1, 5, 1, 4, X, X, X, X, X, 2, 0, 1, 5, X, X, X, X, X, 1, 0, 1, 5, 1, 4,
        X, X, X, 1, 5, 3, 1, X, X, X, X, X, 2, 1, 1, 4, X, X, X, X, X, 2, 1, 2, 0, X, X, X, X, X, 1,
        4, 3, 0, X, X, X, X, X, 5, 2, X, X, X, X, X, X, X, 1, 2, 1, 4, 1, 5, X, X, X, 1, 5, 3, 0, X,
        X, X, X, X, 1, 0, 1, 4, 1, 2, 1, 5, X, 4, 1, 1, 5, X, X, X, X, X, 1, 4, 3, 1, X, X, X, X, X,
        5, 0, X, X, X, X, X, X, X, 4, 0, 1, 4, X, X, X, X, X, 4, 4, X, X, X, X, X, X, X, 2, 4, X, X,
        X, X, X, X, X, 2, 4, 1, 0, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 4, 1, X, X, X, X, X, X,
        X, 3, 1, X, X, X, X, X, X, X, 1, 0, 3, 1, X, X, X, X, X, 2, 0, X, X, X, X, X, X, X, 3, 2, X,
        X, X, X, X, X, X, 2, 4, 1, 2, X, X, X, X, X, 2, 0, 2, 4, X, X, X, X, X, 1, 2, 3, 0, X, X, X,
        X, X, 5, 1, X, X, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 5, 0, X, X, X, X, X, X, X, 3, 0,
        X, X, X, X, X, X, X, 2, 4, X, X, X, X, X, X, X, 3, 6, X, X, X, X, X, X, X, 4, 0, X, X, X, X,
        X, X, X, 4, 0, X, X, X, X, X, X, X, 3, 1, X, X, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 5,
        0, X, X, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 2, 2, X, X, X, X, X, X, X, 1, 2, 3, 6, X,
        X, X, X, X, 5, 0, X, X, X, X, X, X, X, 4, 0, 1, 2, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X,
        5, 1, X, X, X, X, X, X, X, 1, 0, 1, 6, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 1, 6, X, X,
        X, X, X, X, X, 1, 6, X, X, X, X, X, X, X, 1, 0, 1, 6, X, X, X, X, X, 1, 0, 1, 6, X, X, X, X,
        X, 2, 1, 1, 6, X, X, X, X, X, 1, 1, 1, 6, X, X, X, X, X, 1, 1, 1, 0, 1, 6, X, X, X, 2, 0, 1,
        6, X, X, X, X, X, 1, 6, 3, 2, X, X, X, X, X, 2, 2, X, X, X, X, X, X, X, 3, 0, X, X, X, X, X,
        X, X, 2, 2, 1, 0, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 3, 1, X, X, X, X, X, X, X, 4, 0,
        X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 3, 6, X, X, X, X, X, X, X, 2, 4, X, X, X, X,
        X, X, X, 3, 0, X, X, X, X, X, X, X, 2, 4, 1, 0, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 2,
        4, 1, 1, X, X, X, X, X, 1, 1, 3, 0, X, X, X, X, X, 2, 4, 2, 0, X, X, X, X, X, 5, 2, X, X, X,
        X, X, X, X, 3, 2, X, X, X, X, X, X, X, 2, 0, X, X, X, X, X, X, X, 1, 0, 3, 2, X, X, X, X, X,
        3, 1, X, X, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 5, 0, X, X,
        X, X, X, X, X, 2, 4, X, X, X, X, X, X, X, 1, 4, 1, 6, X, X, X, X, X, 1, 0, 1, 4, 1, 6, X, X,
        X, 2, 0, 1, 6, X, X, X, X, X, 1, 6, 3, 1, X, X, X, X, X, 1, 4, 1, 1, 1, 6, X, X, X, 1, 6, 1,
        1, 1, 0, 1, 4, X, 1, 6, 3, 0, X, X, X, X, X, 4, 2, 1, 6, X, X, X, X, X, 2, 2, 1, 4, X, X, X,
        X, X, 1, 4, 3, 0, X, X, X, X, X, 2, 2, 2, 0, X, X, X, X, X, 5, 1, X, X, X, X, X, X, X, 1, 4,
        3, 1, X, X, X, X, X, 4, 0, 1, 4, X, X, X, X, X, 5, 0, X, X, X, X, X, X, X, 4, 4, X, X, X, X,
        X, X, X, 3, 5, X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 3,
        1, X, X, X, X, X, X, X, 1, 1, 3, 5, X, X, X, X, X, 4, 0, 1, 1, X, X, X, X, X, 5, 0, X, X, X,
        X, X, X, X, 4, 2, X, X, X, X, X, X, X, 4, 2, X, X, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X,
        5, 0, X, X, X, X, X, X, X, 2, 1, X, X, X, X, X, X, X, 5, 1, X, X, X, X, X, X, X, 4, 0, X, X,
        X, X, X, X, X, 1, 0, 1, 5, X, X, X, X, X, 1, 5, X, X, X, X, X, X, X, 2, 5, X, X, X, X, X, X,
        X, 2, 5, 1, 0, X, X, X, X, X, 2, 5, 1, 0, X, X, X, X, X, 2, 5, 2, 1, X, X, X, X, X, 3, 1, X,
        X, X, X, X, X, X, 1, 0, 3, 1, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 5, 2, X, X, X, X, X,
        X, X, 3, 2, X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 1, 0, 3, 2, X, X, X, X, X, 5, 1,
        X, X, X, X, X, X, X, 2, 1, X, X, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 3, 0, X, X, X, X,
        X, X, X, 2, 5, X, X, X, X, X, X, X, 3, 4, X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 1,
        0, 3, 4, X, X, X, X, X, 5, 1, X, X, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 5, 0, X, X, X,
        X, X, X, X, 5, 0, X, X, X, X, X, X, X, 1, 4, 1, 2, X, X, X, X, X, 4, 2, X, X, X, X, X, X, X,
        3, 0, X, X, X, X, X, X, X, 4, 2, 1, 0, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 3, 1, X, X,
        X, X, X, X, X, 2, 0, X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 1, 4, X, X, X, X, X, X,
        X, 3, 4, X, X, X, X, X, X, X, 1, 0, 3, 4, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 5, 1, X,
        X, X, X, X, X, X, 4, 1, X, X, X, X, X, X, X, 4, 1, 1, 0, X, X, X, X, X, 3, 0, X, X, X, X, X,
        X, X, 4, 2, X, X, X, X, X, X, X, 4, 2, X, X, X, X, X, X, X, 5, 0, X, X, X, X, X, X, X, 5, 0,
        X, X, X, X, X, X, X, 1, 1, 1, 4, X, X, X, X, X, 3, 1, X, X, X, X, X, X, X, 4, 0, X, X, X, X,
        X, X, X, 2, 0, X, X, X, X, X, X, X, 1, 4, X, X, X, X, X, X, X, 2, 8, X, X, X, X, X, X, X, 3,
        0, X, X, X, X, X, X, X, 3, 0, X, X, X, X, X, X, X, 2, 1, X, X, X, X, X, X, X, 3, 1, X, X, X,
        X, X, X, X, 4, 0, X, X, X, X, X, X, X, 2, 0, X, X, X, X, X, X, X, 1, 2, X, X, X, X, X, X, X,
        3, 2, X, X, X, X, X, X, X, 2, 0, X, X, X, X, X, X, X, 4, 0, X, X, X, X, X, X, X, 1, 1, X, X,
        X, X, X, X, X, 2, 1, X, X, X, X, X, X, X, 1, 0, X, X, X, X, X, X, X, 1, 0, X, X, X, X, X, X,
        X, X, X, X, X, X, X, X, X, X
#undef X
      };

    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Id
      labelEdgeTableLT3d[nCasesLT3d * nLabelEdgeTableLT3dElemSize] = {
#define X -1
        X, X,  X, X,  X, X,  X, X,  X, X,  X, X,  X, 6, 18, X, X,  X, X,  X, X,  X, X,  X, X,  X,
        2, 0,  X, X,  X, X,  X, X,  X, X,  X, X,  X, 2, 12, 2, 14, 4, 18, X, X,  X, X,  X, X,  X,
        2, 12, X, X,  X, X,  X, X,  X, X,  X, X,  X, 2, 0,  1, 18, 2, 3,  3, 18, X, X,  X, X,  X,
        3, 0,  1, 12, X, X,  X, X,  X, X,  X, X,  X, 1, 18, 2, 14, 2, 3,  3, 18, X, X,  X, X,  X,
        2, 3,  X, X,  X, X,  X, X,  X, X,  X, X,  X, 2, 18, 2, 12, 2, 16, 2, 18, X, X,  X, X,  X,
        2, 0,  2, 3,  X, X,  X, X,  X, X,  X, X,  X, 2, 12, 2, 14, 2, 12, 2, 16, 2, 18, X, X,  X,
        1, 12, 3, 3,  X, X,  X, X,  X, X,  X, X,  X, 2, 0,  2, 18, 2, 16, 2, 18, X, X,  X, X,  X,
        3, 0,  3, 3,  X, X,  X, X,  X, X,  X, X,  X, 1, 18, 2, 14, 1, 18, 2, 16, 2, 18, X, X,  X,
        2, 8,  X, X,  X, X,  X, X,  X, X,  X, X,  X, 4, 18, 2, 14, 2, 16, X, X,  X, X,  X, X,  X,
        2, 0,  2, 8,  X, X,  X, X,  X, X,  X, X,  X, 2, 12, 2, 14, 2, 18, 2, 14, 2, 16, X, X,  X,
        2, 12, 2, 8,  X, X,  X, X,  X, X,  X, X,  X, 2, 0,  1, 18, 2, 3,  1, 18, 2, 14, 2, 16, X,
        3, 0,  1, 12, 2, 8,  X, X,  X, X,  X, X,  X, 1, 18, 2, 14, 2, 3,  1, 18, 2, 14, 2, 16, X,
        2, 3,  2, 8,  X, X,  X, X,  X, X,  X, X,  X, 2, 18, 2, 12, 2, 16, 2, 14, 2, 16, X, X,  X,
        2, 0,  2, 3,  2, 8,  X, X,  X, X,  X, X,  X, 2, 12, 2, 14, 2, 12, 2, 16, 2, 14, 2, 16, X,
        1, 12, 3, 3,  2, 8,  X, X,  X, X,  X, X,  X, 2, 0,  2, 18, 2, 16, 2, 14, 2, 16, X, X,  X,
        3, 0,  3, 3,  2, 8,  X, X,  X, X,  X, X,  X, 1, 18, 2, 14, 1, 18, 2, 16, 2, 14, 2, 16, X,
        2, 14, X, X,  X, X,  X, X,  X, X,  X, X,  X, 1, 18, 2, 0,  2, 18, 2, 8,  1, 18, X, X,  X,
        3, 0,  1, 14, X, X,  X, X,  X, X,  X, X,  X, 2, 12, 3, 18, 2, 8,  1, 18, X, X,  X, X,  X,
        1, 12, 1, 14, 1, 12, 1, 14, X, X,  X, X,  X, 4, 0,  2, 3,  1, 18, 2, 8,  1, 18, X, X,  X,
        4, 0,  1, 12, 1, 14, X, X,  X, X,  X, X,  X, 2, 18, 2, 3,  1, 18, 2, 8,  1, 18, X, X,  X,
        1, 14, 2, 3,  1, 14, X, X,  X, X,  X, X,  X, 1, 18, 2, 0,  2, 12, 2, 16, 2, 8,  1, 18, X,
        3, 0,  2, 3,  1, 14, X, X,  X, X,  X, X,  X, 2, 12, 1, 18, 2, 12, 2, 16, 2, 8,  1, 18, X,
        1, 12, 1, 14, 3, 3,  1, 14, X, X,  X, X,  X, 4, 0,  1, 18, 2, 16, 2, 8,  1, 18, X, X,  X,
        4, 0,  3, 3,  1, 14, X, X,  X, X,  X, X,  X, 3, 18, 2, 16, 2, 8,  1, 18, X, X,  X, X,  X,
        1, 14, 3, 8,  X, X,  X, X,  X, X,  X, X,  X, 1, 18, 2, 0,  3, 18, 2, 16, X, X,  X, X,  X,
        3, 0,  3, 8,  X, X,  X, X,  X, X,  X, X,  X, 2, 12, 4, 18, 2, 16, X, X,  X, X,  X, X,  X,
        1, 12, 1, 14, 1, 12, 3, 8,  X, X,  X, X,  X, 4, 0,  2, 3,  2, 18, 2, 16, X, X,  X, X,  X,
        4, 0,  1, 12, 3, 8,  X, X,  X, X,  X, X,  X, 2, 18, 2, 3,  2, 18, 2, 16, X, X,  X, X,  X,
        1, 14, 2, 3,  3, 8,  X, X,  X, X,  X, X,  X, 1, 18, 2, 0,  2, 12, 2, 16, 1, 18, 2, 16, X,
        3, 0,  2, 3,  3, 8,  X, X,  X, X,  X, X,  X, 2, 12, 1, 18, 2, 12, 2, 16, 1, 18, 2, 16, X,
        1, 12, 1, 14, 3, 3,  3, 8,  X, X,  X, X,  X, 4, 0,  1, 18, 2, 16, 1, 18, 2, 16, X, X,  X,
        4, 0,  3, 3,  3, 8,  X, X,  X, X,  X, X,  X, 3, 18, 2, 16, 1, 18, 2, 16, X, X,  X, X,  X,
        6, 18, X, X,  X, X,  X, X,  X, X,  X, X,  X, 4, 0,  4, 3,  4, 8,  X, X,  X, X,  X, X,  X,
        4, 0,  4, 18, X, X,  X, X,  X, X,  X, X,  X, 1, 12, 1, 14, 4, 3,  4, 8,  X, X,  X, X,  X,
        2, 12, 1, 18, 2, 12, 3, 18, X, X,  X, X,  X, 3, 0,  3, 3,  4, 8,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 0,  2, 12, 3, 18, X, X,  X, X,  X, 1, 14, 3, 3,  4, 8,  X, X,  X, X,  X, X,  X,
        2, 18, 4, 3,  2, 18, X, X,  X, X,  X, X,  X, 4, 0,  1, 12, 1, 16, 4, 8,  X, X,  X, X,  X,
        4, 0,  4, 3,  2, 18, X, X,  X, X,  X, X,  X, 1, 12, 1, 14, 1, 12, 1, 16, 4, 8,  X, X,  X,
        2, 12, 2, 18, 2, 3,  2, 18, X, X,  X, X,  X, 3, 0,  1, 16, 4, 8,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 0,  1, 18, 2, 3,  2, 18, X, X,  X, 1, 14, 1, 16, 4, 8,  X, X,  X, X,  X, X,  X,
        4, 18, 4, 8,  X, X,  X, X,  X, X,  X, X,  X, 4, 0,  4, 3,  1, 14, 1, 16, X, X,  X, X,  X,
        4, 0,  2, 18, 4, 8,  X, X,  X, X,  X, X,  X, 1, 12, 1, 14, 4, 3,  1, 14, 1, 16, X, X,  X,
        2, 12, 1, 18, 2, 12, 1, 18, 4, 8,  X, X,  X, 3, 0,  3, 3,  1, 14, 1, 16, X, X,  X, X,  X,
        1, 18, 2, 0,  2, 12, 1, 18, 4, 8,  X, X,  X, 1, 14, 3, 3,  1, 14, 1, 16, X, X,  X, X,  X,
        2, 18, 4, 3,  4, 8,  X, X,  X, X,  X, X,  X, 4, 0,  1, 12, 1, 16, 1, 14, 1, 16, X, X,  X,
        4, 0,  4, 3,  4, 8,  X, X,  X, X,  X, X,  X, 1, 12, 1, 14, 1, 12, 1, 16, 1, 14, 1, 16, X,
        2, 12, 2, 18, 2, 3,  4, 8,  X, X,  X, X,  X, 3, 0,  1, 16, 1, 14, 1, 16, X, X,  X, X,  X,
        1, 18, 2, 0,  1, 18, 2, 3,  4, 8,  X, X,  X, 1, 14, 1, 16, 1, 14, 1, 16, X, X,  X, X,  X,
        1, 18, 2, 14, 2, 18, 2, 14, 1, 18, X, X,  X, 3, 0,  4, 3,  3, 8,  X, X,  X, X,  X, X,  X,
        2, 0,  3, 18, 2, 14, 1, 18, X, X,  X, X,  X, 1, 12, 4, 3,  3, 8,  X, X,  X, X,  X, X,  X,
        2, 12, 2, 14, 2, 12, 1, 18, 2, 14, 1, 18, X, 2, 0,  3, 3,  3, 8,  X, X,  X, X,  X, X,  X,
        2, 18, 2, 12, 1, 18, 2, 14, 1, 18, X, X,  X, 3, 3,  3, 8,  X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 14, 4, 3,  2, 14, 1, 18, X, X,  X, 3, 0,  1, 12, 1, 16, 3, 8,  X, X,  X, X,  X,
        2, 0,  1, 18, 4, 3,  2, 14, 1, 18, X, X,  X, 2, 12, 1, 16, 3, 8,  X, X,  X, X,  X, X,  X,
        2, 12, 2, 14, 1, 18, 2, 3,  2, 14, 1, 18, X, 2, 0,  1, 16, 3, 8,  X, X,  X, X,  X, X,  X,
        3, 18, 2, 3,  2, 14, 1, 18, X, X,  X, X,  X, 1, 16, 3, 8,  X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 14, 3, 18, 2, 8,  X, X,  X, X,  X, 3, 0,  4, 3,  1, 16, X, X,  X, X,  X, X,  X,
        2, 0,  4, 18, 2, 8,  X, X,  X, X,  X, X,  X, 1, 12, 4, 3,  1, 16, X, X,  X, X,  X, X,  X,
        2, 12, 2, 14, 2, 12, 2, 18, 2, 8,  X, X,  X, 2, 0,  3, 3,  1, 16, X, X,  X, X,  X, X,  X,
        2, 18, 2, 12, 2, 18, 2, 8,  X, X,  X, X,  X, 3, 3,  1, 16, X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 14, 4, 3,  1, 18, 2, 8,  X, X,  X, 3, 0,  1, 12, 2, 16, X, X,  X, X,  X, X,  X,
        2, 0,  1, 18, 4, 3,  1, 18, 2, 8,  X, X,  X, 2, 12, 2, 16, X, X,  X, X,  X, X,  X, X,  X,
        2, 12, 2, 14, 1, 18, 2, 3,  1, 18, 2, 8,  X, 2, 0,  2, 16, X, X,  X, X,  X, X,  X, X,  X,
        3, 18, 2, 3,  1, 18, 2, 8,  X, X,  X, X,  X, 2, 16, X, X,  X, X,  X, X,  X, X,  X, X,  X,
        2, 16, X, X,  X, X,  X, X,  X, X,  X, X,  X, 3, 18, 2, 3,  1, 18, 2, 8,  X, X,  X, X,  X,
        2, 0,  2, 16, X, X,  X, X,  X, X,  X, X,  X, 2, 12, 2, 14, 1, 18, 2, 3,  1, 18, 2, 8,  X,
        2, 12, 2, 16, X, X,  X, X,  X, X,  X, X,  X, 2, 0,  1, 18, 4, 3,  1, 18, 2, 8,  X, X,  X,
        3, 0,  1, 12, 2, 16, X, X,  X, X,  X, X,  X, 1, 18, 2, 14, 4, 3,  1, 18, 2, 8,  X, X,  X,
        3, 3,  1, 16, X, X,  X, X,  X, X,  X, X,  X, 2, 18, 2, 12, 2, 18, 2, 8,  X, X,  X, X,  X,
        2, 0,  3, 3,  1, 16, X, X,  X, X,  X, X,  X, 2, 12, 2, 14, 2, 12, 2, 18, 2, 8,  X, X,  X,
        1, 12, 4, 3,  1, 16, X, X,  X, X,  X, X,  X, 2, 0,  4, 18, 2, 8,  X, X,  X, X,  X, X,  X,
        3, 0,  4, 3,  1, 16, X, X,  X, X,  X, X,  X, 1, 18, 2, 14, 3, 18, 2, 8,  X, X,  X, X,  X,
        1, 16, 3, 8,  X, X,  X, X,  X, X,  X, X,  X, 3, 18, 2, 3,  2, 14, 1, 18, X, X,  X, X,  X,
        2, 0,  1, 16, 3, 8,  X, X,  X, X,  X, X,  X, 2, 12, 2, 14, 1, 18, 2, 3,  2, 14, 1, 18, X,
        2, 12, 1, 16, 3, 8,  X, X,  X, X,  X, X,  X, 2, 0,  1, 18, 4, 3,  2, 14, 1, 18, X, X,  X,
        3, 0,  1, 12, 1, 16, 3, 8,  X, X,  X, X,  X, 1, 18, 2, 14, 4, 3,  2, 14, 1, 18, X, X,  X,
        3, 3,  3, 8,  X, X,  X, X,  X, X,  X, X,  X, 2, 18, 2, 12, 1, 18, 2, 14, 1, 18, X, X,  X,
        2, 0,  3, 3,  3, 8,  X, X,  X, X,  X, X,  X, 2, 12, 2, 14, 2, 12, 1, 18, 2, 14, 1, 18, X,
        1, 12, 4, 3,  3, 8,  X, X,  X, X,  X, X,  X, 2, 0,  3, 18, 2, 14, 1, 18, X, X,  X, X,  X,
        3, 0,  4, 3,  3, 8,  X, X,  X, X,  X, X,  X, 1, 18, 2, 14, 2, 18, 2, 14, 1, 18, X, X,  X,
        1, 14, 1, 16, 1, 14, 1, 16, X, X,  X, X,  X, 1, 18, 2, 0,  1, 18, 2, 3,  4, 8,  X, X,  X,
        3, 0,  1, 16, 1, 14, 1, 16, X, X,  X, X,  X, 2, 12, 2, 18, 2, 3,  4, 8,  X, X,  X, X,  X,
        1, 12, 1, 14, 1, 12, 1, 16, 1, 14, 1, 16, X, 4, 0,  4, 3,  4, 8,  X, X,  X, X,  X, X,  X,
        4, 0,  1, 12, 1, 16, 1, 14, 1, 16, X, X,  X, 2, 18, 4, 3,  4, 8,  X, X,  X, X,  X, X,  X,
        1, 14, 3, 3,  1, 14, 1, 16, X, X,  X, X,  X, 1, 18, 2, 0,  2, 12, 1, 18, 4, 8,  X, X,  X,
        3, 0,  3, 3,  1, 14, 1, 16, X, X,  X, X,  X, 2, 12, 1, 18, 2, 12, 1, 18, 4, 8,  X, X,  X,
        1, 12, 1, 14, 4, 3,  1, 14, 1, 16, X, X,  X, 4, 0,  2, 18, 4, 8,  X, X,  X, X,  X, X,  X,
        4, 0,  4, 3,  1, 14, 1, 16, X, X,  X, X,  X, 4, 18, 4, 8,  X, X,  X, X,  X, X,  X, X,  X,
        1, 14, 1, 16, 4, 8,  X, X,  X, X,  X, X,  X, 1, 18, 2, 0,  1, 18, 2, 3,  2, 18, X, X,  X,
        3, 0,  1, 16, 4, 8,  X, X,  X, X,  X, X,  X, 2, 12, 2, 18, 2, 3,  2, 18, X, X,  X, X,  X,
        1, 12, 1, 14, 1, 12, 1, 16, 4, 8,  X, X,  X, 4, 0,  4, 3,  2, 18, X, X,  X, X,  X, X,  X,
        4, 0,  1, 12, 1, 16, 4, 8,  X, X,  X, X,  X, 2, 18, 4, 3,  2, 18, X, X,  X, X,  X, X,  X,
        1, 14, 3, 3,  4, 8,  X, X,  X, X,  X, X,  X, 1, 18, 2, 0,  2, 12, 3, 18, X, X,  X, X,  X,
        3, 0,  3, 3,  4, 8,  X, X,  X, X,  X, X,  X, 2, 12, 1, 18, 2, 12, 3, 18, X, X,  X, X,  X,
        1, 12, 1, 14, 4, 3,  4, 8,  X, X,  X, X,  X, 4, 0,  4, 18, X, X,  X, X,  X, X,  X, X,  X,
        4, 0,  4, 3,  4, 8,  X, X,  X, X,  X, X,  X, 6, 18, X, X,  X, X,  X, X,  X, X,  X, X,  X,
        3, 18, 2, 16, 1, 18, 2, 16, X, X,  X, X,  X, 4, 0,  3, 3,  3, 8,  X, X,  X, X,  X, X,  X,
        4, 0,  1, 18, 2, 16, 1, 18, 2, 16, X, X,  X, 1, 12, 1, 14, 3, 3,  3, 8,  X, X,  X, X,  X,
        2, 12, 1, 18, 2, 12, 2, 16, 1, 18, 2, 16, X, 3, 0,  2, 3,  3, 8,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 0,  2, 12, 2, 16, 1, 18, 2, 16, X, 1, 14, 2, 3,  3, 8,  X, X,  X, X,  X, X,  X,
        2, 18, 2, 3,  2, 18, 2, 16, X, X,  X, X,  X, 4, 0,  1, 12, 3, 8,  X, X,  X, X,  X, X,  X,
        4, 0,  2, 3,  2, 18, 2, 16, X, X,  X, X,  X, 1, 12, 1, 14, 1, 12, 3, 8,  X, X,  X, X,  X,
        2, 12, 4, 18, 2, 16, X, X,  X, X,  X, X,  X, 3, 0,  3, 8,  X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 0,  3, 18, 2, 16, X, X,  X, X,  X, 1, 14, 3, 8,  X, X,  X, X,  X, X,  X, X,  X,
        3, 18, 2, 16, 2, 8,  1, 18, X, X,  X, X,  X, 4, 0,  3, 3,  1, 14, X, X,  X, X,  X, X,  X,
        4, 0,  1, 18, 2, 16, 2, 8,  1, 18, X, X,  X, 1, 12, 1, 14, 3, 3,  1, 14, X, X,  X, X,  X,
        2, 12, 1, 18, 2, 12, 2, 16, 2, 8,  1, 18, X, 3, 0,  2, 3,  1, 14, X, X,  X, X,  X, X,  X,
        1, 18, 2, 0,  2, 12, 2, 16, 2, 8,  1, 18, X, 1, 14, 2, 3,  1, 14, X, X,  X, X,  X, X,  X,
        2, 18, 2, 3,  1, 18, 2, 8,  1, 18, X, X,  X, 4, 0,  1, 12, 1, 14, X, X,  X, X,  X, X,  X,
        4, 0,  2, 3,  1, 18, 2, 8,  1, 18, X, X,  X, 1, 12, 1, 14, 1, 12, 1, 14, X, X,  X, X,  X,
        2, 12, 3, 18, 2, 8,  1, 18, X, X,  X, X,  X, 3, 0,  1, 14, X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 0,  2, 18, 2, 8,  1, 18, X, X,  X, 2, 14, X, X,  X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 14, 1, 18, 2, 16, 2, 14, 2, 16, X, 3, 0,  3, 3,  2, 8,  X, X,  X, X,  X, X,  X,
        2, 0,  2, 18, 2, 16, 2, 14, 2, 16, X, X,  X, 1, 12, 3, 3,  2, 8,  X, X,  X, X,  X, X,  X,
        2, 12, 2, 14, 2, 12, 2, 16, 2, 14, 2, 16, X, 2, 0,  2, 3,  2, 8,  X, X,  X, X,  X, X,  X,
        2, 18, 2, 12, 2, 16, 2, 14, 2, 16, X, X,  X, 2, 3,  2, 8,  X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 14, 2, 3,  1, 18, 2, 14, 2, 16, X, 3, 0,  1, 12, 2, 8,  X, X,  X, X,  X, X,  X,
        2, 0,  1, 18, 2, 3,  1, 18, 2, 14, 2, 16, X, 2, 12, 2, 8,  X, X,  X, X,  X, X,  X, X,  X,
        2, 12, 2, 14, 2, 18, 2, 14, 2, 16, X, X,  X, 2, 0,  2, 8,  X, X,  X, X,  X, X,  X, X,  X,
        4, 18, 2, 14, 2, 16, X, X,  X, X,  X, X,  X, 2, 8,  X, X,  X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 14, 1, 18, 2, 16, 2, 18, X, X,  X, 3, 0,  3, 3,  X, X,  X, X,  X, X,  X, X,  X,
        2, 0,  2, 18, 2, 16, 2, 18, X, X,  X, X,  X, 1, 12, 3, 3,  X, X,  X, X,  X, X,  X, X,  X,
        2, 12, 2, 14, 2, 12, 2, 16, 2, 18, X, X,  X, 2, 0,  2, 3,  X, X,  X, X,  X, X,  X, X,  X,
        2, 18, 2, 12, 2, 16, 2, 18, X, X,  X, X,  X, 2, 3,  X, X,  X, X,  X, X,  X, X,  X, X,  X,
        1, 18, 2, 14, 2, 3,  3, 18, X, X,  X, X,  X, 3, 0,  1, 12, X, X,  X, X,  X, X,  X, X,  X,
        2, 0,  1, 18, 2, 3,  3, 18, X, X,  X, X,  X, 2, 12, X, X,  X, X,  X, X,  X, X,  X, X,  X,
        2, 12, 2, 14, 4, 18, X, X,  X, X,  X, X,  X, 2, 0,  X, X,  X, X,  X, X,  X, X,  X, X,  X,
        6, 18, X, X,  X, X,  X, X,  X, X,  X, X,  X, X, X,  X, X,  X, X,  X, X,  X, X,  X, X,  X
#undef X
      };

    // Return the const array at the index of "arrayIndex".
    // An array boundary check is performed via VISKORES_ASSERT.
    if (this->CopyType == COPY_VERTEXOFFSET)
    {
      if (this->IsData2D)
      {
        VISKORES_ASSERT(arrayIndex < nVertices2d * 2);
        return vertexOffset2d[arrayIndex];
      }
      else
      {
        VISKORES_ASSERT(arrayIndex < nVertices3d * 3);
        return vertexOffset3d[arrayIndex];
      }
    }
    else if (this->CopyType == COPY_EDGETABLE)
    {
      if (this->IsData2D)
      {
        VISKORES_ASSERT(arrayIndex < nEdges2d * 2);
        return edgeTable2d[arrayIndex];
      }
      else
      {
        if (this->IsMarchingCube)
        {
          VISKORES_ASSERT(arrayIndex < nEdgesMC3d * 2);
          return edgeTableMC3d[arrayIndex];
        }
        else
        {
          VISKORES_ASSERT(arrayIndex < nEdgesLT3d * 2);
          return edgeTableLT3d[arrayIndex];
        }
      }
    }
    else if (this->CopyType == COPY_NUMBOUNDTABLE)
    {
      if (this->IsData2D)
      {
        VISKORES_ASSERT(arrayIndex < nCases2d);
        return numLinesTable2d[arrayIndex];
      }
      else
      {
        if (this->IsMarchingCube)
        {
          VISKORES_ASSERT(arrayIndex < nCasesMC3d);
          return numTrianglesTableMC3d[arrayIndex];
        }
        else
        {
          VISKORES_ASSERT(arrayIndex < nCasesLT3d);
          return numTrianglesTableLT3d[arrayIndex];
        }
      }
    }
    else if (this->CopyType == COPY_BOUNDARYTABLE)
    {
      if (this->IsData2D)
      {
        VISKORES_ASSERT(arrayIndex < nLineTableElemSize2d * nCases2d);
        return lineTable2d[arrayIndex];
      }
      else
      {
        if (this->IsMarchingCube)
        {
          VISKORES_ASSERT(arrayIndex < nCasesMC3d * nTriTableMC3dElemSize);
          return triTableMC3d[arrayIndex];
        }
        else
        {
          VISKORES_ASSERT(arrayIndex < nCasesLT3d * nTriTableLT3dElemSize);
          return triTableLT3d[arrayIndex];
        }
      }
    }
    else if (this->CopyType == COPY_LABELEDGETABLE)
    {
      VISKORES_ASSERT(!this->IsData2D && "There is no label edge table for 2D data!");
      if (this->IsMarchingCube)
      {
        VISKORES_ASSERT(arrayIndex < nCasesMC3d * nLabelEdgeTableMC3dElemSize);
        return labelEdgeTableMC3d[arrayIndex];
      }
      else
      {
        VISKORES_ASSERT(arrayIndex < nCasesLT3d * nLabelEdgeTableLT3dElemSize);
        return labelEdgeTableLT3d[arrayIndex];
      }
    }
    // this should not happen! Just in case of fallouts.
    VISKORES_ASSERT(false && "Undefined const array type for copy!");
    return -1;
  }

private:
  const bool IsData2D;
  const bool IsMarchingCube;
  const viskores::IdComponent CopyType;
}; // CopyConstArraysForMarchingCubesDataTablesWorklet

} // namespace extract_top_volume_contours
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
