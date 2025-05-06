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

#include <viskores/filter/geometry_refinement/ConvertToPointCloud.h>

#include <viskores/io/VTKDataSetReader.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

void CheckPointCloudCells(const viskores::cont::UnknownCellSet& cellSet, viskores::Id numPoints)
{
  // A point cloud has the same number of cells as points. All cells are vertex
  // cells with one point. That point index is the same as the cell index.

  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPoints() == numPoints);
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == numPoints);

  for (viskores::Id index = 0; index < numPoints; ++index)
  {
    VISKORES_TEST_ASSERT(cellSet.GetCellShape(index) == viskores::CELL_SHAPE_VERTEX);
    VISKORES_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(index) == 1);

    viskores::Id pointId;
    cellSet.GetCellPointIds(index, &pointId);
    VISKORES_TEST_ASSERT(pointId == index);
  }
}

void CheckPointCloudCells(const viskores::cont::DataSet& dataSet, viskores::Id numPoints)
{
  CheckPointCloudCells(dataSet.GetCellSet(), numPoints);
}

void TryConvertToPointCloud(const viskores::cont::DataSet& dataSet)
{
  {
    std::cout << "  convert to point cloud" << std::endl;
    viskores::filter::geometry_refinement::ConvertToPointCloud convertFilter;
    viskores::cont::DataSet pointCloud = convertFilter.Execute(dataSet);
    CheckPointCloudCells(pointCloud, dataSet.GetNumberOfPoints());

    for (viskores::IdComponent coordId = 0; coordId < dataSet.GetNumberOfCoordinateSystems();
         ++coordId)
    {
      const auto& coords = dataSet.GetCoordinateSystem(coordId);
      std::cout << "    coord system " << coords.GetName() << std::endl;
      VISKORES_TEST_ASSERT(pointCloud.HasCoordinateSystem(coords.GetName()));
    }

    for (viskores::IdComponent fieldId = 0; fieldId < dataSet.GetNumberOfFields(); ++fieldId)
    {
      const auto& field = dataSet.GetField(fieldId);
      std::cout << "    field " << field.GetName() << std::endl;
      switch (field.GetAssociation())
      {
        case viskores::cont::Field::Association::Cells:
          VISKORES_TEST_ASSERT(!pointCloud.HasField(field.GetName()));
          break;
        default:
          VISKORES_TEST_ASSERT(pointCloud.HasField(field.GetName(), field.GetAssociation()));
          break;
      }
    }
  }

  {
    std::cout << "  convert to point cloud with cell data" << std::endl;
    viskores::filter::geometry_refinement::ConvertToPointCloud convertFilter;
    convertFilter.SetAssociateFieldsWithCells(true);
    viskores::cont::DataSet pointCloud = convertFilter.Execute(dataSet);
    CheckPointCloudCells(pointCloud, dataSet.GetNumberOfPoints());

    for (viskores::IdComponent coordId = 0; coordId < dataSet.GetNumberOfCoordinateSystems();
         ++coordId)
    {
      const auto& coords = dataSet.GetCoordinateSystem(coordId);
      std::cout << "    coord system " << coords.GetName() << std::endl;
      VISKORES_TEST_ASSERT(pointCloud.HasCoordinateSystem(coords.GetName()));
    }

    for (viskores::IdComponent fieldId = 0; fieldId < dataSet.GetNumberOfFields(); ++fieldId)
    {
      auto& field = dataSet.GetField(fieldId);
      std::cout << "    field " << field.GetName() << std::endl;
      switch (field.GetAssociation())
      {
        case viskores::cont::Field::Association::Cells:
          VISKORES_TEST_ASSERT(!pointCloud.HasField(field.GetName()));
          break;
        case viskores::cont::Field::Association::Points:
        {
          auto correctAssociation = dataSet.HasCoordinateSystem(field.GetName())
            ? viskores::cont::Field::Association::Points
            : viskores::cont::Field::Association::Cells;
          VISKORES_TEST_ASSERT(pointCloud.HasField(field.GetName(), correctAssociation));
        }
        break;
        default:
          VISKORES_TEST_ASSERT(pointCloud.HasField(field.GetName(), field.GetAssociation()));
          break;
      }
    }
  }
}

void TryFile(const std::string& filename)
{
  std::cout << "Testing " << filename << std::endl;
  std::string fullpath = viskores::cont::testing::Testing::DataPath(filename);
  viskores::io::VTKDataSetReader reader(fullpath);
  TryConvertToPointCloud(reader.ReadDataSet());
}

void Run()
{
  TryFile("uniform/simple_structured_points_bin.vtk");
  TryFile("rectilinear/DoubleGyre_0.vtk");
  TryFile("curvilinear/kitchen.vtk");
  TryFile("unstructured/simple_unstructured_bin.vtk");
}

} // anonymous namespace

int UnitTestConvertToPointCloud(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
