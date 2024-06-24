// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIOSSReaderInternal.h"

#include "vtkIOSSFilesScanner.h"
#include "vtkIOSSReader.h"
#include "vtkIOSSReaderCommunication.h"
#include "vtkIOSSUtilities.h"

#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
#include "vtkExtractGrid.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
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
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ioss_TransformFactory.h)
// clang-format on

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

std::vector<int> vtkIOSSReaderInternal::GetFileIds(
  const std::string& dbasename, int myrank, int numRanks) const
{
  auto iter = this->DatabaseNames.find(dbasename);
  if ((iter == this->DatabaseNames.end()) || (myrank < 0) ||
    (iter->second.ProcessCount == 0 && myrank != 0) ||
    (iter->second.ProcessCount != 0 && myrank >= iter->second.ProcessCount))
  {
    return std::vector<int>();
  }

  // note, number of files may be less than the number of ranks the partitioned
  // file was written out on. that happens when user only chooses a smaller
  // subset.
  int nfiles = iter->second.ProcessCount > 0 ? static_cast<int>(iter->second.Ranks.size()) : 1;

  // this logic is same as diy::ContiguousAssigner::local_gids(..)
  // the goal is split the available set of files into number of ranks in
  // contiguous chunks.
  const int div = nfiles / numRanks;
  const int mod = nfiles % numRanks;

  int from, to;
  if (myrank < mod)
  {
    from = myrank * (div + 1);
  }
  else
  {
    from = mod * (div + 1) + (myrank - mod) * div;
  }

  if (myrank + 1 < mod)
  {
    to = (myrank + 1) * (div + 1);
  }
  else
  {
    to = mod * (div + 1) + (myrank + 1 - mod) * div;
  }

  std::vector<int> fileids;
  for (int fileid = from; fileid < to; ++fileid)
  {
    fileids.push_back(fileid);
  }
  return fileids;
}

bool vtkIOSSReaderInternal::UpdateDatabaseNames(vtkIOSSReader* self)
{
  if (this->DatabaseNamesMTime > this->FileNamesMTime)
  {
    // we may still need filtering if MTime changed, so check that.
    if (self->GetMTime() > this->DatabaseNamesMTime)
    {
      auto subset = this->GenerateSubset(this->UnfilteredDatabaseNames, self);
      if (this->DatabaseNames != subset)
      {
        this->DatabaseNames = std::move(subset);
        this->DatabaseNamesMTime.Modified();
      }
    }
    return (!this->DatabaseNames.empty());
  }

  // Clear cache since we're updating the databases, old caches no longer makes
  // sense.
  this->Cache.Clear();

  // Clear old Ioss::Region's since they may not be correct anymore.
  this->ReleaseRegions();

  auto filenames = this->FileNames;
  auto controller = self->GetController();
  const int myrank = controller ? controller->GetLocalProcessId() : 0;
  const int ranks = controller ? controller->GetNumberOfProcesses() : 1;

  if (myrank == 0)
  {
    if (filenames.size() == 1 && vtkIOSSFilesScanner::IsMetaFile(*filenames.begin()))
    {
      filenames = vtkIOSSFilesScanner::GetFilesFromMetaFile(*filenames.begin());
      // To address issue paraview/paraview/-/issues/22124 we need to scan for related files
      // when reading an ex-timeseries file.
      if (self->GetScanForRelatedFiles())
      {
        filenames = vtkIOSSFilesScanner::GetRelatedFiles(filenames);
      }
    }
    else if (filenames.size() == 1 && *filenames.begin() == "catalyst.bin" && ranks > 1)
    {
      // "catalyst.bin" is a special filename to indicate that we should read from catalyst.
      // To make sure that each node creates a database handle to try to read something
      // from catalyst, we need to create a "filename" for each rank.
      filenames.clear();
      for (int i = 0; i < ranks; ++i)
      {
        filenames.insert("catalyst.bin." + std::to_string(ranks) + "." + std::to_string(i));
      }
    }
    else if (self->GetScanForRelatedFiles())
    {
      filenames = vtkIOSSFilesScanner::GetRelatedFiles(filenames);
    }
  }

  if (!::Broadcast(controller, filenames, 0))
  {
    return false;
  }

  if (filenames.empty())
  {
    vtkErrorWithObjectMacro(self, "No filename specified.");
    return false;
  }

  // process filename to determine the base-name and the `processor_count`, and
  // `my_processor` values.
  // clang-format off
  vtksys::RegularExpression regEx(R"(^(.*)\.([0-9]+)\.([0-9]+)$)");
  // clang-format on

  DatabaseNamesType databases;
  for (auto& fname : filenames)
  {
    if (regEx.find(fname))
    {
      auto dbasename = regEx.match(1);
      auto processor_count = std::atoi(regEx.match(2).c_str());
      auto my_processor = std::atoi(regEx.match(3).c_str());

      auto& info = databases[dbasename];
      if (info.ProcessCount == 0 || info.ProcessCount == processor_count)
      {
        info.ProcessCount = processor_count;
        info.Ranks.insert(my_processor);
      }
      else
      {
        auto fname_name = vtksys::SystemTools::GetFilenameName(fname);
        vtkErrorWithObjectMacro(self,
          "Filenames specified use inconsistent naming schemes. '"
            << fname_name << "' has incorrect processor-count (" << processor_count << "), '"
            << info.ProcessCount << "' was expected.");
        return false;
      }
    }
    else
    {
      databases.insert(std::make_pair(fname, DatabasePartitionInfo()));
    }
  }

  this->UnfilteredDatabaseNames.swap(databases);

  if (vtkLogger::GetCurrentVerbosityCutoff() >= vtkLogger::VERBOSITY_TRACE)
  {
    // let's log.
    vtkLogF(
      TRACE, "Found Ioss databases (%d)", static_cast<int>(this->UnfilteredDatabaseNames.size()));
    std::ostringstream str;
    for (const auto& pair : this->UnfilteredDatabaseNames)
    {
      if (pair.second.ProcessCount > 0)
      {
        // reset ostringstream.
        str.str("");
        str.clear();
        for (auto& rank : pair.second.Ranks)
        {
          str << " " << rank;
        }
        vtkLogF(TRACE, "'%s' [processor_count = %d][ranks = %s]",
          vtksys::SystemTools::GetFilenameName(pair.first).c_str(), pair.second.ProcessCount,
          str.str().c_str());
      }
      else
      {
        vtkLogF(TRACE, "'%s'", vtksys::SystemTools::GetFilenameName(pair.first).c_str());
      }
    }
  }

  this->DatabaseNames = this->GenerateSubset(this->UnfilteredDatabaseNames, self);
  this->DatabaseNamesMTime.Modified();
  return !this->DatabaseNames.empty();
}

