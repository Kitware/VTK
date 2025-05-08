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

#include <viskores/filter/density_estimate/Entropy.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/source/Tangle.h>


namespace
{
void TestEntropy()
{
  ///// make a data set /////
  viskores::source::Tangle tangle;
  tangle.SetCellDimensions({ 32, 32, 32 });
  viskores::cont::DataSet dataSet = tangle.Execute();

  viskores::filter::density_estimate::Entropy entropyFilter;

  ///// calculate entropy of "tangle" field of the data set /////
  entropyFilter.SetNumberOfBins(50); //set number of bins
  entropyFilter.SetActiveField("tangle");
  viskores::cont::DataSet resultEntropy = entropyFilter.Execute(dataSet);

  ///// get entropy from resultEntropy /////
  viskores::cont::ArrayHandle<viskores::Float64> entropy;
  resultEntropy.GetField("entropy").GetData().AsArrayHandle(entropy);
  viskores::cont::ArrayHandle<viskores::Float64>::ReadPortalType portal = entropy.ReadPortal();
  viskores::Float64 entropyFromFilter = portal.Get(0);

  /////// check if calculating entopry is close enough to ground truth value /////
  // At least in one case, we are seeing a result which is off by more than
  // 0.001 due to floating point precision issues.
  // We are seeing this in the Reduce algorithm of the OpenMP backend, due to
  // operator+ being non-commutative for floating point numbers.
  // Therefore, this variance while high, is still allowed.
  // Instead of increasing the error threshold, we will just check against the
  // two known values.
  VISKORES_TEST_ASSERT(fabs(entropyFromFilter - 4.59093) < 0.001 ||
                         fabs(entropyFromFilter - 4.59798) < 0.001,
                       "Entropy calculation is incorrect");
} // TestFieldEntropy
}

int UnitTestEntropyFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestEntropy, argc, argv);
}
