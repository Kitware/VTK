// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Added due to deprecated vtkConduitArrayUtilities::MCGhostArrayToVTKGhostArray
#define VTK_DEPRECATION_LEVEL 0

#include "vtkConduitToDataObject.h"

#include "vtkAMRBox.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArrayIterator.h"
#include "vtkConduitArrayUtilities.h"
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
#include "vtkConduitArrayUtilitiesDevice.h"
#endif
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
#include "vtkSMPTools.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/SystemTools.hxx"

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#include <numeric>
#include <set>

namespace AMRUtils
{
struct LocalInfo
{
  int Rank = 0;
  conduit_index_t NbOfLeaves = 0;
  std::vector<int> BlocksPerLevel = { 0 };
  std::vector<vtkIdType> BlockOffsets;
  vtkIdType NbOfBlocks = 0;
  double Origin[3] = { vtkMath::Inf(), vtkMath::Inf(), vtkMath::Inf() };
  std::map<int, std::pair<int, int>> DomainBlockLevelIds;
};

struct GlobalInfo
{
  int NbOfProcesses = 1;
  vtkIdType NbOfBlocks = 0;
  std::vector<int> BlocksPerLevelAndRank;
  double Origin[3] = { vtkMath::Inf(), vtkMath::Inf(), vtkMath::Inf() };
  vtkIdType NbOfLevels = 0;
};

// ---------------------
// construct structure: nb of blocks per levels, and origin.
// Local origin is the min of all origins found:
// so we will get a Global Origin with a simple min reduction
void ConstructLocalInfo(const conduit_cpp::Node& node, LocalInfo& rankInfo)
{
  double origin[3] = { 0, 0, 0 };

  rankInfo.NbOfLeaves = node.number_of_children();

  for (conduit_index_t cc = 0; cc < rankInfo.NbOfLeaves; ++cc)
  {
    const auto child = node.child(cc);
    if (child.has_path("state"))
    {
      const int level = child["state/level"].to_int32();
      const int domain_id = child["state/domain_id"].to_int32();
      if (std::size_t(level) >= rankInfo.BlocksPerLevel.size())
      {
        rankInfo.BlocksPerLevel.resize(level + 1);
        rankInfo.BlocksPerLevel[level] = 0;
      }
      rankInfo.DomainBlockLevelIds[domain_id] = { level, rankInfo.BlocksPerLevel[level] };
      rankInfo.BlocksPerLevel[level]++;

      origin[0] = child["coordsets/coords/origin/x"].to_float64();
      origin[1] = child["coordsets/coords/origin/y"].to_float64();
      origin[2] = child["coordsets/coords/origin/z"].to_float64();
      // check global origin
      if (origin[0] <= rankInfo.Origin[0] && origin[1] <= rankInfo.Origin[1] &&
        origin[2] <= rankInfo.Origin[2])
      {
        rankInfo.Origin[0] = origin[0];
        rankInfo.Origin[1] = origin[1];
        rankInfo.Origin[2] = origin[2];
      }
    }
  }
}

// ---------------------
// MPI comm: reduce nb of levels, blocks and origin
void GatherInfos(LocalInfo& rankInfo, GlobalInfo& globalInfo)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  const vtkIdType levels_local = vtkIdType(rankInfo.BlocksPerLevel.size());

  if (globalInfo.NbOfProcesses == 1)
  {
    globalInfo.NbOfLevels = levels_local;
    std::copy(rankInfo.Origin, rankInfo.Origin + 3, globalInfo.Origin);
  }
  else if (controller)
  {
    controller->AllReduce(&levels_local, &globalInfo.NbOfLevels, 1, vtkCommunicator::MAX_OP);
    controller->AllReduce(rankInfo.Origin, globalInfo.Origin, 3, vtkCommunicator::MIN_OP);
  }

