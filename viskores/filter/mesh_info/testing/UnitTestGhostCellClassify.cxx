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

#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/CellClassification.h>
#include <viskores/filter/mesh_info/GhostCellClassify.h>

namespace
{

viskores::cont::DataSet MakeUniform(viskores::Id numI, viskores::Id numJ, viskores::Id numK)
{

  viskores::cont::DataSet ds;

  if (numJ == 0 && numK == 0)
    ds = viskores::cont::DataSetBuilderUniform::Create(numI + 1);
  else if (numK == 0)
    ds = viskores::cont::DataSetBuilderUniform::Create(viskores::Id2(numI + 1, numJ + 1));
  else
    ds = viskores::cont::DataSetBuilderUniform::Create(viskores::Id3(numI + 1, numJ + 1, numK + 1));

  return ds;
}

viskores::cont::DataSet MakeRectilinear(viskores::Id numI, viskores::Id numJ, viskores::Id numK)
{
  viskores::cont::DataSet ds;
  std::size_t nx(static_cast<std::size_t>(numI + 1));
  std::size_t ny(static_cast<std::size_t>(numJ + 1));

  std::vector<float> x(nx), y(ny);
  for (std::size_t i = 0; i < nx; i++)
    x[i] = static_cast<float>(i);
  for (std::size_t i = 0; i < ny; i++)
    y[i] = static_cast<float>(i);

  if (numK == 0)
    ds = viskores::cont::DataSetBuilderRectilinear::Create(x, y);
  else
  {
    std::size_t nz(static_cast<std::size_t>(numK + 1));
    std::vector<float> z(nz);
    for (std::size_t i = 0; i < nz; i++)
      z[i] = static_cast<float>(i);
    ds = viskores::cont::DataSetBuilderRectilinear::Create(x, y, z);
  }

  return ds;
}

void TestStructured(std::string GhostFieldName = "")
{
  std::cout << "Testing ghost cells for structured datasets." << std::endl;

  // specify some 2d tests: {numI, numJ, numK, numGhostLayers}.
  std::vector<std::vector<viskores::Id>> tests1D = {
    { 8, 0, 0, 1 }, { 5, 0, 0, 1 }, { 10, 0, 0, 1 }, { 20, 0, 0, 1 }
  };
  std::vector<std::vector<viskores::Id>> tests2D = { { 8, 4, 0, 1 },   { 5, 5, 0, 1 },
                                                     { 10, 10, 0, 1 }, { 10, 5, 0, 1 },
                                                     { 5, 10, 0, 1 },  { 20, 10, 0, 1 },
                                                     { 10, 20, 0, 1 } };
  std::vector<std::vector<viskores::Id>> tests3D = {
    { 8, 8, 10, 1 },   { 5, 5, 5, 1 },    { 10, 10, 10, 1 },    { 10, 5, 10, 1 },  { 5, 10, 10, 1 },
    { 20, 10, 10, 1 }, { 10, 20, 10, 1 }, { 128, 128, 128, 1 }, { 256, 64, 10, 1 }
  };

  std::vector<std::vector<viskores::Id>> tests;

  tests.insert(tests.end(), tests1D.begin(), tests1D.end());
  tests.insert(tests.end(), tests2D.begin(), tests2D.end());
  tests.insert(tests.end(), tests3D.begin(), tests3D.end());

  for (auto& t : tests)
  {
    viskores::Id nx = t[0], ny = t[1], nz = t[2];
    viskores::Id nghost = t[3];
    for (viskores::Id layer = 1; layer <= nghost; layer++)
    {
      viskores::cont::DataSet ds;
      std::vector<std::string> dsTypes = { "uniform", "rectilinear" };

      for (auto& dsType : dsTypes)
      {
        if (dsType == "uniform")
          ds = MakeUniform(nx, ny, nz);
        else if (dsType == "rectilinear")
          ds = MakeRectilinear(nx, ny, nz);

        viskores::filter::mesh_info::GhostCellClassify addGhost;
        if (GhostFieldName != "")
          addGhost.SetGhostCellName(GhostFieldName);
        auto output = addGhost.Execute(ds);

        //Validate the output.
        std::string correctFieldName = viskores::cont::GetGlobalGhostCellFieldName();
        if (GhostFieldName != "")
          correctFieldName = GhostFieldName;
        VISKORES_TEST_ASSERT(output.HasCellField(correctFieldName),
                             "Ghost cells array not found in output");
        viskores::Id numCells = output.GetNumberOfCells();
        auto fieldArray = output.GetCellField(addGhost.GetGhostCellName()).GetData();
        VISKORES_TEST_ASSERT(fieldArray.GetNumberOfValues() == numCells,
                             "Wrong number of values in ghost cell array");

        //Check the number of normal cells.
        viskores::cont::ArrayHandle<viskores::UInt8> ghostArray;
        fieldArray.AsArrayHandle(ghostArray);

        viskores::Id numNormalCells = 0;
        auto portal = ghostArray.ReadPortal();
        constexpr viskores::UInt8 normalCell = viskores::CellClassification::Normal;
        for (viskores::Id i = 0; i < numCells; i++)
          if (portal.Get(i) == normalCell)
            numNormalCells++;

        viskores::Id requiredNumNormalCells = (nx - 2 * layer);
        if (ny > 0)
          requiredNumNormalCells *= (ny - 2 * layer);
        if (nz > 0)
          requiredNumNormalCells *= (nz - 2 * layer);
        VISKORES_TEST_ASSERT(requiredNumNormalCells == numNormalCells,
                             "Wrong number of normal cells");
      }
    }
  }
}

void TestGhostCellClassify()
{
  TestStructured("MyGhostFieldName");
}
}

int UnitTestGhostCellClassify(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestGhostCellClassify, argc, argv);
}
