// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataAssembly.h"
#include "vtkDataObjectToConduit.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkTable.h>
#include <vtkUnstructuredGrid.h>

namespace
{
//----------------------------------------------------------------------------
bool TestNonDataSetObject()
{
  conduit_cpp::Node node;
  vtkNew<vtkTable> table;

  auto previous_verbosity = vtkLogger::GetCurrentVerbosityCutoff();
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);

  bool is_table_supported = vtkDataObjectToConduit::FillConduitNode(table, node);

  vtkLogger::SetStderrVerbosity(previous_verbosity);

  return !is_table_supported;
}

//----------------------------------------------------------------------------
bool TestImageData()
{
  conduit_cpp::Node node;
  vtkNew<vtkImageData> image;

  image->SetDimensions(2, 3, 1);
  image->SetSpacing(10, 20, 30);
  image->SetOrigin(-1, -2, -3);
  image->AllocateScalars(VTK_INT, 1);
  int* dims = image->GetDimensions();

  for (int z = 0; z < dims[2]; z++)
  {
    for (int y = 0; y < dims[1]; y++)
    {
      for (int x = 0; x < dims[0]; x++)
      {
        image->SetScalarComponentFromFloat(x, y, z, 0, 2);
      }
    }
  }
  vtkNew<vtkUnsignedCharArray> ghostCells;
  ghostCells->SetName(vtkDataSetAttributes::GhostArrayName());
  ghostCells->SetNumberOfValues(image->GetNumberOfCells());
  ghostCells->SetValue(0, 0);
  ghostCells->SetValue(1, vtkDataSetAttributes::HIDDENCELL);
  image->GetCellData()->AddArray(ghostCells);

  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(image), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestImageData" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "uniform";
  coords_node["dims/i"] = image->GetDimensions()[0];
  coords_node["dims/j"] = image->GetDimensions()[1];
  coords_node["dims/k"] = image->GetDimensions()[2];
  coords_node["origin/x"] = image->GetOrigin()[0];
  coords_node["origin/y"] = image->GetOrigin()[1];
  coords_node["origin/z"] = image->GetOrigin()[2];
  coords_node["spacing/dx"] = image->GetSpacing()[0];
  coords_node["spacing/dy"] = image->GetSpacing()[1];
  coords_node["spacing/dz"] = image->GetSpacing()[2];

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "uniform";
  topologies_node["coordset"] = "coords";

  auto field_node = expected_node["fields/ImageScalars"];
  field_node["association"] = "vertex";
  field_node["topology"] = "mesh";
  field_node["volume_dependent"] = "false";
  field_node["values"] = std::vector<int>{ 2, 2, 2, 2, 2, 2 };

  auto field_metadata_node = expected_node["state/metadata/vtk_fields/ImageScalars"];
  field_metadata_node["attribute_type"] = "Scalars";

  auto ghost_field_node = expected_node["fields/vtkGhostType"];
  ghost_field_node["association"] = "element";
  ghost_field_node["topology"] = "mesh";
  ghost_field_node["volume_dependent"] = "false";
  ghost_field_node["values"] = std::vector<unsigned char>{ 0, vtkDataSetAttributes::HIDDENCELL };

  auto ghost_field_metadata_node = expected_node["state/metadata/vtk_fields/vtkGhostType"];
  ghost_field_metadata_node["attribute_type"] = "Ghosts";

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestRectilinearGrid()
{
  conduit_cpp::Node node;
  vtkNew<vtkRectilinearGrid> rectilinear_grid;

  rectilinear_grid->SetDimensions(2, 3, 1);

  std::vector<double> x_coordinates = { 0, 2 };
  vtkNew<vtkDoubleArray> xArray;
  xArray->SetArray(x_coordinates.data(), x_coordinates.size(), 1);
  rectilinear_grid->SetXCoordinates(xArray);

  std::vector<double> y_coordinates = { 0, 1, 2 };
  vtkNew<vtkDoubleArray> yArray;
  yArray->SetArray(y_coordinates.data(), y_coordinates.size(), 1);
  rectilinear_grid->SetYCoordinates(yArray);

  std::vector<double> z_coordinates = { 0 };
  vtkNew<vtkDoubleArray> zArray;
  zArray->SetArray(z_coordinates.data(), z_coordinates.size(), 1);
  rectilinear_grid->SetZCoordinates(zArray);

  std::vector<double> field_values = { 0, 0, 1, 2, 2, 4, 3, 6, 4, 8, 5, 10 };
  vtkNew<vtkDoubleArray> fieldArray;
  fieldArray->SetName("rectilinear_field");
  fieldArray->SetNumberOfComponents(2);
  fieldArray->SetNumberOfTuples(6);
  fieldArray->SetArray(field_values.data(), field_values.size(), 1);

  rectilinear_grid->GetPointData()->AddArray(fieldArray);

  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(rectilinear_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestRectilinearGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "rectilinear";
  coords_node["values/x"] = x_coordinates;
  coords_node["values/y"] = y_coordinates;
  coords_node["values/z"] = z_coordinates;

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "rectilinear";
  topologies_node["coordset"] = "coords";

  auto field_node = expected_node["fields/rectilinear_field"];
  field_node["association"] = "vertex";
  field_node["topology"] = "mesh";
  field_node["volume_dependent"] = "false";
  field_node["values/0"] = std::vector<double>{ 0, 1, 2, 3, 4, 5 };
  field_node["values/1"] = std::vector<double>{ 0, 2, 4, 6, 8, 10 };

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestStructuredGrid()
{
  conduit_cpp::Node node;
  vtkNew<vtkStructuredGrid> structured_grid;

  vtkIdType nx = 2, ny = 3, nz = 2;
  auto dataSize = nx * ny * nz;

  vtkNew<vtkDoubleArray> pointValues;
  pointValues->SetNumberOfComponents(1);
  pointValues->SetNumberOfTuples(dataSize);
  for (vtkIdType i = 0; i < dataSize; ++i)
  {
    pointValues->SetValue(i, i);
  }
  pointValues->SetName("point_field");

  auto numberOfCells = (nx - 1) * (ny - 1) * (nz - 1);
  vtkNew<vtkDoubleArray> cellValues;
  cellValues->SetNumberOfTuples(numberOfCells);
  for (vtkIdType i = 0; i < numberOfCells; ++i)
  {
    cellValues->SetValue(i, i * 2.0);
  }
  cellValues->SetName("cell_field");

  vtkNew<vtkPoints> points;
  auto x = 0.0;
  auto y = 0.0;
  auto z = 0.0;
  for (unsigned int k = 0; k < nz; k++)
  {
    z += 2.0;
    for (unsigned int j = 0; j < ny; j++)
    {
      y += 1.0;
      for (unsigned int i = 0; i < nx; i++)
      {
        x += .5;
        points->InsertNextPoint(x, y, z);
      }
    }
  }

  structured_grid->SetDimensions(static_cast<int>(nx), static_cast<int>(ny), static_cast<int>(nz));
  structured_grid->SetPoints(points);
  structured_grid->GetCellData()->SetScalars(cellValues);
  structured_grid->GetPointData()->SetScalars(pointValues);

  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(structured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestStructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] =
    std::vector<float>{ 0.5, 1, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5, 6.0 };
  coords_node["values/y"] = std::vector<float>{ 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6 };
  coords_node["values/z"] = std::vector<float>{ 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4 };

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "structured";
  topologies_node["coordset"] = "coords";
  topologies_node["elements/dims/i"] = 2;
  topologies_node["elements/dims/j"] = 3;
  topologies_node["elements/dims/k"] = 2;

  auto point_field_node = expected_node["fields/point_field"];
  point_field_node["association"] = "vertex";
  point_field_node["topology"] = "mesh";
  point_field_node["volume_dependent"] = "false";
  point_field_node["values"] = std::vector<double>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  auto point_field_metadata_node = expected_node["state/metadata/vtk_fields/point_field"];
  point_field_metadata_node["attribute_type"] = "Scalars";

  auto cell_field_node = expected_node["fields/cell_field"];
  cell_field_node["association"] = "element";
  cell_field_node["topology"] = "mesh";
  cell_field_node["volume_dependent"] = "false";
  cell_field_node["values"] = std::vector<double>{ 0, 2 };

  auto cell_field_metadata_node = expected_node["state/metadata/vtk_fields/cell_field"];
  cell_field_metadata_node["attribute_type"] = "Scalars";

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

double unstructured_grid_points_coordinates[27][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 },
  { 0, 1, 0 }, { 1, 1, 0 }, { 2, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 2, 0, 1 }, { 0, 1, 1 },
  { 1, 1, 1 }, { 2, 1, 1 }, { 0, 1, 2 }, { 1, 1, 2 }, { 2, 1, 2 }, { 0, 1, 3 }, { 1, 1, 3 },
  { 2, 1, 3 }, { 0, 1, 4 }, { 1, 1, 4 }, { 2, 1, 4 }, { 0, 1, 5 }, { 1, 1, 5 }, { 2, 1, 5 },
  { 0, 1, 6 }, { 1, 1, 6 }, { 2, 1, 6 } };

struct
{
  VTKCellType cell_type;
  std::vector<vtkIdType> connectivity;
} unstructured_grid_cell_connectivities[] = { { VTK_HEXAHEDRON, { 0, 1, 4, 3, 6, 7, 10, 9 } },
  { VTK_HEXAHEDRON, { 1, 2, 5, 4, 7, 8, 11, 10 } }, { VTK_TETRA, { 6, 10, 9, 12 } },
  { VTK_TETRA, { 8, 11, 10, 14 } }, { VTK_POLYGON, { 16, 17, 14, 13, 12, 15 } },
  { VTK_TRIANGLE_STRIP, { 18, 15, 19, 16, 20, 17 } }, { VTK_QUAD, { 22, 23, 20, 19 } },
  { VTK_TRIANGLE, { 21, 22, 18 } }, { VTK_TRIANGLE, { 22, 19, 18 } }, { VTK_LINE, { 23, 26 } },
  { VTK_LINE, { 21, 24 } }, { VTK_VERTEX, { 25 } } };

//----------------------------------------------------------------------------
bool TestMixedShapedUnstructuredGrid()
{
  conduit_cpp::Node node;
  vtkNew<vtkUnstructuredGrid> unstructured_grid;

  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);

  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[0].cell_type, // HEXA
    unstructured_grid_cell_connectivities[0].connectivity.size(),
    unstructured_grid_cell_connectivities[0].connectivity.data());
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[2].cell_type, // TETRA
    unstructured_grid_cell_connectivities[2].connectivity.size(),
    unstructured_grid_cell_connectivities[2].connectivity.data());
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[4].cell_type, // POLYGON
    unstructured_grid_cell_connectivities[4].connectivity.size(),
    unstructured_grid_cell_connectivities[4].connectivity.data());
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[6].cell_type, // QUAD
    unstructured_grid_cell_connectivities[6].connectivity.size(),
    unstructured_grid_cell_connectivities[6].connectivity.data());
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[7].cell_type, // TRIANGLE
    unstructured_grid_cell_connectivities[7].connectivity.size(),
    unstructured_grid_cell_connectivities[7].connectivity.data());

  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestMixedShapedUnstructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node0 = expected_node["topologies/mesh"];
  topologies_node0["type"] = "unstructured";
  topologies_node0["coordset"] = "coords";
  topologies_node0["elements/shape"] = "mixed";
  topologies_node0["elements/shape_map/hex"] = VTK_HEXAHEDRON;
  topologies_node0["elements/shape_map/tet"] = VTK_TETRA;
  topologies_node0["elements/shape_map/quad"] = VTK_QUAD;
  topologies_node0["elements/shape_map/tri"] = VTK_TRIANGLE;
  topologies_node0["elements/shape_map/polygonal"] = VTK_POLYGON;
  topologies_node0["elements/shapes"] = std::vector<conduit_uint8>{ 12, 10, 7, 9, 5 };

  if (unstructured_grid->GetCells()->IsStorage64Bit())
  {
    topologies_node0["elements/offsets"] = std::vector<conduit_int64>{ 0, 8, 12, 18, 22 };
    topologies_node0["elements/sizes"] = std::vector<conduit_int64>{ 8, 4, 6, 4, 3 };
    topologies_node0["elements/connectivity"] = std::vector<conduit_int64>{ 0, 1, 4, 3, 6, 7, 10, 9,
      6, 10, 9, 12, 16, 17, 14, 13, 12, 15, 22, 23, 20, 19, 21, 22, 18 };
  }
  else
  {
    topologies_node0["elements/offsets"] = std::vector<conduit_int32>{ 0, 8, 12, 16 };
    topologies_node0["elements/sizes"] = std::vector<conduit_int32>{ 8, 4, 4, 3 };
    topologies_node0["elements/connectivity"] = std::vector<conduit_int32>{ 0, 1, 4, 3, 6, 7, 10, 9,
      6, 10, 9, 12, 16, 17, 14, 13, 12, 15, 22, 23, 20, 19, 21, 22, 18 };
  }

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestHexahedronUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructured_grid;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);
  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[0].cell_type,
    unstructured_grid_cell_connectivities[0].connectivity.size(),
    unstructured_grid_cell_connectivities[0].connectivity.data());
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[1].cell_type,
    unstructured_grid_cell_connectivities[1].connectivity.size(),
    unstructured_grid_cell_connectivities[1].connectivity.data());

  vtkNew<vtkDoubleArray> cellValues;
  cellValues->SetNumberOfTuples(2);
  cellValues->SetValue(0, 10);
  cellValues->SetValue(1, -10);
  cellValues->SetName("cell_field");
  unstructured_grid->GetCellData()->AddArray(cellValues);

  conduit_cpp::Node node;
  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestHexahedronUnstructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node0 = expected_node["topologies/mesh"];
  topologies_node0["type"] = "unstructured";
  topologies_node0["coordset"] = "coords";
  topologies_node0["elements/shape"] = "hex";
  if (unstructured_grid->GetCells()->IsStorage64Bit())
  {
    topologies_node0["elements/connectivity"] =
      std::vector<conduit_int64>{ 0, 1, 4, 3, 6, 7, 10, 9, 1, 2, 5, 4, 7, 8, 11, 10 };
  }
  else
  {
    topologies_node0["elements/connectivity"] =
      std::vector<conduit_int32>{ 0, 1, 4, 3, 6, 7, 10, 9, 1, 2, 5, 4, 7, 8, 11, 10 };
  }

  auto cell_field_node = expected_node["fields/cell_field"];
  cell_field_node["association"] = "element";
  cell_field_node["topology"] = "mesh";
  cell_field_node["volume_dependent"] = "false";
  cell_field_node["values"] = std::vector<double>{ 10, -10 };

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestTetrahedronUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructured_grid;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);
  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[2].cell_type,
    unstructured_grid_cell_connectivities[2].connectivity.size(),
    unstructured_grid_cell_connectivities[2].connectivity.data());
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[3].cell_type,
    unstructured_grid_cell_connectivities[3].connectivity.size(),
    unstructured_grid_cell_connectivities[3].connectivity.data());

  vtkNew<vtkDoubleArray> cellValues;
  cellValues->SetNumberOfTuples(2);
  cellValues->SetValue(0, 10);
  cellValues->SetValue(1, -10);
  cellValues->SetName("cell_field");
  unstructured_grid->GetCellData()->AddArray(cellValues);

  conduit_cpp::Node node;
  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestTetrahedronUnstructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "unstructured";
  topologies_node["coordset"] = "coords";
  topologies_node["elements/shape"] = "tet";
  if (unstructured_grid->GetCells()->IsStorage64Bit())
  {
    topologies_node["elements/connectivity"] =
      std::vector<conduit_int64>{ 6, 10, 9, 12, 8, 11, 10, 14 };
  }
  else
  {
    topologies_node["elements/connectivity"] =
      std::vector<conduit_int32>{ 6, 10, 9, 12, 8, 11, 10, 14 };
  }

  auto cell_field_node = expected_node["fields/cell_field"];
  cell_field_node["association"] = "element";
  cell_field_node["topology"] = "mesh";
  cell_field_node["volume_dependent"] = "false";
  cell_field_node["values"] = std::vector<double>{ 10, -10 };

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestPolygonalUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructured_grid;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);
  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[4].cell_type,
    unstructured_grid_cell_connectivities[4].connectivity.size(),
    unstructured_grid_cell_connectivities[4].connectivity.data());

  conduit_cpp::Node node;
  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestPolygonalUnstructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "unstructured";
  topologies_node["coordset"] = "coords";
  topologies_node["elements/shape"] = "polygonal";
  if (unstructured_grid->GetCells()->IsStorage64Bit())
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int64>{ 16, 17, 14, 13, 12, 15 };
  }
  else
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int32>{ 16, 17, 14, 13, 12, 15 };
  }

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestQuadUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructured_grid;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);
  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[6].cell_type,
    unstructured_grid_cell_connectivities[6].connectivity.size(),
    unstructured_grid_cell_connectivities[6].connectivity.data());

  vtkNew<vtkDoubleArray> pointValues;
  pointValues->SetNumberOfTuples(4);
  pointValues->SetValue(0, 10);
  pointValues->SetValue(1, -10);
  pointValues->SetValue(2, 20);
  pointValues->SetValue(3, -20);
  pointValues->SetName("point_field");
  unstructured_grid->GetPointData()->AddArray(pointValues);

  conduit_cpp::Node node;
  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestQuadUnstructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "unstructured";
  topologies_node["coordset"] = "coords";
  topologies_node["elements/shape"] = "quad";
  if (unstructured_grid->GetCells()->IsStorage64Bit())
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int64>{ 22, 23, 20, 19 };
  }
  else
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int32>{ 22, 23, 20, 19 };
  }

  auto cell_field_node = expected_node["fields/point_field"];
  cell_field_node["association"] = "vertex";
  cell_field_node["topology"] = "mesh";
  cell_field_node["volume_dependent"] = "false";
  cell_field_node["values"] = std::vector<double>{ 10, -10, 20, -20 };

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestTriangleUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructured_grid;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);
  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[7].cell_type,
    unstructured_grid_cell_connectivities[7].connectivity.size(),
    unstructured_grid_cell_connectivities[7].connectivity.data());
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[8].cell_type,
    unstructured_grid_cell_connectivities[8].connectivity.size(),
    unstructured_grid_cell_connectivities[8].connectivity.data());

  vtkNew<vtkDoubleArray> cellValues;
  cellValues->SetNumberOfTuples(2);
  cellValues->SetValue(0, 10);
  cellValues->SetValue(1, -10);
  cellValues->SetName("cell_field");
  unstructured_grid->GetCellData()->AddArray(cellValues);

  conduit_cpp::Node node;
  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestTriangleUnstructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "unstructured";
  topologies_node["coordset"] = "coords";
  topologies_node["elements/shape"] = "tri";
  if (unstructured_grid->GetCells()->IsStorage64Bit())
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int64>{ 21, 22, 18, 22, 19, 18 };
  }
  else
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int32>{ 21, 22, 18, 22, 19, 18 };
  }

  auto cell_field_node = expected_node["fields/cell_field"];
  cell_field_node["association"] = "element";
  cell_field_node["topology"] = "mesh";
  cell_field_node["volume_dependent"] = "false";
  cell_field_node["values"] = std::vector<double>{ 10, -10 };

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestLineUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructured_grid;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);
  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[9].cell_type,
    unstructured_grid_cell_connectivities[9].connectivity.size(),
    unstructured_grid_cell_connectivities[9].connectivity.data());
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[10].cell_type,
    unstructured_grid_cell_connectivities[10].connectivity.size(),
    unstructured_grid_cell_connectivities[10].connectivity.data());

  vtkNew<vtkDoubleArray> cellValues;
  cellValues->SetNumberOfTuples(2);
  cellValues->SetValue(0, 10);
  cellValues->SetValue(1, -10);
  cellValues->SetName("cell_field");
  unstructured_grid->GetCellData()->AddArray(cellValues);

  conduit_cpp::Node node;
  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestLineUnstructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "unstructured";
  topologies_node["coordset"] = "coords";
  topologies_node["elements/shape"] = "line";
  if (unstructured_grid->GetCells()->IsStorage64Bit())
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int64>{ 23, 26, 21, 24 };
  }
  else
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int32>{ 23, 26, 21, 24 };
  }

  auto cell_field_node = expected_node["fields/cell_field"];
  cell_field_node["association"] = "element";
  cell_field_node["topology"] = "mesh";
  cell_field_node["volume_dependent"] = "false";
  cell_field_node["values"] = std::vector<double>{ 10, -10 };

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestPointUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructured_grid;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);
  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[11].cell_type,
    unstructured_grid_cell_connectivities[11].connectivity.size(),
    unstructured_grid_cell_connectivities[11].connectivity.data());

  vtkNew<vtkDoubleArray> cellValues;
  cellValues->SetNumberOfTuples(1);
  cellValues->SetValue(0, 10);
  cellValues->SetName("cell_field");
  unstructured_grid->GetCellData()->AddArray(cellValues);

  conduit_cpp::Node node;
  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructured_grid), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestPointUnstructuredGrid" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "unstructured";
  topologies_node["coordset"] = "coords";
  topologies_node["elements/shape"] = "point";
  if (unstructured_grid->GetCells()->IsStorage64Bit())
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int64>{ 25 };
  }
  else
  {
    topologies_node["elements/connectivity"] = std::vector<conduit_int32>{ 25 };
  }

  auto cell_field_node = expected_node["fields/cell_field"];
  cell_field_node["association"] = "element";
  cell_field_node["topology"] = "mesh";
  cell_field_node["volume_dependent"] = "false";
  cell_field_node["values"] = std::vector<double>{ 10 };

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  is_success = !are_nodes_different;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestPyramidUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructuredGrid;
  std::vector<std::array<double, 3>> pointsCoords = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 1 },
    { 1, 1, 0 }, { 1, 1, 1 }, { 2, 0, 0 } };
  std::vector<vtkIdType> connectivity[2] = { { 1, 2, 3, 4, 0 }, { 1, 2, 3, 4, 5 } };
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 6; i++)
  {
    points->InsertPoint(i, pointsCoords[i].data());
  }
  unstructuredGrid->SetPoints(points);
  unstructuredGrid->Allocate(2);
  unstructuredGrid->InsertNextCell(VTK_PYRAMID, 5, connectivity[0].data());
  unstructuredGrid->InsertNextCell(VTK_PYRAMID, 5, connectivity[1].data());

  vtkNew<vtkDoubleArray> cellValues;
  cellValues->SetNumberOfTuples(2);
  cellValues->SetValue(0, 10);
  cellValues->SetValue(1, -10);
  cellValues->SetName("cell_field");
  unstructuredGrid->GetCellData()->AddArray(cellValues);

  conduit_cpp::Node node;

  if (!vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructuredGrid), node))
  {
    std::cerr << "FillConduitNode failed for TestPyramidUnstructuredGrid" << std::endl;
    return false;
  }

  conduit_cpp::Node expectedNode;
  auto coordsNode = expectedNode["coordsets/coords"];
  coordsNode["type"] = "explicit";
  coordsNode["values/x"] = std::vector<float>{ 0, 1, 1, 1, 1, 2 };
  coordsNode["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 0 };
  coordsNode["values/z"] = std::vector<float>{ 0, 0, 1, 0, 1, 0 };

  auto topologiesNode = expectedNode["topologies/mesh"];
  topologiesNode["type"] = "unstructured";
  topologiesNode["coordset"] = "coords";
  topologiesNode["elements/shape"] = "pyramid";
  if (unstructuredGrid->GetCells()->IsStorage64Bit())
  {
    topologiesNode["elements/connectivity"] =
      std::vector<conduit_int64>{ 1, 2, 3, 4, 0, 1, 2, 3, 4, 5 };
  }
  else
  {
    topologiesNode["elements/connectivity"] =
      std::vector<conduit_int32>{ 1, 2, 3, 4, 0, 1, 2, 3, 4, 5 };
  }

  auto cellFieldNode = expectedNode["fields/cell_field"];
  cellFieldNode["association"] = "element";
  cellFieldNode["topology"] = "mesh";
  cellFieldNode["volume_dependent"] = "false";
  cellFieldNode["values"] = std::vector<double>{ 10, -10 };

  conduit_cpp::Node diffInfo;
  bool areNodesDifferent = node.diff(expectedNode, diffInfo, 1e-6);
  if (areNodesDifferent)
  {
    diffInfo.print();
  }

  return !areNodesDifferent;
}

