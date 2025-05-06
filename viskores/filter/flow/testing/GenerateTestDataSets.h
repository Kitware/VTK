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

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/filter/mesh_info/GhostCellClassify.h>

namespace viskores
{
namespace worklet
{
namespace testing
{

enum class ExplicitDataSetOption
{
  SINGLE = 0,
  CURVILINEAR,
  EXPLICIT
};

inline viskores::cont::DataSet CreateUniformDataSet(const viskores::Bounds& bounds,
                                                    const viskores::Id3& dims,
                                                    bool addGhost = false)
{
  viskores::Vec3f origin(static_cast<viskores::FloatDefault>(bounds.X.Min),
                         static_cast<viskores::FloatDefault>(bounds.Y.Min),
                         static_cast<viskores::FloatDefault>(bounds.Z.Min));
  viskores::Vec3f spacing(static_cast<viskores::FloatDefault>(bounds.X.Length()) /
                            static_cast<viskores::FloatDefault>((dims[0] - 1)),
                          static_cast<viskores::FloatDefault>(bounds.Y.Length()) /
                            static_cast<viskores::FloatDefault>((dims[1] - 1)),
                          static_cast<viskores::FloatDefault>(bounds.Z.Length()) /
                            static_cast<viskores::FloatDefault>((dims[2] - 1)));

  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);

