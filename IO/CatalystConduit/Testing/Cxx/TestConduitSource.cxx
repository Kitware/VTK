// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkConduitSource.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"

#include <catalyst_conduit_blueprint.hpp>

#define VERIFY(x, ...)                                                                             \
  if ((x) == false)                                                                                \
  {                                                                                                \
    vtkLogF(ERROR, __VA_ARGS__);                                                                   \
    return false;                                                                                  \
  }

namespace
{

vtkSmartPointer<vtkDataObject> Convert(const conduit_cpp::Node& node)
{
  vtkNew<vtkConduitSource> source;
  source->SetNode(conduit_cpp::c_node(&node));
  source->Update();
  return source->GetOutputDataObject(0);
}

void CreateUniformMesh(
  unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ, conduit_cpp::Node& res)
{
  // Create the structure
  conduit_cpp::Node coords = res["coordsets/coords"];
  coords["type"] = "uniform";
  conduit_cpp::Node dims = coords["dims"];
  dims["i"] = nptsX;
  dims["j"] = nptsY;

  if (nptsZ > 1)
  {
    dims["k"] = nptsZ;
  }

  // -10 to 10 in each dim
  conduit_cpp::Node origin = coords["origin"];
  origin["x"] = -10.0;
  origin["y"] = -10.0;

  if (nptsZ > 1)
  {
    origin["z"] = -10.0;
  }

  conduit_cpp::Node spacing = coords["spacing"];
  spacing["dx"] = 20.0 / (double)(nptsX - 1);
  spacing["dy"] = 20.0 / (double)(nptsY - 1);

  if (nptsZ > 1)
  {
    spacing["dz"] = 20.0 / (double)(nptsZ - 1);
  }

  res["topologies/mesh/type"] = "uniform";
  res["topologies/mesh/coordset"] = "coords";
}

bool ValidateMeshTypeUniform()
{
  conduit_cpp::Node mesh;
  CreateUniformMesh(3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto img = vtkImageData::SafeDownCast(pds->GetPartition(0));
  VERIFY(img != nullptr, "missing partition 0");
  int dims[3];
  img->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 3, "incorrect y dimension expected=3, got=%d", dims[1]);
  VERIFY(dims[2] == 3, "incorrect z dimension expected=3, got=%d", dims[2]);

  return true;
}

void CreateRectilinearMesh(
  unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ, conduit_cpp::Node& res)
{
  conduit_cpp::Node coords = res["coordsets/coords"];
  coords["type"] = "rectilinear";

  std::vector<double> x;
  x.resize(nptsX);
  std::vector<double> y;
  y.resize(nptsY);
  std::vector<double> z;

  if (nptsZ > 1)
  {
    z.resize(nptsZ);
  }

  double dx = 20.0 / (double)(nptsX - 1);
  double dy = 20.0 / (double)(nptsY - 1);
  double dz = 0.0;

  if (nptsZ > 1)
  {
    dz = 20.0 / (double)(nptsZ - 1);
  }

  for (unsigned int i = 0; i < nptsX; i++)
  {
    x[i] = -10.0 + i * dx;
  }

  for (unsigned int j = 0; j < nptsY; j++)
  {
    y[j] = -10.0 + j * dy;
  }

  if (nptsZ > 1)
  {
    for (unsigned int k = 0; k < nptsZ; k++)
    {
      z[k] = -10.0 + k * dz;
    }
  }

  conduit_cpp::Node coordVals = coords["values"];
  coordVals["x"].set(x);
  coordVals["y"].set(y);

  if (nptsZ > 1)
  {
    coordVals["z"].set(z);
  }

  res["topologies/mesh/type"] = "rectilinear";
  res["topologies/mesh/coordset"] = "coords";
}

bool ValidateMeshTypeRectilinear()
{
  conduit_cpp::Node mesh;
  CreateRectilinearMesh(3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "missing partition 0");
  int dims[3];
  rg->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 3, "incorrect y dimension expected=3, got=%d", dims[1]);
  VERIFY(dims[2] == 3, "incorrect z dimension expected=3, got=%d", dims[2]);

  return true;
}