vtkIOSSReaderInternal::DatabaseNamesType vtkIOSSReaderInternal::GenerateSubset(
  const vtkIOSSReaderInternal::DatabaseNamesType& databases, vtkIOSSReader* self)
{
  int fileRange[2];
  self->GetFileRange(fileRange);
  const int stride = self->GetFileStride();
  if (fileRange[0] >= fileRange[1] || stride < 1 || databases.empty())
  {
    return databases;
  }

  // We need to filter filenames.
  DatabaseNamesType result = databases;
  for (auto& pair : result)
  {
    auto& dbaseInfo = pair.second;
    if (dbaseInfo.ProcessCount <= 0)
    {
      continue;
    }

    // remove all "ranks" not fitting the requested range.
    for (auto iter = dbaseInfo.Ranks.begin(); iter != dbaseInfo.Ranks.end();)
    {
      const int rank = (*iter);
      if ((rank < fileRange[0] || rank >= fileRange[1] || (rank - fileRange[0]) % stride != 0))
      {
        iter = dbaseInfo.Ranks.erase(iter);
      }
      else
      {
        ++iter;
      }
    }
  }

  // remove any databases which have no ranks to be read in.
  for (auto iter = result.begin(); iter != result.end();)
  {
    auto& dbaseInfo = iter->second;
    if (dbaseInfo.ProcessCount > 0 && dbaseInfo.Ranks.empty())
    {
      iter = result.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
  return result;
}

bool vtkIOSSReaderInternal::UpdateTimeInformation(vtkIOSSReader* self)
{
  if (this->TimestepValuesMTime > this->DatabaseNamesMTime)
  {
    return true;
  }

  vtkLogScopeF(TRACE, "UpdateTimeInformation");
  auto controller = self->GetController();
  const auto rank = controller ? controller->GetLocalProcessId() : 0;
  const auto numRanks = controller ? controller->GetNumberOfProcesses() : 1;

  int success = 1;
  if (rank == 0)
  {
    // time values for each database.
    auto& dbase_times = this->DatabaseTimes;
    dbase_times.clear();

    // read all databases to collect timestep information.
    for (const auto& pair : this->DatabaseNames)
    {
      assert(pair.second.ProcessCount == 0 || !pair.second.Ranks.empty());
      const auto fileids = this->GetFileIds(pair.first, rank, numRanks);
      if (fileids.empty())
      {
        continue;
      }
      try
      {
        auto region = this->GetRegion(pair.first, fileids.front());
        dbase_times[pair.first] = vtkIOSSUtilities::GetTime(region);
      }
      catch (std::runtime_error& e)
      {
        vtkErrorWithObjectMacro(self, "Error in UpdateTimeInformation: \n" << e.what());
        success = 0;
        dbase_times.clear();
        break;
      }
    }
  }

  if (numRanks > 1)
  {
    auto& dbase_times = this->DatabaseTimes;
    int msg[2] = { success, static_cast<int>(dbase_times.size()) };
    controller->Broadcast(msg, 2, 0);
    success = msg[0];
    if (success && msg[1] > 0)
    {
      success = ::Broadcast(controller, dbase_times, 0);
    }
    else
    {
      dbase_times.clear();
    }

    // this is a good place for us to sync up format too.
    int iFormat = static_cast<int>(this->Format);
    controller->Broadcast(&iFormat, 1, 0);
    this->Format = static_cast<vtkIOSSUtilities::DatabaseFormatType>(iFormat);
  }

  // Fillup TimestepValues for ease of use later.
  std::set<double> times_set;
  for (auto& pair : this->DatabaseTimes)
  {
    std::transform(pair.second.begin(), pair.second.end(),
      std::inserter(times_set, times_set.end()),
      [](const std::pair<int, double>& otherPair) { return otherPair.second; });
  }
  this->TimestepValues.resize(times_set.size());
  std::copy(times_set.begin(), times_set.end(), this->TimestepValues.begin());
  this->TimestepValuesMTime.Modified();
  return (success == 1);
}

bool vtkIOSSReaderInternal::NeedToUpdateEntityAndFieldSelections(
  vtkIOSSReader* self, const std::vector<DatabaseHandle>& dbaseHandles)
{
  std::set<std::string> databaseNames;
  for (const auto& handle : dbaseHandles)
  {
    databaseNames.insert(handle.first);
  }

  auto controller = self->GetController();
  const auto rank = controller ? controller->GetLocalProcessId() : 0;
  const auto numRanks = controller ? controller->GetNumberOfProcesses() : 1;

  // This has to be done all all ranks since not all files in a database have
  // all the blocks consequently need not have all the fields.
  std::array<std::set<vtkIOSSUtilities::EntityNameType>, vtkIOSSReader::NUMBER_OF_ENTITY_TYPES>
    entity_names;
  std::array<std::set<std::string>, vtkIOSSReader::NUMBER_OF_ENTITY_TYPES> field_names;
  std::set<vtkIOSSUtilities::EntityNameType> bc_names;

  // format should have been set (and synced) across all ranks by now.
  assert(this->Format != vtkIOSSUtilities::UNKNOWN);

  for (const auto& databaseName : databaseNames)
  {
    auto fileids = this->GetFileIds(databaseName, rank, numRanks);

    for (const auto& fileid : fileids)
    {
      if (auto region = this->GetRegion(databaseName, fileid))
      {
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_node_blocks(),
          entity_names[vtkIOSSReader::NODEBLOCK], field_names[vtkIOSSReader::NODEBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_edge_blocks(),
          entity_names[vtkIOSSReader::EDGEBLOCK], field_names[vtkIOSSReader::EDGEBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_face_blocks(),
          entity_names[vtkIOSSReader::FACEBLOCK], field_names[vtkIOSSReader::FACEBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_element_blocks(),
          entity_names[vtkIOSSReader::ELEMENTBLOCK], field_names[vtkIOSSReader::ELEMENTBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_structured_blocks(),
          entity_names[vtkIOSSReader::STRUCTUREDBLOCK],
          field_names[vtkIOSSReader::STRUCTUREDBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_nodesets(),
          entity_names[vtkIOSSReader::NODESET], field_names[vtkIOSSReader::NODESET]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_edgesets(),
          entity_names[vtkIOSSReader::EDGESET], field_names[vtkIOSSReader::EDGESET]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_facesets(),
          entity_names[vtkIOSSReader::FACESET], field_names[vtkIOSSReader::FACESET]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_elementsets(),
          entity_names[vtkIOSSReader::ELEMENTSET], field_names[vtkIOSSReader::ELEMENTSET]);

        // note: for CGNS, the sidesets contain family names for BC. They need to
        // be handled differently from exodus side sets.
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_sidesets(),
          entity_names[vtkIOSSReader::SIDESET], field_names[vtkIOSSReader::SIDESET]);

        // note: for CGNS, the structuredblock elements have nested BC patches. These patches
        // are named as well. Let's collect those names too.
        for (const auto& sb : region->get_structured_blocks())
        {
          const int64_t id = sb->property_exists("id") ? sb->get_property("id").get_int() : 0;
          for (auto& bc : sb->m_boundaryConditions)
          {
            if (!bc.m_bcName.empty())
            {
              bc_names.emplace(static_cast<vtkTypeUInt64>(id), bc.m_bcName);
            }
          }
        }

        // another CGNS idiosyncrasy, we need to read node fields from
        // node_blocks nested under the structured_blocks.
        for (auto& sb : region->get_structured_blocks())
        {
          std::set<vtkIOSSUtilities::EntityNameType> unused;
          vtkIOSSUtilities::GetEntityAndFieldNames(region,
            Ioss::NodeBlockContainer({ &sb->get_node_block() }), unused,
            field_names[vtkIOSSReader::NODEBLOCK]);
        }
      }
      // necessary to avoid errors from IO libraries, e.g. CGNS, about
      // too many files open.
      this->ReleaseHandles();
    }
  }

  bool subsetOrEqual = true;
  for (int i = 0; i < vtkIOSSReader::NUMBER_OF_ENTITY_TYPES; ++i)
  {
    subsetOrEqual &= std::includes(this->EntityNames[i].begin(), this->EntityNames[i].end(),
      entity_names[i].begin(), entity_names[i].end());
  }

  return !subsetOrEqual;
}

bool vtkIOSSReaderInternal::UpdateEntityAndFieldSelections(vtkIOSSReader* self)
{
  if (this->SelectionsMTime > this->DatabaseNamesMTime)
  {
    return true;
  }

  vtkLogScopeF(TRACE, "UpdateEntityAndFieldSelections");
  auto controller = self->GetController();
  const auto rank = controller ? controller->GetLocalProcessId() : 0;
  const auto numRanks = controller ? controller->GetNumberOfProcesses() : 1;

  // This has to be done all all ranks since not all files in a database have
  // all the blocks consequently need not have all the fields.
  std::array<std::set<vtkIOSSUtilities::EntityNameType>, vtkIOSSReader::NUMBER_OF_ENTITY_TYPES>
    entity_names;
  std::array<std::set<std::string>, vtkIOSSReader::NUMBER_OF_ENTITY_TYPES> field_names;
  std::set<vtkIOSSUtilities::EntityNameType> bc_names;

  // format should have been set (and synced) across all ranks by now.
  assert(this->Format != vtkIOSSUtilities::UNKNOWN);

  for (const auto& pair : this->DatabaseNames)
  {
    // We need to read all files to get entity_names and field_names with certainty, because
    // one file might have block_1 and another file might have block_1, block_2. We need to know
    // about all blocks in all files. If we read only the first file, we will not know about
    // block_2.
    auto fileids = this->GetFileIds(pair.first, rank, numRanks);
    // Nonetheless, if you know that all files have the same structure, you can skip reading
    // all files and just read the first file.
    if (!self->GetReadAllFilesToDetermineStructure())
    {
      fileids.resize(rank == 0 ? 1 : 0);
    }

    for (const auto& fileid : fileids)
    {
      if (auto region = this->GetRegion(pair.first, fileid))
      {
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_node_blocks(),
          entity_names[vtkIOSSReader::NODEBLOCK], field_names[vtkIOSSReader::NODEBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_edge_blocks(),
          entity_names[vtkIOSSReader::EDGEBLOCK], field_names[vtkIOSSReader::EDGEBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_face_blocks(),
          entity_names[vtkIOSSReader::FACEBLOCK], field_names[vtkIOSSReader::FACEBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_element_blocks(),
          entity_names[vtkIOSSReader::ELEMENTBLOCK], field_names[vtkIOSSReader::ELEMENTBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_structured_blocks(),
          entity_names[vtkIOSSReader::STRUCTUREDBLOCK],
          field_names[vtkIOSSReader::STRUCTUREDBLOCK]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_nodesets(),
          entity_names[vtkIOSSReader::NODESET], field_names[vtkIOSSReader::NODESET]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_edgesets(),
          entity_names[vtkIOSSReader::EDGESET], field_names[vtkIOSSReader::EDGESET]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_facesets(),
          entity_names[vtkIOSSReader::FACESET], field_names[vtkIOSSReader::FACESET]);
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_elementsets(),
          entity_names[vtkIOSSReader::ELEMENTSET], field_names[vtkIOSSReader::ELEMENTSET]);

        // note: for CGNS, the sidesets contain family names for BC. They need to
        // be handled differently from exodus side sets.
        vtkIOSSUtilities::GetEntityAndFieldNames(region, region->get_sidesets(),
          entity_names[vtkIOSSReader::SIDESET], field_names[vtkIOSSReader::SIDESET]);

        // note: for CGNS, the structuredblock elements have nested BC patches. These patches
        // are named as well. Let's collect those names too.
        for (const auto& sb : region->get_structured_blocks())
        {
          const int64_t id = sb->property_exists("id") ? sb->get_property("id").get_int() : 0;
          for (auto& bc : sb->m_boundaryConditions)
          {
            if (!bc.m_bcName.empty())
            {
              bc_names.emplace(static_cast<vtkTypeUInt64>(id), bc.m_bcName);
            }
          }
        }

        // another CGNS idiosyncrasy, we need to read node fields from
        // node_blocks nested under the structured_blocks.
        for (auto& sb : region->get_structured_blocks())
        {
          std::set<vtkIOSSUtilities::EntityNameType> unused;
          vtkIOSSUtilities::GetEntityAndFieldNames(region,
            Ioss::NodeBlockContainer({ &sb->get_node_block() }), unused,
            field_names[vtkIOSSReader::NODEBLOCK]);
        }
      }
      // necessary to avoid errors from IO libraries, e.g. CGNS, about
      // too many files open.
      this->ReleaseHandles();
    }
  }

  if (numRanks > 1)
  {
    // sync selections across all ranks.
    ::Synchronize(controller, entity_names, entity_names);
    ::Synchronize(controller, field_names, field_names);

    // Sync format. Needed since all ranks may not have read entity information
    // thus may not have format setup correctly.
    int iFormat = static_cast<int>(this->Format);
    controller->Broadcast(&iFormat, 1, 0);
    this->Format = static_cast<vtkIOSSUtilities::DatabaseFormatType>(iFormat);
  }

  // update known block/set names.
  this->EntityNames = entity_names;
  for (int cc = EntityType::ENTITY_START; cc < EntityType::ENTITY_END; ++cc)
  {
    auto entitySelection = self->GetEntitySelection(cc);
    auto& entityIdMap = self->GetEntityIdMap(cc);
    for (auto& name : entity_names[cc])
    {
      entitySelection->AddArray(name.second.c_str(), vtkIOSSReader::GetEntityTypeIsBlock(cc));
      if (name.first != 0)
      {
        entityIdMap[name.second] = name.first;
      }
    }

    auto fieldSelection = self->GetFieldSelection(cc);
    for (auto& name : field_names[cc])
    {
      fieldSelection->AddArray(name.c_str(), vtkIOSSReader::GetEntityTypeIsBlock(cc));
    }
  }

  // Populate DatasetIndexMap.
  unsigned int pdsIdx = 0;
  for (int etype = vtkIOSSReader::NODEBLOCK + 1; etype < vtkIOSSReader::ENTITY_END; ++etype)
  {
    // for sidesets when reading CGNS, use the patch names.
    const auto& namesSet = this->EntityNames[etype];

    // EntityNames are sorted by their exodus "id".
    for (const auto& ename : namesSet)
    {
      auto ioss_etype =
        vtkIOSSUtilities::GetIOSSEntityType(static_cast<vtkIOSSReader::EntityType>(etype));
      this->DatasetIndexMap[std::make_pair(ioss_etype, ename.second)] = pdsIdx++;
    }
  }

  this->SelectionsMTime.Modified();
  return true;
}

bool vtkIOSSReaderInternal::BuildAssembly(
  Ioss::Region* region, vtkDataAssembly* assembly, int root, bool add_leaves)
{
  if (region == nullptr || assembly == nullptr)
  {
    return false;
  }

  // assemblies in Ioss are simply stored as a vector. we need to build graph
  // from that vector of assemblies.
  std::set<const Ioss::GroupingEntity*> root_assemblies;
  for (auto& ioss_assembly : region->get_assemblies())
  {
    assert(ioss_assembly != nullptr);
    root_assemblies.insert(ioss_assembly);

    for (auto child : ioss_assembly->get_members())
    {
      // a child cannot be a root, so remove it.
      root_assemblies.erase(child);
    }
  }

  if (root_assemblies.empty())
  {
    return false;
  }

  std::function<void(const Ioss::Assembly*, int)> processAssembly;
  processAssembly = [&assembly, &processAssembly, &add_leaves, this](
                      const Ioss::Assembly* ioss_assembly, int parent) {
    auto node = assembly->AddNode(
      vtkDataAssembly::MakeValidNodeName(ioss_assembly->name().c_str()).c_str(), parent);
    assembly->SetAttribute(node, "label", ioss_assembly->name().c_str());
    if (ioss_assembly->get_member_type() == Ioss::ASSEMBLY)
    {
      for (auto& child : ioss_assembly->get_members())
      {
        processAssembly(dynamic_cast<const Ioss::Assembly*>(child), node);
      }
    }
    else
    {
      for (auto& child : ioss_assembly->get_members())
      {
        int dsnode = node;
        if (add_leaves)
        {
          dsnode = assembly->AddNode(
            vtkDataAssembly::MakeValidNodeName(child->name().c_str()).c_str(), node);
          assembly->SetAttribute(dsnode, "label", child->name().c_str());
        }
        assembly->AddDataSetIndex(dsnode, this->GetDataSetIndexForEntity(child));
      }
    }
  };

  // to preserve order of assemblies, we iterate over region assemblies.
  for (auto& ioss_assembly : region->get_assemblies())
  {
    if (root_assemblies.find(ioss_assembly) != root_assemblies.end())
    {
      processAssembly(ioss_assembly, root);
    }
  }

  return true;
}

bool vtkIOSSReaderInternal::UpdateAssembly(vtkIOSSReader* self, int* tag)
{
  if (this->AssemblyMTime > this->DatabaseNamesMTime)
  {
    return true;
  }

  vtkLogScopeF(TRACE, "UpdateAssembly");
  this->AssemblyMTime.Modified();

  auto controller = self->GetController();
  const auto rank = controller ? controller->GetLocalProcessId() : 0;
  const auto numRanks = controller ? controller->GetNumberOfProcesses() : 1;

  if (rank == 0)
  {
    // it's unclear how assemblies in Ioss are distributed across partitioned
    // files. so we assume they are duplicated on all only read it from root node.
    const auto handle = this->GetDatabaseHandles(rank, numRanks, 0).front();
    auto region = this->GetRegion(handle);

    this->Assembly = vtk::TakeSmartPointer(vtkDataAssembly::New());
    this->Assembly->SetRootNodeName("Assemblies");
    const auto status = this->BuildAssembly(region, this->Assembly, 0, /*add_leaves=*/true);
    *tag = status ? static_cast<int>(this->AssemblyMTime.GetMTime()) : 0;
    if (numRanks > 1)
    {
      vtkMultiProcessStream stream;

      stream << (*tag);
      stream << this->Assembly->SerializeToXML(vtkIndent());
      controller->Broadcast(stream, 0);
    }
    if (!status)
    {
      this->Assembly = nullptr;
    }
  }
  else
  {
    vtkMultiProcessStream stream;
    controller->Broadcast(stream, 0);

    std::string data;
    stream >> (*tag) >> data;

    if ((*tag) != 0)
    {
      this->Assembly = vtk::TakeSmartPointer(vtkDataAssembly::New());
      this->Assembly->InitializeFromXML(data.c_str());
    }
    else
    {
      this->Assembly = nullptr;
    }
  }

  return true;
}

vtkDataAssembly* vtkIOSSReaderInternal::GetAssembly() const
{
  return this->Assembly;
}

bool vtkIOSSReaderInternal::GenerateOutput(
  vtkPartitionedDataSetCollection* output, vtkIOSSReader* self)
{
  // we skip NODEBLOCK since we never put out NODEBLOCK in the output by itself.
  vtkNew<vtkDataAssembly> assembly;
  assembly->SetRootNodeName("IOSS");
  output->SetDataAssembly(assembly);

  for (int etype = vtkIOSSReader::NODEBLOCK + 1; etype < vtkIOSSReader::ENTITY_END; ++etype)
  {
    // for sidesets when reading CGNS, use the patch names.
    const auto& namesSet = this->EntityNames[etype];

    if (namesSet.empty())
    {
      // skip 0-count entity types; keeps output assembly simpler to read.
      continue;
    }

    const int entity_node =
      assembly->AddNode(vtkIOSSReader::GetDataAssemblyNodeNameForEntityType(etype));

    // check if we are gonna merge all of the blocks/sets of an entity type into a single one
    const bool mergeEntityBlocks =
      this->GetFormat() == vtkIOSSUtilities::DatabaseFormatType::EXODUS &&
      self->GetMergeExodusEntityBlocks();
    if (!mergeEntityBlocks)
    {
      // EntityNames are sorted by their exodus "id".
      for (const auto& ename : namesSet)
      {
        const auto pdsIdx = output->GetNumberOfPartitionedDataSets();
        vtkNew<vtkPartitionedDataSet> parts;
        output->SetPartitionedDataSet(pdsIdx, parts);
        output->GetMetaData(pdsIdx)->Set(vtkCompositeDataSet::NAME(), ename.second.c_str());
        // save for vtkIOSSReader use.
        output->GetMetaData(pdsIdx)->Set(vtkIOSSReader::ENTITY_TYPE(), etype);
        // save for vtkIOSSWriter use.
        output->GetMetaData(pdsIdx)->Set(vtkIOSSReader::ENTITY_ID(), ename.first);
        auto node = assembly->AddNode(
          vtkDataAssembly::MakeValidNodeName(ename.second.c_str()).c_str(), entity_node);
        assembly->SetAttribute(node, "label", ename.second.c_str());
        assembly->AddDataSetIndex(node, pdsIdx);
      }
    }
    else
    {
      const auto mergedEntityName = vtkIOSSReader::GetMergedEntityNameForEntityType(etype);
      // merge all entity blocks into a single partitioned dataset.
      const auto pdsIdx = output->GetNumberOfPartitionedDataSets();
      vtkNew<vtkPartitionedDataSet> parts;
      output->SetPartitionedDataSet(pdsIdx, parts);
      output->GetMetaData(pdsIdx)->Set(vtkCompositeDataSet::NAME(), mergedEntityName);
      // save for vtkIOSSReader use.
      output->GetMetaData(pdsIdx)->Set(vtkIOSSReader::ENTITY_TYPE(), etype);
      // save for vtkIOSSWriter use.
      output->GetMetaData(pdsIdx)->Set(vtkIOSSReader::ENTITY_ID(), etype);
      auto node = assembly->AddNode(
        vtkDataAssembly::MakeValidNodeName(mergedEntityName).c_str(), entity_node);
      assembly->SetAttribute(node, "label", mergedEntityName);
      assembly->AddDataSetIndex(node, pdsIdx);
    }
  }

  return true;
}

bool vtkIOSSReaderInternal::ReadAssemblies(
  vtkPartitionedDataSetCollection* output, const DatabaseHandle& handle)
{
  /**
   * It's not entirely clear how IOSS-assemblies should be made available in the data
   * model. For now, we'll add them under the default vtkDataAssembly associated
   * with the output
   **/
  auto assembly = output->GetDataAssembly();
  assert(assembly != nullptr);

  auto region = this->GetRegion(handle);
  if (!region)
  {
    return false;
  }

  const auto node_assemblies = assembly->AddNode("assemblies");
  if (!this->BuildAssembly(region, assembly, node_assemblies, /*add_leaves=*/true))
  {
    assembly->RemoveNode(node_assemblies);
  }

  return true;
}

Ioss::Region* vtkIOSSReaderInternal::GetRegion(const std::string& dbasename, int fileid)
{
  assert(fileid >= 0);
  auto iter = this->DatabaseNames.find(dbasename);
  assert(iter != this->DatabaseNames.end());

  const bool has_multiple_files = (iter->second.ProcessCount > 0);
  assert(has_multiple_files == false || (fileid < static_cast<int>(iter->second.Ranks.size())));

  auto processor = has_multiple_files ? *std::next(iter->second.Ranks.begin(), fileid) : 0;

  auto riter = this->RegionMap.find(std::make_pair(dbasename, processor));
  if (riter == this->RegionMap.end())
  {
    Ioss::PropertyManager properties;
    if (has_multiple_files)
    {
      properties.add(Ioss::Property("my_processor", processor));
      properties.add(Ioss::Property("processor_count", iter->second.ProcessCount));
    }

    // tell the reader to read all blocks, even if empty. necessary to avoid
    // having to read all files to gather metadata, if possible
    // see paraview/paraview#20873.
    properties.add(Ioss::Property("RETAIN_EMPTY_BLOCKS", "on"));

    // strip trailing underscores in CGNS files to turn separate fields into
    // vectors with components.
    // see https://github.com/sandialabs/seacas/issues/265
    properties.add(Ioss::Property("FIELD_STRIP_TRAILING_UNDERSCORE", "on"));

    // Do not convert variable names to lower case. The default is on.
    // For ex: this resolves a misunderstanding b/w T (temperature) vs t (time)
    properties.add(Ioss::Property("LOWER_CASE_VARIABLE_NAMES", "off"));

    // Only read timestep information from 0th file.
    properties.add(Ioss::Property("EXODUS_CALL_GET_ALL_TIMES", processor == 0 ? "on" : "off"));

    // Split side sets into side-blocks by the element block of the originating side.
    // This allows rendering sides with partial scalars inherited from the element block.
    properties.add(Ioss::Property("SURFACE_SPLIT_TYPE", "BLOCK"));

    // Fillup with user-specified properties.
    Ioss::NameList names;
    this->DatabaseProperties.describe(&names);

    for (const auto& name : names)
    {
      properties.add(this->DatabaseProperties.get(name));
    }

    // If MPI is enabled in the build, Ioss can call MPI routines. We need to
    // make sure that MPI is initialized before calling
    // Ioss::IOFactory::create.
    vtkIOSSUtilities::InitializeEnvironmentForIOSS();
    std::string dtype;
    switch (vtkIOSSUtilities::DetectType(dbasename))
    {
      case vtkIOSSUtilities::DatabaseFormatType::CGNS:
        dtype = "cgns";
        break;
      case vtkIOSSUtilities::DatabaseFormatType::CATALYST:
        dtype = "catalyst";
        break;
      case vtkIOSSUtilities::DatabaseFormatType::EXODUS:
      default:
        dtype = "exodusII";
        break;
    }

    if (vtkLogger::GetCurrentVerbosityCutoff() >= vtkLogger::VERBOSITY_TRACE)
    {
      vtkLogScopeF(TRACE, "Set IOSS database properties");
      for (const auto& name : properties.describe())
      {
        switch (properties.get(name).get_type())
        {
          case Ioss::Property::BasicType::POINTER:
            vtkLog(TRACE, << name << " : " << properties.get(name).get_pointer());
            break;
          case Ioss::Property::BasicType::INTEGER:
            vtkLog(TRACE, << name << " : " << std::to_string(properties.get(name).get_int()));
            break;
          case Ioss::Property::BasicType::INVALID:
            vtkLog(TRACE, << name << " : "
                          << "invalid type");
            break;
          case Ioss::Property::BasicType::REAL:
            vtkLog(TRACE, << name << " : " << std::to_string(properties.get(name).get_real()));
            break;
          case Ioss::Property::BasicType::STRING:
            vtkLog(TRACE, << name << " : " << properties.get(name).get_string());
            break;
          default:
            break;
        }
      }
    }

#ifdef SEACAS_HAVE_MPI
    // As of now netcdf mpi support is not working for IOSSReader
    // because mpi calls are called inside the reader instead of the ioss library
    // so we are using comm_null(), instead of comm_world().
    // In the future, when comm_world() is used and SEACAS_HAVE_MPI is on
    // my_processor and processor_count properties should be removed for exodus.
    // For more info. see Ioex::DatabaseIO::DatabaseIO in the ioss library.
    auto parallelUtilsComm = Ioss::ParallelUtils::comm_null();
#else
    auto parallelUtilsComm = Ioss::ParallelUtils::comm_world();
#endif
    auto dbase = std::unique_ptr<Ioss::DatabaseIO>(Ioss::IOFactory::create(
      this->IOSSReader->DatabaseTypeOverride ? std::string(this->IOSSReader->DatabaseTypeOverride)
                                             : dtype,
      dbasename, Ioss::READ_RESTART, parallelUtilsComm, properties));
    if (dbase == nullptr || !dbase->ok(/*write_message=*/true))
    {
      throw std::runtime_error(
        "Failed to open database " + this->GetRawFileName(DatabaseHandle{ dbasename, fileid }));
    }
    dbase->set_surface_split_type(Ioss::SPLIT_BY_ELEMENT_BLOCK);

    // note: `Ioss::Region` constructor may throw exception.
    auto region = std::make_shared<Ioss::Region>(dbase.get());

    // release the dbase ptr since region (if created successfully) takes over
    // the ownership and calls delete on it when done.
    (void)dbase.release();

    riter =
      this->RegionMap.insert(std::make_pair(std::make_pair(dbasename, processor), region)).first;

    if (this->Format != vtkIOSSUtilities::DatabaseFormatType::UNKNOWN &&
      this->Format != vtkIOSSUtilities::GetFormat(region.get()))
    {
      throw std::runtime_error("Format mismatch! This is unexpected and indicate an error "
                               "in the reader implementation.");
    }
    this->Format = vtkIOSSUtilities::GetFormat(region.get());
  }
  return riter->second.get();
}

std::vector<DatabaseHandle> vtkIOSSReaderInternal::GetDatabaseHandles(
  int piece, int npieces, int timestep) const
{
  std::string dbasename;
  if (timestep >= 0 && timestep < static_cast<int>(this->TimestepValues.size()))
  {
    const double time = this->TimestepValues[timestep];

    // find the right database in a set of restarts;
    for (const auto& pair : this->DatabaseTimes)
    {
      const auto& vector = pair.second;
      auto iter = std::find_if(vector.begin(), vector.end(),
        [&time](const std::pair<int, double>& otherPair) { return otherPair.second == time; });
      if (iter != vector.end())
      {
        // if multiple databases provide the same timestep, we opt to choose
        // the one with a newer end timestep. this follows from the fact that
        // often a restart may be started after "rewinding" a bit to overcome
        // some bad timesteps.
        if (dbasename.empty() || (*this->DatabaseTimes.at(dbasename).rbegin() < *vector.rbegin()))
        {
          dbasename = pair.first;
        }
      }
    }
  }
  else if (timestep <= 0 && this->TimestepValues.empty())
  {
    dbasename = this->DatabaseNames.begin()->first;
  }
  else
  {
    vtkLogF(ERROR, "time stuff is busted!");
    return std::vector<DatabaseHandle>();
  }

  assert(!dbasename.empty());
  const auto fileids = this->GetFileIds(dbasename, piece, npieces);
  std::vector<DatabaseHandle> handles(fileids.size());
  std::transform(fileids.begin(), fileids.end(), handles.begin(),
    [&dbasename](int fileid) { return DatabaseHandle(dbasename, fileid); });
  return handles;
}

std::vector<vtkSmartPointer<vtkDataSet>> vtkIOSSReaderInternal::GetDataSets(
  const std::string& blockname, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSReader* self)
{
  // TODO: ideally, this method shouldn't depend on format but entity type.
  switch (this->Format)
  {
    case vtkIOSSUtilities::DatabaseFormatType::CATALYST:
      switch (vtk_entity_type)
      {
        case EntityType::STRUCTUREDBLOCK:
        case EntityType::SIDESET:
          return this->GetCGNSDataSets(blockname, vtk_entity_type, handle, timestep, self);

        default:
          return this->GetExodusDataSets(blockname, vtk_entity_type, handle, timestep, self);
      }

    case vtkIOSSUtilities::DatabaseFormatType::CGNS:
      switch (vtk_entity_type)
      {
        case EntityType::STRUCTUREDBLOCK:
        case EntityType::SIDESET:
          return this->GetCGNSDataSets(blockname, vtk_entity_type, handle, timestep, self);

        default:
          // not supported for CGNS (AFAIK)
          return {};
      }

    case vtkIOSSUtilities::DatabaseFormatType::EXODUS:
      switch (vtk_entity_type)
      {
        case EntityType::STRUCTUREDBLOCK:
          return {};
        default:
          return this->GetExodusDataSets(blockname, vtk_entity_type, handle, timestep, self);
      }

    default:
      vtkLogF(
        ERROR, "Format not setup correctly or unknown format (%d)", static_cast<int>(this->Format));
      return {};
  }
}

bool vtkIOSSReaderInternal::GetEntityMesh(vtkUnstructuredGrid* entityGrid,
  const std::vector<std::string>& blockNames, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle)
{
  const auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle.first, handle.second);
  if (!region)
  {
    return false;
  }

  // find the first group entity that has a block with cells.
  Ioss::GroupingEntity* first_group_entity = nullptr;
  for (const auto& blockName : blockNames)
  {
    auto local_group_entity = region->get_entity(blockName, ioss_entity_type);
    // if the local group entity does not exist, go to the next one
    if (!local_group_entity)
    {
      continue;
    }
    // get the connectivity of the block of the entity
    const auto blockCellArrayAndType = this->GetTopology(blockName, vtk_entity_type, handle);
    if (!blockCellArrayAndType.empty())
    {
      first_group_entity = local_group_entity;
      break;
    }
  }
  // if there is no valid group entity based on the given blocks, then GetEntityMesh failed
  if (!first_group_entity)
  {
    return false;
  }

  // if we have a cached dataset for the merged entity, it will be saved in the cache
  // using the first group entity and __vtk_merged_mesh__ as the key.
  auto& cache = this->Cache;
  const static std::string cacheKey{ "__vtk_merged_mesh__" };
  if (auto cachedDataset = vtkDataSet::SafeDownCast(cache.Find(first_group_entity, cacheKey)))
  {
    entityGrid->CopyStructure(cachedDataset);
    return true;
  }

  // get the points of the entity
  auto points = this->GetGeometry("nodeblock_1", handle);
  if (!points)
  {
    return false;
  }
  // set the points of the entity
  entityGrid->SetPoints(points);

  std::vector<std::pair<int, vtkSmartPointer<vtkCellArray>>> cellArraysAndType;
  for (const auto& blockName : blockNames)
  {
    auto group_entity = region->get_entity(blockName, ioss_entity_type);
    if (!group_entity)
    {
      continue;
    }
    // get the connectivity of the block of the entity
    const auto blockCellArrayAndType = this->GetTopology(blockName, vtk_entity_type, handle);
    if (blockCellArrayAndType.empty())
    {
      continue;
    }
    cellArraysAndType.insert(
      cellArraysAndType.end(), blockCellArrayAndType.begin(), blockCellArrayAndType.end());
  }
  const auto cellArrayAndTypeCombined = this->CombineTopologies(cellArraysAndType);
  if (cellArrayAndTypeCombined.first == nullptr || cellArrayAndTypeCombined.second == nullptr)
  {
    return false;
  }
  entityGrid->SetCells(cellArrayAndTypeCombined.first, cellArrayAndTypeCombined.second);

  // if we have more than one block, we cache the merged mesh.
  vtkNew<vtkUnstructuredGrid> clone;
  clone->CopyStructure(entityGrid);
  cache.Insert(first_group_entity, cacheKey, clone);
  return true;
}

vtkSmartPointer<vtkDataSet> vtkIOSSReaderInternal::GetExodusEntityDataSet(
  const std::vector<std::string>& blockNames, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSReader* self)
{
  const auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle.first, handle.second);
  if (!region)
  {
    return nullptr;
  }

  vtkNew<vtkUnstructuredGrid> entityGrid;
  if (!this->GetEntityMesh(entityGrid, blockNames, vtk_entity_type, handle))
  {
    return {};
  }
  vtkPointData* entityPD = entityGrid->GetPointData();
  vtkCellData* entityCD = entityGrid->GetCellData();

  auto fieldSelection = self->GetFieldSelection(vtk_entity_type);
  assert(fieldSelection != nullptr);
  auto nodeFieldSelection = self->GetNodeBlockFieldSelection();
  assert(nodeFieldSelection != nullptr);

  size_t numberOfValidBlocks = 0;
  for (const auto& blockName : blockNames)
  {
    auto group_entity = region->get_entity(blockName, ioss_entity_type);
    if (!group_entity)
    {
      continue;
    }

    // get the connectivity of the block of the entity
    const auto blockCellArrayAndType = this->GetTopology(blockName, vtk_entity_type, handle);
    if (blockCellArrayAndType.empty())
    {
      continue;
    }
    ++numberOfValidBlocks;

    // compute number of cells in this block
    vtkIdType blockNumberOfCells = 0;
    for (const auto& cellArrayAndType : blockCellArrayAndType)
    {
      blockNumberOfCells += cellArrayAndType.second->GetNumberOfCells();
    }

    // handle all point data once
    if (numberOfValidBlocks == 1)
    {
      this->GetNodeFields(entityPD, nodeFieldSelection, region, group_entity, handle, timestep,
        self->GetReadIds(), true);
      if (self->GetApplyDisplacements())
      {
        this->ApplyDisplacements(entityGrid, region, group_entity, handle, timestep, true);
      }
    }

    // handle local cell data
    vtkNew<vtkCellData> blockCD;
    this->GetFields(
      blockCD, fieldSelection, region, group_entity, handle, timestep, self->GetReadIds());
    if (self->GetGenerateFileId())
    {
      this->GenerateFileId(blockCD, blockNumberOfCells, group_entity, handle);
    }
    if (self->GetReadIds())
    {
      this->GenerateEntityIdArray(blockCD, blockNumberOfCells, blockName, vtk_entity_type, handle);
    }
    if (numberOfValidBlocks == 1)
    {
      // copy allocate needs to be performed first because we need to build the required arrays
      // for future calls of CopyData
      entityCD->CopyGlobalIdsOn();
      entityCD->CopyAllocate(blockCD, blockNumberOfCells);
    }
    entityCD->CopyData(blockCD, entityCD->GetNumberOfTuples(), blockNumberOfCells, 0);
  }

  return entityGrid;
}

void vtkIOSSReaderInternal::GenerateElementAndSideIds(vtkDataSet* dataset, Ioss::SideSet* sideSet,
  const DatabaseHandle& vtkNotUsed(handle), const std::string& vtkNotUsed(blockname),
  vtkIOSSReader::EntityType vtkNotUsed(vtk_entity_type))
{
#ifdef VTK_DBG_IOSS
  std::cout << "Attempt to add element+side ID array(s) for " << blockname << ".\n";
  int ii = 0;
#endif
  for (const auto& sideBlock : sideSet->get_side_blocks())
  {
    auto sourceBlock = sideBlock->parent_element_block();
    auto sourceBlockOffset = sourceBlock ? sourceBlock->get_offset() : 0;
    auto sourceBlockId =
      (sourceBlock && sourceBlock->property_exists("id") ? sourceBlock->get_property("id").get_int()
                                                         : -1);
    auto sourceBlockSize = sourceBlock ? sourceBlock->entity_count() : 0;
    std::array<vtkIdType, 3> sourceBlockData{ { static_cast<vtkIdType>(sourceBlockId),
      static_cast<vtkIdType>(sourceBlockOffset), static_cast<vtkIdType>(sourceBlockSize) } };
#ifdef VTK_DBG_IOSS
    std::cout << "Sides from block " << ii << " " << sourceBlock << " id " << sourceBlockId
              << " range [" << sourceBlockOffset << ", " << (sourceBlockOffset + sourceBlockSize)
              << "[.\n";
#endif
    // ioss element_side_raw is 1-indexed; make it 0-indexed for VTK.
    auto transform = std::unique_ptr<Ioss::Transform>(Ioss::TransformFactory::create("offset"));
    transform->set_property("offset", -1);

    auto element_side_raw =
      vtkIOSSUtilities::GetData(sideBlock, "element_side_raw", transform.get());
    auto sideBlockType = sideBlock->topology()->base_topology_permutation_name();
    (void)element_side_raw;
    std::ostringstream sideElemName;
    sideElemName << sideSet->name() << "_" << sideBlockType << "_elementblock_" << sourceBlockId;
    element_side_raw->SetName(sideElemName.str().c_str());
    // Add info key ENTITY_ID() holding sourceBlockId for later reference.
    element_side_raw->GetInformation()->Set(vtkIOSSReader::ENTITY_ID(), sourceBlockId);
    dataset->GetFieldData()->AddArray(element_side_raw);
    auto* sideArrayNames =
      vtkStringArray::SafeDownCast(dataset->GetFieldData()->GetAbstractArray("side_set_arrays"));
    auto* sideSourceData =
      vtkIdTypeArray::SafeDownCast(dataset->GetFieldData()->GetArray("side_source_data"));
    if (!sideArrayNames)
    {
      vtkNew<vtkStringArray> tmpSides;
      tmpSides->SetName("side_set_arrays");
      dataset->GetFieldData()->AddArray(tmpSides);
      sideArrayNames = tmpSides;
      vtkNew<vtkIdTypeArray> tmpSource;
      tmpSource->SetName("side_source_data");
      tmpSource->SetNumberOfComponents(3); // Block ID, Block Offset, Block Size.
      dataset->GetFieldData()->AddArray(tmpSource);
      sideSourceData = tmpSource;
    }
    sideArrayNames->InsertNextValue(sideElemName.str().c_str());
    sideArrayNames->InsertNextValue(sideBlockType);
    sideSourceData->InsertNextTypedTuple(sourceBlockData.data());
#ifdef VTK_DBG_IOSS
    std::cout << "  side data " << element_side_raw->GetName() << " "
              << element_side_raw->GetNumberOfTuples() << "×"
              << element_side_raw->GetNumberOfComponents() << " ["
              << element_side_raw->GetRange(0)[0] << "," << element_side_raw->GetRange(0)[1]
              << "] ×"
              << " [" << element_side_raw->GetRange(1)[0] << "," << element_side_raw->GetRange(1)[1]
              << "].\n";
    ++ii;
#endif
  }
}

std::vector<vtkSmartPointer<vtkDataSet>> vtkIOSSReaderInternal::GetExodusDataSets(
  const std::string& blockname, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSReader* self)
{
  const auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle.first, handle.second);
  if (!region)
  {
    return {};
  }

  auto group_entity = region->get_entity(blockname, ioss_entity_type);
  if (!group_entity)
  {
    return {};
  }

  vtkNew<vtkUnstructuredGrid> dataset;
  if (!this->GetMesh(dataset, blockname, vtk_entity_type, handle, self->GetRemoveUnusedPoints()))
  {
    return {};
  }

  // let's read arrays.
  auto fieldSelection = self->GetFieldSelection(vtk_entity_type);
  assert(fieldSelection != nullptr);
  this->GetFields(dataset->GetCellData(), fieldSelection, region, group_entity, handle, timestep,
    self->GetReadIds());

  auto nodeFieldSelection = self->GetNodeBlockFieldSelection();
  assert(nodeFieldSelection != nullptr);
  this->GetNodeFields(dataset->GetPointData(), nodeFieldSelection, region, group_entity, handle,
    timestep, self->GetReadIds());

  if (self->GetApplyDisplacements())
  {
    this->ApplyDisplacements(dataset, region, group_entity, handle, timestep);
  }

  if (self->GetGenerateFileId())
  {
    this->GenerateFileId(dataset->GetCellData(), dataset->GetNumberOfCells(), group_entity, handle);
  }

  if (auto* sideSet = dynamic_cast<Ioss::SideSet*>(group_entity))
  {
    if (self->GetElementAndSideIds())
    {
      this->GenerateElementAndSideIds(dataset, sideSet, handle, blockname, vtk_entity_type);
    }
  }

  if (self->GetReadIds())
  {
    this->GenerateEntityIdArray(
      dataset->GetCellData(), dataset->GetNumberOfCells(), blockname, vtk_entity_type, handle);
  }

  return { dataset.GetPointer() };
}

std::vector<vtkSmartPointer<vtkDataSet>> vtkIOSSReaderInternal::GetCGNSDataSets(
  const std::string& blockname, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSReader* self)
{
  const auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle.first, handle.second);
  if (!region)
  {
    return {};
  }

  if (vtk_entity_type == vtkIOSSReader::STRUCTUREDBLOCK)
  {
    auto groups = vtkIOSSUtilities::GetMatchingStructuredBlocks(region, blockname);
    std::vector<vtkSmartPointer<vtkDataSet>> grids;
    for (auto group_entity : groups)
    {
      vtkNew<vtkStructuredGrid> grid;
      if (!this->GetGeometry(grid, group_entity))
      {
        return {};
      }

      auto fieldSelection = self->GetFieldSelection(vtk_entity_type);
      assert(fieldSelection != nullptr);
      this->GetFields(grid->GetCellData(), fieldSelection, region, group_entity, handle, timestep,
        self->GetReadIds());

      // Next, read node fields from nested node-block
      auto nodeFieldSelection = self->GetNodeBlockFieldSelection();
      assert(nodeFieldSelection != nullptr);
      this->GetNodeFields(grid->GetPointData(), nodeFieldSelection, region, group_entity, handle,
        timestep, self->GetReadIds());

      if (self->GetApplyDisplacements())
      {
        this->ApplyDisplacements(grid, region, group_entity, handle, timestep);
      }

      if (self->GetGenerateFileId())
      {
        this->GenerateFileId(grid->GetCellData(), grid->GetNumberOfCells(), group_entity, handle);
      }

      if (self->GetReadIds())
      {
        this->GenerateEntityIdArray(
          grid->GetCellData(), grid->GetNumberOfCells(), blockname, vtk_entity_type, handle);
      }

      grids.emplace_back(grid.GetPointer());
    }
    return grids;
  }
  else if (vtk_entity_type == vtkIOSSReader::SIDESET)
  {
    std::vector<vtkSmartPointer<vtkDataSet>> result;

    // need to read each side-block.
    auto sideSet = dynamic_cast<Ioss::SideSet*>(region->get_entity(blockname, ioss_entity_type));
    if (!sideSet)
    {
      return {};
    }

    // this is the family name for this side set.
    const auto family = sideSet->name();

    std::map<const Ioss::StructuredBlock*, vtkSmartPointer<vtkDataSet>> fullGridMap;

    // for each side block, find the BC matching the family name and then do extract
    // VOI.
    for (const auto& sideBlock : sideSet->get_side_blocks())
    {
      // for each side block, go to the parent block
      auto parentBlock = dynamic_cast<const Ioss::StructuredBlock*>(sideBlock->parent_block());
      assert(parentBlock != nullptr);
      for (auto& bc : parentBlock->m_boundaryConditions)
      {
        if (bc.m_famName == family)
        {
          // read full grid with fields.
          auto iter = fullGridMap.find(parentBlock);
          if (iter == fullGridMap.end())
          {
            auto grids = this->GetCGNSDataSets(
              parentBlock->name(), vtkIOSSReader::STRUCTUREDBLOCK, handle, timestep, self);
            if (grids.empty())
            {
              continue;
            }
            assert(grids.size() == 1);
            iter = fullGridMap.insert(std::make_pair(parentBlock, grids.front())).first;
          }
          assert(iter != fullGridMap.end() && iter->second != nullptr);

          vtkNew<vtkExtractGrid> extractor;
          extractor->SetInputDataObject(iter->second);

          // extents in bc are starting with 1.
          // so adjust them for VTK
          // clang-format off
          int extents[6] = {
            bc.m_rangeBeg[0] - 1, bc.m_rangeEnd[0]  - 1,
            bc.m_rangeBeg[1] - 1, bc.m_rangeEnd[1]  - 1,
            bc.m_rangeBeg[2] - 1, bc.m_rangeEnd[2]  - 1
          };
          // clang-format on

          extractor->SetVOI(extents);
          extractor->Update();

          auto piece = vtkDataSet::SafeDownCast(extractor->GetOutputDataObject(0));

          vtkNew<vtkStringArray> sideBlockInfo;
          sideBlockInfo->SetName("SideBlock Information");
          sideBlockInfo->SetNumberOfComponents(3);
          sideBlockInfo->SetComponentName(0, "Name");
          sideBlockInfo->SetComponentName(1, "Family");
          sideBlockInfo->SetComponentName(2, "ParentBlock");
          sideBlockInfo->InsertNextValue(sideBlock->name());
          sideBlockInfo->InsertNextValue(family);
          sideBlockInfo->InsertNextValue(parentBlock->name());
          piece->GetFieldData()->AddArray(sideBlockInfo);
          result.emplace_back(piece);
        }
      }
    }

    return result;
  }

  return {};
}

bool vtkIOSSReaderInternal::GetMesh(vtkUnstructuredGrid* dataset, const std::string& blockname,
  vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle,
  bool remove_unused_points)
{
  auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockname, ioss_entity_type);
  if (!group_entity)
  {
    return false;
  }

  auto& cache = this->Cache;
  const std::string cacheKey{ "__vtk_mesh__" };
  if (auto cachedDataset = vtkDataSet::SafeDownCast(cache.Find(group_entity, cacheKey)))
  {
    dataset->CopyStructure(cachedDataset);
    return true;
  }

  if (!this->GetTopology(dataset, blockname, vtk_entity_type, handle) ||
    !this->GetGeometry(dataset, "nodeblock_1", handle))
  {
    return false;
  }

  if (remove_unused_points)
  {
    // let's prune unused points.
    vtkNew<vtkRemoveUnusedPoints> pruner;
    pruner->SetOriginalPointIdsArrayName("__vtk_mesh_original_pt_ids__");
    pruner->SetInputDataObject(dataset);
    pruner->Update();

    auto pruned = pruner->GetOutput();
    // cache original pt ids;  this is used in `GetNodeFields`.
    if (auto originalIds = pruned->GetPointData()->GetArray("__vtk_mesh_original_pt_ids__"))
    {
      cache.Insert(group_entity, "__vtk_mesh_original_pt_ids__", originalIds);
      // cache mesh
      dataset->CopyStructure(pruned);
      cache.Insert(group_entity, cacheKey, pruned);
      return true;
    }

    return false;
  }
  else
  {
    vtkNew<vtkUnstructuredGrid> clone;
    clone->CopyStructure(dataset);
    cache.Insert(group_entity, cacheKey, clone);
    return true;
  }
}

bool vtkIOSSReaderInternal::GetMesh(vtkStructuredGrid* grid, const std::string& blockname,
  vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle)
{
  vtkLogScopeF(TRACE, "GetMesh(%s)", blockname.c_str());
  assert(
    vtk_entity_type == vtkIOSSReader::STRUCTUREDBLOCK || vtk_entity_type == vtkIOSSReader::SIDESET);

  if (vtk_entity_type == vtkIOSSReader::STRUCTUREDBLOCK)
  {
    auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
    auto region = this->GetRegion(handle);
    auto group_entity =
      dynamic_cast<Ioss::StructuredBlock*>(region->get_entity(blockname, ioss_entity_type));
    if (!group_entity)
    {
      return false;
    }

    return this->GetGeometry(grid, group_entity);
  }
  else if (vtk_entity_type == vtkIOSSReader::SIDESET)
  {
    auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
    auto region = this->GetRegion(handle);
    auto sideSet = dynamic_cast<Ioss::SideSet*>(region->get_entity(blockname, ioss_entity_type));
    if (!sideSet)
    {
      return false;
    }

    // this is the family name for this side set.
    const auto family = sideSet->name();

    // for each side block, find the BC matching the family name and then do extract
    // VOI.
    for (const auto& sideBlock : sideSet->get_side_blocks())
    {
      // for each side block, go to the parent block
      auto parentBlock = dynamic_cast<const Ioss::StructuredBlock*>(sideBlock->parent_block());
      assert(parentBlock != nullptr);
      for (auto& bc : parentBlock->m_boundaryConditions)
      {
        if (bc.m_famName == family)
        {
          vtkNew<vtkStructuredGrid> fullGrid;
          this->GetGeometry(fullGrid, parentBlock);
          break;
        }
      }
    }

    abort();
  }
  else
  {
    throw std::runtime_error("Unsupported 'GetMesh' call for entity type.");
  }
}

bool vtkIOSSReaderInternal::GenerateEntityIdArray(vtkCellData* cd, vtkIdType numberOfCells,
  const std::string& blockname, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle)
{
  auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockname, ioss_entity_type);
  const bool group_id_exists = group_entity && group_entity->property_exists("id");

  auto& cache = this->Cache;
  if (group_id_exists)
  {
    const std::string cacheKey{ "__vtk_entity_id__" };
    if (auto cachedArray = vtkIdTypeArray::SafeDownCast(cache.Find(group_entity, cacheKey)))
    {
      cd->AddArray(cachedArray);
    }
    else
    {
      vtkNew<vtkIdTypeArray> objectId;
      objectId->SetNumberOfTuples(numberOfCells);
      objectId->FillValue(static_cast<vtkIdType>(group_entity->get_property("id").get_int()));
      objectId->SetName("object_id");
      cache.Insert(group_entity, cacheKey, objectId);
      cd->AddArray(objectId);
    }
  }
  const bool group_original_id_exists =
    group_entity && group_entity->property_exists("original_id");
  if (group_original_id_exists)
  {
    const std::string cacheKey{ "__vtk_original_entity_id__" };
    if (auto cachedArray = vtkIdTypeArray::SafeDownCast(cache.Find(group_entity, cacheKey)))
    {
      cd->AddArray(cachedArray);
    }
    else
    {
      vtkNew<vtkIdTypeArray> originalObjectId;
      originalObjectId->SetNumberOfTuples(numberOfCells);
      originalObjectId->FillValue(
        static_cast<vtkIdType>(group_entity->get_property("original_id").get_int()));
      originalObjectId->SetName("original_object_id");
      cache.Insert(group_entity, cacheKey, originalObjectId);
      cd->AddArray(originalObjectId);
    }
  }

  return group_id_exists || group_original_id_exists;
}

std::vector<std::pair<int, vtkSmartPointer<vtkCellArray>>> vtkIOSSReaderInternal::GetTopology(
  const std::string& blockname, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle)
{
  auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockname, ioss_entity_type);
  if (!group_entity)
  {
    return {};
  }

  vtkLogScopeF(TRACE, "GetTopology (%s)[file=%s]", blockname.c_str(),
    this->GetRawFileName(handle, true).c_str());
  std::vector<std::pair<int, vtkSmartPointer<vtkCellArray>>> blocks;
  if (ioss_entity_type == Ioss::EntityType::SIDESET)
  {
    // For side sets, the topology is stored in nested elements called
    // SideBlocks. Since we split side sets by element block, each sideblock can be
    // treated as a regular entity block.
    assert(group_entity->get_database()->get_surface_split_type() == Ioss::SPLIT_BY_ELEMENT_BLOCK);
    auto sideSet = static_cast<Ioss::SideSet*>(group_entity);
    for (auto sideBlock : sideSet->get_side_blocks())
    {
      int cell_type = VTK_EMPTY_CELL;
      auto cellarray = vtkIOSSUtilities::GetConnectivity(sideBlock, cell_type, &this->Cache);
      if (cellarray != nullptr && cell_type != VTK_EMPTY_CELL)
      {
        blocks.emplace_back(cell_type, cellarray);
      }
    }
  }
  else
  {
    int cell_type = VTK_EMPTY_CELL;
    auto cellarray = vtkIOSSUtilities::GetConnectivity(group_entity, cell_type, &this->Cache);
    if (cell_type != VTK_EMPTY_CELL && cellarray != nullptr)
    {
      blocks.emplace_back(cell_type, cellarray);
    }
  }
  return blocks;
}

