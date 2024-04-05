// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIOSSReader.h"
#include "vtkIOSSFilesScanner.h"
#include "vtkIOSSReaderCommunication.h"
#include "vtkIOSSReaderInternal.h"
#include "vtkIOSSUtilities.h"

#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
#include "vtkExtractGrid.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkMultiProcessStreamSerialization.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkRemoveUnusedPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

// clang-format off
#include VTK_IOSS(Ionit_Initializer.h)
#include VTK_IOSS(Ioss_Assembly.h)
#include VTK_IOSS(Ioss_DatabaseIO.h)
#include VTK_IOSS(Ioss_EdgeBlock.h)
#include VTK_IOSS(Ioss_EdgeSet.h)
#include VTK_IOSS(Ioss_ElementBlock.h)
#include VTK_IOSS(Ioss_ElementSet.h)
#include VTK_IOSS(Ioss_ElementTopology.h)
#include VTK_IOSS(Ioss_FaceBlock.h)
#include VTK_IOSS(Ioss_FaceSet.h)
#include VTK_IOSS(Ioss_IOFactory.h)
#include VTK_IOSS(Ioss_NodeBlock.h)
#include VTK_IOSS(Ioss_NodeSet.h)
#include VTK_IOSS(Ioss_Region.h)
#include VTK_IOSS(Ioss_SideBlock.h)
#include VTK_IOSS(Ioss_SideSet.h)
#include VTK_IOSS(Ioss_StructuredBlock.h)
// clang-format on

#include <array>
#include <cassert>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

//============================================================================
vtkStandardNewMacro(vtkIOSSReader);
vtkCxxSetObjectMacro(vtkIOSSReader, Controller, vtkMultiProcessController);
vtkInformationKeyMacro(vtkIOSSReader, ENTITY_TYPE, Integer);
vtkInformationKeyMacro(vtkIOSSReader, ENTITY_ID, Integer);
//----------------------------------------------------------------------------
vtkIOSSReader::vtkIOSSReader()
  : AssemblyTag(0)
  , Internals(new vtkIOSSReaderInternal(this))
  , Controller(nullptr)
  , Caching(false)
  , MergeExodusEntityBlocks(false)
  , ElementAndSideIds(true)
  , GenerateFileId(false)
  , ScanForRelatedFiles(true)
  , ReadIds(true)
  , RemoveUnusedPoints(true)
  , ApplyDisplacements(true)
  , ReadAllFilesToDetermineStructure(true)
  , ReadGlobalFields(true)
  , ReadQAAndInformationRecords(true)
  , DatabaseTypeOverride(nullptr)
  , FileRange{ 0, -1 }
  , FileStride{ 1 }
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  // default - treat numeric suffixes as separate vtk data arrays.
  this->AddProperty("IGNORE_REALN_FIELDS", "on");
  // default - empty field suffix separators, fieldX, fieldY, fieldZ are recognized
  this->AddProperty("FIELD_SUFFIX_SEPARATOR", "");
}

//----------------------------------------------------------------------------
vtkIOSSReader::~vtkIOSSReader()
{
  this->SetDatabaseTypeOverride(nullptr);
  this->SetController(nullptr);
  delete this->Internals;
}