//----------------------------------------------------------------------------
bool TestWedgeUnstructuredGrid()
{
  vtkNew<vtkUnstructuredGrid> unstructuredGrid;
  std::vector<std::array<double, 3>> pointsCoords = { { 0, 0, 0 }, { 0, 1, 0 }, { 1, 0, 0 },
    { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 }, { 2, 0, 0 }, { 2, 1, 0 } };
  std::vector<vtkIdType> connectivity[2] = { { 2, 3, 4, 5, 0, 1 }, { 2, 3, 4, 5, 6, 7 } };
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 8; i++)
  {
    points->InsertPoint(i, pointsCoords[i].data());
  }
  unstructuredGrid->SetPoints(points);
  unstructuredGrid->Allocate(2);
  unstructuredGrid->InsertNextCell(VTK_WEDGE, 6, connectivity[0].data());
  unstructuredGrid->InsertNextCell(VTK_WEDGE, 6, connectivity[1].data());

  vtkNew<vtkDoubleArray> cellValues;
  cellValues->SetNumberOfTuples(2);
  cellValues->SetValue(0, 10);
  cellValues->SetValue(1, -10);
  cellValues->SetName("cell_field");
  unstructuredGrid->GetCellData()->AddArray(cellValues);

  conduit_cpp::Node node;

  if (!vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(unstructuredGrid), node))
  {
    std::cerr << "FillConduitNode failed for TestWedgeUnstructuredGrid" << std::endl;
    return false;
  }

  conduit_cpp::Node expectedNode;
  auto coordsNode = expectedNode["coordsets/coords"];
  coordsNode["type"] = "explicit";
  coordsNode["values/x"] = std::vector<float>{ 0, 0, 1, 1, 1, 1, 2, 2 };
  coordsNode["values/y"] = std::vector<float>{ 0, 1, 0, 0, 1, 1, 0, 1 };
  coordsNode["values/z"] = std::vector<float>{ 0, 0, 0, 1, 0, 1, 0, 0 };

  auto topologiesNode = expectedNode["topologies/mesh"];
  topologiesNode["type"] = "unstructured";
  topologiesNode["coordset"] = "coords";
  topologiesNode["elements/shape"] = "wedge";
  if (unstructuredGrid->GetCells()->IsStorage64Bit())
  {
    topologiesNode["elements/connectivity"] =
      std::vector<conduit_int64>{ 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 7 };
  }
  else
  {
    topologiesNode["elements/connectivity"] =
      std::vector<conduit_int32>{ 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 7 };
  }

  auto cellFieldNode = expectedNode["fields/cell_field"];
  cellFieldNode["association"] = "element";
  cellFieldNode["topology"] = "mesh";
  cellFieldNode["volume_dependent"] = "false";
  cellFieldNode["values"] = std::vector<double>{ 10, -10 };

  conduit_cpp::Node diffInfo;
  bool areNodesDifferent = node.diff(expectedNode, diffInfo, 1e-6);
  if (areNodesDifferent)
  {
    diffInfo.print();
  }

  return !areNodesDifferent;
}