std::pair<vtkSmartPointer<vtkUnsignedCharArray>, vtkSmartPointer<vtkCellArray>>
vtkIOSSReaderInternal::CombineTopologies(
  const std::vector<std::pair<int, vtkSmartPointer<vtkCellArray>>>& topologicalBlocks)
{
  if (topologicalBlocks.empty())
  {
    return { nullptr, nullptr };
  }
  else if (topologicalBlocks.size() == 1)
  {
    const int cell_type = topologicalBlocks[0].first;
    const auto cellarray = topologicalBlocks[0].second;
    auto cellTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
    cellTypes->SetNumberOfTuples(cellarray->GetNumberOfCells());
    cellTypes->FillValue(cell_type);
    return { cellTypes, cellarray };
  }
  else
  {
    vtkIdType numCells = 0, connectivitySize = 0;
    for (const auto& block : topologicalBlocks)
    {
      const auto cellarray = block.second;
      numCells += cellarray->GetNumberOfCells();
      connectivitySize += cellarray->GetNumberOfConnectivityEntries();
    }
    // this happens when side block has mixed topological elements.
    vtkNew<vtkCellArray> appendedCellArray;
    appendedCellArray->AllocateExact(numCells, connectivitySize);
    vtkNew<vtkUnsignedCharArray> cellTypesArray;
    cellTypesArray->SetNumberOfTuples(numCells);
    auto ptr = cellTypesArray->GetPointer(0);
    for (auto& block : topologicalBlocks)
    {
      const int cell_type = block.first;
      const auto cellarray = block.second;
      appendedCellArray->Append(cellarray);
      ptr =
        std::fill_n(ptr, block.second->GetNumberOfCells(), static_cast<unsigned char>(cell_type));
    }
    return { cellTypesArray, appendedCellArray };
  }
}

