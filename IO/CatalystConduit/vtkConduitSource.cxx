// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConduitSource.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkConduitArrayUtilities.h"
#include "vtkConvertToMultiBlockDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataAssembly.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <set>

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
int GetAssociation(const std::string& assoc)
{
  if (assoc == "element")
  {
    return vtkDataObject::CELL;
  }
  else if (assoc == "vertex")
  {
    return vtkDataObject::POINT;
  }

  throw std::runtime_error("unsupported association " + assoc);
}

//----------------------------------------------------------------------------
int GetCellType(const std::string& shape)
{
  if (shape == "point")
  {
    return VTK_VERTEX;
  }
  else if (shape == "line")
  {
    return VTK_LINE;
  }
  else if (shape == "tri")
  {
    return VTK_TRIANGLE;
  }
  else if (shape == "quad")
  {
    return VTK_QUAD;
  }
  else if (shape == "tet")
  {
    return VTK_TETRA;
  }
  else if (shape == "hex")
  {
    return VTK_HEXAHEDRON;
  }
  else if (shape == "polyhedral")
  {
    return VTK_POLYHEDRON;
  }
  else if (shape == "polygonal")
  {
    return VTK_POLYGON;
  }
  else
  {
    throw std::runtime_error("unsupported shape " + shape);
  }
}

//----------------------------------------------------------------------------
// internal: get number of points in VTK cell type.
static vtkIdType GetNumberOfPointsInCellType(int vtk_cell_type)
{
  switch (vtk_cell_type)
  {
    case VTK_VERTEX:
      return 1;
    case VTK_LINE:
      return 2;
    case VTK_TRIANGLE:
      return 3;
    case VTK_QUAD:
    case VTK_TETRA:
      return 4;
    case VTK_HEXAHEDRON:
      return 8;
    default:
      throw std::runtime_error("unsupported cell type " + std::to_string(vtk_cell_type));
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> CreatePoints(const conduit_cpp::Node& coords)
{
  if (coords["type"].as_string() != "explicit")
  {
    throw std::runtime_error("invalid node!");
  }

  conduit_cpp::Node values = coords["values"];
  auto array = vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values), "coords");
  if (array == nullptr)
  {
    throw std::runtime_error("failed to convert to VTK array!");
  }
  if (array->GetNumberOfComponents() < 3)
  {
    array = vtkConduitArrayUtilities::SetNumberOfComponents(array, 3);
  }
  else if (array->GetNumberOfComponents() > 3)
  {
    throw std::runtime_error("points cannot have more than 3 components!");
  }

  vtkNew<vtkPoints> pts;
  pts->SetData(array);
  return pts;
}

//----------------------------------------------------------------------------
void SetPolyhedralCells(
  vtkUnstructuredGrid* grid, vtkCellArray* elements, vtkCellArray* subelements)
{
  vtkNew<vtkCellArray> connectivity;
  vtkNew<vtkIdTypeArray> faces;
  vtkNew<vtkIdTypeArray> faceLocations;

  connectivity->AllocateEstimate(elements->GetNumberOfCells(), 10);
  faces->Allocate(subelements->GetConnectivityArray()->GetNumberOfTuples());
  faceLocations->Allocate(elements->GetNumberOfCells());

  auto eIter = vtk::TakeSmartPointer(elements->NewIterator());
  auto seIter = vtk::TakeSmartPointer(subelements->NewIterator());

  std::vector<vtkIdType> cellPoints;
  for (eIter->GoToFirstCell(); !eIter->IsDoneWithTraversal(); eIter->GoToNextCell())
  {
    // init;
    cellPoints.clear();

    // get cell from 'elements'.
    vtkIdType size;
    vtkIdType const* seIds;
    eIter->GetCurrentCell(size, seIds);

    faceLocations->InsertNextValue(faces->GetNumberOfTuples());
    faces->InsertNextValue(size); // number-of-cell-faces.
    for (vtkIdType fIdx = 0; fIdx < size; ++fIdx)
    {
      seIter->GoToCell(seIds[fIdx]);

      vtkIdType ptSize;
      vtkIdType const* ptIds;
      seIter->GetCurrentCell(ptSize, ptIds);
      faces->InsertNextValue(ptSize); // number-of-face-points.
      for (vtkIdType ptIdx = 0; ptIdx < ptSize; ++ptIdx)
      {
        faces->InsertNextValue(ptIds[ptIdx]);
      }

      // accumulate pts from all faces in this cell to build the 'connectivity' array.
      std::copy(ptIds, ptIds + ptSize, std::back_inserter(cellPoints));
    }

    connectivity->InsertNextCell(
      static_cast<vtkIdType>(cellPoints.size()), cellPoints.empty() ? nullptr : cellPoints.data());
  }

  connectivity->Squeeze();
  faces->Squeeze();
  faceLocations->Squeeze();

  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfTuples(connectivity->GetNumberOfCells());
  cellTypes->FillValue(static_cast<unsigned char>(VTK_POLYHEDRON));
  grid->SetCells(cellTypes, connectivity, faceLocations, faces);
}

