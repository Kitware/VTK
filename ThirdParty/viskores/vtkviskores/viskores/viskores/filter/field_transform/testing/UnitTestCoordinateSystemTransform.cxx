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
#include <viskores/filter/field_transform/CylindricalCoordinateTransform.h>
#include <viskores/filter/field_transform/SphericalCoordinateTransform.h>

#include <vector>

namespace
{

enum CoordinateType
{
  CART = 0,
  CYL,
  SPH
};

viskores::cont::DataSet MakeTestDataSet(const CoordinateType& cType)
{
  viskores::cont::DataSet dataSet;

  std::vector<viskores::Vec3f> coordinates;
  const viskores::Id dim = 5;
  if (cType == CART)
  {
    for (viskores::Id j = 0; j < dim; ++j)
    {
      viskores::FloatDefault z =
        static_cast<viskores::FloatDefault>(j) / static_cast<viskores::FloatDefault>(dim - 1);
      for (viskores::Id i = 0; i < dim; ++i)
      {
        viskores::FloatDefault x =
          static_cast<viskores::FloatDefault>(i) / static_cast<viskores::FloatDefault>(dim - 1);
        viskores::FloatDefault y = (x * x + z * z) / 2.0f;
        coordinates.push_back(viskores::make_Vec(x + 0, y + 0, z + 0));
      }
    }
  }
  else if (cType == CYL)
  {
    viskores::FloatDefault R = 1.0f;
    for (viskores::Id j = 0; j < dim; j++)
    {
      viskores::FloatDefault Z =
        static_cast<viskores::FloatDefault>(j) / static_cast<viskores::FloatDefault>(dim - 1);
      for (viskores::Id i = 0; i < dim; i++)
      {
        viskores::FloatDefault Theta = viskores::TwoPif() *
          (static_cast<viskores::FloatDefault>(i) / static_cast<viskores::FloatDefault>(dim - 1));
        coordinates.push_back(viskores::make_Vec(R, Theta, Z));
      }
    }
  }
  else if (cType == SPH)
  {
    //Spherical coordinates have some degenerate cases, so provide some good cases.
    viskores::FloatDefault R = 1.0f;
    viskores::FloatDefault eps = viskores::Epsilon<float>();
    std::vector<viskores::FloatDefault> Thetas = { eps,
                                                   viskores::Pif() / 4.0f,
                                                   viskores::Pif() / 3.0f,
                                                   viskores::Pif() / 2.0f,
                                                   viskores::Pif() - eps };
    std::vector<viskores::FloatDefault> Phis = { eps,
                                                 viskores::TwoPif() / 4.0f,
                                                 viskores::TwoPif() / 3.0f,
                                                 viskores::TwoPif() / 2.0f,
                                                 viskores::TwoPif() - eps };
    for (auto& Theta : Thetas)
      for (auto& Phi : Phis)
        coordinates.push_back(viskores::make_Vec(R, Theta, Phi));
  }

  viskores::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
    viskores::cont::make_CoordinateSystem("coordinates", coordinates, viskores::CopyFlag::On));

  viskores::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (viskores::Id j = 0; j < dim - 1; ++j)
  {
    for (viskores::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(viskores::CELL_SHAPE_QUAD,
                      4,
                      viskores::make_Vec<viskores::Id>(
                        j * dim + i, j * dim + i + 1, (j + 1) * dim + i + 1, (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells(viskores::Id(coordinates.size()));

  dataSet.SetCellSet(cellSet);
  return dataSet;
}

void ValidateCoordTransform(const viskores::cont::DataSet& ds,
                            const viskores::cont::DataSet& dsTrn,
                            const std::vector<bool>& isAngle)
{
  auto points = ds.GetCoordinateSystem().GetDataAsMultiplexer();
  auto pointsTrn = dsTrn.GetCoordinateSystem().GetDataAsMultiplexer();
  VISKORES_TEST_ASSERT(points.GetNumberOfValues() == pointsTrn.GetNumberOfValues(),
                       "Incorrect number of points in point transform");

  auto pointsPortal = points.ReadPortal();
  auto pointsTrnPortal = pointsTrn.ReadPortal();

  for (viskores::Id i = 0; i < points.GetNumberOfValues(); i++)
  {
    viskores::Vec3f p = pointsPortal.Get(i);
    viskores::Vec3f r = pointsTrnPortal.Get(i);
    bool isEqual = true;
    for (viskores::IdComponent j = 0; j < 3; j++)
    {
      if (isAngle[static_cast<std::size_t>(j)])
        isEqual &= (test_equal(p[j], r[j]) || test_equal(p[j] + viskores::TwoPif(), r[j]) ||
                    test_equal(p[j], r[j] + viskores::TwoPif()));
      else
        isEqual &= test_equal(p[j], r[j]);
    }
    VISKORES_TEST_ASSERT(isEqual, "Wrong result for PointTransform worklet");
  }
}
}

void TestCoordinateSystemTransform()
{
  std::cout << "Testing CylindricalCoordinateTransform Filter" << std::endl;

  //Test cartesian to cyl
  viskores::cont::DataSet dsCart = MakeTestDataSet(CART);
  viskores::filter::field_transform::CylindricalCoordinateTransform cylTrn;

  cylTrn.SetCartesianToCylindrical();
  cylTrn.SetUseCoordinateSystemAsField(true);
  viskores::cont::DataSet carToCylDataSet = cylTrn.Execute(dsCart);

  cylTrn.SetCylindricalToCartesian();
  cylTrn.SetUseCoordinateSystemAsField(true);
  viskores::cont::DataSet cylToCarDataSet = cylTrn.Execute(carToCylDataSet);
  ValidateCoordTransform(dsCart, cylToCarDataSet, { false, false, false });

  //Test cyl to cart.
  viskores::cont::DataSet dsCyl = MakeTestDataSet(CYL);
  cylTrn.SetCylindricalToCartesian();
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylToCarDataSet = cylTrn.Execute(dsCyl);

  cylTrn.SetCartesianToCylindrical();
  cylTrn.SetUseCoordinateSystemAsField(true);
  carToCylDataSet = cylTrn.Execute(cylToCarDataSet);
  ValidateCoordTransform(dsCyl, carToCylDataSet, { false, true, false });

  std::cout << "Testing SphericalCoordinateTransform Filter" << std::endl;

  viskores::filter::field_transform::SphericalCoordinateTransform sphTrn;
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetCartesianToSpherical();
  viskores::cont::DataSet carToSphDataSet = sphTrn.Execute(dsCart);

  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetSphericalToCartesian();
  viskores::cont::DataSet sphToCarDataSet = sphTrn.Execute(carToSphDataSet);
  ValidateCoordTransform(dsCart, sphToCarDataSet, { false, true, true });

  viskores::cont::DataSet dsSph = MakeTestDataSet(SPH);
  sphTrn.SetSphericalToCartesian();
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphToCarDataSet = sphTrn.Execute(dsSph);

  sphTrn.SetCartesianToSpherical();
  sphTrn.SetUseCoordinateSystemAsField(true);
  carToSphDataSet = sphTrn.Execute(sphToCarDataSet);
  ValidateCoordTransform(dsSph, carToSphDataSet, { false, true, true });
}


int UnitTestCoordinateSystemTransform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCoordinateSystemTransform, argc, argv);
}
