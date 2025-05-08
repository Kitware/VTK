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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/filter/field_transform/GenerateIds.h>

namespace
{

viskores::cont::UnknownArrayHandle GenerateArray(
  const viskores::filter::field_transform::GenerateIds& self,
  viskores::Id size)
{
  viskores::cont::ArrayHandleIndex indexArray(size);

  if (self.GetUseFloat())
  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> output;
    viskores::cont::ArrayCopy(indexArray, output);
    return output;
  }
  else
  {
    viskores::cont::ArrayHandle<viskores::Id> output;
    viskores::cont::ArrayCopy(indexArray, output);
    return output;
  }
}

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace field_transform
{
viskores::cont::DataSet GenerateIds::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::cont::DataSet output = this->CreateResult(input);

  if (this->GetGeneratePointIds())
  {
    output.AddPointField(this->GetPointFieldName(),
                         GenerateArray(*this, input.GetNumberOfPoints()));
  }

  if (this->GetGenerateCellIds())
  {
    output.AddCellField(this->GetCellFieldName(), GenerateArray(*this, input.GetNumberOfCells()));
  }

  return output;
}
} // namespace field_transform
} // namespace viskores::filter
} // namespace viskores