struct MixedPolyhedralCells
{
  conduit_cpp::Node* ElementShapes;
  conduit_cpp::Node* ElementSizes;
  conduit_cpp::Node* ElementOffsets;

  conduit_cpp::Node* SubElementSizes;
  conduit_cpp::Node* SubElementOffsets;

  MixedPolyhedralCells(conduit_cpp::Node* elementShapes, conduit_cpp::Node* elementSizes,
    conduit_cpp::Node* elementOffsets, conduit_cpp::Node* subElementSizes,
    conduit_cpp::Node* subElementOffsets)
    : ElementShapes(elementShapes)
    , ElementSizes(elementSizes)
    , ElementOffsets(elementOffsets)
    , SubElementSizes(subElementSizes)
    , SubElementOffsets(subElementOffsets)
  {
  }

  template <typename ConnectivityArray, typename SubConnectivityArray>
  void operator()(ConnectivityArray* elementConnectivity,
    SubConnectivityArray* subElementConnectivity, vtkUnstructuredGrid* ug)
  {
    using ConnectivityArrayType = vtk::GetAPIType<ConnectivityArray>;
    using SubConnectivityArrayType = vtk::GetAPIType<SubConnectivityArray>;

    const auto elementShapesArray =
      vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(this->ElementShapes));
    const auto elementSizesArray =
      vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(this->ElementSizes));
    const auto elementOffsetsArray =
      vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(this->ElementOffsets));

    vtkSmartPointer<vtkDataArray> subElementSizesArray(nullptr), subElementOffsetsArray(nullptr);
    if (this->SubElementSizes != nullptr)
    {
      subElementSizesArray =
        vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(this->SubElementSizes));
    }
    if (this->SubElementOffsets != nullptr)
    {
      subElementOffsetsArray =
        vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(this->SubElementOffsets));
    }

    auto elementShapesRange = vtk::DataArrayValueRange(elementShapesArray);
    auto elementSizesRange = vtk::DataArrayValueRange(elementSizesArray);
    auto elementOffsetsRange = vtk::DataArrayValueRange(elementOffsetsArray);

    assert(elementShapesRange.size() == elementSizesRange.size());
    assert(elementShapesRange.size() == elementOffsetsRange.size());

    auto elementSizesIterator = elementSizesRange.begin();
    auto elementOffsetsIterator = elementOffsetsRange.begin();

    const vtkNew<vtkUnsignedCharArray> cellTypes;
    const vtkNew<vtkCellArray> connectivity;
    const vtkNew<vtkIdTypeArray> faces;
    const vtkNew<vtkIdTypeArray> faceLocations;

    for (const auto& cellType : elementShapesRange)
    {
      auto type = static_cast<unsigned char>(cellType);
      cellTypes->InsertNextValue(type);
      if (type == VTK_POLYHEDRON)
      {
        assert(subElementSizesArray != nullptr);
        assert(subElementOffsetsArray != nullptr);

        std::set<vtkIdType> cellPointSet;
        auto nCellFaces = static_cast<vtkIdType>(*elementSizesIterator++);
        auto offset = static_cast<vtkIdType>(*elementOffsetsIterator++);
        const vtkIdType faceMaxId = faces->GetMaxId() + 1;
        faceLocations->InsertNextValue(faceMaxId);
        faces->InsertNextValue(nCellFaces);

        auto elementRange =
          vtk::DataArrayValueRange(elementConnectivity, offset, offset + nCellFaces);

        for (const ConnectivityArrayType faceId : elementRange)
        {
          const vtkIdType nFacePts = subElementSizesArray->GetVariantValue(faceId).ToLongLong();
          const vtkIdType faceOffset = subElementOffsetsArray->GetVariantValue(faceId).ToLongLong();

          auto facePtRange =
            vtk::DataArrayValueRange(subElementConnectivity, faceOffset, faceOffset + nFacePts);

          faces->InsertNextValue(nFacePts);
          for (const SubConnectivityArrayType ptId : facePtRange)
          {
            faces->InsertNextValue(ptId);
            cellPointSet.insert(ptId);
          }
        }

        connectivity->InsertNextCell(static_cast<int>(cellPointSet.size()));
        for (const vtkIdType cellPoint : cellPointSet)
        {
          connectivity->InsertCellPoint(cellPoint);
        }
      }
      else
      {
        auto npts = static_cast<vtkIdType>(*elementSizesIterator++);
        auto offset = static_cast<vtkIdType>(*elementOffsetsIterator++);
        auto elementRange = vtk::DataArrayValueRange(elementConnectivity, offset, offset + npts);

        connectivity->InsertNextCell(static_cast<int>(npts));
        for (const ConnectivityArrayType item : elementRange)
        {
          connectivity->InsertCellPoint(static_cast<vtkIdType>(item));
        }
        faceLocations->InsertNextValue(-1);
      }
    }

    if (faces->GetNumberOfValues() > 0)
    {
      ug->SetCells(cellTypes, connectivity, faceLocations, faces);
    }
    else
    {
      ug->SetCells(cellTypes, connectivity, nullptr, nullptr);
    }
  }
};

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> GetMesh(
  const conduit_cpp::Node& topologyNode, const conduit_cpp::Node& coordsets)
{
  // get the coordset for this topology element.
  auto coords = coordsets[topologyNode["coordset"].as_string()];
  if (topologyNode["type"].as_string() == "uniform" && coords["type"].as_string() == "uniform")
  {
    vtkNew<vtkImageData> img;
    int dims[3] = { 1, 1, 1 };
    const char* dims_paths[] = { "dims/i", "dims/j", "dims/k" };
    double origin[3] = { 0, 0, 0 };
    const char* origin_paths[] = { "origin/x", "origin/y", "origin/z" };
    double spacing[3] = { 1, 1, 1 };
    const char* spacing_paths[] = { "spacing/dx", "spacing/dy", "spacing/dz" };
    for (int cc = 0; cc < 3; ++cc)
    {
      if (coords.has_path(dims_paths[cc]))
      {
        dims[cc] = coords[dims_paths[cc]].to_int32();
      }
      if (coords.has_path(origin_paths[cc]))
      {
        origin[cc] = coords[origin_paths[cc]].to_double();
      }
      if (coords.has_path(spacing_paths[cc]))
      {
        spacing[cc] = coords[spacing_paths[cc]].to_double();
      }
    }
    img->SetOrigin(origin);
    img->SetSpacing(spacing);
    img->SetDimensions(dims);
    return img;
  }

  if (topologyNode["type"].as_string() == "rectilinear" &&
    coords["type"].as_string() == "rectilinear")
  {
    vtkNew<vtkRectilinearGrid> rg;

    conduit_cpp::Node values_x;
    const bool has_x_values = coords.has_path("values/x");
    if (has_x_values)
    {
      values_x = coords["values/x"];
    }

    conduit_cpp::Node values_y;
    const bool has_y_values = coords.has_path("values/y");
    if (has_y_values)
    {
      values_y = coords["values/y"];
    }

    conduit_cpp::Node values_z;
    const bool has_z_values = coords.has_path("values/z");
    if (has_z_values)
    {
      values_z = coords["values/z"];
    }

    vtkIdType x_dimension = 1;
    vtkSmartPointer<vtkDataArray> xArray;
    if (has_x_values)
    {
      xArray =
        vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values_x), "xcoords");
      x_dimension = xArray->GetNumberOfTuples();
    }

    vtkIdType y_dimension = 1;
    vtkSmartPointer<vtkDataArray> yArray;
    if (has_y_values)
    {
      yArray =
        vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values_y), "ycoords");
      y_dimension = yArray->GetNumberOfTuples();
    }

    vtkIdType z_dimension = 1;
    vtkSmartPointer<vtkDataArray> zArray;
    if (has_z_values)
    {
      zArray =
        vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values_z), "zcoords");
      z_dimension = zArray->GetNumberOfTuples();
    }
    rg->SetDimensions(x_dimension, y_dimension, z_dimension);

    if (has_x_values)
    {
      rg->SetXCoordinates(xArray);
    }
    if (has_y_values)
    {
      rg->SetYCoordinates(yArray);
    }
    if (has_z_values)
    {
      rg->SetZCoordinates(zArray);
    }
    return rg;
  }

  if (topologyNode["type"].as_string() == "structured" && coords["type"].as_string() == "explicit")
  {
    vtkNew<vtkStructuredGrid> sg;
    sg->SetPoints(CreatePoints(coords));
    sg->SetDimensions(
      topologyNode.has_path("elements/dims/i") ? topologyNode["elements/dims/i"].to_int32() + 1 : 1,
      topologyNode.has_path("elements/dims/j") ? topologyNode["elements/dims/j"].to_int32() + 1 : 1,
      topologyNode.has_path("elements/dims/k") ? topologyNode["elements/dims/k"].to_int32() + 1
                                               : 1);
    return sg;
  }

  if (coords["type"].as_string() == "explicit" &&
    topologyNode["type"].as_string() == "unstructured" && topologyNode.has_path("elements/shape"))
  {
    vtkNew<vtkUnstructuredGrid> ug;
    std::string shape = topologyNode["elements/shape"].as_string();
    if (shape != "mixed")
    {
      conduit_cpp::Node connectivity = topologyNode["elements/connectivity"];
      const conduit_cpp::DataType dtype0 = connectivity.dtype();
      const auto nb_cells = dtype0.number_of_elements();
      ug->SetPoints(CreatePoints(coords));
      const auto vtk_cell_type = GetCellType(topologyNode["elements/shape"].as_string());
      if (nb_cells > 0)
      {
        if (vtk_cell_type == VTK_POLYHEDRON)
        {
          // polyhedra uses O2M and not M2C arrays, so need to process it
          // differently.
          conduit_cpp::Node t_elements = topologyNode["elements"];
          conduit_cpp::Node t_subelements = topologyNode["subelements"];
          auto elements = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
            conduit_cpp::c_node(&t_elements), "connectivity");
          auto subelements = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
            conduit_cpp::c_node(&t_subelements), "connectivity");

          // currently, this is an ugly deep-copy. Once vtkUnstructuredGrid is modified
          // as proposed here (vtk/vtk#18190), this will get simpler.
          SetPolyhedralCells(ug, elements, subelements);
        }
        else if (vtk_cell_type == VTK_POLYGON)
        {
          // polygons use O2M and not M2C arrays, so need to process it
          // differently.
          conduit_cpp::Node t_elements = topologyNode["elements"];
          auto cellArray = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
            conduit_cpp::c_node(&t_elements), "connectivity");
          ug->SetCells(vtk_cell_type, cellArray);
        }
        else
        {
          const auto cell_size = GetNumberOfPointsInCellType(vtk_cell_type);
          auto cellArray = vtkConduitArrayUtilities::MCArrayToVTKCellArray(
            cell_size, conduit_cpp::c_node(&connectivity));
          ug->SetCells(vtk_cell_type, cellArray);
        }
      }
    }
    else if (topologyNode.has_path("elements/shape_map") &&
      topologyNode.has_path("elements/shapes"))
    {
      // mixed shapes definition
      conduit_cpp::Node shape_map = topologyNode["elements/shape_map"];

      // check presence of polyhedra
      bool hasPolyhedra(false);
      conduit_index_t nCells = shape_map.number_of_children();
      for (conduit_index_t i = 0; i < nCells; ++i)
      {
        auto child = shape_map.child(i);
        int cellType = child.to_int32();
        hasPolyhedra |= (cellType == VTK_POLYHEDRON);
      }
      // if polyhedra are present, the subelements should be present as well.
      if (hasPolyhedra &&
        !(topologyNode.has_path("subelements/shape") &&
          topologyNode.has_path("subelements/shape_map") &&
          topologyNode.has_path("subelements/shapes")))
      {
        throw std::runtime_error("no subelements found for polyhedral cell definition.");
      }
      if (nCells > 0)
      {
        ug->SetPoints(CreatePoints(coords));

        conduit_cpp::Node t_elementShapes = topologyNode["elements/shapes"];
        conduit_cpp::Node t_elementSizes = topologyNode["elements/sizes"];
        conduit_cpp::Node t_elementOffsets = topologyNode["elements/offsets"];
        conduit_cpp::Node t_elementConnectivity = topologyNode["elements/connectivity"];

        auto elementConnectivity =
          vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&t_elementConnectivity));

        if (elementConnectivity == nullptr)
        {
          throw std::runtime_error("element/connectivity not available (nullptr)");
        }

        conduit_cpp::Node* p_subElementSizes(nullptr);
        conduit_cpp::Node* p_subElementOffsets(nullptr);

        vtkSmartPointer<vtkDataArray> subConnectivity(nullptr);
        if (hasPolyhedra)
        {
          // get the face nodes for size, offset and connectivity
          conduit_cpp::Node t_subElementSizes = topologyNode["subelements/sizes"];
          conduit_cpp::Node t_subElementOffsets = topologyNode["subelements/offsets"];
          conduit_cpp::Node t_subElementConnectivity = topologyNode["subelements/connectivity"];

          p_subElementOffsets = &t_subElementOffsets;
          p_subElementSizes = &t_subElementSizes;

          subConnectivity = vtkConduitArrayUtilities::MCArrayToVTKArray(
            conduit_cpp::c_node(&t_subElementConnectivity));

          if (subConnectivity == nullptr)
          {
            throw std::runtime_error("subelements/connectivity not available (nullptr)");
          }
        }

        // dispatch the mcarrays to create the mixed element grid
        MixedPolyhedralCells worker(&t_elementShapes, &t_elementSizes, &t_elementOffsets,
          p_subElementSizes, p_subElementOffsets);
        if (!vtkArrayDispatch::Dispatch2::Execute(elementConnectivity, subConnectivity, worker, ug))
        {
          worker(elementConnectivity.GetPointer(), subConnectivity.GetPointer(), ug);
        }
      }
    }
    // if there are no cells in the Conduit mesh, return an empty ug
    return ug;
  }

  throw std::runtime_error("unsupported topology or coordset");
}