void CreateCoords(
  unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ, conduit_cpp::Node& res)
{
  conduit_cpp::Node coords = res["coordsets/coords"];
  conduit_cpp::Node coordVals = coords["values"];
  coords["type"] = "explicit";

  unsigned int npts = nptsX * nptsY;

  if (nptsZ > 1)
  {
    npts *= nptsZ;
  }

  std::vector<double> x;
  x.resize(npts);
  std::vector<double> y;
  y.resize(npts);
  std::vector<double> z;

  if (nptsZ > 1)
  {
    z.resize(npts);
  }

  double dx = 20.0 / double(nptsX - 1);
  double dy = 20.0 / double(nptsY - 1);

  double dz = 0.0;

  if (nptsZ > 1)
  {
    dz = 20.0 / double(nptsZ - 1);
  }

  unsigned int idx = 0;
  unsigned int outer = 1;
  if (nptsZ > 1)
  {
    outer = nptsZ;
  }

  for (unsigned int k = 0; k < outer; k++)
  {
    double cz = -10.0 + k * dz;

    for (unsigned int j = 0; j < nptsY; j++)
    {
      double cy = -10.0 + j * dy;

      for (unsigned int i = 0; i < nptsX; i++)
      {
        x[idx] = -10.0 + i * dx;
        y[idx] = cy;

        if (nptsZ > 1)
        {
          z[idx] = cz;
        }

        idx++;
      }
    }
  }

  coordVals["x"].set(x);
  coordVals["y"].set(y);
  if (nptsZ > 1)
  {
    coordVals["z"].set(z);
  }
}

void CreateStructuredMesh(
  unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ, conduit_cpp::Node& res)
{
  CreateCoords(nptsX, nptsY, nptsZ, res);

  res["topologies/mesh/type"] = "structured";
  res["topologies/mesh/coordset"] = "coords";
  res["topologies/mesh/elements/dims/i"] = nptsX - 1;
  res["topologies/mesh/elements/dims/j"] = nptsY - 1;
  if (nptsZ > 0)
  {
    res["topologies/mesh/elements/dims/k"] = nptsZ - 1;
  }
}

bool ValidateMeshTypeStructured()
{
  conduit_cpp::Node mesh;
  CreateStructuredMesh(3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto sg = vtkStructuredGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(sg != nullptr, "missing partition 0");
  int dims[3];
  sg->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 3, "incorrect y dimension expected=3, got=%d", dims[1]);
  VERIFY(dims[2] == 3, "incorrect z dimension expected=3, got=%d", dims[2]);

  return true;
}

void CreateTrisMesh(unsigned int nptsX, unsigned int nptsY, conduit_cpp::Node& res)
{
  CreateStructuredMesh(nptsX, nptsY, 1, res);

  unsigned int nElementX = nptsX - 1;
  unsigned int nElementY = nptsY - 1;
  unsigned int nElements = nElementX * nElementY;

  res["topologies/mesh/type"] = "unstructured";
  res["topologies/mesh/coordset"] = "coords";
  res["topologies/mesh/elements/shape"] = "tri";

  std::vector<unsigned int> connectivity;
  connectivity.resize(nElements * 6);

  unsigned int idx = 0;
  for (unsigned int j = 0; j < nElementY; j++)
  {
    unsigned int yoff = j * (nElementX + 1);

    for (unsigned int i = 0; i < nElementX; i++)
    {
      // two tris per quad.
      connectivity[idx + 0] = yoff + i;
      connectivity[idx + 1] = yoff + i + (nElementX + 1);
      connectivity[idx + 2] = yoff + i + 1 + (nElementX + 1);

      connectivity[idx + 3] = yoff + i;
      connectivity[idx + 4] = yoff + i + 1;
      connectivity[idx + 5] = yoff + i + 1 + (nElementX + 1);

      idx += 6;
    }
  }

  res["topologies/mesh/elements/connectivity"].set(connectivity);

  // Need also to define 'fields' for cell array
  conduit_cpp::Node resFields = res["fields/field"];
  resFields["association"] = "element";
  resFields["topology"] = "mesh";
  resFields["volume_dependent"] = "false";

  unsigned int numberofValues = nElements * 2;
  std::vector<double> values;
  values.resize(numberofValues);
  for (unsigned int i = 0; i < numberofValues; i++)
  {
    values[i] = i + 0.0;
  }
  resFields["values"].set(values);
}

