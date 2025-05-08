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

#include <viskores/filter/scalar_topology/ContourTreeUniform.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

using viskores::cont::testing::MakeTestDataSet;

class TestContourTreeUniform
{
public:
  //
  // Create a uniform 2D structured cell set as input with values for contours
  //
  void TestContourTree_Mesh2D_DEM_Triangulation() const
  {
    std::cout << "Testing ContourTree_Mesh2D Filter" << std::endl;

    // Create the input uniform cell set with values to contour
    viskores::cont::DataSet inDataSet = MakeTestDataSet().Make2DUniformDataSet1();


    // Convert 2D mesh of values into contour tree, pairs of vertex ids
    viskores::filter::scalar_topology::ContourTreeMesh2D contourTreeMesh2D;
    contourTreeMesh2D.SetActiveField("pointvar");

    // Output data set is pairs of saddle and peak vertex IDs
    viskores::cont::DataSet result = contourTreeMesh2D.Execute(inDataSet);

    viskores::cont::Field Result = result.GetField("saddlePeak");
    viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>> saddlePeak;
    Result.GetData().AsArrayHandle(saddlePeak);

    VISKORES_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 7),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), viskores::make_Pair(0, 12)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), viskores::make_Pair(4, 13)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), viskores::make_Pair(12, 13)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), viskores::make_Pair(12, 18)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), viskores::make_Pair(12, 20)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), viskores::make_Pair(13, 14)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), viskores::make_Pair(13, 19)),
                         "Wrong result for ContourTree filter");
  }

  //
  // Create a uniform 3D structured cell set as input with values for contours
  //
  void TestContourTree_Mesh3D_DEM_Triangulation() const
  {
    std::cout << "Testing ContourTree_Mesh3D Filter" << std::endl;

    // Create the input uniform cell set with values to contour
    viskores::cont::DataSet inDataSet = MakeTestDataSet().Make3DUniformDataSet1();

    // Convert 2D mesh of values into contour tree, pairs of vertex ids
    viskores::filter::scalar_topology::ContourTreeMesh3D contourTreeMesh3D;
    contourTreeMesh3D.SetActiveField("pointvar");

    // Output data set is pairs of saddle and peak vertex IDs
    viskores::cont::DataSet result = contourTreeMesh3D.Execute(inDataSet);

    viskores::cont::Field Result = result.GetField("saddlePeak");
    viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>> saddlePeak;
    Result.GetData().AsArrayHandle(saddlePeak);

    VISKORES_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 9),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), viskores::make_Pair(0, 67)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), viskores::make_Pair(31, 42)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), viskores::make_Pair(42, 43)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), viskores::make_Pair(42, 56)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), viskores::make_Pair(56, 67)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), viskores::make_Pair(56, 92)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), viskores::make_Pair(62, 67)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), viskores::make_Pair(81, 92)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), viskores::make_Pair(92, 93)),
                         "Wrong result for ContourTree filter");
  }

  void operator()() const
  {
    this->TestContourTree_Mesh2D_DEM_Triangulation();
    this->TestContourTree_Mesh3D_DEM_Triangulation();
  }
};
}

int UnitTestContourTreeUniformFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContourTreeUniform(), argc, argv);
}