//----------------------------------------------------------------------------
bool TestMixedShapePolyData()
{
  vtkNew<vtkPolyData> poly_data;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  poly_data->SetPoints(points);
  poly_data->Allocate(100);
  poly_data->InsertNextCell(unstructured_grid_cell_connectivities[10].cell_type,
    unstructured_grid_cell_connectivities[10].connectivity.size(),
    unstructured_grid_cell_connectivities[10].connectivity.data());
  poly_data->InsertNextCell(unstructured_grid_cell_connectivities[11].cell_type,
    unstructured_grid_cell_connectivities[11].connectivity.size(),
    unstructured_grid_cell_connectivities[11].connectivity.data());

  auto previous_verbosity = vtkLogger::GetCurrentVerbosityCutoff();
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);

  conduit_cpp::Node node;
  bool is_filling_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(poly_data), node);

  vtkLogger::SetStderrVerbosity(previous_verbosity);

  return !is_filling_success;
}

//----------------------------------------------------------------------------
bool TestUnstructuredGrid()
{
  bool is_success = true;

  is_success &= TestMixedShapedUnstructuredGrid();
  is_success &= TestHexahedronUnstructuredGrid();
  is_success &= TestTetrahedronUnstructuredGrid();
  is_success &= TestPolygonalUnstructuredGrid();
  is_success &= TestQuadUnstructuredGrid();
  is_success &= TestTriangleUnstructuredGrid();
  is_success &= TestLineUnstructuredGrid();
  is_success &= TestPointUnstructuredGrid();
  is_success &= TestPyramidUnstructuredGrid();
  is_success &= TestWedgeUnstructuredGrid();

  return is_success;
}

