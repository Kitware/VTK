// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataObjectToConduit.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkTypeUInt8Array.h"
#include "vtkUnstructuredGrid.h"

#include <catalyst_conduit.hpp>

namespace
{

//----------------------------------------------------------------------------
bool IsMixedShape(vtkUnstructuredGrid* unstructured_grid)
{
  auto* cell_types = unstructured_grid->GetDistinctCellTypesArray();
  return cell_types->GetNumberOfTuples() > 1;
}

//----------------------------------------------------------------------------
bool IsMixedShape(vtkPolyData* grid)
{
  // WARNING: This is inefficient
  vtkNew<vtkCellTypes> cell_types;
  grid->GetCellTypes(cell_types);
  return cell_types->GetNumberOfTypes() > 1;
}

//----------------------------------------------------------------------------
vtkCellArray* GetCells(vtkUnstructuredGrid* ugrid, int)
{
  return ugrid->GetCells();
}

//----------------------------------------------------------------------------
vtkCellArray* GetCells(vtkPolyData* polydata, int cellType)
{
  switch (cellType)
  {
    case VTK_POLYGON:
    case VTK_QUAD:
    case VTK_TRIANGLE:
      return polydata->GetPolys();
    case VTK_LINE:
      return polydata->GetLines();
    case VTK_VERTEX:
      return polydata->GetVerts();
    default:
      vtkLog(ERROR, << "Unsupported cell type in polydata. Cell type: "
                    << vtkCellTypes::GetClassNameFromTypeId(cellType));
      return nullptr;
  }
}
//----------------------------------------------------------------------------
bool IsSignedIntegralType(int data_type)
{
  constexpr bool is_char_type_signed = (CHAR_MIN == SCHAR_MIN) && (CHAR_MAX == SCHAR_MAX);

  return (is_char_type_signed && (data_type == VTK_CHAR)) || (data_type == VTK_SIGNED_CHAR) ||
    (data_type == VTK_SHORT) || (data_type == VTK_INT) || (data_type == VTK_LONG) ||
    (data_type == VTK_ID_TYPE) || (data_type == VTK_LONG_LONG) || (data_type == VTK_TYPE_INT64);
}

//----------------------------------------------------------------------------
bool IsUnsignedIntegralType(int data_type)
{
  constexpr bool is_char_type_signed = (CHAR_MIN == SCHAR_MIN) && (CHAR_MAX == SCHAR_MAX);

  return (!is_char_type_signed && (data_type == VTK_CHAR)) || (data_type == VTK_UNSIGNED_CHAR) ||
    (data_type == VTK_UNSIGNED_SHORT) || (data_type == VTK_UNSIGNED_INT) ||
    (data_type == VTK_UNSIGNED_LONG) || (data_type == VTK_ID_TYPE) ||
    (data_type == VTK_UNSIGNED_LONG_LONG);
}

//----------------------------------------------------------------------------
bool IsFloatType(int data_type)
{
  return ((data_type == VTK_FLOAT) || (data_type == VTK_DOUBLE));
}

//----------------------------------------------------------------------------
bool ConvertDataArrayToMCArray(vtkDataArray* data_array, int offset, int stride,
  conduit_cpp::Node& conduit_node, int array_size = -1)
{
  stride = std::max(stride, 1);

  conduit_index_t number_of_elements;
  if (array_size == -1)
  {
    number_of_elements = data_array->GetNumberOfValues() / stride;
  }
  else
  {
    number_of_elements = array_size / stride;
  }

  int data_type = data_array->GetDataType();
  int data_type_size = data_array->GetDataTypeSize();
  int array_type = data_array->GetArrayType();

  if (array_type != vtkAbstractArray::AoSDataArrayTemplate)
  {
    vtkLog(ERROR, "Unsupported data array type: " << data_array->GetDataTypeAsString());
    return false;
  }

  // The code below uses the legacy GetVoidPointer on purpose to get zero copy.
  bool is_supported = true;
  if (IsSignedIntegralType(data_type))
  {
    switch (data_type_size)
    {
      case 1:
        conduit_node.set_external_int8_ptr((conduit_int8*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_int8), stride * sizeof(conduit_int8));
        break;

      case 2:
        conduit_node.set_external_int16_ptr((conduit_int16*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_int16), stride * sizeof(conduit_int16));
        break;

      case 4:
        conduit_node.set_external_int32_ptr((conduit_int32*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_int32), stride * sizeof(conduit_int32));
        break;

      case 8:
        conduit_node.set_external_int64_ptr((conduit_int64*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_int64), stride * sizeof(conduit_int64));
        break;

      default:
        is_supported = false;
    }
  }
  else if (IsUnsignedIntegralType(data_type))
  {
    switch (data_type_size)
    {
      case 1:
        conduit_node.set_external_uint8_ptr((conduit_uint8*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_uint8), stride * sizeof(conduit_uint8));
        break;

      case 2:
        conduit_node.set_external_uint16_ptr((conduit_uint16*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_uint16), stride * sizeof(conduit_uint16));
        break;

      case 4:
        conduit_node.set_external_uint32_ptr((conduit_uint32*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_uint32), stride * sizeof(conduit_uint32));
        break;

      case 8:
        conduit_node.set_external_uint64_ptr((conduit_uint64*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_uint64), stride * sizeof(conduit_uint64));
        break;

      default:
        is_supported = false;
    }
  }
  else if (IsFloatType(data_type))
  {
    switch (data_type_size)
    {
      case 4:
        conduit_node.set_external_float32_ptr((conduit_float32*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_float32), stride * sizeof(conduit_float32));
        break;

      case 8:
        conduit_node.set_external_float64_ptr((conduit_float64*)data_array->GetVoidPointer(0),
          number_of_elements, offset * sizeof(conduit_float64), stride * sizeof(conduit_float64));
        break;

      default:
        is_supported = false;
    }
  }

  if (!is_supported)
  {
    vtkLog(ERROR,
      "Unsupported data array type: " << data_array->GetDataTypeAsString()
                                      << " size: " << data_type_size << " type: " << array_type);
  }

  return is_supported;
}

//----------------------------------------------------------------------------
bool ConvertDataArrayToMCArray(vtkDataArray* data_array, conduit_cpp::Node& conduit_node,
  const std::vector<std::string> names = std::vector<std::string>())
{
  size_t nComponents = data_array->GetNumberOfComponents();
  if (nComponents > 1)
  {
    bool success = true;
    for (size_t i = 0; i < nComponents; ++i)
    {
      conduit_cpp::Node component_node;
      if (i < names.size())
      {
        component_node = conduit_node[names[i]];
      }
      else
      {
        component_node = conduit_node[std::to_string(i)];
      }
      success = success && ConvertDataArrayToMCArray(data_array, i, nComponents, component_node);
    }
    return success;
  }
  else
  {
    return ConvertDataArrayToMCArray(data_array, 0, 0, conduit_node);
  }
}

//----------------------------------------------------------------------------
bool FillMixedShape(vtkPolyData* dataset, conduit_cpp::Node& topologies_node)
{
  (void)dataset; // Avoid compiler warning for unused variable
  (void)topologies_node;

  return false; // Not implemented (yet)
}

//----------------------------------------------------------------------------
bool FillMixedShape(vtkUnstructuredGrid* dataset, conduit_cpp::Node& topologies_node)
{
  const auto number_of_cells = dataset->GetNumberOfCells();
  topologies_node["elements/shape"].set("mixed");

  auto shape_map = topologies_node["elements/shape_map"];
  shape_map["hex"] = VTK_HEXAHEDRON;
  shape_map["tet"] = VTK_TETRA;
  shape_map["quad"] = VTK_QUAD;
  shape_map["tri"] = VTK_TRIANGLE;
  shape_map["polygonal"] = VTK_POLYGON;

  auto offsets = dataset->GetCells()->GetOffsetsArray();
  auto shapes = dataset->GetCellTypesArray();

  vtkNew<vtkIdTypeArray> sizes;
  sizes->SetName("vtkCellSizes");
  sizes->SetNumberOfTuples(number_of_cells);
  for (vtkIdType i = 0; i < number_of_cells; i++)
  {
    sizes->SetValue(i, dataset->GetCellSize(i));
  }

  // We need allocated heap memory for the size array, which is not stored natively in the
  // dataset. To avoid Conduit making a copy or a memory leak, we attach the array to the
  // dataset. Warning : this creates a side-effect, impacting the input dataset by adding a
  // field.
  dataset->GetCellData()->AddArray(sizes);

  auto offsets_node = topologies_node["elements/offsets"];
  auto shapes_node = topologies_node["elements/shapes"];
  auto sizes_node = topologies_node["elements/sizes"];

  // Conduit offsets array is of size `number_of_cells` and not `number_of_cells + 1`.
  if (!ConvertDataArrayToMCArray(offsets, 0, 0, offsets_node, number_of_cells) ||
    !ConvertDataArrayToMCArray(shapes, shapes_node) ||
    !ConvertDataArrayToMCArray(sizes, sizes_node))
  {
    vtkLogF(ERROR, "ConvertDataArrayToMCArray failed for mixed shapes unstructured grid.");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
template <class T>
bool FillTopology(T* dataset, conduit_cpp::Node& conduit_node)
{
  const char* datasetType = dataset->GetClassName();

  auto coords_node = conduit_node["coordsets/coords"];

  coords_node["type"] = "explicit";

  auto values_node = coords_node["values"];
  auto* points = dataset->GetPoints();

  if (points)
  {
    if (!ConvertDataArrayToMCArray(points->GetData(), values_node, { "x", "y", "z" }))
    {
      vtkLogF(ERROR, "ConvertPoints failed for %s.", datasetType);
      return false;
    }
  }
  else
  {
    values_node["x"] = std::vector<float>();
    values_node["y"] = std::vector<float>();
    values_node["z"] = std::vector<float>();
  }

  auto topologies_node = conduit_node["topologies/mesh"];
  topologies_node["type"] = "unstructured";
  topologies_node["coordset"] = "coords";

  int cell_type = VTK_VERTEX;

  if (IsMixedShape(dataset))
  {
    if (!FillMixedShape(dataset, topologies_node))
    {
      vtkLogF(ERROR, "%s with mixed shape type partially supported.", datasetType);
      return false;
    }
  }
  else
  {
    const auto number_of_cells = dataset->GetNumberOfCells();

    if (number_of_cells > 0)
    {
      cell_type = dataset->GetCellType(0);
    }

    switch (cell_type)
    {
      case VTK_HEXAHEDRON:
        topologies_node["elements/shape"] = "hex";
        break;
      case VTK_TETRA:
        topologies_node["elements/shape"] = "tet";
        break;
      case VTK_POLYGON:
        topologies_node["elements/shape"] = "polygonal";
        break;
      case VTK_QUAD:
        topologies_node["elements/shape"] = "quad";
        break;
      case VTK_TRIANGLE:
        topologies_node["elements/shape"] = "tri";
        break;
      case VTK_LINE:
        topologies_node["elements/shape"] = "line";
        break;
      case VTK_VERTEX:
        topologies_node["elements/shape"] = "point";
        break;
      default:
        vtkLogF(ERROR, "Unsupported cell type in %s. Cell type: %s", datasetType,
          vtkCellTypes::GetClassNameFromTypeId(cell_type));
        return false;
    }
  }

  auto cell_connectivity = GetCells(dataset, cell_type);
  auto connectivity_node = topologies_node["elements/connectivity"];

  if (!ConvertDataArrayToMCArray(cell_connectivity->GetConnectivityArray(), connectivity_node))
  {
    vtkLogF(ERROR, "ConvertDataArrayToMCArray failed for %s.", datasetType);
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool FillTopology(vtkDataSet* data_set, conduit_cpp::Node& conduit_node)
{
  if (auto imageData = vtkImageData::SafeDownCast(data_set))
  {
    auto coords_node = conduit_node["coordsets/coords"];

    coords_node["type"] = "uniform";

    int* dimensions = imageData->GetDimensions();
    coords_node["dims/i"] = dimensions[0];
    coords_node["dims/j"] = dimensions[1];
    coords_node["dims/k"] = dimensions[2];

    double* origin = imageData->GetOrigin();
    coords_node["origin/x"] = origin[0];
    coords_node["origin/y"] = origin[1];
    coords_node["origin/z"] = origin[2];

    double* spacing = imageData->GetSpacing();
    coords_node["spacing/dx"] = spacing[0];
    coords_node["spacing/dy"] = spacing[1];
    coords_node["spacing/dz"] = spacing[2];

    auto topologies_node = conduit_node["topologies/mesh"];
    topologies_node["type"] = "uniform";
    topologies_node["coordset"] = "coords";
  }
  else if (auto rectilinear_grid = vtkRectilinearGrid::SafeDownCast(data_set))
  {
    auto coords_node = conduit_node["coordsets/coords"];

    coords_node["type"] = "rectilinear";

    auto x_values_node = coords_node["values/x"];
    if (!ConvertDataArrayToMCArray(rectilinear_grid->GetXCoordinates(), x_values_node))
    {
      vtkLog(ERROR, "Failed ConvertDataArrayToMCArray for values/x");
      return false;
    }

    auto y_values_node = coords_node["values/y"];
    if (!ConvertDataArrayToMCArray(rectilinear_grid->GetYCoordinates(), y_values_node))
    {
      vtkLog(ERROR, "Failed ConvertDataArrayToMCArray for values/y");
      return false;
    }

    auto z_values_node = coords_node["values/z"];
    if (!ConvertDataArrayToMCArray(rectilinear_grid->GetZCoordinates(), z_values_node))
    {
      vtkLog(ERROR, "Failed ConvertDataArrayToMCArray for values/z");
      return false;
    }

    auto topologies_node = conduit_node["topologies/mesh"];
    topologies_node["type"] = "rectilinear";
    topologies_node["coordset"] = "coords";
  }
  else if (auto structured_grid = vtkStructuredGrid::SafeDownCast(data_set))
  {
    auto coords_node = conduit_node["coordsets/coords"];

    coords_node["type"] = "explicit";

    auto values_node = coords_node["values"];
    if (!ConvertDataArrayToMCArray(
          structured_grid->GetPoints()->GetData(), values_node, { "x", "y", "z" }))
    {
      vtkLog(ERROR, "Failed ConvertPoints for structured grid");
      return false;
    }

    auto topologies_node = conduit_node["topologies/mesh"];
    topologies_node["type"] = "structured";
    topologies_node["coordset"] = "coords";
    int dimensions[3];
    structured_grid->GetDimensions(dimensions);
    topologies_node["elements/dims/i"] = dimensions[0];
    topologies_node["elements/dims/j"] = dimensions[1];
    topologies_node["elements/dims/k"] = dimensions[2];
  }
  else if (auto unstructured_grid = vtkUnstructuredGrid::SafeDownCast(data_set))
  {
    return FillTopology(unstructured_grid, conduit_node);
  }
  else if (auto polydata = vtkPolyData::SafeDownCast(data_set))
  {
    return FillTopology(polydata, conduit_node);
  }
  else
  {
    vtkLog(ERROR, "Unsupported data set type: " << data_set->GetClassName());
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool FillFields(
  vtkFieldData* field_data, const std::string& association, conduit_cpp::Node& conduit_node)
{
  bool is_success = true;

  int array_count = field_data->GetNumberOfArrays();
  for (int array_index = 0; is_success && array_index < array_count; ++array_index)
  {
    auto array = field_data->GetAbstractArray(array_index);
    auto name = array->GetName();
    if (!name)
    {
      vtkLogF(WARNING, "Unnamed array, it will be ignored.");
      continue;
    }

    if (association.empty())
    {
      // VTK Field Data are translated to state/fields childs.
      auto field_node = conduit_node["state/fields"][name];

      if (auto string_array = vtkStringArray::SafeDownCast(array))
      {
        if (string_array->GetNumberOfValues() > 0)
        {
          field_node.set_string(string_array->GetValue(0));
          if (string_array->GetNumberOfValues() > 1)
          {
            vtkLog(WARNING,
              "The string array '" << string_array->GetName()
                                   << "' contains more than one element. Only the first one will "
                                      "be converted to conduit node.");
          }
        }
      }
      else if (auto data_array = vtkDataArray::SafeDownCast(array))
      {
        is_success = ConvertDataArrayToMCArray(data_array, field_node);
      }
      else
      {
        vtkLogF(ERROR, "Unknown array type '%s' in Field Data.", name);
        is_success = false;
      }
    }
    else if (auto data_array = vtkDataArray::SafeDownCast(array))
    {
      auto field_node = conduit_node["fields"][name];
      field_node["association"] = association;
      field_node["topology"] = "mesh";
      field_node["volume_dependent"] = "false";

      auto values_node = field_node["values"];
      is_success = ConvertDataArrayToMCArray(data_array, values_node);
    }
    else
    {
      vtkLogF(ERROR, "Unknown array type '%s' associated to: %s", name, association.c_str());
      is_success = false;
    }
  }

  return is_success;
}

//----------------------------------------------------------------------------
bool FillFields(vtkDataSet* data_set, conduit_cpp::Node& conduit_node)
{
  if (auto cell_data = data_set->GetCellData())
  {
    if (!FillFields(cell_data, "element", conduit_node))
    {
      vtkVLog(vtkLogger::VERBOSITY_ERROR, "FillFields with element failed.");
      return false;
    }
  }

  if (auto point_data = data_set->GetPointData())
  {
    if (!FillFields(point_data, "vertex", conduit_node))
    {
      vtkVLog(vtkLogger::VERBOSITY_ERROR, "FillFields with vertex failed.");
      return false;
    }
  }

  if (auto field_data = data_set->GetFieldData())
  {
    if (!FillFields(field_data, "", conduit_node))
    {
      vtkVLog(vtkLogger::VERBOSITY_ERROR, "FillFields with field data failed.");
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool FillConduitNodeFromDataSet(vtkDataSet* data_set, conduit_cpp::Node& conduit_node)
{
  return FillFields(data_set, conduit_node) && FillTopology(data_set, conduit_node);
}

} // anonymous namespace

namespace vtkDataObjectToConduit
{
VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
bool FillConduitNode(vtkDataObject* data_object, conduit_cpp::Node& conduit_node)
{
  auto data_set = vtkDataSet::SafeDownCast(data_object);
  if (!data_set)
  {
    vtkLogF(ERROR, "Only Data Set objects are supported in vtkDataObjectToConduit.");
    return false;
  }

  return FillConduitNodeFromDataSet(data_set, conduit_node);
}

VTK_ABI_NAMESPACE_END
} // vtkDataObjectToConduit namespace