//----------------------------------------------------------------------------
bool RequestMesh(vtkPartitionedDataSet* output, const conduit_cpp::Node& node)
{
  conduit_cpp::Node info;
  if (!conduit_cpp::BlueprintMesh::verify(node, info))
  {
    vtkLogF(ERROR, "Mesh blueprint verification failed!");
    return false;
  }

  std::map<std::string, vtkSmartPointer<vtkDataSet>> datasets;

  // process "topologies".
  auto topologies = node["topologies"];
  conduit_index_t nchildren = topologies.number_of_children();
  for (conduit_index_t i = 0; i < nchildren; ++i)
  {
    auto child = topologies.child(i);
    try
    {
      if (auto ds = detail::GetMesh(child, node["coordsets"]))
      {
        auto idx = output->GetNumberOfPartitions();
        output->SetPartition(idx, ds);
        output->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), child.name().c_str());
        datasets[child.name()] = ds;
      }
    }
    catch (std::exception& e)
    {
      vtkLogF(ERROR, "failed to process '../topologies/%s'.", child.name().c_str());
      vtkLogF(ERROR, "ERROR: \n%s\n", e.what());
      return false;
    }
  }

  // process "fields"
  if (!node.has_path("fields"))
  {
    return true;
  }

  auto fields = node["fields"];
  nchildren = fields.number_of_children();
  for (conduit_index_t i = 0; i < nchildren; ++i)
  {
    auto fieldNode = fields.child(i);
    const auto fieldname = fieldNode.name();
    try
    {
      auto dataset = datasets.at(fieldNode["topology"].as_string());
      const auto vtk_association = detail::GetAssociation(fieldNode["association"].as_string());
      auto dsa = dataset->GetAttributes(vtk_association);
      auto values = fieldNode["values"];
      size_t dataset_size;
      if (values.number_of_children() == 0)
      {
        dataset_size = values.dtype().number_of_elements();
      }
      else
      {
        dataset_size = values.child(0).dtype().number_of_elements();
      }
      if (dataset_size > 0)
      {
        auto array =
          vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values), fieldname);
        if (array->GetNumberOfTuples() != dataset->GetNumberOfElements(vtk_association))
        {
          throw std::runtime_error("mismatched tuple count!");
        }
        dsa->AddArray(array);
      }
    }
    catch (std::exception& e)
    {
      vtkLogF(ERROR, "failed to process '../fields/%s'.", fieldname.c_str());
      vtkLogF(ERROR, "ERROR: \n%s\n", e.what());
      return false;
    }
  }

  return true;
}