bool ValidateMeshTypeUnstructured()
{
  conduit_cpp::Node mesh;
  // generate simple explicit tri-based 2d 'basic' mesh
  CreateTrisMesh(3, 3, mesh);

  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(ug != nullptr, "missing partition 0");

  VERIFY(ug->GetNumberOfPoints() == 9, "incorrect number of points, expected 9, got %lld",
    ug->GetNumberOfPoints());
  VERIFY(ug->GetNumberOfCells() == 8, "incorrect number of cells, expected 8, got %lld",
    ug->GetNumberOfCells());
  VERIFY(ug->GetCellData()->GetArray("field") != nullptr, "missing 'field' cell-data array");
  return true;
}

bool CheckFieldDataMeshConversion(conduit_cpp::Node& mesh_node, int expected_number_of_arrays,
  const std::string& expected_array_name, int expected_number_of_components,
  std::vector<vtkVariant> expected_values)
{
  auto data = Convert(mesh_node);
  auto field_data = data->GetFieldData();
  VERIFY(field_data->GetNumberOfArrays() == expected_number_of_arrays,
    "incorrect number of arrays in field data, expected 0, got %d",
    field_data->GetNumberOfArrays());

  if (expected_number_of_arrays > 0)
  {
    auto field_array = field_data->GetAbstractArray(0);

    VERIFY(std::string(field_array->GetName()) == expected_array_name,
      "wrong array name, expected \"integer_field_data\", got %s", field_array->GetName());
    VERIFY(field_array->GetNumberOfComponents() == expected_number_of_components,
      "wrong number of component");
    VERIFY(static_cast<size_t>(field_array->GetNumberOfTuples()) == expected_values.size(),
      "wrong number of tuples");
    for (size_t i = 0; i < expected_values.size(); ++i)
    {
      VERIFY(field_array->GetVariantValue(i) == expected_values[i], "wrong value");
    }
  }

  return true;
}

bool ValidateFieldData()
{
  conduit_cpp::Node mesh;
  CreateUniformMesh(3, 3, 3, mesh);

  auto field_data_node = mesh["state/fields"];

  auto empty_field_data = field_data_node["empty_field_data"];
  VERIFY(CheckFieldDataMeshConversion(mesh, 0, empty_field_data.name(), 0, {}),
    "Verification failed for empty field data.");

  field_data_node.remove(0);
  auto integer_field_data = field_data_node["integer_field_data"];
  integer_field_data.set_int64(42);
  VERIFY(CheckFieldDataMeshConversion(mesh, 1, integer_field_data.name(), 1, { 42 }),
    "Verification failed for integer field data.");

  field_data_node.remove(0);
  auto float_field_data = field_data_node["float_field_data"];
  float_field_data.set_float64(5.0);
  VERIFY(CheckFieldDataMeshConversion(mesh, 1, float_field_data.name(), 1, { 5.0 }),
    "Verification failed for float field data.");

  field_data_node.remove(0);
  auto string_field_data = field_data_node["string_field_data"];
  string_field_data.set_string("test");
  VERIFY(CheckFieldDataMeshConversion(mesh, 1, string_field_data.name(), 1, { "test" }),
    "Verification failed for string field data.");

  field_data_node.remove(0);
  auto integer_vector_field_data = field_data_node["integer_vector_field_data"];
  integer_vector_field_data.set_int64_vector({ 1, 2, 3 });
  VERIFY(CheckFieldDataMeshConversion(mesh, 1, integer_vector_field_data.name(), 1, { 1, 2, 3 }),
    "Verification failed for integer vector field data.");

  field_data_node.remove(0);
  auto float_vector_field_data = field_data_node["float_vector_field_data"];
  float_vector_field_data.set_float64_vector({ 4.0, 5.0, 6.0 });
  VERIFY(
    CheckFieldDataMeshConversion(mesh, 1, float_vector_field_data.name(), 1, { 4.0, 5.0, 6.0 }),
    "Verification failed for float vector field data.");

  field_data_node.remove(0);
  std::vector<int> integer_buffer = { 123, 456, 789 };
  auto external_integer_vector_field_data = field_data_node["external_integer_vector"];
  external_integer_vector_field_data.set_external_int32_ptr(
    integer_buffer.data(), integer_buffer.size());
  VERIFY(CheckFieldDataMeshConversion(
           mesh, 1, external_integer_vector_field_data.name(), 1, { 123, 456, 789 }),
    "Verification failed for external integer vector field data.");

  return true;
}

