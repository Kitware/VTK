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

// include guard
#ifndef viskores_worklet_contourtree_augmented_array_transforms_h
#define viskores_worklet_contourtree_augmented_array_transforms_h

// global libraries
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleTransform.h>

// local includes
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>


namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{


// permute routines
template <typename ValueType, typename ArrayType>
inline void PermuteArrayWithMaskedIndex(const ArrayType& input,
                                        IdArrayType& permute,
                                        ArrayType& output)
{ // permuteValues()
  using transform_type =
    viskores::cont::ArrayHandleTransform<IdArrayType, MaskedIndexFunctor<ValueType>>;
  using permute_type = viskores::cont::ArrayHandlePermutation<transform_type, ArrayType>;

  // resize the output, i.e., output.resize(permute.size());
  viskores::Id permNumValues = permute.GetNumberOfValues();
  viskores::Id outNumValues = output.GetNumberOfValues();
  if (permNumValues > outNumValues)
  {
    output.Allocate(permNumValues);
  }
  else if (permNumValues < outNumValues)
  {
    output.Allocate(permNumValues, viskores::CopyFlag::On);
  } // else the output has already the correct size

  // The following is equivalent to doing the following in serial
  //
  // for (viskores::Id entry = 0; entry < permute.size(); entry++)
  //     output[entry] = input[MaskedIndex(permute[entry])];
  //
  // Apply the MaskedIndex functor to the permute array. ArrayHandleTransform is a fancy viskores array, i.e.,
  // the function is applied on-the-fly without creating a copy of the array.
  transform_type maskedPermuteIndex =
    viskores::cont::make_ArrayHandleTransform(permute, MaskedIndexFunctor<ValueType>());
  // Permute the input array based on the maskedPermuteIndex array. Again, ArrayHandlePermutation is a
  // fancy viskores array so that we do not actually copy any data here
  permute_type permutedInput(maskedPermuteIndex, input);
  // Finally, copy the permuted values to the output array
  viskores::cont::ArrayCopyDevice(permutedInput, output);
} // permuteValues()


// permute value type arrays
template <typename ArrayType>
inline void PermuteArrayWithRawIndex(const ArrayType& input,
                                     IdArrayType& permute,
                                     ArrayType& output)
{ // PermuteArrayWithRawIndex()
  using permute_type = viskores::cont::ArrayHandlePermutation<IdArrayType, ArrayType>;

  // resize the output, i.e., output.resize(permute.size());
  viskores::Id permNumValues = permute.GetNumberOfValues();
  viskores::Id outNumValues = output.GetNumberOfValues();
  if (permNumValues > outNumValues)
  {
    output.Allocate(permNumValues);
  }
  else if (permNumValues < outNumValues)
  {
    output.Allocate(permNumValues, viskores::CopyFlag::On);
  } // else the output has already the correct size

  // The following is equivalent to doing the following in serial
  //
  // for (viskores::Id entry = 0; entry < permute.size(); entry++)
  //     output[entry] = input[permute[entry]];
  //
  // fancy viskores array so that we do not actually copy any data here
  permute_type permutedInput(permute, input);
  // Finally, copy the permuted values to the output array
  viskores::cont::ArrayCopyDevice(permutedInput, output);
} // PermuteArrayWithRawIndex()


// transform functor used in ContourTreeMesh to flag indicies as other when using the CombinedVectorClass
struct MarkOther
{
  VISKORES_EXEC_CONT
  MarkOther() {}

  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id idx) const { return idx | CV_OTHER_FLAG; }
};

// transform functor needed for ScanExclusive calculation. Return 1 if vertex is critical else 0.
struct OneIfCritical
{
  VISKORES_EXEC_CONT
  OneIfCritical() {}

  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id x) const { return x != 1 ? 1 : 0; }
};

// transform functor needed for ScanExclusive calculation in FindSuperAndHyperNodes. Return 1 if vertex is a supernode, else 0.
struct OneIfSupernode
{
  VISKORES_EXEC_CONT
  OneIfSupernode() {}

  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id x) const { return IsSupernode(x) ? 1 : 0; }
};

// transform functor needed for ScanExclusive calculation in FindSuperAndHyperNodes. Return 1 if vertex is a hypernode, else 0.
struct OneIfHypernode
{
  VISKORES_EXEC_CONT
  OneIfHypernode() {}

  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id x) const { return IsHypernode(x) ? 1 : 0; }
};


} // namespace contourtree_augmented
} // worklet
} // viskores

// tail of include guard
#endif
