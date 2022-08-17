/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConduitSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellData.h"
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
#include "vtkVectorOperators.h"

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

void CreateStructuredMesh(
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

}

int TestConduitSource(int, char*[])
{
  return ValidateMeshTypeUniform() && ValidateMeshTypeRectilinear() &&
      ValidateMeshTypeStructured() && ValidateMeshTypeUnstructured() && ValidateFieldData() &&
      ValidateRectlinearGridWithDifferentDimensions() && Validate1DRectilinearGrid()
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
