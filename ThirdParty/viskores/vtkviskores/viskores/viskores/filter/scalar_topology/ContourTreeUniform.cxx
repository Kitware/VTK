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
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/scalar_topology/ContourTreeUniform.h>
#include <viskores/filter/scalar_topology/worklet/ContourTreeUniform.h>

namespace viskores
{
namespace filter
{
namespace scalar_topology
{
//-----------------------------------------------------------------------------
ContourTreeMesh2D::ContourTreeMesh2D()
{
  this->SetOutputFieldName("saddlePeak");
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet ContourTreeMesh2D::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("ContourTreeMesh2D expects point field input.");
  }

  // Collect sizing information from the dataset
  viskores::cont::CellSetStructured<2> cellSet;
  input.GetCellSet().AsCellSet(cellSet);

  viskores::Id2 pointDimensions = cellSet.GetPointDimensions();
  viskores::Id nRows = pointDimensions[0];
  viskores::Id nCols = pointDimensions[1];

  viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>> saddlePeak;

  auto resolveType = [&](const auto& concrete)
  {
    viskores::worklet::ContourTreeMesh2D worklet;
    worklet.Run(concrete, nRows, nCols, saddlePeak);
  };
  this->CastAndCallScalarField(field, resolveType);

  return this->CreateResultField(input,
                                 this->GetOutputFieldName(),
                                 viskores::cont::Field::Association::WholeDataSet,
                                 saddlePeak);
}

//-----------------------------------------------------------------------------
ContourTreeMesh3D::ContourTreeMesh3D()
{
  this->SetOutputFieldName("saddlePeak");
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet ContourTreeMesh3D::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  // Collect sizing information from the dataset
  viskores::cont::CellSetStructured<3> cellSet;
  input.GetCellSet().AsCellSet(cellSet);

  viskores::Id3 pointDimensions = cellSet.GetPointDimensions();
  viskores::Id nRows = pointDimensions[0];
  viskores::Id nCols = pointDimensions[1];
  viskores::Id nSlices = pointDimensions[2];

  viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>> saddlePeak;

  auto resolveType = [&](const auto& concrete)
  {
    viskores::worklet::ContourTreeMesh3D worklet;
    worklet.Run(concrete, nRows, nCols, nSlices, saddlePeak);
  };
  this->CastAndCallScalarField(field, resolveType);

  return this->CreateResultField(input,
                                 this->GetOutputFieldName(),
                                 viskores::cont::Field::Association::WholeDataSet,
                                 saddlePeak);
}
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
