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

#ifndef viskores_cont_testing_MakeTestDataSet_h
#define viskores_cont_testing_MakeTestDataSet_h

// The relative path of Testing.h is unknown, the only thing that we can assume
// is that it is located in the same directory as this header file. This is
// because the testing directory is reserved for test executables and not
// libraries, the viskores_cont_testing module has to put this file in
// viskores/cont/testlib instead of viskores/cont/testing where you normally would
// expect it.
#include "Testing.h"

#include <viskores/cont/DataSet.h>

#include <viskores/cont/testlib/viskores_cont_testing_export.h>

#include <numeric>

namespace viskores
{
namespace cont
{
namespace testing
{

class VISKORES_CONT_TESTING_EXPORT MakeTestDataSet
{
public:
  // 1D uniform datasets.
  viskores::cont::DataSet Make1DUniformDataSet0();
  viskores::cont::DataSet Make1DUniformDataSet1();
  viskores::cont::DataSet Make1DUniformDataSet2();

  // 1D explicit datasets.
  viskores::cont::DataSet Make1DExplicitDataSet0();

  // 2D uniform datasets.
  viskores::cont::DataSet Make2DUniformDataSet0();
  viskores::cont::DataSet Make2DUniformDataSet1();
  viskores::cont::DataSet Make2DUniformDataSet2();
  viskores::cont::DataSet Make2DUniformDataSet3();

  // 3D uniform datasets.
  viskores::cont::DataSet Make3DUniformDataSet0();
  viskores::cont::DataSet Make3DUniformDataSet1();
  viskores::cont::DataSet Make3DUniformDataSet2();
  viskores::cont::DataSet Make3DUniformDataSet3(viskores::Id3 dims = viskores::Id3(10));
  viskores::cont::DataSet Make3DUniformDataSet4();
  viskores::cont::DataSet Make3DRegularDataSet0();
  viskores::cont::DataSet Make3DRegularDataSet1();

  //2D rectilinear
  viskores::cont::DataSet Make2DRectilinearDataSet0();

  //3D rectilinear
  viskores::cont::DataSet Make3DRectilinearDataSet0();

  // 2D explicit datasets.
  viskores::cont::DataSet Make2DExplicitDataSet0();

  // 3D explicit datasets.
  viskores::cont::DataSet Make3DExplicitDataSet0();
  viskores::cont::DataSet Make3DExplicitDataSet1();
  viskores::cont::DataSet Make3DExplicitDataSet2();
  viskores::cont::DataSet Make3DExplicitDataSet3();
  viskores::cont::DataSet Make3DExplicitDataSet4();
  viskores::cont::DataSet Make3DExplicitDataSet5();
  viskores::cont::DataSet Make3DExplicitDataSet6();
  viskores::cont::DataSet Make3DExplicitDataSet7();
  viskores::cont::DataSet Make3DExplicitDataSet8();
  viskores::cont::DataSet Make3DExplicitDataSetZoo();
  viskores::cont::DataSet Make3DExplicitDataSetPolygonal();
  viskores::cont::DataSet Make3DExplicitDataSetCowNose();
};

} // namespace testing
} // namespace cont
} // namespace viskores

#endif //viskores_cont_testing_MakeTestDataSet_h
