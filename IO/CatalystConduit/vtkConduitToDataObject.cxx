// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConduitToDataObject.h"

#include "vtkAMRBox.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkConduitArrayUtilities.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkOverlappingAMR.h"
#include "vtkParallelAMRUtilities.h"
#include "vtkPartitionedDataSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#include <numeric>
#include <set>

namespace vtkConduitToDataObject
{
VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
bool FillPartionedDataSet(vtkPartitionedDataSet* output, const conduit_cpp::Node& node)
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
      if (auto ds = CreateMesh(child, node["coordsets"]))
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

  // add field data at leaf level
  if (node.has_path("state/fields"))
  {
    for (const auto& dataset : datasets)
    {
      AddFieldData(dataset.second.Get(), node["state/fields"]);
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
    const auto& fieldname = fieldNode.name();
    try
    {
      auto dataset = datasets.at(fieldNode["topology"].as_string());
      const auto vtk_association = GetAssociation(fieldNode["association"].as_string());
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
        vtkSmartPointer<vtkDataArray> array;
        if (fieldname == "ascent_ghosts")
        {
          // convert ascent ghost information into VTK ghost information
          // the VTK array is named vtkDataSetAttributes::GhostArrayName()
          // and has different values.
          array = vtkConduitArrayUtilities::MCGhostArrayToVTKGhostArray(
            conduit_cpp::c_node(&values), dsa->IsA("vtkCellData"));
        }
        else
        {
          array =
            vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values), fieldname);
          if (array->GetNumberOfTuples() != dataset->GetNumberOfElements(vtk_association))
          {
            throw std::runtime_error("mismatched tuple count!");
          }
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

//----------------------------------------------------------------------------
bool FillAMRMesh(vtkOverlappingAMR* amr, const conduit_cpp::Node& node)
{
  const int default_refinement_ratio = 2;

  int nprocs = 1;
  int rank = 0;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller)
  {
    // if VTK was initialized properly controller should be non-null but that's not always
    // the case so safer to check if controller is available
    nprocs = controller->GetNumberOfProcesses();
    rank = controller->GetLocalProcessId();
  }
  // pre-allocate the levels
  const auto leaves_on_node = node.number_of_children();
  std::vector<int> blocksPerLevelLocal(1);
  std::map<int, std::pair<int, int>> domainID2LvlID;
  vtkIdType blocks_local = 0;
  vtkIdType blocks_global = 0;
  double local_origin[3] = { vtkMath::Inf(), vtkMath::Inf(), vtkMath::Inf() };
  double origin[3] = { 0, 0, 0 };
  double spacing[3] = { 0, 0, 0 };

  for (conduit_index_t cc = 0; cc < leaves_on_node; ++cc)
  {
    const auto child = node.child(cc);
    if (child.has_path("state"))
    {
      const int level = child["state/level"].to_int32();
      const int domain_id = child["state/domain_id"].to_int32();
      if (size_t(level) >= blocksPerLevelLocal.size())
      {
        blocksPerLevelLocal.resize(level + 1);
        blocksPerLevelLocal[level] = 0;
      }
      domainID2LvlID[domain_id] = { level, blocksPerLevelLocal[level] };
      blocksPerLevelLocal[level]++;

      origin[0] = child["coordsets/coords/origin/x"].to_float64();
      origin[1] = child["coordsets/coords/origin/y"].to_float64();
      origin[2] = child["coordsets/coords/origin/z"].to_float64();
      // check global origin
      if (origin[0] <= local_origin[0] && origin[1] <= local_origin[1] &&
        origin[2] <= local_origin[2])
      {
        local_origin[0] = origin[0];
        local_origin[1] = origin[1];
        local_origin[2] = origin[2];
      }
    }
  }

  const vtkIdType levels_local = vtkIdType(blocksPerLevelLocal.size());

  vtkIdType levels_global = 0;
  double global_origin[3] = { vtkMath::Inf(), vtkMath::Inf(), vtkMath::Inf() };
  if (nprocs == 1)
  {
    levels_global = levels_local;
    std::copy(local_origin, local_origin + 3, global_origin);
  }
  else if (controller)
  {
    controller->AllReduce(&levels_local, &levels_global, 1, vtkCommunicator::MAX_OP);
    controller->AllReduce(local_origin, global_origin, 3, vtkCommunicator::MIN_OP);
  }

  // need the total number of blocks across all processes
  blocksPerLevelLocal.resize(levels_global, 0); // set the extra values created to 0
  // globalBlockCount has block information for each process separated
  std::vector<int> globalBlockCount(levels_global * nprocs);
  // the ordering of the blocks for AMR is first all level 0 blocks, then all level 1 blocks, ...
  // at each level we order based on proc rank first and then local id
  if (nprocs == 1)
  {
    globalBlockCount = blocksPerLevelLocal;
  }
  else if (controller)
  {
    controller->AllGather(blocksPerLevelLocal.data(), globalBlockCount.data(), levels_global);
  }

  blocks_local = vtkIdType(domainID2LvlID.size());
  blocks_global = std::accumulate(globalBlockCount.begin(), globalBlockCount.end(), 0);

  // the offset for the start of each block at each level
  std::vector<vtkIdType> offset_local(levels_global, 0);
  if (nprocs > 1)
  {
    for (vtkIdType l = 0; l < levels_global; l++)
    {
      vtkIdType offset(0);
      for (int p = 0; p < rank; p++)
      {
        offset += globalBlockCount[l + p * levels_global];
      }
      offset_local[l] = offset;
    }
  }
  std::vector<int> blocksPerLevelGlobal(levels_global, 0);
  for (vtkIdType l = 0; l < levels_global; l++)
  {
    for (int p = 0; p < nprocs; p++)
    {
      blocksPerLevelGlobal[l] += globalBlockCount[l + p * levels_global];
    }
  }
  amr->Initialize(levels_global, blocksPerLevelGlobal.data());
  for (int l = 0; l < levels_global; ++l)
  {
    for (int b = 0; b < blocksPerLevelGlobal[l]; ++b)
    {
      amr->SetDataSet(l, b, nullptr);
    }
  }

  // set origin
  amr->SetOrigin(global_origin);

  for (conduit_index_t cc = 0; cc < leaves_on_node; ++cc)
  {
    // set the spacing for each level via amr->SetSpacing();
    const auto child = node.child(cc);
    if (child.has_path("state"))
    {
      int pdims[3] = { 0, 0, 0 };
      const int domain_id = child["state/domain_id"].to_int32();
      const int level = child["state/level"].to_int32();

      origin[0] = child["coordsets/coords/origin/x"].to_float64();
      origin[1] = child["coordsets/coords/origin/y"].to_float64();
      origin[2] = child["coordsets/coords/origin/z"].to_float64();
      spacing[0] = child["coordsets/coords/spacing/dx"].to_float64();
      spacing[1] = child["coordsets/coords/spacing/dy"].to_float64();
      spacing[2] = child["coordsets/coords/spacing/dz"].to_float64();
      pdims[0] = child["coordsets/coords/dims/i"].to_int32();
      pdims[1] = child["coordsets/coords/dims/j"].to_int32();
      pdims[2] = child["coordsets/coords/dims/k"].to_int32();

      vtkNew<vtkUniformGrid> ug;
      ug->Initialize();
      ug->SetOrigin(origin);
      ug->SetSpacing(spacing);
      ug->SetDimensions(pdims);

      const auto fields = child["fields"];
      AddFieldData(ug, fields, true);

      vtkAMRBox box(origin, pdims, spacing, global_origin, amr->GetGridDescription());
      // set level spacing
      amr->SetSpacing(level, spacing);
      amr->SetAMRBox(domainID2LvlID[domain_id].first,
        domainID2LvlID[domain_id].second + offset_local[domainID2LvlID[domain_id].first], box);
      amr->SetDataSet(domainID2LvlID[domain_id].first,
        domainID2LvlID[domain_id].second + offset_local[domainID2LvlID[domain_id].first], ug);
      amr->SetRefinementRatio(level, default_refinement_ratio);
      if (child.has_path("nestsets/nest/windows"))
      {
        const auto& windows = child["nestsets/nest/windows"];
        const auto window_count = windows.number_of_children();
        for (int i = 0; i < window_count; ++i)
        {
          const auto& window = windows.child(i);
          if (window.has_path("ratio") && window.has_path("domain_type"))
          {
            amr->SetRefinementRatio(level, window["ratio/i"].to_int32());
            break;
          }
        }
      }
    }
  }

  // distribute AMRBoxes to all processes
  if (nprocs > 1 && controller)
  {
    std::vector<vtkIdType> boxBoundsOffsets(nprocs);
    std::vector<vtkIdType> boxBoundsCounts(nprocs);
    std::vector<int> boxExtentsLocal(8 * blocks_local, 0);
    std::vector<int> boxExtentsGlobal(8 * blocks_global, 0);

    for (int p = 0; p < nprocs; ++p)
    {
      int num_blocks = 0;
      for (int l = 0; l < levels_global; l++)
      {
        num_blocks += globalBlockCount[l + p * levels_global];
      }
      boxBoundsCounts[p] = num_blocks * 8;
      if (p > 0)
      {
        boxBoundsOffsets[p] = num_blocks * 8 + boxBoundsOffsets[p - 1];
      }
    }

    int local_index = 0;
    for (std::map<int, std::pair<int, int>>::const_iterator it = domainID2LvlID.begin();
         it != domainID2LvlID.end(); ++it)
    {
      int level = it->second.first;
      int id = it->second.second + offset_local[level];

      vtkAMRBox box = amr->GetAMRBox(level, id);
      const int* loCorner = box.GetLoCorner();
      const int* hiCorner = box.GetHiCorner();
      boxExtentsLocal[8 * local_index + 0] = level;
      boxExtentsLocal[8 * local_index + 1] = id;
      boxExtentsLocal[8 * local_index + 2] = loCorner[0];
      boxExtentsLocal[8 * local_index + 3] = loCorner[1];
      boxExtentsLocal[8 * local_index + 4] = loCorner[2];
      boxExtentsLocal[8 * local_index + 5] = hiCorner[0];
      boxExtentsLocal[8 * local_index + 6] = hiCorner[1];
      boxExtentsLocal[8 * local_index + 7] = hiCorner[2];
      ++local_index;
    }

    controller->AllGatherV(boxExtentsLocal.data(), boxExtentsGlobal.data(), boxExtentsLocal.size(),
      boxBoundsCounts.data(), boxBoundsOffsets.data());
    for (int i = 0; i < blocks_global; ++i)
    {
      int level = boxExtentsGlobal[8 * i + 0];
      int id = boxExtentsGlobal[8 * i + 1];
      int* dims = &boxExtentsGlobal[8 * i + 2];
      vtkAMRBox box(dims[0], dims[1], dims[2], dims[3], dims[4], dims[5]);
      amr->SetAMRBox(level, id, box);
    }
  }
  if (nprocs == 1)
  {
    vtkAMRUtilities::BlankCells(amr);
  }
  else if (controller)
  {
    vtkParallelAMRUtilities::BlankCells(amr, controller);
  }

  return true;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> CreateMesh(
  const conduit_cpp::Node& topology, const conduit_cpp::Node& coordsets)
{
  // get the coordset for this topology element.
  auto coords = coordsets[topology["coordset"].as_string()];
  if (topology["type"].as_string() == "uniform" && coords["type"].as_string() == "uniform")
  {
    return CreateImageData(coords);
  }

  if (topology["type"].as_string() == "rectilinear" && coords["type"].as_string() == "rectilinear")
  {
    return CreateRectilinearGrid(coords);
  }

  if (topology["type"].as_string() == "structured" && coords["type"].as_string() == "explicit")
  {
    return CreateStructuredGrid(topology, coords);
  }

  if (coords["type"].as_string() == "explicit" && topology["type"].as_string() == "unstructured" &&
    topology.has_path("elements/shape"))
  {
    std::string shape = topology["elements/shape"].as_string();
    if (shape != "mixed")
    {
      return CreateMonoShapedUnstructuredGrid(topology, coords);
    }
    else if (topology.has_path("elements/shape_map") && topology.has_path("elements/shapes"))
    {
      return CreateMixedUnstructuredGrid(topology, coords);
    }
    // if there are no cells in the Conduit mesh, return an empty ug
    return vtkSmartPointer<vtkUnstructuredGrid>::New();
  }

  throw std::runtime_error("unsupported topology or coordset");
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> CreateImageData(const conduit_cpp::Node& coordset)
{
  auto image = vtkSmartPointer<vtkImageData>::New();
  int dims[3] = { 1, 1, 1 };
  const char* dims_paths[] = { "dims/i", "dims/j", "dims/k" };
  double origin[3] = { 0, 0, 0 };
  const char* origin_paths[] = { "origin/x", "origin/y", "origin/z" };
  double spacing[3] = { 1, 1, 1 };
  const char* spacing_paths[] = { "spacing/dx", "spacing/dy", "spacing/dz" };
  for (int cc = 0; cc < 3; ++cc)
  {
    if (coordset.has_path(dims_paths[cc]))
    {
      dims[cc] = coordset[dims_paths[cc]].to_int32();
    }
    if (coordset.has_path(origin_paths[cc]))
    {
      origin[cc] = coordset[origin_paths[cc]].to_double();
    }
    if (coordset.has_path(spacing_paths[cc]))
    {
      spacing[cc] = coordset[spacing_paths[cc]].to_double();
    }
  }
  image->SetOrigin(origin);
  image->SetSpacing(spacing);
  image->SetDimensions(dims);

  return image;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkRectilinearGrid> CreateRectilinearGrid(const conduit_cpp::Node& coordset)
{
  auto rectilinearGrid = vtkSmartPointer<vtkRectilinearGrid>::New();

  conduit_cpp::Node values_x;
  const bool has_x_values = coordset.has_path("values/x");
  if (has_x_values)
  {
    values_x = coordset["values/x"];
  }

  conduit_cpp::Node values_y;
  const bool has_y_values = coordset.has_path("values/y");
  if (has_y_values)
  {
    values_y = coordset["values/y"];
  }

  conduit_cpp::Node values_z;
  const bool has_z_values = coordset.has_path("values/z");
  if (has_z_values)
  {
    values_z = coordset["values/z"];
  }

  vtkIdType x_dimension = 1;
  vtkSmartPointer<vtkDataArray> xArray;
  if (has_x_values)
  {
    xArray = vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values_x), "xcoords");
    x_dimension = xArray->GetNumberOfTuples();
  }

  vtkIdType y_dimension = 1;
  vtkSmartPointer<vtkDataArray> yArray;
  if (has_y_values)
  {
    yArray = vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values_y), "ycoords");
    y_dimension = yArray->GetNumberOfTuples();
  }

  vtkIdType z_dimension = 1;
  vtkSmartPointer<vtkDataArray> zArray;
  if (has_z_values)
  {
    zArray = vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values_z), "zcoords");
    z_dimension = zArray->GetNumberOfTuples();
  }
  rectilinearGrid->SetDimensions(x_dimension, y_dimension, z_dimension);

  if (has_x_values)
  {
    rectilinearGrid->SetXCoordinates(xArray);
  }
  if (has_y_values)
  {
    rectilinearGrid->SetYCoordinates(yArray);
  }
  if (has_z_values)
  {
    rectilinearGrid->SetZCoordinates(zArray);
  }

  return rectilinearGrid;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkStructuredGrid> CreateStructuredGrid(
  const conduit_cpp::Node& topology, const conduit_cpp::Node& coordset)
{
  auto sg = vtkSmartPointer<vtkStructuredGrid>::New();

  sg->SetPoints(CreatePoints(coordset));
  sg->SetDimensions(
    topology.has_path("elements/dims/i") ? topology["elements/dims/i"].to_int32() + 1 : 1,
    topology.has_path("elements/dims/j") ? topology["elements/dims/j"].to_int32() + 1 : 1,
    topology.has_path("elements/dims/k") ? topology["elements/dims/k"].to_int32() + 1 : 1);
  return sg;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> CreateMonoShapedUnstructuredGrid(
  const conduit_cpp::Node& topologyNode, const conduit_cpp::Node& coordset)
{
  auto unstructured = vtkSmartPointer<vtkUnstructuredGrid>::New();
  conduit_cpp::Node connectivity = topologyNode["elements/connectivity"];
  const conduit_cpp::DataType dtype0 = connectivity.dtype();
  const auto nb_cells = dtype0.number_of_elements();
  unstructured->SetPoints(CreatePoints(coordset));
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
      SetPolyhedralCells(unstructured, elements, subelements);
    }
    else if (vtk_cell_type == VTK_POLYGON)
    {
      // polygons use O2M and not M2C arrays, so need to process it
      // differently.
      conduit_cpp::Node t_elements = topologyNode["elements"];
      auto cellArray = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
        conduit_cpp::c_node(&t_elements), "connectivity");
      unstructured->SetCells(vtk_cell_type, cellArray);
    }
    else
    {
      const auto cell_size = GetNumberOfPointsInCellType(vtk_cell_type);
      auto cellArray = vtkConduitArrayUtilities::MCArrayToVTKCellArray(
        cell_size, conduit_cpp::c_node(&connectivity));
      unstructured->SetCells(vtk_cell_type, cellArray);
    }
  }

  return unstructured;
}

