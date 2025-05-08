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

#ifndef _viskores_filter_testing_VolumeHelper_h_
#define _viskores_filter_testing_VolumeHelper_h_

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

class VolumeHelper
{
public:
  typedef std::tuple<viskores::Id, viskores::Id, viskores::Id, viskores::Id, viskores::Id> Volume;
  std::vector<Volume> volumes;
  void Parse(const std::string& str);
  void Load(const std::string& filename);
  void Print(std::ostream& out) const;
};

inline void VolumeHelper::Parse(const std::string& str)
{
  std::stringstream in(str);
  std::string temp;

  in >> temp;
  in >> temp;
  in >> temp;

  while (!in.eof())
  {
    std::vector<viskores::Id> ids;

    for (int i = 0; i < 5; ++i)
    {
      viskores::Id id;

      in >> temp;
      in >> id;
      ids.push_back(id);
    }

    if (in.fail())
      break;

    Volume vol{ ids[0], ids[1], ids[2], ids[3], ids[4] };
    if (std::find(this->volumes.begin(), this->volumes.end(), vol) == this->volumes.end())
    {
      this->volumes.push_back(vol);
    }
  }

  std::sort(this->volumes.begin(), this->volumes.end());
}

inline void VolumeHelper::Print(std::ostream& out) const
{
  out << "============" << std::endl << "Contour Tree" << std::endl;

  for (auto it = std::begin(this->volumes); it != std::end(this->volumes); ++it)
  {
    Volume volume(*it);

    out << "H: " << std::setw(VOLUME_PRINT_WIDTH) << std::get<0>(volume)
        << " L: " << std::setw(VOLUME_PRINT_WIDTH) << std::get<1>(volume)
        << " VH: " << std::setw(VOLUME_PRINT_WIDTH) << std::get<2>(volume)
        << " VR: " << std::setw(VOLUME_PRINT_WIDTH) << std::get<3>(volume)
        << " VL: " << std::setw(VOLUME_PRINT_WIDTH) << std::get<4>(volume) << std::endl;
  }
}

inline void VolumeHelper::Load(const std::string& filename)
{
  this->volumes.clear();

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