  // need the total number of blocks across all processes
  rankInfo.BlocksPerLevel.resize(globalInfo.NbOfLevels, 0); // set the extra values created to 0
  globalInfo.BlocksPerLevelAndRank.resize(globalInfo.NbOfLevels * globalInfo.NbOfProcesses);
  // the ordering of the blocks for AMR is first all level 0 blocks, then all level 1 blocks, ...
  // at each level we order based on proc rank first and then local id
  if (globalInfo.NbOfProcesses == 1)
  {
    globalInfo.BlocksPerLevelAndRank = rankInfo.BlocksPerLevel;
  }
  else if (controller)
  {
    controller->AllGather(rankInfo.BlocksPerLevel.data(), globalInfo.BlocksPerLevelAndRank.data(),
      globalInfo.NbOfLevels);
  }

  rankInfo.NbOfBlocks = vtkIdType(rankInfo.DomainBlockLevelIds.size());
  globalInfo.NbOfBlocks = std::accumulate(
    globalInfo.BlocksPerLevelAndRank.begin(), globalInfo.BlocksPerLevelAndRank.end(), 0);

  // the offset for the start of each block at each level
  rankInfo.BlockOffsets.resize(globalInfo.NbOfLevels, 0);
  if (globalInfo.NbOfProcesses > 1)
  {
    for (vtkIdType level = 0; level < globalInfo.NbOfLevels; level++)
    {
      vtkIdType offset(0);
      for (int rank = 0; rank < rankInfo.Rank; rank++)
      {
        offset += globalInfo.BlocksPerLevelAndRank[level + rank * globalInfo.NbOfLevels];
      }
      rankInfo.BlockOffsets[level] = offset;
    }
  }
}

// ---------------------
// initialize AMR: each rank has same structure
// nb of Levels and nb of Blocks per level.
// init each bloc with nullptr
void InitializeLocalAMR(GlobalInfo& globalInfo, vtkOverlappingAMR* amr)
{
  std::vector<int> blocksPerLevelGlobal(globalInfo.NbOfLevels, 0);
  for (vtkIdType level = 0; level < globalInfo.NbOfLevels; level++)
  {
    for (int rank = 0; rank < globalInfo.NbOfProcesses; rank++)
    {
      blocksPerLevelGlobal[level] +=
        globalInfo.BlocksPerLevelAndRank[level + rank * globalInfo.NbOfLevels];
    }
  }
  amr->Initialize(globalInfo.NbOfLevels, blocksPerLevelGlobal.data());
  for (int level = 0; level < globalInfo.NbOfLevels; ++level)
  {
    for (int block = 0; block < blocksPerLevelGlobal[level]; ++block)
    {
      amr->SetDataSet(level, block, nullptr);
    }
  }

  // set origin
  amr->SetOrigin(globalInfo.Origin);
}

