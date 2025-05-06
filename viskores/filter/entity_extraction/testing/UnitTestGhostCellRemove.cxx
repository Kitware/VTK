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
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/entity_extraction/GhostCellRemove.h>

namespace
{

viskores::cont::ArrayHandle<viskores::UInt8> StructuredGhostCellArray(viskores::Id nx,
                                                                      viskores::Id ny,
                                                                      viskores::Id nz,
                                                                      int numLayers,
                                                                      bool addMidGhost = false)
{
  viskores::Id numCells = nx * ny;
  if (nz > 0)
    numCells *= nz;

  constexpr viskores::UInt8 normalCell = viskores::CellClassification::Normal;
  constexpr viskores::UInt8 duplicateCell = viskores::CellClassification::Ghost;

  viskores::cont::ArrayHandle<viskores::UInt8> ghosts;
  ghosts.Allocate(numCells);
  auto portal = ghosts.WritePortal();
  for (viskores::Id i = 0; i < numCells; i++)
  {
    if (numLayers == 0)
      portal.Set(i, normalCell);
    else
      portal.Set(i, duplicateCell);
  }

  if (numLayers > 0)
  {
    //2D case
    if (nz == 0)
    {
      for (viskores::Id i = numLayers; i < nx - numLayers; i++)
        for (viskores::Id j = numLayers; j < ny - numLayers; j++)
          portal.Set(j * nx + i, normalCell);
    }
    else
    {
      for (viskores::Id i = numLayers; i < nx - numLayers; i++)
        for (viskores::Id j = numLayers; j < ny - numLayers; j++)
          for (viskores::Id k = numLayers; k < nz - numLayers; k++)
            portal.Set(k * nx * ny + j * nx + i, normalCell);
    }
  }

  if (addMidGhost)
  {
    if (nz == 0)
    {
      viskores::Id mi = numLayers + (nx - numLayers) / 2;
      viskores::Id mj = numLayers + (ny - numLayers) / 2;
      portal.Set(mj * nx + mi, duplicateCell);
    }
    else
    {
      viskores::Id mi = numLayers + (nx - numLayers) / 2;
      viskores::Id mj = numLayers + (ny - numLayers) / 2;
      viskores::Id mk = numLayers + (nz - numLayers) / 2;
      portal.Set(mk * nx * ny + mj * nx + mi, duplicateCell);
    }
  }
  return ghosts;
}

viskores::cont::DataSet MakeUniform(viskores::Id numI,
                                    viskores::Id numJ,
                                    viskores::Id numK,
                                    int numLayers,
                                    std::string& ghostName,
                                    bool addMidGhost = false)
{
  viskores::cont::DataSet ds;

  if (numK == 0)
    ds = viskores::cont::DataSetBuilderUniform::Create(viskores::Id2(numI + 1, numJ + 1));
  else
    ds = viskores::cont::DataSetBuilderUniform::Create(viskores::Id3(numI + 1, numJ + 1, numK + 1));
  auto ghosts = StructuredGhostCellArray(numI, numJ, numK, numLayers, addMidGhost);

  if (ghostName == "default")
  {
    ds.SetGhostCellField(ghosts);
  }
  else
  {
    ds.SetGhostCellField(ghostName, ghosts);
  }
  return ds;
}

viskores::cont::DataSet MakeRectilinear(viskores::Id numI,
                                        viskores::Id numJ,
                                        viskores::Id numK,
                                        int numLayers,
                                        std::string& ghostName,
                                        bool addMidGhost = false)
{
  viskores::cont::DataSet ds;
  auto nx(static_cast<std::size_t>(numI + 1));
  auto ny(static_cast<std::size_t>(numJ + 1));

  std::vector<float> x(nx), y(ny);
  for (std::size_t i = 0; i < nx; i++)
    x[i] = static_cast<float>(i);
  for (std::size_t i = 0; i < ny; i++)
    y[i] = static_cast<float>(i);

  if (numK == 0)
    ds = viskores::cont::DataSetBuilderRectilinear::Create(x, y);
  else
  {
    auto nz(static_cast<std::size_t>(numK + 1));
    std::vector<float> z(nz);
    for (std::size_t i = 0; i < nz; i++)
      z[i] = static_cast<float>(i);
    ds = viskores::cont::DataSetBuilderRectilinear::Create(x, y, z);
  }

  auto ghosts = StructuredGhostCellArray(numI, numJ, numK, numLayers, addMidGhost);

  if (ghostName == "default")
  {
    ds.SetGhostCellField(ghosts);
  }
  else
  {
    ds.SetGhostCellField(ghostName, ghosts);
  }

  return ds;
}

template <class CellSetType, viskores::IdComponent NDIM>
static void MakeExplicitCells(const CellSetType& cellSet,
                              viskores::Vec<viskores::Id, NDIM>& dims,
                              viskores::cont::ArrayHandle<viskores::IdComponent>& numIndices,
                              viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
                              viskores::cont::ArrayHandle<viskores::Id>& conn)
{
  using Connectivity = viskores::internal::ConnectivityStructuredInternals<NDIM>;

  viskores::Id nCells = cellSet.GetNumberOfCells();
  viskores::Id connLen = (NDIM == 2 ? nCells * 4 : nCells * 8);

  conn.Allocate(connLen);
  shapes.Allocate(nCells);
  numIndices.Allocate(nCells);

  Connectivity structured;
  structured.SetPointDimensions(dims);

  viskores::Id idx = 0;
  for (viskores::Id i = 0; i < nCells; i++)
  {
    auto ptIds = structured.GetPointsOfCell(i);
    for (viskores::IdComponent j = 0; j < NDIM; j++, idx++)
      conn.WritePortal().Set(idx, ptIds[j]);

    shapes.WritePortal().Set(
      i, (NDIM == 4 ? viskores::CELL_SHAPE_QUAD : viskores::CELL_SHAPE_HEXAHEDRON));
    numIndices.WritePortal().Set(i, NDIM);
  }
}

viskores::cont::DataSet MakeExplicit(viskores::Id numI,
                                     viskores::Id numJ,
                                     viskores::Id numK,
                                     int numLayers,
                                     std::string& ghostName)
{
  using CoordType = viskores::Vec3f_32;

  viskores::cont::DataSet dsUniform = MakeUniform(numI, numJ, numK, numLayers, ghostName);

  auto coordData = dsUniform.GetCoordinateSystem(0).GetDataAsMultiplexer();
  viskores::Id numPts = coordData.GetNumberOfValues();
  viskores::cont::ArrayHandle<CoordType> explCoords;

  explCoords.Allocate(numPts);
  auto explPortal = explCoords.WritePortal();
  auto cp = coordData.ReadPortal();
  for (viskores::Id i = 0; i < numPts; i++)
    explPortal.Set(i, cp.Get(i));

  viskores::cont::UnknownCellSet cellSet = dsUniform.GetCellSet();
  viskores::cont::ArrayHandle<viskores::Id> conn;
  viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
  viskores::cont::ArrayHandle<viskores::UInt8> shapes;
  viskores::cont::DataSet ds;

  if (cellSet.IsType<viskores::cont::CellSetStructured<2>>())
  {
    viskores::Id2 dims(numI, numJ);
    MakeExplicitCells(
      cellSet.AsCellSet<viskores::cont::CellSetStructured<2>>(), dims, numIndices, shapes, conn);
    ds = viskores::cont::DataSetBuilderExplicit::Create(
      explCoords, viskores::CellShapeTagQuad(), 4, conn, "coordinates");
  }
  else if (cellSet.IsType<viskores::cont::CellSetStructured<3>>())
  {
    viskores::Id3 dims(numI, numJ, numK);
    MakeExplicitCells(
      cellSet.AsCellSet<viskores::cont::CellSetStructured<3>>(), dims, numIndices, shapes, conn);
    ds = viskores::cont::DataSetBuilderExplicit::Create(
      explCoords, viskores::CellShapeTagHexahedron(), 8, conn, "coordinates");
  }

  auto ghosts = StructuredGhostCellArray(numI, numJ, numK, numLayers);

  if (ghostName == "default")
  {
    ds.SetGhostCellField(ghosts);
  }
  else
  {
    ds.SetGhostCellField(ghostName, ghosts);
  }

  return ds;
}

void TestGhostCellRemove()
{
  // specify some 2d tests: {numI, numJ, numK, numGhostLayers}.
  std::vector<std::vector<viskores::Id>> tests2D = { { 4, 4, 0, 2 },   { 5, 5, 0, 2 },
                                                     { 10, 10, 0, 3 }, { 10, 5, 0, 2 },
                                                     { 5, 10, 0, 2 },  { 20, 10, 0, 3 },
                                                     { 10, 20, 0, 3 } };
  std::vector<std::vector<viskores::Id>> tests3D = { { 4, 4, 4, 2 },    { 5, 5, 5, 2 },
                                                     { 10, 10, 10, 3 }, { 10, 5, 10, 2 },
                                                     { 5, 10, 10, 2 },  { 20, 10, 10, 3 },
                                                     { 10, 20, 10, 3 } };

  std::vector<std::vector<viskores::Id>> tests;

  tests.insert(tests.end(), tests2D.begin(), tests2D.end());
  tests.insert(tests.end(), tests3D.begin(), tests3D.end());

  for (auto& t : tests)
  {
    viskores::Id nx = t[0], ny = t[1], nz = t[2];
    int nghost = static_cast<int>(t[3]);
    for (int layer = 0; layer < nghost; layer++)
    {
      std::vector<std::string> dsTypes = { "uniform", "rectilinear", "explicit" };
      for (auto& dsType : dsTypes)
      {
        std::vector<std::string> nameTypes = { "default", "user-specified" };
        for (auto& nameType : nameTypes)
        {
          viskores::cont::DataSet ds;
          if (dsType == "uniform")
            ds = MakeUniform(nx, ny, nz, layer, nameType);
          else if (dsType == "rectilinear")
            ds = MakeRectilinear(nx, ny, nz, layer, nameType);
          else if (dsType == "explicit")
            ds = MakeExplicit(nx, ny, nz, layer, nameType);

          std::vector<std::string> removeType = { "all", "byType" };
          for (auto& rt : removeType)
          {
            viskores::filter::entity_extraction::GhostCellRemove ghostCellRemoval;
            ghostCellRemoval.SetRemoveGhostField(true);

            if (rt == "all")
              ghostCellRemoval.SetTypesToRemoveToAll();
            else if (rt == "byType")
              ghostCellRemoval.SetTypesToRemove(viskores::CellClassification::Ghost);

            auto output = ghostCellRemoval.Execute(ds);
            viskores::Id numCells = output.GetNumberOfCells();

            //Validate the output.

            viskores::Id numCellsReq = (nx - 2 * layer) * (ny - 2 * layer);
            if (nz != 0)
              numCellsReq *= (nz - 2 * layer);

            VISKORES_TEST_ASSERT(numCellsReq == numCells, "Wrong number of cells in output");
            if (dsType == "uniform" || dsType == "rectilinear")
            {
              if (nz == 0)
              {
                VISKORES_TEST_ASSERT(
                  output.GetCellSet().CanConvert<viskores::cont::CellSetStructured<2>>(),
                  "Wrong cell type for explicit conversion");
              }
              else if (nz > 0)
              {
                VISKORES_TEST_ASSERT(
                  output.GetCellSet().CanConvert<viskores::cont::CellSetStructured<3>>(),
                  "Wrong cell type for explicit conversion");
              }
            }
            else
            {
              VISKORES_TEST_ASSERT(output.GetCellSet().IsType<viskores::cont::CellSetExplicit<>>(),
                                   "Wrong cell type for explicit conversion");
            }
          }

          // For structured, test the case where we have a ghost in the 'middle' of the cells.
          // This will produce an explicit cellset.
          if (dsType == "uniform" || dsType == "rectilinear")
          {
            if (dsType == "uniform")
              ds = MakeUniform(nx, ny, nz, layer, nameType, true);
            else if (dsType == "rectilinear")
              ds = MakeRectilinear(nx, ny, nz, layer, nameType, true);

            viskores::filter::entity_extraction::GhostCellRemove ghostCellRemoval;
            ghostCellRemoval.SetRemoveGhostField(true);
            auto output = ghostCellRemoval.Execute(ds);
            VISKORES_TEST_ASSERT(output.GetCellSet().IsType<viskores::cont::CellSetExplicit<>>(),
                                 "Wrong cell type for explicit conversion");
          }
        }
      }
    }
  }
}
}

int UnitTestGhostCellRemove(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestGhostCellRemove, argc, argv);
}
