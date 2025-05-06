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

#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/density_estimate/NDHistogram.h>

namespace
{

viskores::cont::DataSet MakeTestDataSet()
{
  viskores::cont::DataSet dataSet;

  const int nVerts = 100;

  viskores::Float32 fieldA[nVerts] = { 8,  10, 9,  8,  14, 11, 12, 9,  19, 7, 8,  11, 7,  10, 11,
                                       11, 11, 6,  8,  8,  7,  15, 9,  7,  8, 10, 9,  10, 10, 12,
                                       7,  6,  14, 10, 14, 10, 7,  11, 13, 9, 13, 11, 10, 10, 12,
                                       12, 7,  12, 10, 11, 12, 8,  13, 9,  5, 12, 11, 9,  5,  9,
                                       12, 9,  6,  10, 11, 9,  9,  11, 9,  7, 7,  18, 16, 13, 12,
                                       8,  10, 11, 9,  8,  17, 3,  15, 15, 9, 10, 10, 8,  10, 9,
                                       7,  9,  8,  10, 13, 9,  7,  11, 7,  10 };

  viskores::Float32 fieldB[nVerts] = { 24, 19, 28, 19, 25, 28, 25, 22, 27, 26, 35, 26, 30, 28, 24,
                                       23, 21, 31, 20, 11, 21, 22, 14, 25, 20, 24, 24, 21, 24, 29,
                                       26, 21, 32, 29, 23, 28, 31, 25, 23, 30, 18, 24, 22, 25, 33,
                                       24, 22, 23, 21, 17, 20, 28, 30, 18, 20, 32, 25, 24, 32, 15,
                                       27, 24, 27, 19, 30, 27, 17, 24, 29, 23, 22, 19, 24, 19, 28,
                                       24, 25, 24, 25, 30, 24, 31, 30, 27, 25, 25, 25, 15, 29, 23,
                                       29, 29, 21, 25, 35, 24, 28, 10, 31, 23 };

  viskores::Float32 fieldC[nVerts] = { 3,  1, 4,  6, 5, 4, 8,  7, 2, 9, 2, 0, 0,  4,  3, 2, 5,
                                       2,  3, 6,  3, 8, 3, 4,  3, 3, 2, 7, 2, 10, 9,  6, 1, 1,
                                       4,  7, 3,  3, 1, 4, 4,  3, 9, 4, 4, 7, 3,  2,  4, 7, 3,
                                       3,  2, 10, 1, 6, 2, 2,  3, 8, 3, 3, 6, 9,  4,  1, 4, 3,
                                       16, 7, 0,  1, 8, 7, 13, 3, 5, 0, 3, 8, 10, 3,  5, 5, 1,
                                       5,  2, 1,  3, 2, 5, 3,  4, 3, 3, 3, 3, 1,  13, 2 };

  // Set point scalars
  dataSet.AddField(viskores::cont::make_Field(
    "fieldA", viskores::cont::Field::Association::Points, fieldA, nVerts, viskores::CopyFlag::On));
  dataSet.AddField(viskores::cont::make_Field(
    "fieldB", viskores::cont::Field::Association::Points, fieldB, nVerts, viskores::CopyFlag::On));
  dataSet.AddField(viskores::cont::make_Field(
    "fieldC", viskores::cont::Field::Association::Points, fieldC, nVerts, viskores::CopyFlag::On));

  return dataSet;
}

void RunTest()
{
  viskores::cont::DataSet ds = MakeTestDataSet();

  viskores::filter::density_estimate::NDHistogram ndHistFilter;

  ndHistFilter.AddFieldAndBin("fieldA", 4);
  ndHistFilter.AddFieldAndBin("fieldB", 4);
  ndHistFilter.AddFieldAndBin("fieldC", 4);

  //The return data set contains fieldNames.size() + 1 fields
  //The first "fieldNames.size()"" fields are the binId arrays for inputs field
  //And their order and field names are the same as the order and name in fieldNames
  //The name of last fields in the dataset is "Frequency"
  //This field contains all the frequencies of the N-Dims histogram
  //The result histogram is stored in sparse representation
  //(Do not store and return zero frequency bins)
  //All fields in return dataset must have the same length
  //So, e.g. (FieldA[i], FieldB[i], FieldC[i], Frequency[i] ) is a bin in the histogram
  //First three numbers are binID for FieldA, FieldB, FieldC
  //Frequency[i] is frequency for this bin
  viskores::cont::DataSet outputData = ndHistFilter.Execute(ds);

  // Ground truth ND histogram
  viskores::Id gtNonSparseBins = 33;
  viskores::Id gtIdx0[33] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                              2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3 };
  viskores::Id gtIdx1[33] = { 1, 1, 2, 3, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3,
                              0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 1, 1, 2, 2, 2, 3 };
  viskores::Id gtIdx2[33] = { 0, 1, 1, 0, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 3,
                              0, 0, 1, 0, 1, 2, 3, 0, 1, 2, 0, 2, 0, 1, 2, 1 };
  viskores::Id gtFreq[33] = { 1, 1, 1, 3,  2, 1, 1, 6, 6, 3, 17, 8, 2, 6, 2, 1, 2,
                              1, 1, 4, 11, 4, 1, 1, 3, 3, 1, 1,  1, 1, 1, 2, 1 };

  // Check result
  viskores::Id nonSparseBins = outputData.GetField(0).GetNumberOfValues();
  VISKORES_TEST_ASSERT(nonSparseBins == gtNonSparseBins, "Incorrect ND-histogram Filter results");

  viskores::cont::ArrayHandle<viskores::Id> binId0;
  outputData.GetField("fieldA").GetData().AsArrayHandle(binId0);
  viskores::cont::ArrayHandle<viskores::Id> binId1;
  outputData.GetField("fieldB").GetData().AsArrayHandle(binId1);
  viskores::cont::ArrayHandle<viskores::Id> binId2;
  outputData.GetField("fieldC").GetData().AsArrayHandle(binId2);
  viskores::cont::ArrayHandle<viskores::Id> freqs;
  outputData.GetField("Frequency").GetData().AsArrayHandle(freqs);
  for (int i = 0; i < nonSparseBins; i++)
  {
    viskores::Id idx0 = binId0.WritePortal().Get(i);
    viskores::Id idx1 = binId1.WritePortal().Get(i);
    viskores::Id idx2 = binId2.WritePortal().Get(i);
    viskores::Id f = freqs.WritePortal().Get(i);
    VISKORES_TEST_ASSERT(idx0 == gtIdx0[i] && idx1 == gtIdx1[i] && idx2 == gtIdx2[i] &&
                           f == gtFreq[i],
                         "Incorrect ND-histogram Filter results");
  }
}

} // anonymous namespace

int UnitTestNDHistogramFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTest, argc, argv);
}
