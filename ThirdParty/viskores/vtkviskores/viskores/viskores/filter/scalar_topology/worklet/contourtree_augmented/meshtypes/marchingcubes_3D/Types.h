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

#ifndef viskores_worklet_contourtree_augmented_mesh_dem_triangulation_3D_marchingcubes_types_h
#define viskores_worklet_contourtree_augmented_mesh_dem_triangulation_3D_marchingcubes_types_h

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVec.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace m3d_marchingcubes
{

// Constants and case tables
static constexpr viskores::Int8 FrontBit = 1 << 4;
static constexpr viskores::Int8 BackBit = 1 << 5;
static constexpr viskores::Int8 TopBit = 1 << 2;
static constexpr viskores::Int8 BottomBit = 1 << 3;
static constexpr viskores::Int8 LeftBit = 1 << 0;
static constexpr viskores::Int8 RightBit = 1 << 1;

static constexpr viskores::IdComponent N_EDGE_NEIGHBOURS = 6;
static constexpr viskores::IdComponent N_FACE_NEIGHBOURS = 18;
static constexpr viskores::IdComponent N_ALL_NEIGHBOURS = 26;

// EdgeBoundaryDetectionMasks
VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int8 EdgeBoundaryDetectionMasks[N_ALL_NEIGHBOURS] = {
  FrontBit,
  TopBit,
  LeftBit,
  RightBit,
  BottomBit,
  BackBit,
  FrontBit | TopBit,
  FrontBit | LeftBit,
  FrontBit | RightBit,
  FrontBit | BottomBit,
  TopBit | LeftBit,
  TopBit | RightBit,
  BottomBit | LeftBit,
  BottomBit | RightBit,
  BackBit | TopBit,
  BackBit | LeftBit,
  BackBit | RightBit,
  BackBit | BottomBit,
  FrontBit | TopBit | LeftBit,
  FrontBit | TopBit | RightBit,
  FrontBit | BottomBit | LeftBit,
  FrontBit | BottomBit | RightBit,
  BackBit | TopBit | LeftBit,
  BackBit | TopBit | RightBit,
  BackBit | BottomBit | LeftBit,
  BackBit | BottomBit | RightBit
};
// Viskores type for the EdgeBoundaryDetectionMasks
using EdgeBoundaryDetectionMasksType = viskores::cont::ArrayHandle<viskores::Int8>;

// Number of permutation vectors in CubeVertexPermutations
constexpr viskores::UInt8 CubeVertexPermutations_NumPermutations = 8;
// Length of a single permutation vector in the CubeVertexPermutations array
constexpr viskores::UInt8 CubeVertexPermutations_PermVecLength = 7;
// Viskores type for the CubeVertexPermutations
using CubeVertexPermutationsType =
  viskores::cont::ArrayHandleGroupVec<viskores::cont::ArrayHandle<viskores::IdComponent>,
                                      CubeVertexPermutations_PermVecLength>;
/* CubeVertexPermutations will be used as a 2D array of [8, 7]
   * The array is flattened here to ease conversion in viskores
   */
VISKORES_STATIC_CONSTEXPR_ARRAY viskores::IdComponent
  CubeVertexPermutations[CubeVertexPermutations_NumPermutations *
                         CubeVertexPermutations_PermVecLength] = {
    3,  4,  5, 13, 16, 17, 25, 3,  4,  0,  13, 8, 9,  21, 3,  1,  5, 11, 16,
    14, 23, 2, 4,  5,  12, 15, 17, 24, 3,  1,  0, 11, 8,  6,  19, 2, 4,  0,
    12, 7,  9, 20, 2,  1,  5,  10, 15, 14, 22, 2, 1,  0,  10, 7,  6, 18
  };


// number of vertex connection pairs contained in LinkVertexConnectionsSix
constexpr viskores::UInt8 LinkVertexConnectionsSix_NumPairs = 3;
// number of components defining a vertex connection
constexpr viskores::UInt8 VertexConnections_VecLength = 2;
// VISKORES-M type for the LinkVertexConnectionsEighteen and LinkVertexConnectionsSix
using LinkVertexConnectionsType =
  typename viskores::cont::ArrayHandleGroupVec<viskores::cont::ArrayHandle<viskores::IdComponent>,
                                               VertexConnections_VecLength>;
/* LinkVertexConnectionsSix[ will be used as a 2D array of [3, 3]
   * The array is flattened here to ease conversion in viskores
   */
VISKORES_STATIC_CONSTEXPR_ARRAY viskores::IdComponent
  LinkVertexConnectionsSix[LinkVertexConnectionsSix_NumPairs * VertexConnections_VecLength] = {
    0, 1, 0, 2, 1, 2
  };

// number of vertex connection pairs contained in LinkVertexConnectionsSix
VISKORES_STATIC_CONSTEXPR_ARRAY viskores::UInt8 LinkVertexConnectionsEighteen_NumPairs = 15;
/* LinkVertexConnectionsEighteen[ will be used as a 2D array of [3, 3]
   * The array is flattened here to ease conversion in viskores
   */
VISKORES_STATIC_CONSTEXPR_ARRAY viskores::IdComponent
  LinkVertexConnectionsEighteen[LinkVertexConnectionsEighteen_NumPairs *
                                VertexConnections_VecLength] = { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5,
                                                                 1, 2, 1, 3, 1, 4, 1, 5, 2, 3,
                                                                 2, 4, 2, 5, 3, 4, 3, 5, 4, 5 };

// VISKORES-M type for the InCubeConnectionsEighteen and InCubeConnectionsSix
using InCubeConnectionsType = typename viskores::cont::ArrayHandle<viskores::UInt32>;
VISKORES_STATIC_CONSTEXPR_ARRAY viskores::UInt8 InCubeConnectionsSix_NumElements = 128;
VISKORES_STATIC_CONSTEXPR_ARRAY viskores::UInt32
  InCubeConnectionsSix[InCubeConnectionsSix_NumElements] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 1, 0, 2, 0, 7,
    0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 1, 0, 0, 4, 7, 0, 0, 0, 0, 0, 2, 4, 7, 0, 0, 0, 1, 0, 2, 4, 7,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 1, 0, 2, 4, 7,
    0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 1, 0, 2, 4, 7, 0, 0, 0, 1, 0, 2, 4, 7, 0, 0, 0, 1, 0, 2, 4, 7
  };

