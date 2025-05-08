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

#ifndef _viskores_filter_testing_SuperArchHelper_h_
#define _viskores_filter_testing_SuperArchHelper_h_

#include <algorithm>
#include <viskores/Types.h>

namespace viskores
{
namespace filter
{
namespace testing
{
namespace contourtree_uniform_distributed
{

class SuperArcHelper
{
public:
  std::vector<viskores::Id3> branches;
  void Parse(const std::string& str);
  void Print(std::ostream& out) const;
  void Load(const std::string& filename);
  bool static Compare(const viskores::Id3& LHS, const viskores::Id3& RHS);
};

inline bool SuperArcHelper::Compare(const viskores::Id3& LHS, const viskores::Id3& RHS)
{
  std::vector<viskores::Id> v1{ LHS[0], LHS[1], LHS[2] };
  std::vector<viskores::Id> v2{ RHS[0], RHS[1], RHS[2] };

  return std::lexicographical_compare(v1.begin(), v1.end(), v2.begin(), v2.end());
}

inline void SuperArcHelper::Parse(const std::string& str)
{
  std::stringstream in(str);

  while (!in.eof())
  {
    viskores::Id3 branch;

    in >> branch[0] >> branch[1] >> branch[2];

    if (in.fail())
      break;

    if (std::find(this->branches.begin(), this->branches.end(), branch) == this->branches.end())
    {
      this->branches.push_back(branch);
    }
  }

  std::sort(this->branches.begin(), this->branches.end(), Compare);
}

inline void SuperArcHelper::Print(std::ostream& out) const
{
  for (auto it = std::begin(branches); it != std::end(branches); ++it)
  {
    viskores::Id3 branch(*it);

    out << branch[0] << '\t' << branch[1] << '\t' << branch[2] << std::endl;
  }
}

inline void SuperArcHelper::Load(const std::string& filename)
{
  this->branches.clear();

  std::ifstream in(filename);
  std::stringstream buffer;

  buffer << in.rdbuf();
  this->Parse(buffer.str());
}

}
}
}
} // viskores::filter::testing::contourtree_uniform_distributed

#endif
