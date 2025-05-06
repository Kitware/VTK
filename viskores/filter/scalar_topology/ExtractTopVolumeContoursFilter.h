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

#ifndef viskores_filter_scalar_topology_ExtractTopVolumeContoursFilter_h
#define viskores_filter_scalar_topology_ExtractTopVolumeContoursFilter_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/scalar_topology/viskores_filter_scalar_topology_export.h>

namespace viskores
{
namespace filter
{
namespace scalar_topology
{

/// \brief Compute branch decompostion from distributed contour tree
class VISKORES_FILTER_SCALAR_TOPOLOGY_EXPORT ExtractTopVolumeContoursFilter
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT ExtractTopVolumeContoursFilter() = default;

  VISKORES_CONT void SetMarchingCubes(const bool& marchingCubes)
  {
    this->IsMarchingCubes = marchingCubes;
  }

  VISKORES_CONT void SetShiftIsovalueByEpsilon(const bool& shiftIsovalueByEps)
  {
    this->IsShiftIsovalueByEpsilon = shiftIsovalueByEps;
  }

  VISKORES_CONT bool GetMarchingCubes() { return this->IsMarchingCubes; }
  VISKORES_CONT bool GetShiftIsovalueByEpsilon() { return this->IsShiftIsovalueByEpsilon; }
  VISKORES_CONT viskores::cont::LogLevel GetTimingsLogLevel() { return this->TimingsLogLevel; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet&) override;
  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& inData) override;

  bool IsMarchingCubes = false;
  bool IsShiftIsovalueByEpsilon = false;

  /// Log level to be used for outputting timing information. Default is viskores::cont::LogLevel::Perf
  viskores::cont::LogLevel TimingsLogLevel = viskores::cont::LogLevel::Perf;
};

} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