bool ValidateRectlinearGridWithDifferentDimensions()
{
  conduit_cpp::Node mesh;
  CreateRectilinearMesh(3, 2, 1, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "invalid partition at index 0");
  int dims[3];
  rg->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 2, "incorrect y dimension expected=2, got=%d", dims[1]);
  VERIFY(dims[2] == 1, "incorrect z dimension expected=1, got=%d", dims[2]);

  return true;
}

bool Validate1DRectilinearGrid()
{
  conduit_cpp::Node mesh;
  auto coords = mesh["coordsets/coords"];
  coords["type"] = "rectilinear";
  coords["values/x"].set_float64_vector({ 5.0, 6.0, 7.0 });
  auto topo_mesh = mesh["topologies/mesh"];
  topo_mesh["type"] = "rectilinear";
  topo_mesh["coordset"] = "coords";
  auto field = mesh["fields/field"];
  field["association"] = "element";
  field["topology"] = "mesh";
  field["volume_dependent"] = "false";
  field["values"].set_float64_vector({ 0.0, 1.0 });

  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "invalid partition at index 0");
  int dims[3];
  rg->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 1, "incorrect y dimension expected=1, got=%d", dims[1]);
  VERIFY(dims[2] == 1, "incorrect z dimension expected=1, got=%d", dims[2]);

  return true;
}

inline unsigned int calc(unsigned int i, unsigned int j, unsigned int k, unsigned int I,
  unsigned int J, unsigned int K, unsigned int nx, unsigned int ny)
{
  return (i + I) + (j + J) * nx + (k + K) * (nx * ny);
}

void CreateMixedUnstructuredMesh2D(unsigned int npts_x, unsigned int npts_y, conduit_cpp::Node& res)
{
  conduit_cpp::Node mesh;
  CreateCoords(npts_x, npts_y, 1, res);

  const unsigned int nele_x = npts_x - 1;
  const unsigned int nele_y = npts_y - 1;

  res["state/time"] = 3.1415;
  res["state/cycle"] = 100UL;

  res["topologies/mesh/type"] = "unstructured";
  res["topologies/mesh/coordset"] = "coords";

  res["topologies/mesh/elements/shape"] = "mixed";
  res["topologies/mesh/elements/shape_map/quad"] = VTK_QUAD;
  res["topologies/mesh/elements/shape_map/tri"] = VTK_TRIANGLE;

  const unsigned int nele_x2 = nele_x / 2;
  const unsigned int nquads = nele_y * nele_x2;
  const unsigned int ntris = nele_y * 2 * (nele_x2 + nele_x % 2);
  const unsigned int nele = nquads + ntris;

  std::vector<unsigned int> connectivity, shapes, sizes, offsets;
  shapes.resize(nele);
  sizes.resize(nele);
  offsets.resize(nele);
  offsets[0] = 0;
  connectivity.resize(nquads * 4 + ntris * 3);

  size_t idx_elem = 0;
  size_t idx = 0;

  for (unsigned int j = 0; j < nele_y; ++j)
  {
    for (unsigned int i = 0; i < nele_x; ++i)
    {
      if (i % 2 == 0)
      {
        constexpr int TrianglePointCount = 3;
        shapes[idx_elem + 0] = VTK_TRIANGLE;
        shapes[idx_elem + 1] = VTK_TRIANGLE;
        sizes[idx_elem + 0] = 3;
        sizes[idx_elem + 1] = 3;

        offsets[idx_elem + 1] = offsets[idx_elem + 0] + TrianglePointCount;
        if (idx_elem + 2 < offsets.size())
        {
          offsets[idx_elem + 2] = offsets[idx_elem + 1] + TrianglePointCount;
        }

        connectivity[idx + 0] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 1] = calc(1, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 2] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);

        connectivity[idx + 3] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 4] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 5] = calc(0, 1, 0, i, j, 0, npts_x, npts_y);

        idx_elem += 2;
        idx += 6;
      }
      else
      {
        constexpr int QuadPointCount = 4;
        shapes[idx_elem] = VTK_QUAD;

        sizes[idx_elem] = 4;
        if (idx_elem + 1 < offsets.size())
        {
          offsets[idx_elem + 1] = offsets[idx_elem + 0] + QuadPointCount;
        }

        connectivity[idx + 0] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 1] = calc(1, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 2] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 3] = calc(0, 1, 0, i, j, 0, npts_x, npts_y);

        idx_elem += 1;
        idx += 4;
      }
    }
  }

  auto elements = res["topologies/mesh/elements"];
  elements["shapes"].set(shapes);
  elements["sizes"].set(sizes);
  elements["offsets"].set(offsets);
  elements["connectivity"].set(connectivity);
}

