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

bool ValidateMeshTypeUniform()
{
  conduit_cpp::Node mesh;
  conduit_cpp::BlueprintMesh::Example::basic("uniform", 3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto img = vtkImageData::SafeDownCast(pds->GetPartition(0));
  VERIFY(img != nullptr, "missing partition 0");
  VERIFY(vtkVector3i(img->GetDimensions()) == vtkVector3i(3, 3, 3),
    "incorrect dimensions, expected=3x3x3, got=%dx%dx%d", img->GetDimensions()[0],
    img->GetDimensions()[1], img->GetDimensions()[2]);

  return true;
}

bool ValidateMeshTypeRectilinear()
{
  conduit_cpp::Node mesh;
  conduit_cpp::BlueprintMesh::Example::basic("rectilinear", 3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "missing partition 0");
  VERIFY(vtkVector3i(rg->GetDimensions()) == vtkVector3i(3, 3, 3),
    "incorrect dimensions, expected=3x3x3, got=%dx%dx%d", rg->GetDimensions()[0],
    rg->GetDimensions()[1], rg->GetDimensions()[2]);

  return true;
}

bool ValidateMeshTypeStructured()
{
  conduit_cpp::Node mesh;
  conduit_cpp::BlueprintMesh::Example::basic("structured", 3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto sg = vtkStructuredGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(sg != nullptr, "missing partition 0");
  VERIFY(vtkVector3i(sg->GetDimensions()) == vtkVector3i(3, 3, 3),
    "incorrect dimensions, expected=3x3x3, got=%dx%dx%d", sg->GetDimensions()[0],
    sg->GetDimensions()[1], sg->GetDimensions()[2]);

  return true;
}

bool ValidateMeshTypeUnstructured()
{
  conduit_cpp::Node mesh;
  // generate simple explicit tri-based 2d 'basic' mesh
  conduit_cpp::BlueprintMesh::Example::basic("tris", 3, 3, 0, mesh);

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
  conduit_cpp::BlueprintMesh::Example::basic("uniform", 3, 3, 3, mesh);

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
  conduit_cpp::BlueprintMesh::Example::basic("rectilinear", 3, 2, 1, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "invalid partition at index 0");
  VERIFY(vtkVector3i(rg->GetDimensions()) == vtkVector3i(3, 2, 1),
    "incorrect dimensions, expected=3x2x1, got=%dx%dx%d", rg->GetDimensions()[0],
    rg->GetDimensions()[1], rg->GetDimensions()[2]);

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
  VERIFY(vtkVector3i(rg->GetDimensions()) == vtkVector3i(3, 1, 1),
    "incorrect dimensions, expected=3x1x1, got=%dx%dx%d", rg->GetDimensions()[0],
    rg->GetDimensions()[1], rg->GetDimensions()[2]);

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