/**
 * Internal struct to be passed to a worker.
 * See CreateMixedUnstructuredGrid.
 */
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
    const vtkNew<vtkCellArray> faces;
    const vtkNew<vtkCellArray> faceLocations;
    vtkIdType numFace = 0;

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
        faceLocations->InsertNextCell(nCellFaces);

        auto elementRange =
          vtk::DataArrayValueRange(elementConnectivity, offset, offset + nCellFaces);

        for (const ConnectivityArrayType faceId : elementRange)
        {
          const vtkIdType nFacePts = subElementSizesArray->GetVariantValue(faceId).ToLongLong();
          const vtkIdType faceOffset = subElementOffsetsArray->GetVariantValue(faceId).ToLongLong();
          faceLocations->InsertCellPoint(numFace++);

          auto facePtRange =
            vtk::DataArrayValueRange(subElementConnectivity, faceOffset, faceOffset + nFacePts);

          faces->InsertNextCell(nFacePts);
          for (const SubConnectivityArrayType ptId : facePtRange)
          {
            faces->InsertCellPoint(ptId);
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
        faceLocations->InsertNextCell(0);
      }
    }

    if (faces->GetNumberOfCells() > 0)
    {
      ug->SetPolyhedralCells(cellTypes, connectivity, faceLocations, faces);
    }
    else
    {
      ug->SetPolyhedralCells(cellTypes, connectivity, nullptr, nullptr);
    }
  }
};

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> CreateMixedUnstructuredGrid(
  const conduit_cpp::Node& topologyNode, const conduit_cpp::Node& coords)
{
  auto unstructured = vtkSmartPointer<vtkUnstructuredGrid>::New();
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
    unstructured->SetPoints(CreatePoints(coords));

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

      subConnectivity =
        vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&t_subElementConnectivity));

      if (subConnectivity == nullptr)
      {
        throw std::runtime_error("subelements/connectivity not available (nullptr)");
      }
    }

    // dispatch the mcarrays to create the mixed element grid
    MixedPolyhedralCells worker(
      &t_elementShapes, &t_elementSizes, &t_elementOffsets, p_subElementSizes, p_subElementOffsets);
    if (!vtkArrayDispatch::Dispatch2::Execute(
          elementConnectivity, subConnectivity, worker, unstructured))
    {
      worker(elementConnectivity.GetPointer(), subConnectivity.GetPointer(), unstructured);
    }
  }

  return unstructured;
}

