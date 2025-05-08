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

// NOTE: To save test time, we reduced test coverage on September 2th 2024. The
// tests still running should be sufficient to uncovoer any issues due to Viskores
// changes. However, if we continue development of the contour tree algorithm,
// we should re-enable the additional tests as they have revealed problems in
// the past.
// #define ENABLE_ADDITIONAL_TESTS

#include "TestingContourTreeUniformDistributedFilter.h"

namespace
{
using viskores::filter::testing::contourtree_uniform_distributed::TestContourTreeFile;
using viskores::filter::testing::contourtree_uniform_distributed::TestContourTreePresimplification;
using viskores::filter::testing::contourtree_uniform_distributed::
  TestContourTreeUniformDistributed5x6x7;
using viskores::filter::testing::contourtree_uniform_distributed::
  TestContourTreeUniformDistributed8x9;

class TestContourTreeUniformDistributedFilter
{
public:
  void operator()() const
  {
    using viskores::cont::testing::Testing;
#ifdef ENABLE_ADDITIONAL_TESTS
    TestContourTreeUniformDistributed8x9(2);
    // TestContourTreeUniformDistributed8x9(3); CRASH???
    TestContourTreeUniformDistributed8x9(4);
#endif
    TestContourTreeUniformDistributed8x9(8);
    TestContourTreeUniformDistributed8x9(16);

#ifdef ENABLE_ADDITIONAL_TESTS
    TestContourTreeUniformDistributed5x6x7(2, false);
    TestContourTreeUniformDistributed5x6x7(4, false);
#endif
    TestContourTreeUniformDistributed5x6x7(8, false);
    TestContourTreeUniformDistributed5x6x7(16, false);
#ifdef ENABLE_ADDITIONAL_TESTS
    TestContourTreeUniformDistributed5x6x7(2, true);
    TestContourTreeUniformDistributed5x6x7(4, true);
#endif
    TestContourTreeUniformDistributed5x6x7(8, true);
    TestContourTreeUniformDistributed5x6x7(16, true);
#ifdef ENABLE_ADDITIONAL_TESTS
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.ct_txt"),
                        2);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.ct_txt"),
                        4);
#endif
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.ct_txt"),
                        8);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.ct_txt"),
                        16);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.augment_hierarchical_tree.ct_txt"),
                        2,
                        false,
                        0,
                        1,
                        true,
                        false);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.augment_hierarchical_tree.ct_txt"),
                        4,
                        false,
                        0,
                        1,
                        true,
                        false);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.augment_hierarchical_tree.ct_txt"),
                        4,
                        false,
                        0,
                        1,
                        true,
                        false,
                        false);

    // tests for contour tree presimplification on 2D vanc dataset
#ifdef ENABLE_ADDITIONAL_TESTS
    TestContourTreePresimplification(
      "vanc", // dataset name
      "var",  // field name
      viskores::cont::testing::Testing::RegressionImagePath(
        "vanc.presimplification.ct_txt"),                                 // ground truth file name
      2,                                                                  // nBlocks
      viskores::cont::testing::Testing::DataPath("rectilinear/vanc.vtk"), // dataset file path
      1                                                                   // presimplifyThreshold
    );
#endif
    TestContourTreePresimplification(
      "vanc",
      "var",
      viskores::cont::testing::Testing::RegressionImagePath("vanc.presimplification.ct_txt"),
      4,
      viskores::cont::testing::Testing::DataPath("rectilinear/vanc.vtk"),
      1);
#ifdef ENABLE_ADDITIONAL_TESTS
    TestContourTreePresimplification(
      "vanc",
      "var",
      viskores::cont::testing::Testing::RegressionImagePath("vanc.presimplification.ct_txt"),
      2,
      viskores::cont::testing::Testing::DataPath("rectilinear/vanc.vtk"),
      4);
#endif
    TestContourTreePresimplification(
      "vanc",
      "var",
      viskores::cont::testing::Testing::RegressionImagePath("vanc.presimplification.ct_txt"),
      4,
      viskores::cont::testing::Testing::DataPath("rectilinear/vanc.vtk"),
      4);

    // test for contour tree presimplification on 3D 5x6x7 dataset
#ifdef ENABLE_ADDITIONAL_TESTS
    TestContourTreePresimplification(
      "5x6x7",    // dataset name
      "pointvar", // field name
      viskores::cont::testing::Testing::RegressionImagePath(
        "5x6x7.presimplification.ct_txt"),                                // ground truth file name
      2,                                                                  // nBlocks
      viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet4(), // dataset preset,
      2                                                                   // presimplifyThreshold
    );
    TestContourTreePresimplification(
      "5x6x7",
      "pointvar",
      viskores::cont::testing::Testing::RegressionImagePath("5x6x7.presimplification.ct_txt"),
      4,
      viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet4(),
      2);
#endif
    TestContourTreePresimplification(
      "5x6x7",
      "pointvar",
      viskores::cont::testing::Testing::RegressionImagePath("5x6x7.presimplification.ct_txt"),
      8,
      viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet4(),
      2);
#ifdef ENABLE_ADDITIONAL_TESTS
    TestContourTreePresimplification(
      "5x6x7",
      "pointvar",
      viskores::cont::testing::Testing::RegressionImagePath("5x6x7.presimplification.ct_txt"),
      2,
      viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet4(),
      4);
    TestContourTreePresimplification(
      "5x6x7",
      "pointvar",
      viskores::cont::testing::Testing::RegressionImagePath("5x6x7.presimplification.ct_txt"),
      4,
      viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet4(),
      4);
#endif
    TestContourTreePresimplification(
      "5x6x7",
      "pointvar",
      viskores::cont::testing::Testing::RegressionImagePath("5x6x7.presimplification.ct_txt"),
      8,
      viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet4(),
      4);
  }
};
}

int UnitTestContourTreeUniformDistributedFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(
    TestContourTreeUniformDistributedFilter(), argc, argv);
}