// ---------------------
// Fill local data
void FillLocalData(const conduit_cpp::Node& child, const LocalInfo& rankInfo,
  const GlobalInfo& globalInfo, vtkOverlappingAMR* amr)
{
  double origin[3];
  double spacing[3];

  if (!child.has_path("state"))
  {
    return;
  }

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

  if (child.has_path("fields"))
  {
    const auto fields = child["fields"];
    vtkConduitToDataObject::AddFieldData(ug, fields, true);
  }

  vtkAMRBox box(origin, pdims, spacing, globalInfo.Origin, amr->GetGridDescription());
  // set level spacing
  amr->SetSpacing(level, spacing);
  amr->SetAMRBox(
    level, rankInfo.DomainBlockLevelIds.at(domain_id).second + rankInfo.BlockOffsets[level], box);
  amr->SetDataSet(
    level, rankInfo.DomainBlockLevelIds.at(domain_id).second + rankInfo.BlockOffsets[level], ug);

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

// distribute AMRBoxes to all processes
void DistributeAMRBoxes(
  const LocalInfo& rankInfo, const GlobalInfo& globalInfo, vtkOverlappingAMR* amr)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  if (globalInfo.NbOfProcesses == 1 || !controller)
  {
    return;
  }
  std::vector<vtkIdType> boxBoundsOffsets(globalInfo.NbOfProcesses, 0);
  std::vector<vtkIdType> boxBoundsCounts(globalInfo.NbOfProcesses);
  std::vector<int> boxExtentsLocal(8 * rankInfo.NbOfBlocks, 0);
  std::vector<int> boxExtentsGlobal(8 * globalInfo.NbOfBlocks, 0);

  for (int rank = 0; rank < globalInfo.NbOfProcesses; ++rank)
  {
    int num_blocks = 0;
    for (int level = 0; level < globalInfo.NbOfLevels; level++)
    {
      num_blocks += globalInfo.BlocksPerLevelAndRank[level + rank * globalInfo.NbOfLevels];
    }
    boxBoundsCounts[rank] = num_blocks * 8;
    if (rank > 0)
    {
      boxBoundsOffsets[rank] = boxBoundsCounts[rank - 1] + boxBoundsOffsets[rank - 1];
    }
  }

  int local_index = 0;
  for (std::map<int, std::pair<int, int>>::const_iterator it = rankInfo.DomainBlockLevelIds.begin();
       it != rankInfo.DomainBlockLevelIds.end(); ++it)
  {
    int level = it->second.first;
    int id = it->second.second + rankInfo.BlockOffsets[level];

    vtkAMRBox box = amr->GetAMRBox(level, id);
    const int* loCorner = box.GetLoCorner();
    const int* hiCorner = box.GetHiCorner();
    int offset = 8 * local_index;
    boxExtentsLocal[offset + 0] = level;
    boxExtentsLocal[offset + 1] = id;
    boxExtentsLocal[offset + 2] = loCorner[0];
    boxExtentsLocal[offset + 3] = loCorner[1];
    boxExtentsLocal[offset + 4] = loCorner[2];
    boxExtentsLocal[offset + 5] = hiCorner[0];
    boxExtentsLocal[offset + 6] = hiCorner[1];
    boxExtentsLocal[offset + 7] = hiCorner[2];
    ++local_index;
  }

  controller->AllGatherV(boxExtentsLocal.data(), boxExtentsGlobal.data(), boxExtentsLocal.size(),
    boxBoundsCounts.data(), boxBoundsOffsets.data());
  for (int block = 0; block < globalInfo.NbOfBlocks; ++block)
  {
    int level = boxExtentsGlobal[8 * block];
    int id = boxExtentsGlobal[8 * block + 1];
    int* dims = &boxExtentsGlobal[8 * block + 2];
    vtkAMRBox box(dims[0], dims[1], dims[2], dims[3], dims[4], dims[5]);
    amr->SetAMRBox(level, id, box);
  }

  // set homogeneous spacing
  std::vector<double> local_spacings(globalInfo.NbOfLevels, 0.);
  for (int level = 0; level < globalInfo.NbOfLevels; level++)
  {
    double lvl_spacing[3];
    amr->GetSpacing(level, lvl_spacing);
    local_spacings[level] = lvl_spacing[0];
  }
  std::vector<double> global_spacing(globalInfo.NbOfLevels);
  controller->AllReduce(
    local_spacings.data(), global_spacing.data(), globalInfo.NbOfLevels, vtkCommunicator::MAX_OP);
  for (int level = 0; level < globalInfo.NbOfLevels; level++)
  {
    // spacing is homogeneous in all 3 directions.
    double lvl_spacing[3] = { global_spacing[level], global_spacing[level], global_spacing[level] };
    amr->SetSpacing(level, lvl_spacing);
  }
}
};

namespace vtkConduitToDataObject
{
VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
struct FieldMetadata
{
  vtkSmartPointer<vtkDataArray> ValuesToReplace = nullptr;
  vtkSmartPointer<vtkDataArray> ReplacementValues = nullptr;
  std::string AttributeType;