//----------------------------------------------------------------------------
bool AddFieldData(vtkDataObject* output, const conduit_cpp::Node& stateFields, bool isAMReX)
{
  auto field_data = output->GetFieldData();
  auto number_of_children = stateFields.number_of_children();
  for (conduit_index_t child_index = 0; child_index < number_of_children; ++child_index)
  {
    auto field_node = stateFields.child(child_index);
    const auto& field_name = field_node.name();

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
          if (isAMReX)
          {
            auto ug = vtkUniformGrid::SafeDownCast(output);
            const auto vtk_association = GetAssociation(field_node["association"].as_string());
            auto dsa = ug->GetAttributes(vtk_association);
            dsa->AddArray(dataArray);
          }
          else
          {
            field_data->AddArray(dataArray);
          }
        }

        if ((field_name == "time" || field_name == "TimeValue") && field_node.dtype().is_float())
        {
          // let's also set DATA_TIME_STEP.
          output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), field_node.to_float64());
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

  auto pts = vtkSmartPointer<vtkPoints>::New();
  pts->SetData(array);
  return pts;
}

//----------------------------------------------------------------------------
void SetPolyhedralCells(
  vtkUnstructuredGrid* grid, vtkCellArray* elements, vtkCellArray* subelements)
{
  vtkNew<vtkCellArray> connectivity;
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkCellArray> faceLocations;

  connectivity->AllocateEstimate(elements->GetNumberOfCells(), 10);
  faces->AllocateExact(
    subelements->GetNumberOfCells(), subelements->GetConnectivityArray()->GetNumberOfTuples());
  faceLocations->AllocateExact(elements->GetNumberOfCells(), subelements->GetNumberOfCells());

  auto eIter = vtk::TakeSmartPointer(elements->NewIterator());
  auto seIter = vtk::TakeSmartPointer(subelements->NewIterator());

  std::vector<vtkIdType> cellPoints;
  vtkIdType faceNum = 0;
  for (eIter->GoToFirstCell(); !eIter->IsDoneWithTraversal(); eIter->GoToNextCell())
  {
    // init;
    cellPoints.clear();

    // get cell from 'elements'.
    vtkIdType size;
    vtkIdType const* seIds;
    eIter->GetCurrentCell(size, seIds);

    faceLocations->InsertNextCell(size);
    for (vtkIdType fIdx = 0; fIdx < size; ++fIdx)
    {
      faceLocations->InsertCellPoint(faceNum++);
      seIter->GoToCell(seIds[fIdx]);

      vtkIdType ptSize;
      vtkIdType const* ptIds;
      seIter->GetCurrentCell(ptSize, ptIds);
      faces->InsertNextCell(ptSize, ptIds);
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
  grid->SetPolyhedralCells(cellTypes, connectivity, faceLocations, faces);
}

//----------------------------------------------------------------------------
vtkIdType GetNumberOfPointsInCellType(int vtk_cell_type)
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
VTK_ABI_NAMESPACE_END
} // vtkDataObjectToConduit namespace