VISKORES_STATIC_CONSTEXPR_ARRAY viskores::UInt8 InCubeConnectionsEighteen_NumElements = 128;
VISKORES_STATIC_CONSTEXPR_ARRAY viskores::UInt32
  InCubeConnectionsEighteen[InCubeConnectionsEighteen_NumElements] = {
    0,     0,     0,     1,     0,     2,     32,    35,    0,     4,     64,    69,    0,
    518,   608,   615,   0,     8,     0,     137,   1024,  1034,  1184,  1195,  4096,  4108,
    4288,  4301,  5632,  5646,  5856,  5871,  0,     0,     256,   273,   2048,  2066,  2336,
    2355,  8192,  8212,  8512,  8533,  10752, 10774, 11104, 11127, 16384, 16408, 16768, 16793,
    19456, 19482, 19872, 19899, 28672, 28700, 29120, 29149, 32256, 32286, 32736, 32767, 0,
    0,     0,     1,     0,     2,     32,    35,    0,     4,     64,    69,    512,   518,
    608,   615,   0,     8,     128,   137,   1024,  1034,  1184,  1195,  4096,  4108,  4288,
    4301,  5632,  5646,  5856,  5871,  0,     16,    256,   273,   2048,  2066,  2336,  2355,
    8192,  8212,  8512,  8533,  10752, 10774, 11104, 11127, 16384, 16408, 16768, 16793, 19456,
    19482, 19872, 19899, 28672, 28700, 29120, 29149, 32256, 32286, 32736, 32767
  };




} // mesh_dem_types_2d_freudenthal
} // contourtree_augmented
} // worklet
} // viskores

#endif