  static vtkDataSetAttributes::AttributeTypes GetDataSetAttributeType(
    const std::string& otherAttributeTypeName)
  {
    for (int i = 0; i < vtkDataSetAttributes::AttributeTypes::NUM_ATTRIBUTES; ++i)
    {
      const std::string attributeTypeName = vtkDataSetAttributes::GetAttributeTypeAsString(i);
      if (vtksys::SystemTools::UpperCase(otherAttributeTypeName) ==
        vtksys::SystemTools::UpperCase(attributeTypeName))
      {
        return static_cast<vtkDataSetAttributes::AttributeTypes>(i);
      }
    }
    return vtkDataSetAttributes::AttributeTypes::NUM_ATTRIBUTES;
  }

  static bool IsGhostsAttributeType(const std::string& otherAttributeTypeName)
  {
    return vtksys::SystemTools::UpperCase(otherAttributeTypeName) == "GHOSTS";
  }
};

//----------------------------------------------------------------------------
struct ReplaceValuesWorker
{
  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T* valuesToReplace, Array2T* replacementValues, Array3T* array) const
  {
    const vtkIdType numValuesToReplace = valuesToReplace->GetNumberOfTuples();
    auto valuesToReplaceRange = vtk::DataArrayValueRange(valuesToReplace);
    auto replacementValuesRange = vtk::DataArrayValueRange(replacementValues);
    auto arrayRange = vtk::DataArrayValueRange(array);

    vtkSMPTools::For(0, array->GetNumberOfTuples(),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType inputIdx = begin; inputIdx < end; ++inputIdx)
        {
          for (vtkIdType repValueId = 0; repValueId < numValuesToReplace; ++repValueId)
          {
            if (valuesToReplaceRange[repValueId] == arrayRange[inputIdx])
            {
              arrayRange[inputIdx] = replacementValuesRange[repValueId];
              break;
            }
          }
        }
      });
  }
};