//----------------------------------------------------------------------------
int vtkIOSSReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetDisplacementMagnitude(double magnitude)
{
  const double oldMagnitude = this->Internals->GetDisplacementMagnitude();
  this->Internals->SetDisplacementMagnitude(magnitude);
  if (magnitude != oldMagnitude)
  {
    this->Modified();
  }
}
//----------------------------------------------------------------------------
double vtkIOSSReader::GetDisplacementMagnitude()
{
  return this->Internals->GetDisplacementMagnitude();
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetGroupNumericVectorFieldComponents(bool value)
{
  // invert the property - group implies considering realN fields.
  // not grouping implies ignoring realN fields.
  this->AddProperty("IGNORE_REALN_FIELDS", value ? "off" : "on");
}

//----------------------------------------------------------------------------
bool vtkIOSSReader::GetGroupNumericVectorFieldComponents()
{
  return this->Internals->DatabaseProperties.get("IGNORE_REALN_FIELDS").get_string() == "off";
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetFieldSuffixSeparator(const char* value)
{
  vtkDebugMacro("Setting FIELD_SUFFIX_SEPARATOR " << (value ? "on" : "off"));
  this->AddProperty("FIELD_SUFFIX_SEPARATOR", value);
}

//----------------------------------------------------------------------------
std::string vtkIOSSReader::GetFieldSuffixSeparator()
{
  return this->Internals->DatabaseProperties.get("FIELD_SUFFIX_SEPARATOR").get_string();
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetScanForRelatedFiles(bool val)
{
  if (this->ScanForRelatedFiles != val)
  {
    this->ScanForRelatedFiles = val;
    auto& internals = (*this->Internals);
    internals.FileNamesMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetCaching(bool val)
{
  if (this->Caching != val)
  {
    this->Internals->ClearCache();
    this->Caching = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetMergeExodusEntityBlocks(bool val)
{
  if (this->MergeExodusEntityBlocks != val)
  {
    // clear cache to ensure we read appropriate points/point data.
    this->Internals->ClearCache();
    this->MergeExodusEntityBlocks = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetElementAndSideIds(bool val)
{
  if (this->ElementAndSideIds != val)
  {
    // Clear cache to ensure we regenerate with/without the side-set metadata.
    this->Internals->ClearCache();
    this->ElementAndSideIds = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetFileName(const char* fname)
{
  auto& internals = (*this->Internals);
  if (fname == nullptr)
  {
    if (!internals.FileNames.empty())
    {
      internals.FileNames.clear();
      internals.FileNamesMTime.Modified();
      this->Modified();
    }
    return;
  }

  if (internals.FileNames.size() == 1 && *internals.FileNames.begin() == fname)
  {
    return;
  }

  internals.FileNames.clear();
  internals.FileNames.insert(fname);
  internals.FileNamesMTime.Modified();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkIOSSReader::AddFileName(const char* fname)
{
  auto& internals = (*this->Internals);
  if (fname != nullptr && !internals.FileNames.insert(fname).second)
  {
    internals.FileNamesMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::ClearFileNames()
{
  auto& internals = (*this->Internals);
  if (!internals.FileNames.empty())
  {
    internals.FileNames.clear();
    internals.FileNamesMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* vtkIOSSReader::GetFileName(int index) const
{
  auto& internals = (*this->Internals);
  if (static_cast<int>(internals.FileNames.size()) > index)
  {
    auto iter = std::next(internals.FileNames.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkIOSSReader::GetNumberOfFileNames() const
{
  auto& internals = (*this->Internals);
  return static_cast<int>(internals.FileNames.size());
}

//----------------------------------------------------------------------------
int vtkIOSSReader::ReadMetaData(vtkInformation* metadata)
{
  vtkLogScopeF(TRACE, "ReadMetaData");
  vtkIOSSUtilities::CaptureNonErrorMessages captureMessagesRAII;

  auto& internals = (*this->Internals);
  if (!internals.UpdateDatabaseNames(this))
  {
    return 0;
  }

  // read time information and generate that.
  if (!internals.UpdateTimeInformation(this))
  {
    return 0;
  }
  else
  {
    // add timesteps to metadata
    const auto& timesteps = internals.GetTimeSteps();
    if (!timesteps.empty())
    {
      metadata->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timesteps.data(),
        static_cast<int>(timesteps.size()));
      double time_range[2] = { timesteps.front(), timesteps.back() };
      metadata->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), time_range, 2);
    }
    else
    {
      metadata->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      metadata->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }
  }

  // read field/entity selection meta-data. i.e. update vtkDataArraySelection
  // instances for all available entity-blocks, entity-sets, and their
  // corresponding data arrays.
  if (!internals.UpdateEntityAndFieldSelections(this))
  {
    return 0;
  }

  // read assembly information.
  if (!internals.UpdateAssembly(this, &this->AssemblyTag))
  {
    return 0;
  }

  metadata->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkIOSSReader::ReadMesh(
  int piece, int npieces, int vtkNotUsed(nghosts), int timestep, vtkDataObject* output)
{
  auto& internals = (*this->Internals);
  vtkIOSSUtilities::CaptureNonErrorMessages captureMessagesRAII;

  if (!internals.UpdateDatabaseNames(this))
  {
    // this should not be necessary. ReadMetaData returns false when
    // `UpdateDatabaseNames` fails. At which point vtkReaderAlgorithm should
    // never call `RequestData` leading to a call to this method. However, it
    // does, for some reason. Hence, adding this check here.
    // ref: paraview/paraview#19951.
    return 0;
  }

  // This is the first method that gets called when generating data.
  // Reset internal cache counters, so we can flush fields not accessed.
  internals.ResetCacheAccessCounts();

  auto collection = vtkPartitionedDataSetCollection::SafeDownCast(output);

  // setup output based on the block/set selections (and those available in the
  // database).
  if (!internals.GenerateOutput(collection, this))
  {
    vtkErrorMacro("Failed to generate output.");
    return 0;
  }

  std::set<unsigned int> selectedAssemblyIndices;
  if (!internals.Selectors.empty() && internals.GetAssembly() != nullptr)
  {
    std::vector<std::string> selectors(internals.Selectors.size());
    std::copy(internals.Selectors.begin(), internals.Selectors.end(), selectors.begin());
    auto assembly = internals.GetAssembly();
    auto nodes = assembly->SelectNodes(selectors);
    auto dsindices = assembly->GetDataSetIndices(nodes);
    selectedAssemblyIndices.insert(dsindices.begin(), dsindices.end());
  }

  // dbaseHandles are handles for individual files this instance will to read to
  // satisfy the request. Can be >= 0.
  const auto dbaseHandles = internals.GetDatabaseHandles(piece, npieces, timestep);

  // Read global data. Since this should be same on all ranks, we only read on
  // root node and broadcast it to all. This helps us easily handle the case
  // where the number of reading-ranks is more than writing-ranks.
  auto controller = this->GetController();
  const auto rank = controller ? controller->GetLocalProcessId() : 0;
  const auto numRanks = controller ? controller->GetNumberOfProcesses() : 1;
  if (!dbaseHandles.empty() && rank == 0)
  {
    // Read global data. Since global data is expected to be identical on all
    // files in a partitioned collection, we can read it from the first
    // dbaseHandle alone.
    if (this->ReadGlobalFields)
    {
      internals.GetGlobalFields(collection->GetFieldData(), dbaseHandles[0], timestep);
    }

    if (this->ReadQAAndInformationRecords)
    {
      internals.GetQAAndInformationRecords(collection->GetFieldData(), dbaseHandles[0]);
    }

    // Handle assemblies.
    internals.ReadAssemblies(collection, dbaseHandles[0]);
  }

  // check if we are going to merge all the blocks/sets of an entity type into a single one
  const bool mergeEntityBlocks =
    internals.GetFormat() == vtkIOSSUtilities::DatabaseFormatType::EXODUS &&
    this->GetMergeExodusEntityBlocks();
  if (!mergeEntityBlocks)
  {
    for (unsigned int pdsIdx = 0; pdsIdx < collection->GetNumberOfPartitionedDataSets(); ++pdsIdx)
    {
      const std::string blockName(
        collection->GetMetaData(pdsIdx)->Get(vtkCompositeDataSet::NAME()));
      const auto entity_type = collection->GetMetaData(pdsIdx)->Get(ENTITY_TYPE());
      const auto vtk_entity_type = static_cast<vtkIOSSReader::EntityType>(entity_type);

      auto selection = this->GetEntitySelection(vtk_entity_type);
      if (!selection->ArrayIsEnabled(blockName.c_str()) &&
        selectedAssemblyIndices.find(pdsIdx) == selectedAssemblyIndices.end())
      {
        // skip disabled blocks.
        continue;
      }

      auto pds = collection->GetPartitionedDataSet(pdsIdx);
      assert(pds != nullptr);
      for (const auto& handle : dbaseHandles)
      {
        try
        {
          auto datasets = internals.GetDataSets(blockName, vtk_entity_type, handle, timestep, this);
          for (auto& ds : datasets)
          {
            pds->SetPartition(pds->GetNumberOfPartitions(), ds);
          }
        }
        catch (const std::runtime_error& e)
        {
          vtkLogF(ERROR,
            "Error reading entity block (or set) named '%s' from '%s'; skipping. Details: %s",
            blockName.c_str(), internals.GetRawFileName(handle).c_str(), e.what());
        }
        // Note: Consider using the inner ReleaseHandles (and not the outer) for debugging purposes
        // internals.ReleaseHandles();
      }
    }
  }
  else
  {
    for (unsigned int pdsIdx = 0; pdsIdx < collection->GetNumberOfPartitionedDataSets(); ++pdsIdx)
    {
      const auto entity_type = collection->GetMetaData(pdsIdx)->Get(ENTITY_TYPE());
      const auto vtk_entity_type = static_cast<vtkIOSSReader::EntityType>(entity_type);
      auto selection = this->GetEntitySelection(vtk_entity_type);

      // get all the active block names for this entity type.
      std::vector<std::string> blockNames;
      for (int i = 0; i < selection->GetNumberOfArrays(); ++i)
      {
        if (selection->ArrayIsEnabled(selection->GetArrayName(i)))
        {
          blockNames.emplace_back(selection->GetArrayName(i));
        }
      }

      if (blockNames.empty())
      {
        // skip disabled blocks.
        continue;
      }

      auto pds = collection->GetPartitionedDataSet(pdsIdx);
      assert(pds != nullptr);
      for (const auto& handle : dbaseHandles)
      {
        try
        {
          auto dataset =
            internals.GetExodusEntityDataSet(blockNames, vtk_entity_type, handle, timestep, this);
          if (dataset != nullptr)
          {
            pds->SetPartition(pds->GetNumberOfPartitions(), dataset);
          }
        }
        catch (const std::runtime_error& e)
        {
          vtkLogF(ERROR, "Error reading entity named '%s' from '%s'; skipping. Details: %s",
            vtkIOSSReader::GetDataAssemblyNodeNameForEntityType(entity_type),
            internals.GetRawFileName(handle).c_str(), e.what());
        }
        // Note: Consider using the inner ReleaseHandles (and not the outer) for debugging
        // purposes internals.ReleaseHandles();
      }
    }
  }
  internals.ReleaseHandles();

  if (numRanks > 1)
  {
    vtkNew<vtkUnstructuredGrid> temp;
    vtkMultiProcessStream stream;
    if (rank == 0)
    {
      temp->GetFieldData()->ShallowCopy(collection->GetFieldData());
      stream << collection->GetDataAssembly()->SerializeToXML(vtkIndent());
    }
    controller->Broadcast(temp, 0);
    controller->Broadcast(stream, 0);
    if (rank > 0)
    {
      collection->GetFieldData()->ShallowCopy(temp->GetFieldData());

      std::string xml;
      stream >> xml;
      collection->GetDataAssembly()->InitializeFromXML(xml.c_str());
    }
  }

  if (!this->GetCaching() ||
    internals.GetFormat() == vtkIOSSUtilities::DatabaseFormatType::CATALYST)
  {
    // We don't want to hold on to the cache for longer than the RequestData pass.
    // For we clear it entirely here.
    internals.ClearCache();
  }
  else
  {
    internals.ClearCacheUnused();
  }
  internals.ReleaseRegions();
  return 1;
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkIOSSReader::GetEntitySelection(int type)
{
  if (type < 0 || type >= NUMBER_OF_ENTITY_TYPES)
  {
    vtkErrorMacro("Invalid type '" << type
                                   << "'. Supported values are "
                                      "vtkIOSSReader::NODEBLOCK (0), ... vtkIOSSReader::SIDESET ("
                                   << vtkIOSSReader::SIDESET << ").");
    return nullptr;
  }
  return this->EntitySelection[type];
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkIOSSReader::GetFieldSelection(int type)
{
  if (type < 0 || type >= NUMBER_OF_ENTITY_TYPES)
  {
    vtkErrorMacro("Invalid type '" << type
                                   << "'. Supported values are "
                                      "vtkIOSSReader::NODEBLOCK (0), ... vtkIOSSReader::SIDESET ("
                                   << vtkIOSSReader::SIDESET << ").");
    return nullptr;
  }
  return this->EntityFieldSelection[type];
}

//----------------------------------------------------------------------------
const std::map<std::string, vtkTypeInt64>& vtkIOSSReader::GetEntityIdMap(int type) const
{
  if (type < 0 || type >= NUMBER_OF_ENTITY_TYPES)
  {
    vtkErrorMacro("Invalid type '" << type
                                   << "'. Supported values are "
                                      "vtkIOSSReader::NODEBLOCK (0), ... vtkIOSSReader::SIDESET ("
                                   << vtkIOSSReader::SIDESET << ").");
    return this->EntityIdMap[NUMBER_OF_ENTITY_TYPES];
  }

  return this->EntityIdMap[type];
}

std::map<std::string, vtkTypeInt64>& vtkIOSSReader::GetEntityIdMap(int type)
{
  if (type < 0 || type >= NUMBER_OF_ENTITY_TYPES)
  {
    vtkErrorMacro("Invalid type '" << type
                                   << "'. Supported values are "
                                      "vtkIOSSReader::NODEBLOCK (0), ... vtkIOSSReader::SIDESET ("
                                   << vtkIOSSReader::SIDESET << ").");
    return this->EntityIdMap[NUMBER_OF_ENTITY_TYPES];
  }

  return this->EntityIdMap[type];
}

//----------------------------------------------------------------------------
vtkStringArray* vtkIOSSReader::GetEntityIdMapAsString(int type) const
{
  if (type < 0 || type >= NUMBER_OF_ENTITY_TYPES)
  {
    vtkErrorMacro("Invalid type '" << type
                                   << "'. Supported values are "
                                      "vtkIOSSReader::NODEBLOCK (0), ... vtkIOSSReader::SIDESET ("
                                   << vtkIOSSReader::SIDESET << ").");
    return this->EntityIdMapStrings[NUMBER_OF_ENTITY_TYPES];
  }

  const auto& map = this->GetEntityIdMap(type);
  auto& strings = this->EntityIdMapStrings[type];
  strings->SetNumberOfTuples(map.size() * 2);

  vtkIdType index = 0;
  for (const auto& pair : map)
  {
    strings->SetValue(index++, pair.first);
    strings->SetValue(index++, std::to_string(pair.second));
  }

  return strings;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkIOSSReader::GetMTime()
{
  auto mtime = this->Superclass::GetMTime();
  for (int cc = ENTITY_START; cc < ENTITY_END; ++cc)
  {
    mtime = std::max(mtime, this->EntitySelection[cc]->GetMTime());
    mtime = std::max(mtime, this->EntityFieldSelection[cc]->GetMTime());
  }
  return mtime;
}

//----------------------------------------------------------------------------
void vtkIOSSReader::RemoveAllEntitySelections()
{
  for (int cc = ENTITY_START; cc < ENTITY_END; ++cc)
  {
    this->GetEntitySelection(cc)->RemoveAllArrays();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::RemoveAllFieldSelections()
{
  for (int cc = ENTITY_START; cc < ENTITY_END; ++cc)
  {
    this->GetFieldSelection(cc)->RemoveAllArrays();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetRemoveUnusedPoints(bool val)
{
  if (this->RemoveUnusedPoints != val)
  {
    // clear cache to ensure we read appropriate points/point data.
    this->Internals->ClearCache();
    this->RemoveUnusedPoints = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetReadAllFilesToDetermineStructure(bool val)
{
  if (this->ReadAllFilesToDetermineStructure != val)
  {
    this->ReadAllFilesToDetermineStructure = val;
    this->Internals->ResetDatabaseNamesMTime();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* vtkIOSSReader::GetDataAssemblyNodeNameForEntityType(int type)
{
  switch (type)
  {
    case NODEBLOCK:
      return "node_blocks";
    case EDGEBLOCK:
      return "edge_blocks";
    case FACEBLOCK:
      return "face_blocks";
    case ELEMENTBLOCK:
      return "element_blocks";
    case STRUCTUREDBLOCK:
      return "structured_blocks";
    case NODESET:
      return "node_sets";
    case EDGESET:
      return "edge_sets";
    case FACESET:
      return "face_sets";
    case ELEMENTSET:
      return "element_sets";
    case SIDESET:
      return "side_sets";
    default:
      vtkLogF(ERROR, "Invalid type '%d'", type);
      return nullptr;
  }
}

//----------------------------------------------------------------------------
const char* vtkIOSSReader::GetMergedEntityNameForEntityType(int type)
{
  switch (type)
  {
    case NODEBLOCK:
      return "merged_node_blocks";
    case EDGEBLOCK:
      return "merged_edge_blocks";
    case FACEBLOCK:
      return "merged_face_blocks";
    case ELEMENTBLOCK:
      return "merged_element_blocks";
    case STRUCTUREDBLOCK:
      return "merged_structured_blocks";
    case NODESET:
      return "merged_node_sets";
    case EDGESET:
      return "merged_edge_sets";
    case FACESET:
      return "merged_face_sets";
    case ELEMENTSET:
      return "merged_element_sets";
    case SIDESET:
      return "merged_side_sets";
    default:
      vtkLogF(ERROR, "Invalid type '%d'", type);
      return nullptr;
  }
}

//----------------------------------------------------------------------------
bool vtkIOSSReader::DoTestFilePatternMatching()
{
  return vtkIOSSFilesScanner::DoTestFilePatternMatching();
}

//----------------------------------------------------------------------------
vtkTypeBool vtkIOSSReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  const auto status = this->Superclass::ProcessRequest(request, inInfo, outInfo);

  auto& internals = (*this->Internals);
  internals.ReleaseHandles();
  return status;
}

//----------------------------------------------------------------------------
template <typename T>
bool updateProperty(Ioss::PropertyManager& pm, const std::string& name, const T& value,
  Ioss::Property::BasicType type, T (Ioss::Property::*getter)() const)
{
  if (!pm.exists(name) || !pm.get(name).is_valid() || pm.get(name).get_type() != type ||
    (pm.get(name).*getter)() != value)
  {
    pm.add(Ioss::Property(name, value));
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkIOSSReader::AddProperty(const char* name, int value)
{
  auto& internals = (*this->Internals);
  auto& pm = internals.DatabaseProperties;
  if (updateProperty<int64_t>(pm, name, value, Ioss::Property::INTEGER, &Ioss::Property::get_int))
  {
    internals.Reset();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::AddProperty(const char* name, double value)
{
  auto& internals = (*this->Internals);
  auto& pm = internals.DatabaseProperties;
  if (updateProperty<double>(pm, name, value, Ioss::Property::REAL, &Ioss::Property::get_real))
  {
    internals.Reset();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::AddProperty(const char* name, void* value)
{
  auto& internals = (*this->Internals);
  auto& pm = internals.DatabaseProperties;
  if (updateProperty<void*>(pm, name, value, Ioss::Property::POINTER, &Ioss::Property::get_pointer))
  {
    internals.Reset();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::AddProperty(const char* name, const char* value)
{
  auto& internals = (*this->Internals);
  auto& pm = internals.DatabaseProperties;
  if (updateProperty<std::string>(
        pm, name, value, Ioss::Property::STRING, &Ioss::Property::get_string))
  {
    internals.Reset();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::RemoveProperty(const char* name)
{
  auto& internals = (*this->Internals);
  auto& pm = internals.DatabaseProperties;
  if (pm.exists(name))
  {
    pm.erase(name);
    internals.Reset();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::ClearProperties()
{
  auto& internals = (*this->Internals);
  auto& pm = internals.DatabaseProperties;
  if (pm.count() > 0)
  {
    Ioss::NameList names;
    pm.describe(&names);
    for (const auto& name : names)
    {
      pm.erase(name);
    }
    internals.Reset();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkIOSSReader::GetAssembly()
{
  auto& internals = (*this->Internals);
  return internals.GetAssembly();
}

//----------------------------------------------------------------------------
bool vtkIOSSReader::AddSelector(const char* selector)
{
  auto& internals = (*this->Internals);
  if (selector != nullptr && internals.Selectors.insert(selector).second)
  {
    this->Modified();
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkIOSSReader::ClearSelectors()
{
  auto& internals = (*this->Internals);
  if (!internals.Selectors.empty())
  {
    internals.Selectors.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIOSSReader::SetSelector(const char* selector)
{
  this->ClearSelectors();
  this->AddSelector(selector);
}

//----------------------------------------------------------------------------
int vtkIOSSReader::GetNumberOfSelectors() const
{
  auto& internals = (*this->Internals);
  return static_cast<int>(internals.Selectors.size());
}

//----------------------------------------------------------------------------
const char* vtkIOSSReader::GetSelector(int index) const
{
  auto& internals = (*this->Internals);
  if (index >= 0 && index < this->GetNumberOfSelectors())
  {
    auto iter = std::next(internals.Selectors.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkIOSSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GenerateFileId: " << this->GenerateFileId << endl;
  os << indent << "ScanForRelatedFiles: " << this->ScanForRelatedFiles << endl;
  os << indent << "FileRange: " << this->FileRange[0] << ", " << this->FileRange[1] << endl;
  os << indent << "FileStride: " << this->FileStride << endl;
  os << indent << "ReadIds: " << this->ReadIds << endl;
  os << indent << "RemoveUnusedPoints: " << this->RemoveUnusedPoints << endl;
  os << indent << "ApplyDisplacements: " << this->ApplyDisplacements << endl;
  os << indent << "DisplacementMagnitude: " << this->Internals->GetDisplacementMagnitude() << endl;
  os << indent << "ReadGlobalFields: " << this->ReadGlobalFields << endl;
  os << indent << "ReadQAAndInformationRecords: " << this->ReadQAAndInformationRecords << endl;
  os << indent << "DatabaseTypeOverride: "
     << (this->DatabaseTypeOverride ? this->DatabaseTypeOverride : "(nullptr)") << endl;

  os << indent << "NodeBlockSelection: " << endl;
  this->GetNodeBlockSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeBlockSelection: " << endl;
  this->GetEdgeBlockSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "FaceBlockSelection: " << endl;
  this->GetFaceBlockSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ElementBlockSelection: " << endl;
  this->GetElementBlockSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "StructuredBlockSelection: " << endl;
  this->GetStructuredBlockSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "NodeSetSelection: " << endl;
  this->GetNodeSetSelection()->PrintSelf(os, indent.GetNextIndent());

  os << indent << "NodeBlockFieldSelection: " << endl;
  this->GetNodeBlockFieldSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeBlockFieldSelection: " << endl;
  this->GetEdgeBlockFieldSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "FaceBlockFieldSelection: " << endl;
  this->GetFaceBlockFieldSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ElementBlockFieldSelection: " << endl;
  this->GetElementBlockFieldSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "StructuredBlockFieldSelection: " << endl;
  this->GetStructuredBlockFieldSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "NodeSetFieldSelection: " << endl;
  this->GetNodeSetFieldSelection()->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
