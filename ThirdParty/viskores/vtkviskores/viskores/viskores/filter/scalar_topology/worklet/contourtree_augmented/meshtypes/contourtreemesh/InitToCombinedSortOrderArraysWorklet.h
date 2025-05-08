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
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

#ifndef viskores_worklet_contourtree_augmented_contourtree_mesh_inc_init_to_combined_sort_order_arrays_worklet_h
#define viskores_worklet_contourtree_augmented_contourtree_mesh_inc_init_to_combined_sort_order_arrays_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_contourtree_mesh_inc
{


class InitToCombinedSortOrderArraysWorklet : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    FieldIn overallSortIndex,              // (input) overallSortIndex
    FieldIn overallSortOrder,              // (input) overallSortOrder
    WholeArrayOut thisToCombinedSortOrder, // (output) thisToCombinedSortOrder
    WholeArrayOut otherToCombinedSortOrder // (output) otherToCombinedSortOrder
  );
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1 InputDomain;

  // Default Constructor
  VISKORES_EXEC_CONT
  InitToCombinedSortOrderArraysWorklet() {}

  template <typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& overallSortIndexVal,
                                const viskores::Id& overallSortOrderVal,
                                const OutFieldPortalType& thisToCombinedSortOrderPortal,
                                const OutFieldPortalType& otherToCombinedSortOrderPortal) const
  {
    if (IsThis(overallSortOrderVal))
    {
      thisToCombinedSortOrderPortal.Set(MaskedIndex(overallSortOrderVal), overallSortIndexVal);
    }
    else
    {
      otherToCombinedSortOrderPortal.Set(MaskedIndex(overallSortOrderVal), overallSortIndexVal);
    }

    // In serial this worklet implements the following operation
    // for (indexVector::size_type i = 0; i < overallSortOrder.size(); ++i)
    //   {
    //     if (CombinedIndexVector::IsThis(overallSortOrder[i]))
    //         thisToCombinedSortOrder[MaskedIndex(overallSortOrder[MaskedIndex(i)])] = overallSortIndex[i];
    //     else
    //         otherToCombinedSortOrder[MaskedIndex(overallSortOrder[MaskedIndex(i)])] = overallSortIndex[i];
    //   }
  }

}; // InitToCombinedSortOrderArraysWorklet


} // namespace mesh_dem_contourtree_mesh_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