bool vtkIOSSReaderInternal::GetTopology(vtkUnstructuredGrid* grid, const std::string& blockname,
  vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle)
{
  const auto cellArraysWithCellType = this->GetTopology(blockname, vtk_entity_type, handle);
  const auto cellArrayAndCellTypesCombined = this->CombineTopologies(cellArraysWithCellType);
  if (cellArrayAndCellTypesCombined.first == nullptr ||
    cellArrayAndCellTypesCombined.second == nullptr)
  {
    return false;
  }
  grid->SetCells(cellArrayAndCellTypesCombined.first, cellArrayAndCellTypesCombined.second);
  return true;
}

vtkSmartPointer<vtkPoints> vtkIOSSReaderInternal::GetGeometry(
  const std::string& blockname, const DatabaseHandle& handle)
{
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockname, Ioss::EntityType::NODEBLOCK);
  if (!group_entity)
  {
    return nullptr;
  }
  vtkLogScopeF(TRACE, "GetGeometry(%s)[file=%s]", blockname.c_str(),
    this->GetRawFileName(handle, true).c_str());
  return vtkIOSSUtilities::GetMeshModelCoordinates(group_entity, &this->Cache);
}

bool vtkIOSSReaderInternal::GetGeometry(
  vtkUnstructuredGrid* grid, const std::string& blockname, const DatabaseHandle& handle)
{
  auto pts = this->GetGeometry(blockname, handle);
  if (pts)
  {
    grid->SetPoints(pts);
    return true;
  }
  return false;
}