//----------------------------------------------------------------------------
bool FillPartitionedDataSet(vtkPartitionedDataSet* output, const conduit_cpp::Node& node)
{
#if !VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  // conduit verify_shapes_node dereferences the shapes array to compare
  // values with the values in the shapes_map
  // if the shapes array is in device memory this test crashes
  // https://github.com/LLNL/conduit/issues/1404
  conduit_cpp::Node info;
  if (!conduit_cpp::BlueprintMesh::verify(node, info))
  {
    vtkLogF(ERROR, "Mesh blueprint verification failed!");
    return false;
  }
  vtkLogF(TRACE, "Mesh blueprint verified!");
#endif
  std::map<std::string, vtkSmartPointer<vtkDataSet>> datasets;

  // process "topologies".
  auto topologies = node["topologies"];

  for (conduit_index_t i = 0, nchildren = topologies.number_of_children(); i < nchildren; ++i)
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

  // read "state/metadata/vtk_fields"
  std::map<std::string, FieldMetadata> fieldMetadata;
  if (node.has_path("state/metadata/vtk_fields"))
  {
    auto fieldsMetadata = node["state/metadata/vtk_fields"];
    for (conduit_index_t i = 0, nchildren = fieldsMetadata.number_of_children(); i < nchildren; ++i)
    {
      auto fieldMetadataNode = fieldsMetadata.child(i);
      const auto& name = fieldMetadataNode.name();
      try
      {
        // read values_to_replace and replacement_values if they exist
        if (fieldMetadataNode.has_path("values_to_replace") &&
          fieldMetadataNode.has_path("replacement_values"))
        {
          auto valuesToReplace = fieldMetadataNode["values_to_replace"];
          fieldMetadata[name].ValuesToReplace =
            vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&valuesToReplace));
          auto replacementValues = fieldMetadataNode["replacement_values"];
          fieldMetadata[name].ReplacementValues =
            vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&replacementValues));
          if (fieldMetadata[name].ValuesToReplace->GetNumberOfTuples() !=
            fieldMetadata[name].ReplacementValues->GetNumberOfTuples())
          {
            vtkLogF(ERROR,
              "values_to_replace and replacement_values should have equal size for field '%s'.",
              name.c_str());
            return false;
          }
          if (fieldMetadata[name].ValuesToReplace->GetNumberOfComponents() != 1 ||
            fieldMetadata[name].ReplacementValues->GetNumberOfComponents() != 1)
          {
            vtkLogF(ERROR,
              "values_to_replace and replacement_values should have 1 component for field '%s'.",
              name.c_str());
            return false;
          }
        }
        // read attribute type if it exists
        if (fieldMetadataNode.has_path("attribute_type"))
        {
          const std::string& attributeType = fieldMetadataNode["attribute_type"].as_string();
          // check if the attribute type is valid
          if (FieldMetadata::GetDataSetAttributeType(attributeType) !=
              vtkDataSetAttributes::AttributeTypes::NUM_ATTRIBUTES ||
            FieldMetadata::IsGhostsAttributeType(attributeType))
          {
            fieldMetadata[name].AttributeType = attributeType;
          }
          else
          {
            vtkLogF(
              ERROR, "invalid attribute type '%s' for '%s'.", attributeType.c_str(), name.c_str());
            return false;
          }
        }
      }
      catch (std::exception& e)
      {
        vtkLogF(ERROR, "failed to process '../state/metadata/vtk_fields/%s'.", name.c_str());
        vtkLogF(ERROR, "ERROR: \n%s\n", e.what());
        return false;
      }
    }
  }
  auto fields = node["fields"];
  for (conduit_index_t i = 0, nchildren = fields.number_of_children(); i < nchildren; ++i)
  {
    auto fieldNode = fields.child(i);
    const auto& fieldname = fieldNode.name();
    try
    {
      auto dataset = datasets.at(fieldNode["topology"].as_string());
      const auto vtk_association = GetAssociation(fieldNode["association"].as_string());
      auto dsa = dataset->GetAttributes(vtk_association);
      auto values = fieldNode["values"];
      std::size_t dataset_size;
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
        // This code path should be removed once MCGhostArrayToVTKGhostArray is removed.
        if (fieldname == "ascent_ghosts")
        {
          // convert ascent ghost information into VTK ghost information
          // the VTK array is named vtkDataSetAttributes::GhostArrayName()
          // and has different values.
          auto array = vtkConduitArrayUtilities::MCGhostArrayToVTKGhostArray(
            conduit_cpp::c_node(&values), dsa->IsA("vtkCellData"));
          dsa->AddArray(array);
          continue;
        }
        vtkSmartPointer<vtkDataArray> array =
          vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&values), fieldname);
        if (array->GetNumberOfTuples() != dataset->GetNumberOfElements(vtk_association))
        {
          throw std::runtime_error("mismatched tuple count!");
        }
        if (fieldMetadata.find(fieldname) != fieldMetadata.end())
        {
          const auto& metadata = fieldMetadata[fieldname];
          // replace values if needed
          if (metadata.ValuesToReplace && metadata.ReplacementValues)
          {
            ReplaceValuesWorker replaceValuesWorker;
            if (!vtkArrayDispatch::Dispatch3SameValueType::Execute(metadata.ValuesToReplace.Get(),
                  metadata.ReplacementValues.Get(), array.Get(), replaceValuesWorker))
            {
              replaceValuesWorker(
                metadata.ValuesToReplace.Get(), metadata.ReplacementValues.Get(), array.Get());
            }
          }
          // extract the attribute type, and change the array name if needed
          auto dsaAttributeType = vtkDataSetAttributes::AttributeTypes::NUM_ATTRIBUTES;
          if (!metadata.AttributeType.empty())
          {
            dsaAttributeType = FieldMetadata::GetDataSetAttributeType(metadata.AttributeType);
            if (FieldMetadata::IsGhostsAttributeType(metadata.AttributeType))
            {
              // convert its name to the VTK ghost array name
              array->SetName(vtkDataSetAttributes::GhostArrayName());
              // ensure the array is unsigned char
              if (!array->IsA("vtkUnsignedCharArray"))
              {
                auto ghostArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
                ghostArray->DeepCopy(array);
                array = ghostArray;
              }
            }
          }
          if (dsaAttributeType != vtkDataSetAttributes::AttributeTypes::NUM_ATTRIBUTES)
          {
            dsa->SetAttribute(array, dsaAttributeType);
          }
          else
          {
            dsa->AddArray(array);
          }
        }
        else
        {
          dsa->AddArray(array);
        }
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
bool FillPartionedDataSet(vtkPartitionedDataSet* output, const conduit_cpp::Node& meshNode)
{
  return FillPartitionedDataSet(output, meshNode);
}

