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


#ifndef _BRANCHCOMPILER_H_
#define _BRANCHCOMPILER_H_

#include <iomanip>
#include <iostream>
#include <vector>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

typedef std::tuple<viskores::Id, viskores::Id> Branch;

class BranchCompiler
{ // class BranchCompiler
public:
  std::vector<Branch> branches;

  void Parse(std::istream& in);
  void Print(std::ostream& out) const;
  void Load(const std::string& filename);
}; // class BranchCompiler

inline void BranchCompiler::Parse(std::istream& in)
{ // Parse()
  // variables tracking the best & worst so far for this extent
  viskores::Id currentBranch = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
  // this is slightly tricky, since we don't know the range of the data type
  // yet, but we can initialize to 0 for both floats and integers, then test on
  // current branch
  viskores::Float64 highValue = 0;
  viskores::Float64 lowValue = 0;
  viskores::Id highEnd = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
  viskores::Id lowEnd = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

  // values to read in
  viskores::Id nextBranch;
  viskores::Float64 nextValue;
  viskores::Id nextSupernode;

  while (true)
  { // until stream goes bad
    // read the next triple
    in >> nextBranch >> nextValue >> nextSupernode;

    // test in the middle before processing
    if (in.eof())
      break;

    // test to see if the branch is different from the current one
    if (nextBranch != currentBranch)
    { // new branch
      // special test for initial one
      if (!viskores::worklet::contourtree_augmented::NoSuchElement(currentBranch))
      {
        this->branches.push_back(std::make_tuple(highEnd, lowEnd));
      }

      // set the high & low value ends to this one
      highValue = nextValue;
      lowValue = nextValue;
      highEnd = nextSupernode;
      lowEnd = nextSupernode;

      // and reset the branch ID
      currentBranch = nextBranch;
    } // new branch
    else
    { // existing branch
      // test value with simulation of simplicity
      if ((nextValue > highValue) || ((nextValue == highValue) && (nextSupernode > highEnd)))
      { // new high end
        highEnd = nextSupernode;
        highValue = nextValue;
      } // new high end
      // test value with simulation of simplicity
      else if ((nextValue < lowValue) || ((nextValue == lowValue) && (nextSupernode < lowEnd)))
      { // new low end
        lowEnd = nextSupernode;
        lowValue = nextValue;
      } // new low end
    }   // existing branch
  }     // until stream goes bad

  this->branches.push_back(std::make_tuple(highEnd, lowEnd));
  std::sort(this->branches.begin(), this->branches.end());
} // Parse()

inline void BranchCompiler::Print(std::ostream& out) const
{
  for (auto it = std::begin(this->branches); it != std::end(this->branches); ++it)
  {
    Branch branch(*it);

    out << std::setw(12) << std::get<0>(branch) << std::setw(14) << std::get<1>(branch)
        << std::endl;
  }
}

inline void BranchCompiler::Load(const std::string& filename)
{
  this->branches.clear();

  std::ifstream in(filename);

  while (!in.eof())
  {

    viskores::Id highEnd, lowEnd;

    in >> highEnd >> lowEnd;

    if (in.fail())
      break;

    this->branches.push_back(std::make_tuple(highEnd, lowEnd));
  }

  std::sort(this->branches.begin(), this->branches.end());
}

} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