bool vtkIOSSReaderInternal::GetGeometry(
  vtkStructuredGrid* grid, const Ioss::StructuredBlock* groupEntity)
{
  auto& sblock = (*groupEntity);

  int extents[6];
  extents[0] = static_cast<int>(sblock.get_property("offset_i").get_int());
  extents[1] = extents[0] + static_cast<int>(sblock.get_property("ni").get_int());
  extents[2] = static_cast<int>(sblock.get_property("offset_j").get_int());
  extents[3] = extents[2] + static_cast<int>(sblock.get_property("nj").get_int());
  extents[4] = static_cast<int>(sblock.get_property("offset_k").get_int());
  extents[5] = extents[4] + static_cast<int>(sblock.get_property("nk").get_int());

  assert(
    sblock.get_property("node_count").get_int() == vtkStructuredData::GetNumberOfPoints(extents));
  assert(
    sblock.get_property("cell_count").get_int() == vtkStructuredData::GetNumberOfCells(extents));

  // set extents on grid.
  grid->SetExtent(extents);

  // now read the points.
  auto points = vtkIOSSUtilities::GetMeshModelCoordinates(&sblock, &this->Cache);
  grid->SetPoints(points);
  assert(points->GetNumberOfPoints() == vtkStructuredData::GetNumberOfPoints(extents));
  return true;
}