//----------------------------------------------------------------------------
bool FillAMRMesh(vtkOverlappingAMR* amr, const conduit_cpp::Node& node)
{
  AMRUtils::LocalInfo rankInfo;
  AMRUtils::GlobalInfo globalInfo;

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller)
  {
    // if VTK was initialized properly controller should be non-null but that's not always
    // the case so safer to check if controller is available
    globalInfo.NbOfProcesses = controller->GetNumberOfProcesses();
    rankInfo.Rank = controller->GetLocalProcessId();
  }
  AMRUtils::ConstructLocalInfo(node, rankInfo);

  AMRUtils::GatherInfos(rankInfo, globalInfo);

  AMRUtils::InitializeLocalAMR(globalInfo, amr);

  for (conduit_index_t cc = 0; cc < rankInfo.NbOfLeaves; ++cc)
  {
    const auto child = node.child(cc);
    AMRUtils::FillLocalData(child, rankInfo, globalInfo, amr);
  }

  AMRUtils::DistributeAMRBoxes(rankInfo, globalInfo, amr);

  if (globalInfo.NbOfProcesses == 1)
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

  if (coords["type"].as_string() == "explicit" && topology["type"].as_string() == "points")
  {
    auto pointset = vtkSmartPointer<vtkPointSet>::New();
    pointset->SetPoints(CreatePoints(coords));
    return pointset;
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
/**
 * The "const" of values_xyz is necessary to avoid creating a new object.
 * If value_xyz is not const, coordset["values/xyz"] must NOT be const either
 * to call the correct copy constructor.
 */
vtkSmartPointer<vtkRectilinearGrid> CreateRectilinearGrid(const conduit_cpp::Node& coordset)
{
  auto rectilinearGrid = vtkSmartPointer<vtkRectilinearGrid>::New();

  const bool has_x_values = coordset.has_path("values/x");
  const conduit_cpp::Node values_x = has_x_values ? coordset["values/x"] : conduit_cpp::Node();
  const bool has_y_values = coordset.has_path("values/y");
  const conduit_cpp::Node values_y = has_y_values ? coordset["values/y"] : conduit_cpp::Node();
  const bool has_z_values = coordset.has_path("values/z");
  const conduit_cpp::Node values_z = has_z_values ? coordset["values/z"] : conduit_cpp::Node();

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
  vtkIdType numberOfPoints = unstructured->GetNumberOfPoints();
  const auto vtk_cell_type = GetCellType(topologyNode["elements/shape"].as_string());
  if (nb_cells > 0)
  {
    if (vtk_cell_type == VTK_POLYHEDRON)
    {
      int8_t id;
      bool working;
      bool isDevicePointer =
        vtkConduitArrayUtilities::IsDevicePointer(connectivity.element_ptr(0), id, working);
      if (isDevicePointer)
      {
        throw std::runtime_error("Viskores does not support VTK_POLYHEDRON cell type");
      }
      // polyhedra uses O2M and not M2C arrays, so need to process it
      // differently.
      conduit_cpp::Node t_elements = topologyNode["elements"];
      conduit_cpp::Node t_subelements = topologyNode["subelements"];
      auto elements = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
        numberOfPoints, conduit_cpp::c_node(&t_elements));
      auto subelements = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
        numberOfPoints, conduit_cpp::c_node(&t_subelements));

      SetPolyhedralCells(unstructured, elements, subelements);
    }
    else if (vtk_cell_type == VTK_POLYGON)
    {
      // polygons use O2M and not M2C arrays, so need to process it
      // differently.
      conduit_cpp::Node t_elements = topologyNode["elements"];
      auto cellArray = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
        numberOfPoints, conduit_cpp::c_node(&t_elements));
      unstructured->SetCells(vtk_cell_type, cellArray);
    }
    else
    {
      const auto cell_size = GetNumberOfPointsInCellType(vtk_cell_type);
      auto cellArray = vtkConduitArrayUtilities::MCArrayToVTKCellArray(
        numberOfPoints, vtk_cell_type, cell_size, conduit_cpp::c_node(&connectivity));
      unstructured->SetCells(vtk_cell_type, cellArray);
    }
  }

  return unstructured;
}