//----------------------------------------------------------------------------
bool TestPointSet()
{
  vtkNew<vtkPointSet> point_set;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  point_set->SetPoints(points);

  conduit_cpp::Node node;
  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(point_set), node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestPointSet" << std::endl;
    return is_success;
  }

  conduit_cpp::Node expected_node;
  auto coords_node = expected_node["coordsets/coords"];
  coords_node["type"] = "explicit";
  coords_node["values/x"] = std::vector<float>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1,
    2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
  coords_node["values/y"] = std::vector<float>{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  coords_node["values/z"] = std::vector<float>{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3,
    3, 4, 4, 4, 5, 5, 5, 6, 6, 6 };

  auto topologies_node = expected_node["topologies/mesh"];
  topologies_node["type"] = "points";
  topologies_node["coordset"] = "coords";
  topologies_node["elements/shape"] = "point";

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
  }

  conduit_cpp::Node diff_info_blueprint;
  int is_blueprint_valid = conduit_cpp::Blueprint::verify("mesh", node, diff_info_blueprint);
  if (is_blueprint_valid != 1)
  {
    diff_info_blueprint.print();
  }

  is_success = !are_nodes_different && is_blueprint_valid == 1;

  return is_success;
}

//----------------------------------------------------------------------------
bool TestComposite()
{
  vtkNew<vtkImageData> image;
  image->SetDimensions(2, 3, 1);

  vtkNew<vtkUnstructuredGrid> unstructured_grid;
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27; i++)
  {
    points->InsertPoint(i, unstructured_grid_points_coordinates[i]);
  }
  unstructured_grid->SetPoints(points);
  unstructured_grid->Allocate(100);
  unstructured_grid->InsertNextCell(unstructured_grid_cell_connectivities[6].cell_type,
    unstructured_grid_cell_connectivities[6].connectivity.size(),
    unstructured_grid_cell_connectivities[6].connectivity.data());

  vtkNew<vtkPartitionedDataSet> pds1;
  pds1->SetNumberOfPartitions(1);
  constexpr int IMAGE_ID = 0, UG_ID = 1;
  pds1->SetPartition(IMAGE_ID, image);

  vtkNew<vtkPartitionedDataSet> pds2;
  pds1->SetNumberOfPartitions(2);
  pds1->SetPartition(IMAGE_ID, unstructured_grid);
  pds1->SetPartition(UG_ID, unstructured_grid);

  vtkNew<vtkPartitionedDataSetCollection> pdc;
  pdc->SetNumberOfPartitionedDataSets(2);
  pdc->SetPartitionedDataSet(IMAGE_ID, pds1);
  pdc->SetPartitionedDataSet(UG_ID, pds2);

  vtkNew<vtkDataAssembly> assembly;
  int imageId = assembly->AddNode("Image");
  int ugId = assembly->AddNode("UG");
  int sub = assembly->AddNode("subset");
  int subsub = assembly->AddNode("subsub", sub);

  assembly->AddDataSetIndex(imageId, IMAGE_ID);
  assembly->AddDataSetIndex(ugId, UG_ID);
  assembly->AddDataSetIndex(subsub, IMAGE_ID);
  assembly->AddDataSetIndex(subsub, UG_ID);

  pdc->SetDataAssembly(assembly);

  conduit_cpp::Node node;
  bool is_success = vtkDataObjectToConduit::FillConduitNode(pdc, node);

  if (!is_success)
  {
    std::cerr << "FillConduitNode failed for TestComposite" << std::endl;
    return is_success;
  }

  if (node.number_of_children() != 3)
  {
    std::cerr << "Expected 3 children but got " << node.number_of_children() << std::endl;
    return false;
  }
  for (conduit_index_t datasetId = 0; datasetId < 2; ++datasetId)
  {
    const auto mesh_node = node.child(datasetId);
    conduit_cpp::Node info;
    int is_valid =
      mesh_node.name() == "assembly" || conduit_cpp::Blueprint::verify("mesh", mesh_node, info);
    if (!is_valid)
    {
      info.print();
      is_success = false;
    }
  }

  conduit_cpp::Node expected_assembly;
  expected_assembly["Image"] = "partition0";
  expected_assembly["UG"] = "partition1";
  auto subsub0 = expected_assembly["subset/subsub"].append();
  subsub0.set("partition0");
  auto subsub1 = expected_assembly["subset/subsub"].append();
  subsub1.set("partition1");

  conduit_cpp::Node diff_info;
  bool are_nodes_different = node["assembly"].diff(expected_assembly, diff_info, 1e-6);
  if (are_nodes_different)
  {
    diff_info.print();
    is_success = false;
  }

  return is_success;
}
}
//----------------------------------------------------------------------------
int TestDataObjectToConduit(int, char*[])
{
  bool is_success = true;

  is_success &= ::TestNonDataSetObject();
  is_success &= ::TestImageData();
  is_success &= ::TestRectilinearGrid();
  is_success &= ::TestStructuredGrid();
  is_success &= ::TestUnstructuredGrid();
  is_success &= ::TestMixedShapePolyData();
  is_success &= ::TestPointSet();
  is_success &= ::TestComposite();

  return is_success ? EXIT_SUCCESS : EXIT_FAILURE;
}