vtkSmartPointer<vtkAbstractArray> vtkIOSSReaderInternal::GetField(const std::string& fieldname,
  Ioss::Region* region, Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle,
  int timestep, vtkIdTypeArray* ids_to_extract, const std::string& cache_key_suffix)
{
  const auto get_field = [&fieldname, &region, &timestep, &handle, this](
                           Ioss::GroupingEntity* entity) -> vtkSmartPointer<vtkAbstractArray> {
    if (!entity->field_exists(fieldname))
    {
      return nullptr;
    }

    if (!vtkIOSSUtilities::IsFieldTransient(entity, fieldname))
    {
      // non-time dependent field.
      return vtkIOSSUtilities::GetData(entity, fieldname, /*transform=*/nullptr, &this->Cache);
    }

    // determine state for transient data.
    const auto& stateVector = this->DatabaseTimes[handle.first];
    if (stateVector.empty())
    {
      // see paraview/paraview#20658 for why this is needed.
      return nullptr;
    }

    auto iter =
      std::find_if(stateVector.begin(), stateVector.end(), [&](const std::pair<int, double>& pair) {
        return pair.second == this->TimestepValues[timestep];
      });

    if (iter == stateVector.end())
    {
      throw std::runtime_error("Invalid timestep chosen: " + std::to_string(timestep));
    }
    const int state = iter->first;
    region->begin_state(state);
    try
    {
      const std::string key = "__vtk_transient_" + fieldname + "_" + std::to_string(state) + "__";
      auto f =
        vtkIOSSUtilities::GetData(entity, fieldname, /*transform=*/nullptr, &this->Cache, key);
      region->end_state(state);
      return f;
    }
    catch (...)
    {
      region->end_state(state);
      std::rethrow_exception(std::current_exception());
    }
  };

  const auto get_field_for_entity = [&]() {
    if (group_entity->type() == Ioss::EntityType::SIDESET)
    {
      // sidesets need to be handled specially. For sidesets, the fields are
      // available on nested sideblocks.
      std::vector<vtkSmartPointer<vtkAbstractArray>> arrays;
      auto sideSet = static_cast<Ioss::SideSet*>(group_entity);
      for (auto sideBlock : sideSet->get_side_blocks())
      {
        if (auto array = get_field(sideBlock))
        {
          arrays.push_back(array);
        }
      }
      return ::JoinArrays(arrays);
    }
    else
    {
      return get_field(group_entity);
    }
  };

  auto& cache = this->Cache;
  const std::string cacheKey =
    (vtkIOSSUtilities::IsFieldTransient(group_entity, fieldname)
        ? "__vtk_transientfield_" + fieldname + std::to_string(timestep) + "__"
        : "__vtk_field_" + fieldname + "__") +
    cache_key_suffix;
  if (auto cached = vtkAbstractArray::SafeDownCast(cache.Find(group_entity, cacheKey)))
  {
    return cached;
  }

  auto full_field = get_field_for_entity();
  if (full_field != nullptr && ids_to_extract != nullptr)
  {
    // subset the field.
    vtkNew<vtkIdList> list;
    // this is a shallow copy.
    list->SetArray(ids_to_extract->GetPointer(0), ids_to_extract->GetNumberOfTuples());

    vtkSmartPointer<vtkAbstractArray> clone;
    clone.TakeReference(full_field->NewInstance());
    clone->SetName(full_field->GetName());
    clone->SetNumberOfComponents(full_field->GetNumberOfComponents());
    clone->SetNumberOfTuples(list->GetNumberOfIds());
    full_field->GetTuples(list, clone);

    // get back the data pointer from the idlist
    list->Release();

    // convert field if needed for VTK e.g. ids have to be `vtkIdTypeArray`.
    clone = this->ConvertFieldForVTK(clone);

    cache.Insert(group_entity, cacheKey, clone);
    return clone;
  }
  else
  {
    // convert field if needed for VTK e.g. ids have to be `vtkIdTypeArray`.
    full_field = this->ConvertFieldForVTK(full_field);

    cache.Insert(group_entity, cacheKey, full_field);
    return full_field;
  }
}