bool ValidateMeshTypeMixed2D()
{
  conduit_cpp::Node mesh;
  CreateMixedUnstructuredMesh2D(5, 5, mesh);
  const auto data = Convert(mesh);

  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(0));

  // 16 triangles, 4 quads: 24 cells
  VERIFY(ug->GetNumberOfCells() == 24, "expected 24 cells, got %lld", ug->GetNumberOfCells());
  VERIFY(ug->GetNumberOfPoints() == 25, "Expected 25 points, got %lld", ug->GetNumberOfPoints());

  // check cell types
  const auto it = vtkSmartPointer<vtkCellIterator>::Take(ug->NewCellIterator());
  int nTris(0), nQuads(0);
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    const int cellType = it->GetCellType();
    switch (cellType)
    {
      case VTK_TRIANGLE:
      {
        ++nTris;
        break;
      }
      case VTK_QUAD:
      {
        ++nQuads;
        break;
      }
      default:
      {
        vtkLog(ERROR, "Expected only triangles and quads.");
        return false;
      }
    }
  }

  return true;
}

void CreateMixedUnstructuredMesh(
  unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ, conduit_cpp::Node& res)
{
  conduit_cpp::Node mesh;
  CreateCoords(nptsX, nptsY, nptsZ, res);

  res["state/time"] = 3.1415;
  res["state/cycle"] = 100UL;

  res["topologies/mesh/type"] = "unstructured";
  res["topologies/mesh/coordset"] = "coords";

  const unsigned int nElementX = nptsX - 1;
  const unsigned int nElementY = nptsY - 1;
  const unsigned int nElementZ = nptsZ - 1;

  const unsigned int nElementX2 = nElementX / 2;
  // one hexa divided into 3 tetras and one polyhedron (prism)
  const unsigned int nTet = 3 * nElementZ * nElementY * (nElementX2 + nElementX % 2);
  const unsigned int nPolyhedra = nElementZ * nElementY * (nElementX2 + nElementX % 2);
  // one hexa as hexahedron
  const unsigned int nHex = nElementZ * nElementY * nElementX2;

  const unsigned int nFaces = 5 * nPolyhedra;
  const unsigned int nEle = nTet + nHex + nPolyhedra;

  res["topologies/mesh/elements/shape"] = "mixed";
  res["topologies/mesh/elements/shape_map/polyhedral"] = VTK_POLYHEDRON;
  res["topologies/mesh/elements/shape_map/tet"] = VTK_TETRA;
  res["topologies/mesh/elements/shape_map/hex"] = VTK_HEXAHEDRON;

  res["topologies/mesh/subelements/shape"] = "mixed";
  res["topologies/mesh/subelements/shape_map/quad"] = VTK_QUAD;
  res["topologies/mesh/subelements/shape_map/tri"] = VTK_TRIANGLE;

  std::vector<unsigned int> elem_connectivity, elem_shapes, elem_sizes, elem_offsets;
  elem_shapes.resize(nEle);
  elem_sizes.resize(nEle);
  elem_offsets.resize(nEle);
  elem_connectivity.resize(nTet * 4 + nPolyhedra * 5 + nHex * 8);
  elem_offsets[0] = 0;

  std::vector<unsigned int> subelem_connectivity, subelem_shapes, subelem_sizes, subelem_offsets;
  subelem_shapes.resize(nFaces);
  subelem_sizes.resize(nFaces);
  subelem_offsets.resize(nFaces);
  subelem_connectivity.resize(nPolyhedra * 18);
  subelem_offsets[0] = 0;

  unsigned int idx_elem = 0;
  unsigned int idx = 0;
  unsigned int idx_elem2 = 0;
  unsigned int idx2 = 0;
  unsigned int polyhedronCounter = 0;

  for (unsigned int k = 0; k < nElementZ; ++k)
  {
    for (unsigned int j = 0; j < nElementZ; ++j)
    {
      for (unsigned int i = 0; i < nElementX; ++i)
      {
        if (i % 2 == 1) // hexahedron
        {
          constexpr int HexaPointCount = 8;

          elem_shapes[idx_elem] = VTK_HEXAHEDRON;
          elem_sizes[idx_elem] = HexaPointCount;
          if (idx_elem + 1 < elem_offsets.size())
          {
            elem_offsets[idx_elem + 1] = elem_offsets[idx_elem] + HexaPointCount;
          }

          elem_connectivity[idx + 0] = calc(0, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 1] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 2] = calc(1, 1, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 3] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 4] = calc(0, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 5] = calc(1, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 6] = calc(1, 1, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 7] = calc(0, 1, 1, i, j, k, nptsX, nptsY);

          idx_elem += 1;
          idx += HexaPointCount;
        }
        else // 3 tets, one polyhedron
        {
          elem_shapes[idx_elem + 0] = VTK_TETRA;
          elem_shapes[idx_elem + 1] = VTK_TETRA;
          elem_shapes[idx_elem + 2] = VTK_TETRA;
          elem_shapes[idx_elem + 3] = VTK_POLYHEDRON;

          constexpr int TetraPointCount = 4;
          constexpr int WedgeFaceCount = 5;
          constexpr int TrianglePointCount = 3;
          constexpr int QuadPointCount = 4;

          elem_sizes[idx_elem + 0] = TetraPointCount;
          elem_sizes[idx_elem + 1] = TetraPointCount;
          elem_sizes[idx_elem + 2] = TetraPointCount;
          elem_sizes[idx_elem + 3] = WedgeFaceCount;

          elem_offsets[idx_elem + 1] = elem_offsets[idx_elem + 0] + TetraPointCount;
          elem_offsets[idx_elem + 2] = elem_offsets[idx_elem + 1] + TetraPointCount;
          elem_offsets[idx_elem + 3] = elem_offsets[idx_elem + 2] + TetraPointCount;
          if (idx_elem + 4 < elem_offsets.size())
          {
            elem_offsets[idx_elem + 4] = elem_offsets[idx_elem + 3] + WedgeFaceCount;
          }

          elem_connectivity[idx + 0] = calc(0, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 1] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 2] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 3] = calc(0, 0, 1, i, j, k, nptsX, nptsY);

          elem_connectivity[idx + 4] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 5] = calc(1, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 6] = calc(0, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 7] = calc(0, 1, 1, i, j, k, nptsX, nptsY);

          elem_connectivity[idx + 8] = calc(0, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 9] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 10] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 11] = calc(1, 0, 0, i, j, k, nptsX, nptsY);

          // note: there are no shared faces in this example
          elem_connectivity[idx + 12] = 0 + WedgeFaceCount * polyhedronCounter;
          elem_connectivity[idx + 13] = 1 + WedgeFaceCount * polyhedronCounter;
          elem_connectivity[idx + 14] = 2 + WedgeFaceCount * polyhedronCounter;
          elem_connectivity[idx + 15] = 3 + WedgeFaceCount * polyhedronCounter;
          elem_connectivity[idx + 16] = 4 + WedgeFaceCount * polyhedronCounter;

          subelem_shapes[idx_elem2 + 0] = VTK_QUAD;
          subelem_shapes[idx_elem2 + 1] = VTK_QUAD;
          subelem_shapes[idx_elem2 + 2] = VTK_QUAD;
          subelem_shapes[idx_elem2 + 3] = VTK_TRIANGLE;
          subelem_shapes[idx_elem2 + 4] = VTK_TRIANGLE;

          subelem_sizes[idx_elem2 + 0] = QuadPointCount;
          subelem_sizes[idx_elem2 + 1] = QuadPointCount;
          subelem_sizes[idx_elem2 + 2] = QuadPointCount;
          subelem_sizes[idx_elem2 + 3] = TrianglePointCount;
          subelem_sizes[idx_elem2 + 4] = TrianglePointCount;

          subelem_offsets[idx_elem2 + 1] = subelem_offsets[idx_elem2 + 0] + QuadPointCount;
          subelem_offsets[idx_elem2 + 2] = subelem_offsets[idx_elem2 + 1] + QuadPointCount;
          subelem_offsets[idx_elem2 + 3] = subelem_offsets[idx_elem2 + 2] + QuadPointCount;
          subelem_offsets[idx_elem2 + 4] = subelem_offsets[idx_elem2 + 3] + TrianglePointCount;
          if (idx_elem2 + 5 < subelem_offsets.size())
          {
            subelem_offsets[idx_elem2 + 5] = subelem_offsets[idx_elem2 + 4] + TrianglePointCount;
          }

          subelem_connectivity[idx2 + 0] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 1] = calc(1, 0, 1, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 2] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 3] = calc(0, 1, 0, i, j, k, nptsX, nptsY);

          subelem_connectivity[idx2 + 4] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 5] = calc(1, 1, 0, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 6] = calc(1, 1, 1, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 7] = calc(1, 0, 1, i, j, k, nptsX, nptsY);

          subelem_connectivity[idx2 + 8] = calc(1, 1, 0, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 9] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 10] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 11] = calc(1, 1, 1, i, j, k, nptsX, nptsY);

          subelem_connectivity[idx2 + 12] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 13] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 14] = calc(1, 1, 0, i, j, k, nptsX, nptsY);

          subelem_connectivity[idx2 + 15] = calc(1, 1, 1, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 16] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
          subelem_connectivity[idx2 + 17] = calc(1, 0, 1, i, j, k, nptsX, nptsY);

          idx_elem += 4; // three tets, 1 polyhedron
          idx += 3 * TetraPointCount + WedgeFaceCount;
          polyhedronCounter += 1;
          idx_elem2 += WedgeFaceCount; // five faces on the polyhedron
          idx2 += 3 * QuadPointCount + 2 * TrianglePointCount;
        }
      }
    }
  }

  auto elements = res["topologies/mesh/elements"];
  elements["shapes"].set(elem_shapes);
  elements["offsets"].set(elem_offsets);
  elements["sizes"].set(elem_sizes);
  elements["connectivity"].set(elem_connectivity);

  auto subelements = res["topologies/mesh/subelements"];
  subelements["shapes"].set(subelem_shapes);
  subelements["offsets"].set(subelem_offsets);
  subelements["sizes"].set(subelem_sizes);
  subelements["connectivity"].set(subelem_connectivity);
}