/**
 * See CreateMixedUnstructuredGrid.
 */
void SetMixedPolyhedralCells(
  vtkUnstructuredGrid* ug, vtkDataArray* shapes, vtkCellArray* elements, vtkCellArray* subelements)
{
  auto cellTypes = vtk::MakeSmartPointer(vtkUnsignedCharArray::SafeDownCast(shapes));
  if (!cellTypes)
  {
    cellTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
    cellTypes->DeepCopy(shapes);
  }
  // if there are no subelements
  if (!subelements || subelements->GetNumberOfCells() == 0)
  {
    // This is a simple case where we have a mixed cell type, but no polyhedra.
    ug->SetPolyhedralCells(cellTypes, elements, nullptr, nullptr);
    return;
  }

  vtkNew<vtkCellArray> connectivity;
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkCellArray> faceLocations;
  subelements->IsStorage64Bit()
    ? faces->ConvertTo64BitStorage() && faceLocations->ConvertTo64BitStorage()
    : faces->ConvertTo64BitStorage() && faceLocations->ConvertTo32BitStorage();

  connectivity->AllocateEstimate(elements->GetNumberOfCells(), 10);
  faces->AllocateExact(
    subelements->GetNumberOfCells(), subelements->GetConnectivityArray()->GetNumberOfTuples());
  faceLocations->AllocateExact(elements->GetNumberOfCells(), subelements->GetNumberOfCells());

  vtkIdType numCellFaces, numFacePointIDs, numCellPointIDs;
  const vtkIdType *cellGlobalFaceIDs, *facePointIDs, *cellPointIDs;
  std::set<vtkIdType> cellPointIDsSet;
  vtkIdType globalFaceId = 0;
  auto cellTypesRange = vtk::DataArrayValueRange<1>(cellTypes);
  for (vtkIdType i = 0, numCells = elements->GetNumberOfCells(); i < numCells; ++i)
  {
    const unsigned char& cellType = cellTypesRange[i];
    if (cellType == VTK_POLYHEDRON)
    {
      cellPointIDsSet.clear();
      // https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#polyhedra
      // This in conduit describes a polyhedron' global face IDs, and not its point IDs.
      // Even after https://gitlab.kitware.com/vtk/vtk/-/issues/18190 was resolved, the conduit
      // format is still different from the VTK format, so we need to do some conversions for VTK.
      elements->GetCellAtId(i, numCellFaces, cellGlobalFaceIDs);

      faceLocations->InsertNextCell(numCellFaces);
      for (vtkIdType j = 0; j < numCellFaces; ++j)
      {
        faceLocations->InsertCellPoint(globalFaceId++);

        subelements->GetCellAtId(cellGlobalFaceIDs[j], numFacePointIDs, facePointIDs);
        // If VTK' polyhedron format had a notion of global face IDs, we could just use
        // subelements as faces, instead of copying each face, but sadly that's not true.
        faces->InsertNextCell(numFacePointIDs, facePointIDs);
        // accumulate point IDs from all faces in this polyhedron
        cellPointIDsSet.insert(facePointIDs, facePointIDs + numFacePointIDs);
      }

      // Insert the points IDs of this polyhedron into the 'connectivity' array.
      connectivity->InsertNextCell(static_cast<int>(cellPointIDsSet.size()));
      for (const auto& pt : cellPointIDsSet)
      {
        connectivity->InsertCellPoint(pt);
      }
    }
    else
    {
      // A normal cell's point IDs that are just copied over.
      elements->GetCellAtId(i, numCellPointIDs, cellPointIDs);
      connectivity->InsertNextCell(numCellPointIDs, cellPointIDs);
      // This indicates that this cell has no faces that need to be recorded.
      faceLocations->InsertNextCell(0);
    }
  }

  connectivity->Squeeze();
  faces->Squeeze();
  faceLocations->Squeeze();

  ug->SetPolyhedralCells(cellTypes, connectivity, faceLocations, faces);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> CreateMixedUnstructuredGrid(
  const conduit_cpp::Node& topologyNode, const conduit_cpp::Node& coords)
{
  auto unstructured = vtkSmartPointer<vtkUnstructuredGrid>::New();
  // mixed shapes definition
  conduit_cpp::Node shape_map = topologyNode["elements/shape_map"];
  auto connectivity = topologyNode["elements/connectivity"];
  int8_t id;
  bool working;
  bool isDevicePointer =
    vtkConduitArrayUtilities::IsDevicePointer(connectivity.element_ptr(0), id, working);
  if (isDevicePointer && !working)
  {
    throw std::runtime_error("Viskores does not support device" + std::to_string(id));
  }

  // check presence of polyhedra
  bool hasPolyhedra(false);
  conduit_index_t nCells = shape_map.number_of_children();
  for (conduit_index_t i = 0; i < nCells && !hasPolyhedra; ++i)
  {
    auto child = shape_map.child(i);
    int cellType = child.to_int32();
    hasPolyhedra |= (cellType == VTK_POLYHEDRON);
  }
  if (isDevicePointer && hasPolyhedra)
  {
    throw std::runtime_error("Viskores does not support VTK_POLYHEDRON cell type");
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
    auto numberOfPoints = unstructured->GetNumberOfPoints();

    conduit_cpp::Node t_elements = topologyNode["elements"];
    conduit_cpp::Node t_elementShapes = topologyNode["elements/shapes"];

    auto shapes =
      vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&t_elementShapes));
    auto elements = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
      numberOfPoints, conduit_cpp::c_node(&t_elements));
    if (!elements || !shapes)
    {
      throw std::runtime_error("elements or elements/shapes not available (nullptr)");
    }

    if (hasPolyhedra)
    {
      conduit_cpp::Node t_subelements = topologyNode["subelements"];
      auto subelements = vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
        numberOfPoints, conduit_cpp::c_node(&t_subelements));
      if (!subelements)
      {
        throw std::runtime_error("subelements not available (nullptr)");
      }
      SetMixedPolyhedralCells(unstructured, shapes, elements, subelements);
    }
    else
    {
      SetMixedPolyhedralCells(unstructured, shapes, elements, nullptr);
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
      std::size_t dataset_size = 0;
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

        if ((field_name == "time" || field_name == "TimeValue") && field_node.dtype().is_number())
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
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfTuples(elements->GetNumberOfCells());
  cellTypes->FillValue(static_cast<unsigned char>(VTK_POLYHEDRON));
  SetMixedPolyhedralCells(grid, cellTypes, elements, subelements);
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
    case VTK_PYRAMID:
      return 5;
    case VTK_WEDGE:
      return 6;
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
  else if (shape == "wedge")
  {
    return VTK_WEDGE;
  }
  else if (shape == "pyramid")
  {
    return VTK_PYRAMID;
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
