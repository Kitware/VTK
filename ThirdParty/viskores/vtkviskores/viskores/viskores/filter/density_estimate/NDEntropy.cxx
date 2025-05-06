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
#include <viskores/filter/density_estimate/NDEntropy.h>
#include <viskores/filter/density_estimate/worklet/NDimsEntropy.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
void NDEntropy::AddFieldAndBin(const std::string& fieldName, viskores::Id numOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumOfBins.push_back(numOfBins);
}

VISKORES_CONT viskores::cont::DataSet NDEntropy::DoExecute(const viskores::cont::DataSet& inData)
{
  viskores::worklet::NDimsEntropy ndEntropy;
  ndEntropy.SetNumOfDataPoints(inData.GetField(0).GetNumberOfValues());

  // Add field one by one
  // (By using AddFieldAndBin(), the length of FieldNames and NumOfBins must be the same)
  for (size_t i = 0; i < FieldNames.size(); i++)
  {
    ndEntropy.AddField(inData.GetField(FieldNames[i]).GetData(), NumOfBins[i]);
  }

  // Run worklet to calculate multi-variate entropy
  viskores::cont::ArrayHandle<viskores::Float64> entropyHandle;
  viskores::Float64 entropy = ndEntropy.Run();

  entropyHandle.Allocate(1);
  entropyHandle.WritePortal().Set(0, entropy);

  viskores::cont::DataSet outputData;
  outputData.AddField(
    { "Entropy", viskores::cont::Field::Association::WholeDataSet, entropyHandle });
  // The output is a "summary" of the input, no need to map fields
  return outputData;
}
} // namespace density_estimate
} // namespace filter
} // namespace viskores