  if (addGhost)
  {
    viskores::filter::mesh_info::GhostCellClassify addGhostFilter;
    return addGhostFilter.Execute(ds);
  }
  return ds;
}

inline viskores::cont::DataSet CreateRectilinearDataSet(const viskores::Bounds& bounds,
                                                        const viskores::Id3& dims,
                                                        bool addGhost = false)
{
  viskores::cont::DataSetBuilderRectilinear dataSetBuilder;
  std::vector<viskores::FloatDefault> xvals, yvals, zvals;

  viskores::Vec3f spacing(static_cast<viskores::FloatDefault>(bounds.X.Length()) /
                            static_cast<viskores::FloatDefault>((dims[0] - 1)),
                          static_cast<viskores::FloatDefault>(bounds.Y.Length()) /
                            static_cast<viskores::FloatDefault>((dims[1] - 1)),
                          static_cast<viskores::FloatDefault>(bounds.Z.Length()) /
                            static_cast<viskores::FloatDefault>((dims[2] - 1)));
  xvals.resize((size_t)dims[0]);
  xvals[0] = static_cast<viskores::FloatDefault>(bounds.X.Min);
  for (size_t i = 1; i < (size_t)dims[0]; i++)
    xvals[i] = xvals[i - 1] + spacing[0];

  yvals.resize((size_t)dims[1]);
  yvals[0] = static_cast<viskores::FloatDefault>(bounds.Y.Min);
  for (size_t i = 1; i < (size_t)dims[1]; i++)
    yvals[i] = yvals[i - 1] + spacing[1];

  zvals.resize((size_t)dims[2]);
  zvals[0] = static_cast<viskores::FloatDefault>(bounds.Z.Min);
  for (size_t i = 1; i < (size_t)dims[2]; i++)
    zvals[i] = zvals[i - 1] + spacing[2];

  viskores::cont::DataSet ds = dataSetBuilder.Create(xvals, yvals, zvals);

  if (addGhost)
  {
    viskores::filter::mesh_info::GhostCellClassify addGhostFilter;
    return addGhostFilter.Execute(ds);
  }
  return ds;
}

template <class CellSetType, viskores::IdComponent NDIM>
inline void MakeExplicitCells(const CellSetType& cellSet,
                              viskores::Vec<viskores::Id, NDIM>& cellDims,
                              viskores::cont::ArrayHandle<viskores::IdComponent>& numIndices,
                              viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
                              viskores::cont::ArrayHandle<viskores::Id>& conn)
{
  using Connectivity = viskores::internal::ConnectivityStructuredInternals<NDIM>;

  viskores::Id nCells = cellSet.GetNumberOfCells();
  viskores::IdComponent nVerts = (NDIM == 2 ? 4 : 8);
  viskores::Id connLen = (NDIM == 2 ? nCells * 4 : nCells * 8);

  conn.Allocate(connLen);
  shapes.Allocate(nCells);
  numIndices.Allocate(nCells);

  Connectivity structured;
  structured.SetPointDimensions(cellDims + viskores::Vec<viskores::Id, NDIM>(1));

  auto connPortal = conn.WritePortal();
  auto shapesPortal = shapes.WritePortal();
  auto numIndicesPortal = numIndices.WritePortal();
  viskores::Id connectionIndex = 0;
  for (viskores::Id cellIndex = 0; cellIndex < nCells; cellIndex++)
  {
    auto ptIds = structured.GetPointsOfCell(cellIndex);
    for (viskores::IdComponent vertexIndex = 0; vertexIndex < nVerts;
         vertexIndex++, connectionIndex++)
      connPortal.Set(connectionIndex, ptIds[vertexIndex]);

    shapesPortal.Set(cellIndex,
                     (NDIM == 2 ? viskores::CELL_SHAPE_QUAD : viskores::CELL_SHAPE_HEXAHEDRON));
    numIndicesPortal.Set(cellIndex, nVerts);
  }
}

inline viskores::cont::DataSet CreateExplicitFromStructuredDataSet(const viskores::Bounds& bounds,
                                                                   const viskores::Id3& dims,
                                                                   ExplicitDataSetOption option,
                                                                   bool addGhost = false)
{
  using CoordType = viskores::Vec3f;
  auto input = CreateUniformDataSet(bounds, dims, addGhost);

  auto inputCoords = input.GetCoordinateSystem(0).GetData();
  viskores::cont::ArrayHandle<CoordType> explCoords;
  viskores::cont::ArrayCopy(inputCoords, explCoords);

  viskores::cont::UnknownCellSet cellSet = input.GetCellSet();
  viskores::cont::ArrayHandle<viskores::Id> conn;
  viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
  viskores::cont::ArrayHandle<viskores::UInt8> shapes;
  viskores::cont::DataSet output;
  viskores::cont::DataSetBuilderExplicit dsb;

  using Structured2DType = viskores::cont::CellSetStructured<2>;
  using Structured3DType = viskores::cont::CellSetStructured<3>;

  switch (option)
  {
    case ExplicitDataSetOption::SINGLE:
      if (cellSet.IsType<Structured2DType>())
      {
        Structured2DType cells2D = cellSet.AsCellSet<Structured2DType>();
        viskores::Id2 cellDims = cells2D.GetCellDimensions();
        MakeExplicitCells(cells2D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, viskores::CellShapeTagQuad(), 4, conn, "coordinates");
      }
      else
      {
        Structured3DType cells3D = cellSet.AsCellSet<Structured3DType>();
        viskores::Id3 cellDims = cells3D.GetCellDimensions();
        MakeExplicitCells(cells3D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, viskores::CellShapeTagHexahedron(), 8, conn, "coordinates");
      }
      break;

    case ExplicitDataSetOption::CURVILINEAR:
      // In this case the cell set/connectivity is the same as the input
      // Only the coords are no longer Uniform / Rectilinear
      output.SetCellSet(cellSet);
      output.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", explCoords));
      break;

    case ExplicitDataSetOption::EXPLICIT:
      if (cellSet.IsType<Structured2DType>())
      {
        Structured2DType cells2D = cellSet.AsCellSet<Structured2DType>();
        viskores::Id2 cellDims = cells2D.GetCellDimensions();
        MakeExplicitCells(cells2D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, shapes, numIndices, conn, "coordinates");
      }
      else
      {
        Structured3DType cells3D = cellSet.AsCellSet<Structured3DType>();
        viskores::Id3 cellDims = cells3D.GetCellDimensions();
        MakeExplicitCells(cells3D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, shapes, numIndices, conn, "coordinates");
      }
      break;
  }

  if (addGhost)
  {
    output.SetGhostCellField(input.GetGhostCellField());
  }

  return output;
}

inline std::vector<viskores::cont::DataSet> CreateAllDataSets(const viskores::Bounds& bounds,
                                                              const viskores::Id3& dims,
                                                              bool addGhost)
{
  std::vector<viskores::cont::DataSet> dataSets;

  dataSets.push_back(viskores::worklet::testing::CreateUniformDataSet(bounds, dims, addGhost));
  /*
  dataSets.push_back(viskores::worklet::testing::CreateRectilinearDataSet(bounds, dims, addGhost));
  dataSets.push_back(viskores::worklet::testing::CreateExplicitFromStructuredDataSet(
    bounds, dims, ExplicitDataSetOption::SINGLE, addGhost));
  dataSets.push_back(viskores::worklet::testing::CreateExplicitFromStructuredDataSet(
    bounds, dims, ExplicitDataSetOption::CURVILINEAR, addGhost));
  dataSets.push_back(viskores::worklet::testing::CreateExplicitFromStructuredDataSet(
    bounds, dims, ExplicitDataSetOption::EXPLICIT, addGhost));
  */

  return dataSets;
}

inline std::vector<viskores::cont::PartitionedDataSet> CreateAllDataSets(
  const std::vector<viskores::Bounds>& bounds,
  const std::vector<viskores::Id3>& dims,
  bool addGhost)
{
  std::vector<viskores::cont::PartitionedDataSet> pds;
  std::vector<std::vector<viskores::cont::DataSet>> dataSets;

  VISKORES_ASSERT(bounds.size() == dims.size());
  std::size_t n = bounds.size();
  for (std::size_t i = 0; i < n; i++)
  {
    auto dsVec = CreateAllDataSets(bounds[i], dims[i], addGhost);
    std::size_t n2 = dsVec.size();
    if (i == 0)
      dataSets.resize(n2);
    for (std::size_t j = 0; j < n2; j++)
      dataSets[j].push_back(dsVec[j]);
  }

  for (auto& dsVec : dataSets)
    pds.push_back(viskores::cont::PartitionedDataSet(dsVec));

  return pds;
}


inline std::vector<viskores::cont::PartitionedDataSet> CreateAllDataSets(
  const std::vector<viskores::Bounds>& bounds,
  const viskores::Id3& dim,
  bool addGhost)
{
  std::vector<viskores::Id3> dims(bounds.size(), dim);
  return CreateAllDataSets(bounds, dims, addGhost);
}


} //namespace testing
} //namespace worklet
} //namespace viskores