bool ValidateMeshTypeMixed()
{
  conduit_cpp::Node mesh;
  constexpr int nX = 5, nY = 5, nZ = 5;
  CreateMixedUnstructuredMesh(5, 5, 5, mesh);
  const auto data = Convert(mesh);

  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  const auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(0));

  VERIFY(ug->GetNumberOfPoints() == nX * nY * nZ, "expected %d points got %lld", nX * nY * nZ,
    ug->GetNumberOfPoints());

  // 160 cells expected: 4 layers of
  //                     - 2 columns with 4 hexahedra
  //                     - 2 columns with 4 polyhedra (wedges) and 12 tetra
  //                     96 tetras + 32 hexas + 32 polyhedra
  VERIFY(ug->GetNumberOfCells() == 160, "expected 160 cells, got %lld", ug->GetNumberOfCells());

  // check cell types
  const auto it = vtkSmartPointer<vtkCellIterator>::Take(ug->NewCellIterator());

  int nPolyhedra(0), nTetra(0), nHexa(0), nCells(0);
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    ++nCells;
    const int cellType = it->GetCellType();
    switch (cellType)
    {
      case VTK_POLYHEDRON:
      {
        ++nPolyhedra;
        const vtkIdType nFaces = it->GetNumberOfFaces();
        VERIFY(nFaces == 5, "Expected 5 faces, got %lld", nFaces);
        break;
      }
      case VTK_HEXAHEDRON:
      {
        ++nHexa;
        break;
      }
      case VTK_TETRA:
      {
        ++nTetra;
        break;
      }
      default:
      {
        vtkLog(ERROR, "Expected only tetras, hexas and polyhedra.");
        return false;
      }
    }
  }

  VERIFY(nCells == 160, "Expected 160 cells, got %d", nCells);
  VERIFY(nTetra == 96, "Expected 96 tetras, got %d", nTetra);
  VERIFY(nHexa == 32, "Expected 32 hexahedra, got %d", nHexa);
  VERIFY(nPolyhedra == 32, "Expected 32 polyhedra, got %d", nPolyhedra);

  return true;
}

} // end namespace

int TestConduitSource(int, char*[])
{
  return ValidateMeshTypeUniform() && ValidateMeshTypeRectilinear() &&
      ValidateMeshTypeStructured() && ValidateMeshTypeUnstructured() && ValidateFieldData() &&
      ValidateRectlinearGridWithDifferentDimensions() && Validate1DRectilinearGrid() &&
      ValidateMeshTypeMixed() && ValidateMeshTypeMixed2D()
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