bool AddGlobalData(vtkDataObject* output, const conduit_cpp::Node& globalFields)
{
  auto fd = output->GetFieldData();

  const conduit_index_t nchildren = globalFields.number_of_children();
  for (conduit_index_t idx = 0; idx < nchildren; idx++)
  {
    const auto child = globalFields.child(idx);
    std::string fieldName = child.name();

    if (child.dtype().is_string())
    {
      vtkNew<vtkStringArray> stringArray;
      stringArray->SetName(fieldName.c_str());
      stringArray->InsertNextValue(child.as_string().c_str());
      fd->AddArray(stringArray);
      continue;
    }

    auto array =
      vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&child), fieldName);
    fd->AddArray(array);

    if ((fieldName == "time" || fieldName == "TimeValue") && child.dtype().is_float())
    {
      // let's also set DATA_TIME_STEP.
      output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), child.to_float64());
    }
  }

  return true;
}

bool AddFieldData(vtkDataObject* output, const conduit_cpp::Node& stateFields)
{
  auto field_data = output->GetFieldData();
  auto number_of_children = stateFields.number_of_children();
  for (conduit_index_t child_index = 0; child_index < number_of_children; ++child_index)
  {
    auto field_node = stateFields.child(child_index);
    const auto field_name = field_node.name();

    try
    {
      size_t dataset_size = 0;
      if (field_node.number_of_children() == 0)
      {
        dataset_size = field_node.dtype().number_of_elements();
      }
      else
      {
        dataset_size = field_node.child(0).dtype().number_of_elements();
      }

      if (dataset_size > 0)
      {
        vtkSmartPointer<vtkAbstractArray> dataArray;
        if (field_node.dtype().is_string())
        {
          auto stringArray = vtkSmartPointer<vtkStringArray>::New();
          stringArray->SetNumberOfTuples(1);
          stringArray->SetValue(0, field_node.as_string().c_str());
          dataArray = stringArray;
          dataArray->SetName(field_name.c_str());
        }
        else
        {
          dataArray = vtkConduitArrayUtilities::MCArrayToVTKArray(
            conduit_cpp::c_node(&field_node), field_name);
        }

        if (dataArray)
        {
          field_data->AddArray(dataArray);
        }
      }
    }
    catch (std::exception& e)
    {
      vtkLogF(ERROR, "failed to process '../state/fields/%s'.", field_name.c_str());
      vtkLogF(ERROR, "ERROR: \n%s\n", e.what());
      return false;
    }
  }
  return true;
}

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN

class vtkConduitSource::vtkInternals
{
public:
  conduit_cpp::Node Node;
  conduit_cpp::Node GlobalFieldsNode;
  conduit_cpp::Node AssemblyNode;
  bool GlobalFieldsNodeValid{ false };
  bool AssemblyNodeValid{ false };
};

vtkStandardNewMacro(vtkConduitSource);
//----------------------------------------------------------------------------
vtkConduitSource::vtkConduitSource()
  : Internals(new vtkConduitSource::vtkInternals())
  , UseMultiMeshProtocol(false)
  , OutputMultiBlock(false)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkConduitSource::~vtkConduitSource() = default;

//----------------------------------------------------------------------------
void vtkConduitSource::SetNode(const conduit_node* node)
{
  if (conduit_cpp::c_node(&this->Internals->Node) == node)
  {
    return;
  }
  this->Internals->Node = conduit_cpp::cpp_node(const_cast<conduit_node*>(node));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkConduitSource::SetGlobalFieldsNode(const conduit_node* node)
{
  if (this->Internals->GlobalFieldsNodeValid &&
    conduit_cpp::c_node(&this->Internals->GlobalFieldsNode) == node)
  {
    return;
  }

  if (node)
  {
    this->Internals->GlobalFieldsNode = conduit_cpp::cpp_node(const_cast<conduit_node*>(node));
  }
  this->Internals->GlobalFieldsNodeValid = (node != nullptr);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkConduitSource::SetAssemblyNode(const conduit_node* node)
{
  if (this->Internals->AssemblyNodeValid &&
    conduit_cpp::c_node(&this->Internals->AssemblyNode) == node)
  {
    return;
  }

  if (node)
  {
    this->Internals->AssemblyNode = conduit_cpp::cpp_node(const_cast<conduit_node*>(node));
  }
  this->Internals->AssemblyNodeValid = (node != nullptr);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkConduitSource::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  const int dataType = this->OutputMultiBlock
    ? VTK_MULTIBLOCK_DATA_SET
    : this->UseMultiMeshProtocol ? VTK_PARTITIONED_DATA_SET_COLLECTION : VTK_PARTITIONED_DATA_SET;

  return this->SetOutputDataObject(dataType, outputVector->GetInformationObject(0), /*exact=*/true)
    ? 1
    : 0;
}

//----------------------------------------------------------------------------
int vtkConduitSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  auto& internals = (*this->Internals);
  vtkDataObject* real_output = vtkDataObject::GetData(outputVector, 0);
  if (this->UseMultiMeshProtocol)
  {
    vtkNew<vtkPartitionedDataSetCollection> pdc_output;
    const auto& node = internals.Node;
    const auto count = node.number_of_children();
    pdc_output->SetNumberOfPartitionedDataSets(static_cast<unsigned int>(count));

    std::map<std::string, unsigned int> name_map;
    for (conduit_index_t cc = 0; cc < count; ++cc)
    {
      const auto child = node.child(cc);
      auto pd = pdc_output->GetPartitionedDataSet(static_cast<unsigned int>(cc));
      assert(pd != nullptr);
      if (!detail::RequestMesh(pd, child))
      {
        vtkLogF(ERROR, "Failed reading mesh '%s'", child.name().c_str());
        real_output->Initialize();
        return 0;
      }

      // set the mesh name.
      pdc_output->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), child.name().c_str());
      name_map[child.name()] = static_cast<unsigned int>(cc);

      // set field data.
      if (child.has_path("state/fields"))
      {
        detail::AddFieldData(pd, child["state/fields"]);
      }
    }

    if (internals.AssemblyNodeValid)
    {
      vtkNew<vtkDataAssembly> assembly;
      std::function<void(int, const conduit_cpp::Node&)> helper;
      helper = [&name_map, &assembly, &helper](int parent, const conduit_cpp::Node& node) {
        if (node.dtype().is_object())
        {
          for (conduit_index_t cc = 0; cc < node.number_of_children(); ++cc)
          {
            auto child = node.child(cc);
            auto nodeName = vtkDataAssembly::MakeValidNodeName(child.name().c_str());
            auto childId = assembly->AddNode(nodeName.c_str(), parent);
            assembly->SetAttribute(childId, "label", child.name().c_str());
            helper(childId, child);
          }
        }
        else if (node.dtype().is_list())
        {
          for (conduit_index_t cc = 0; cc < node.number_of_children(); ++cc)
          {
            auto child = node.child(cc);
            if (!child.dtype().is_string())
            {
              vtkLogF(ERROR, "list cannot have non-string items!");
              continue;
            }
            helper(parent, node.child(cc));
          }
        }
        else if (node.dtype().is_string())
        {
          auto value = node.as_string();
          auto iter = name_map.find(node.as_string());
          if (iter != name_map.end())
          {
            assembly->AddDataSetIndex(parent, iter->second);
          }
          else
          {
            vtkLogF(ERROR, "Assembly referring to unknown node '%s'. Skipping.", value.c_str());
          }
        }
      };
      // assembly->SetRootNodeName(....); What should this be?
      helper(assembly->GetRootNode(), internals.AssemblyNode);
      pdc_output->SetDataAssembly(assembly);
    }
    real_output->ShallowCopy(pdc_output);
  }
  else
  {
    vtkNew<vtkPartitionedDataSet> pd_output;
    if (!detail::RequestMesh(pd_output, internals.Node))
    {
      vtkLogF(ERROR, "Failed reading mesh from '%s'", internals.Node.name().c_str());
      real_output->Initialize();
      return 0;
    }
    real_output->ShallowCopy(pd_output);
  }

  if (this->OutputMultiBlock)
  {
    vtkNew<vtkConvertToMultiBlockDataSet> converter;
    converter->SetInputData(real_output);
    converter->Update();
    real_output->ShallowCopy(converter->GetOutput());
  }

  if (internals.GlobalFieldsNodeValid)
  {
    detail::AddGlobalData(real_output, internals.GlobalFieldsNode);
  }

  if (internals.Node.has_path("state/fields"))
  {
    detail::AddFieldData(real_output, internals.Node["state/fields"]);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkConduitSource::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  if (!this->Internals->GlobalFieldsNodeValid)
  {
    return 1;
  }

  auto& node = this->Internals->GlobalFieldsNode;
  if (node.has_path("time"))
  {
    double time = node["time"].to_float64();
    double timesteps[2] = { time, time };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &time, 1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timesteps, 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkConduitSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