bool vtkIOSSReaderInternal::GetFields(vtkDataSetAttributes* dsa, vtkDataArraySelection* selection,
  Ioss::Region* region, Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle,
  int timestep, bool read_ioss_ids, vtkIdTypeArray* ids_to_extract /*=nullptr*/,
  const std::string& cache_key_suffix /*= std::string()*/)
{
  std::vector<std::string> fieldnames;
  std::string globalIdsFieldName;
  if (read_ioss_ids)
  {
    switch (group_entity->type())
    {
      case Ioss::EntityType::NODEBLOCK:
      case Ioss::EntityType::EDGEBLOCK:
      case Ioss::EntityType::FACEBLOCK:
      case Ioss::EntityType::ELEMENTBLOCK:
        fieldnames.emplace_back("ids");
        globalIdsFieldName = "ids";
        break;

      case Ioss::EntityType::NODESET:
        break;

      case Ioss::EntityType::STRUCTUREDBLOCK:
        if (vtkPointData::SafeDownCast(dsa))
        {
          fieldnames.emplace_back("cell_node_ids");
        }
        else
        {
          fieldnames.emplace_back("cell_ids");
        }
        // note: unlike for Exodus, there ids are not unique
        // across blocks and hence are not flagged as global ids.
        break;

      case Ioss::EntityType::EDGESET:
      case Ioss::EntityType::FACESET:
      case Ioss::EntityType::ELEMENTSET:
      case Ioss::EntityType::SIDESET:
        fieldnames.emplace_back("element_side");
        break;

      default:
        break;
    }
  }
  for (int cc = 0; selection != nullptr && cc < selection->GetNumberOfArrays(); ++cc)
  {
    if (selection->GetArraySetting(cc))
    {
      fieldnames.emplace_back(selection->GetArrayName(cc));
    }
  }
  for (const auto& fieldname : fieldnames)
  {
    if (auto array = this->GetField(
          fieldname, region, group_entity, handle, timestep, ids_to_extract, cache_key_suffix))
    {
      if (fieldname == globalIdsFieldName)
      {
        dsa->SetGlobalIds(vtkDataArray::SafeDownCast(array));
      }
      else if (fieldname == vtkDataSetAttributes::GhostArrayName())
      {
        // Handle vtkGhostType attribute specially. Convert it to the expected vtkUnsignedCharArray.
        vtkNew<vtkUnsignedCharArray> ghostArray;
        ghostArray->SetName(vtkDataSetAttributes::GhostArrayName());
        ghostArray->SetNumberOfComponents(1);
        ghostArray->SetNumberOfTuples(array->GetNumberOfTuples());

        ghostArray->CopyComponent(0, vtkDataArray::SafeDownCast(array), 0);
        dsa->AddArray(ghostArray);
      }
      else
      {
        dsa->AddArray(array);
      }
    }
  }

  return true;
}

