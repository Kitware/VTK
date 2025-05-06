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
//=======================================================================================
//
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// HierarchicalAugmenter.h
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_distributed_hierarchical_augmenter_hierarchical_augmenter_in_out_data_h
#define viskores_worklet_contourtree_distributed_hierarchical_augmenter_hierarchical_augmenter_in_out_data_h


#include <iostream> // std::cout
#include <sstream>  // std::stringstrea
#include <string>   // std::string


namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_augmenter
{

/// Class for storing input or output data for the HierarchicalAugmenter. The  data is factored out in this class to
/// allow for modular code and easue reuse, since the input and output require the same types of array parameters
template <typename FieldType>
class HierarchicalAugmenterInOutData
{ // class HierarchicalAugmenter
public:
  viskores::worklet::contourtree_augmented::IdArrayType GlobalRegularIds;
  viskores::cont::ArrayHandle<FieldType> DataValues;
  viskores::worklet::contourtree_augmented::IdArrayType SupernodeIds;
  viskores::worklet::contourtree_augmented::IdArrayType Superparents;
  viskores::worklet::contourtree_augmented::IdArrayType SuperparentRounds;
  viskores::worklet::contourtree_augmented::IdArrayType WhichRounds;

  /// empty constructor
  HierarchicalAugmenterInOutData() {}

  /// main constructor
  HierarchicalAugmenterInOutData(
    viskores::worklet::contourtree_augmented::IdArrayType& globalRegularIds,
    viskores::cont::ArrayHandle<FieldType>& dataValues,
    viskores::worklet::contourtree_augmented::IdArrayType& supernodeIds,
    viskores::worklet::contourtree_augmented::IdArrayType& superparents,
    viskores::worklet::contourtree_augmented::IdArrayType& superparentRounds,
    viskores::worklet::contourtree_augmented::IdArrayType& whichRounds)
    : GlobalRegularIds(globalRegularIds)
    , DataValues(dataValues)
    , SupernodeIds(supernodeIds)
    , Superparents(superparents)
    , SuperparentRounds(superparentRounds)
    , WhichRounds(whichRounds)
  {
  }

  /// Destructor
  ~HierarchicalAugmenterInOutData() = default;

  /// Print contents fo this objects
  std::string DebugPrint(std::string message, const char* fileName, long lineNum);

}; // class HierarchicalAugmenterInOutData

template <typename FieldType>
std::string HierarchicalAugmenterInOutData<FieldType>::DebugPrint(std::string message,
                                                                  const char* fileName,
                                                                  long lineNum)
{
  // DebugPrint()
  std::stringstream resultStream;
  resultStream << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << std::endl;
  resultStream << message << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Global Regular Ids", this->GlobalRegularIds, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintValues(
    "Data Values", this->DataValues, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Supernode Ids", this->SupernodeIds, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Superparents", this->Superparents, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Superparent Rounds", this->SuperparentRounds, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Which Rounds", this->WhichRounds, -1, resultStream);
  return resultStream.str();
}

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

namespace viskoresdiy
{

// Struct to serialize ContourTreeMesh objects (i.e., load/save) needed in parralle for DIY
template <typename FieldType>
struct Serialization<viskores::worklet::contourtree_distributed::hierarchical_augmenter::
                       HierarchicalAugmenterInOutData<FieldType>>
{
  static void save(viskoresdiy::BinaryBuffer& bb,
                   const viskores::worklet::contourtree_distributed::hierarchical_augmenter::
                     HierarchicalAugmenterInOutData<FieldType>& ha)
  {
    viskoresdiy::save(bb, ha.GlobalRegularIds);
    viskoresdiy::save(bb, ha.DataValues);
    viskoresdiy::save(bb, ha.SupernodeIds);
    viskoresdiy::save(bb, ha.Superparents);
    viskoresdiy::save(bb, ha.SuperparentRounds);
    viskoresdiy::save(bb, ha.WhichRounds);
  }

  static void load(viskoresdiy::BinaryBuffer& bb,
                   viskores::worklet::contourtree_distributed::hierarchical_augmenter::
                     HierarchicalAugmenterInOutData<FieldType>& ha)
  {
    viskoresdiy::load(bb, ha.GlobalRegularIds);
    viskoresdiy::load(bb, ha.DataValues);
    viskoresdiy::load(bb, ha.SupernodeIds);
    viskoresdiy::load(bb, ha.Superparents);
    viskoresdiy::load(bb, ha.SuperparentRounds);
    viskoresdiy::load(bb, ha.WhichRounds);
  }
};

} // namespace mangled_viskoresdiy_namespace

#endif
