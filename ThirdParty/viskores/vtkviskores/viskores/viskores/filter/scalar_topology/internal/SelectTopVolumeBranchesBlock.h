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

#ifndef viskores_filter_scalar_topology_internal_SelectTopVolumeBranchesBlock_h
#define viskores_filter_scalar_topology_internal_SelectTopVolumeBranchesBlock_h

#include <viskores/cont/DataSet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/BranchDecompositionTreeMaker.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/TopVolumeBranchData.h>

namespace viskores
{
namespace filter
{
namespace scalar_topology
{
namespace internal
{

struct SelectTopVolumeBranchesBlock
{
  SelectTopVolumeBranchesBlock(viskores::Id localBlockNo, int globalBlockId);

  // Block metadata
  viskores::Id LocalBlockNo;
  int GlobalBlockId;

  // the data class for branch arrays (e.g., branch root global regular IDs, branch volume, etc.)
  TopVolumeBranchData TopVolumeData;

  // the factory class to compute the relation of top-volume branches
  BranchDecompositionTreeMaker BDTMaker;

  // Destroy function allowing DIY to own blocks and clean them up after use
  static void Destroy(void* b) { delete static_cast<SelectTopVolumeBranchesBlock*>(b); }

  // compute the volume of local branches, and sort them by volume
  void SortBranchByVolume(const viskores::cont::DataSet& bdDataSet, const viskores::Id totalVolume);

  // choose the top branches by volume
  void SelectLocalTopVolumeBranches(const viskores::cont::DataSet& bdDataSet,
                                    const viskores::Id nSavedBranches);

  // compute the branch decomposition tree (implicitly) for top branches
  void ComputeTopVolumeBranchHierarchy(const viskores::cont::DataSet& bdDataSet);

  // exclude branches whose volume <= presimplifyThreshold
  viskores::Id ExcludeTopVolumeBranchByThreshold(const viskores::Id presimplifyThreshold);
};

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
#endif