bool vtkIOSSReaderInternal::GetNodeFields(vtkDataSetAttributes* dsa,
  vtkDataArraySelection* selection, Ioss::Region* region, Ioss::GroupingEntity* group_entity,
  const DatabaseHandle& handle, int timestep, bool read_ioss_ids, bool mergeExodusEntityBlocks)
{
  if (group_entity->type() == Ioss::EntityType::STRUCTUREDBLOCK)
  {
    // CGNS
    // node fields are stored under nested node block. So use that.
    auto sb = dynamic_cast<Ioss::StructuredBlock*>(group_entity);
    auto& nodeBlock = sb->get_node_block();
    if (!this->GetFields(
          dsa, selection, region, &nodeBlock, handle, timestep, /*read_ioss_ids=*/false))
    {
      return false;
    }

    // for STRUCTUREDBLOCK, the node ids are read from the SB itself, and not
    // the nested nodeBlock.
    return read_ioss_ids
      ? this->GetFields(dsa, nullptr, region, sb, handle, timestep, /*read_ioss_ids=*/true)
      : true;
  }
  else
  {
    // Exodus
    const auto blockname = group_entity->name();
    auto& cache = this->Cache;
    vtkIdTypeArray* vtk_raw_ids_array = !mergeExodusEntityBlocks
      ? vtkIdTypeArray::SafeDownCast(cache.Find(group_entity, "__vtk_mesh_original_pt_ids__"))
      : nullptr;
    const std::string cache_key_suffix = vtk_raw_ids_array != nullptr ? blockname : std::string();

    auto nodeblock = region->get_entity("nodeblock_1", Ioss::EntityType::NODEBLOCK);
    return this->GetFields(dsa, selection, region, nodeblock, handle, timestep, read_ioss_ids,
      vtk_raw_ids_array, cache_key_suffix);
  }
}

bool vtkIOSSReaderInternal::GenerateFileId(vtkDataSetAttributes* cellData, vtkIdType numberOfCells,
  Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle)
{
  if (!group_entity)
  {
    return false;
  }

  auto& cache = this->Cache;
  if (auto file_ids = vtkDataArray::SafeDownCast(cache.Find(group_entity, "__vtk_file_ids__")))
  {
    assert(numberOfCells == file_ids->GetNumberOfTuples());
    cellData->AddArray(file_ids);
    return true;
  }

  vtkNew<vtkIntArray> file_ids;
  file_ids->SetName("file_id");
  file_ids->SetNumberOfTuples(numberOfCells);

  int fileId = handle.second;

  // from index get original file rank number, if possible and use that.
  try
  {
    const auto& dbaseInfo = this->DatabaseNames.at(handle.first);
    if (dbaseInfo.ProcessCount != 0)
    {
      assert(fileId >= 0 && fileId < static_cast<decltype(fileId)>(dbaseInfo.Ranks.size()));
      fileId = *std::next(dbaseInfo.Ranks.begin(), fileId);
    }
  }
  catch (std::out_of_range&)
  {
  }

  std::fill(file_ids->GetPointer(0), file_ids->GetPointer(0) + numberOfCells, fileId);
  cache.Insert(group_entity, "__vtk_file_ids__", file_ids.GetPointer());
  cellData->AddArray(file_ids);
  return true;
}

vtkSmartPointer<vtkAbstractArray> vtkIOSSReaderInternal::ConvertFieldForVTK(vtkAbstractArray* array)
{
  if (array == nullptr || array->GetName() == nullptr || strcmp(array->GetName(), "ids") != 0)
  {
    return array;
  }

  if (vtkIdTypeArray::SafeDownCast(array))
  {
    return array;
  }

  vtkNew<vtkIdTypeArray> ids;
  ids->DeepCopy(array);
  return ids;
}

bool vtkIOSSReaderInternal::ApplyDisplacements(vtkPointSet* grid, Ioss::Region* region,
  Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle, int timestep,
  bool mergeExodusEntityBlocks)
{
  if (!group_entity)
  {
    return false;
  }

  auto& cache = this->Cache;
  const auto xformPtsCacheKeyEnding =
    std::to_string(timestep) + std::to_string(std::hash<double>{}(this->DisplacementMagnitude));
  const auto xformPtsCacheKey = !mergeExodusEntityBlocks
    ? "__vtk_xformed_pts_" + xformPtsCacheKeyEnding
    : "__vtk_merged_xformed_pts_" + xformPtsCacheKeyEnding;
  if (auto xformedPts = vtkPoints::SafeDownCast(cache.Find(group_entity, xformPtsCacheKey)))
  {
    assert(xformedPts->GetNumberOfPoints() == grid->GetNumberOfPoints());
    grid->SetPoints(xformedPts);
    return true;
  }

  vtkSmartPointer<vtkDataArray> array;

  if (group_entity->type() == Ioss::EntityType::STRUCTUREDBLOCK)
  {
    // CGNS
    // node fields are stored under nested node block. So use that.
    auto sb = dynamic_cast<Ioss::StructuredBlock*>(group_entity);
    auto& nodeBlock = sb->get_node_block();
    auto displ_array_name = vtkIOSSUtilities::GetDisplacementFieldName(&nodeBlock);
    if (displ_array_name.empty())
    {
      return false;
    }

    array = vtkDataArray::SafeDownCast(
      this->GetField(displ_array_name, region, &nodeBlock, handle, timestep));
  }
  else
  {
    // EXODUS
    // node fields are stored in global node-block from which we need to subset based on the "ids"
    // for those current block.
    auto nodeBlock = region->get_entity("nodeblock_1", Ioss::EntityType::NODEBLOCK);
    auto displ_array_name = vtkIOSSUtilities::GetDisplacementFieldName(nodeBlock);
    if (displ_array_name.empty())
    {
      return false;
    }

    auto vtk_raw_ids_array = !mergeExodusEntityBlocks
      ? vtkIdTypeArray::SafeDownCast(cache.Find(group_entity, "__vtk_mesh_original_pt_ids__"))
      : nullptr;
    const std::string cache_key_suffix =
      vtk_raw_ids_array != nullptr ? group_entity->name() : std::string();
    array = vtkDataArray::SafeDownCast(this->GetField(
      displ_array_name, region, nodeBlock, handle, timestep, vtk_raw_ids_array, cache_key_suffix));
  }

  if (array)
  {
    // NOTE: array maybe 2 component for 2d dataset; but our points are always 3D.
    auto pts = grid->GetPoints();
    auto numPts = pts->GetNumberOfPoints();

    assert(array->GetNumberOfTuples() == numPts && array->GetNumberOfComponents() <= 3);

    vtkNew<vtkPoints> xformedPts;
    xformedPts->SetDataType(pts->GetDataType());
    xformedPts->SetNumberOfPoints(pts->GetNumberOfPoints());
    vtkVector3d coords{ 0.0 }, displ{ 0.0 };
    for (vtkIdType cc = 0; cc < numPts; ++cc)
    {
      pts->GetPoint(cc, coords.GetData());
      array->GetTuple(cc, displ.GetData());
      for (int i = 0; i < 3; ++i)
      {
        displ[i] *= this->DisplacementMagnitude;
      }
      xformedPts->SetPoint(cc, (coords + displ).GetData());
    }

    grid->SetPoints(xformedPts);
    cache.Insert(group_entity, xformPtsCacheKey, xformedPts);
    return true;
  }
  return false;
}

bool vtkIOSSReaderInternal::GetQAAndInformationRecords(
  vtkFieldData* fd, const DatabaseHandle& handle)
{
  auto region = this->GetRegion(handle);
  if (!region)
  {
    return false;
  }

  const auto& qa = region->get_qa_records();
  vtkNew<vtkStringArray> qa_records;
  qa_records->SetName("QA Records");
  qa_records->SetNumberOfComponents(4);
  qa_records->Allocate(static_cast<vtkIdType>(qa.size()));
  qa_records->SetComponentName(0, "Code Name");
  qa_records->SetComponentName(1, "QA Descriptor");
  qa_records->SetComponentName(2, "Date");
  qa_records->SetComponentName(3, "Time");
  for (auto& name : qa)
  {
    qa_records->InsertNextValue(name);
  }

  const auto& info = region->get_information_records();
  vtkNew<vtkStringArray> info_records;
  info_records->SetName("Information Records");
  info_records->SetNumberOfComponents(1);
  info_records->Allocate(static_cast<vtkIdType>(info.size()));
  for (auto& n : info)
  {
    info_records->InsertNextValue(n);
  }

  fd->AddArray(info_records);
  fd->AddArray(qa_records);
  return true;
}

bool vtkIOSSReaderInternal::GetGlobalFields(
  vtkFieldData* fd, const DatabaseHandle& handle, int timestep)
{
  auto region = this->GetRegion(handle);
  if (!region)
  {
    return false;
  }

  Ioss::NameList fieldNames;
  region->field_describe(&fieldNames);
  for (const auto& name : fieldNames)
  {
    switch (region->get_fieldref(name).get_role())
    {
      case Ioss::Field::ATTRIBUTE:
      case Ioss::Field::REDUCTION:
        if (auto array = this->GetField(name, region, region, handle, timestep))
        {
          fd->AddArray(array);
        }
        break;
      default:
        break;
    }
  }
  return true;
}

VTK_ABI_NAMESPACE_END
