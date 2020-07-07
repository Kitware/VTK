/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIossReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIossReader.h"
#include "vtkIossFilesScanner.h"
#include "vtkIossUtilities.h"

#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
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
#include "vtkReaderExecutive.h"
#include "vtkRemoveUnusedPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(init/Ionit_Initializer.h)
#include VTK_IOSS(Ioss_DatabaseIO.h)
#include VTK_IOSS(Ioss_EdgeBlock.h)
#include VTK_IOSS(Ioss_EdgeSet.h)
#include VTK_IOSS(Ioss_ElementBlock.h)
#include VTK_IOSS(Ioss_ElementSet.h)
#include VTK_IOSS(Ioss_FaceBlock.h)
#include VTK_IOSS(Ioss_FaceSet.h)
#include VTK_IOSS(Ioss_IOFactory.h)
#include VTK_IOSS(Ioss_NodeBlock.h)
#include VTK_IOSS(Ioss_NodeSet.h)
#include VTK_IOSS(Ioss_Region.h)
#include VTK_IOSS(Ioss_SideBlock.h)
#include VTK_IOSS(Ioss_SideSet.h)
// clang-format on

#include <array>
#include <cassert>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <utility>

struct DatabaseParitionInfo
{
  int ProcessCount = 0;
  std::set<int> Ranks;
};

// Opaque handle used to identify a specific Region
using DatabaseHandle = std::pair<std::string, int>;

namespace
{

template <typename T>
bool Synchronize(vtkMultiProcessController* controller, T& data, T& result)
{
  if (controller == nullptr || controller->GetNumberOfProcesses() <= 1)
  {
    return true;
  }

  vtkMultiProcessStream stream;
  stream << data;

  std::vector<vtkMultiProcessStream> all_streams;
  if (controller->AllGather(stream, all_streams))
  {
    for (auto& s : all_streams)
    {
      s >> result;
    }
    return true;
  }

  return false;
}

template <typename T>
bool Broadcast(vtkMultiProcessController* controller, T& data, int root)
{
  if (controller == nullptr || controller->GetNumberOfProcesses() <= 1)
  {
    return true;
  }
  if (controller->GetLocalProcessId() == root)
  {
    vtkMultiProcessStream stream;
    stream << data;
    return controller->Broadcast(stream, root) != 0;
  }
  else
  {
    data = T();
    vtkMultiProcessStream stream;
    if (controller->Broadcast(stream, root))
    {
      stream >> data;
      return true;
    }
    return false;
  }
}

vtkSmartPointer<vtkAbstractArray> JoinArrays(
  const std::vector<vtkSmartPointer<vtkAbstractArray>>& arrays)
{
  if (arrays.size() == 0)
  {
    return nullptr;
  }
  else if (arrays.size() == 1)
  {
    return arrays[0];
  }

  vtkIdType numTuples = 0;
  for (auto& array : arrays)
  {
    numTuples += array->GetNumberOfTuples();
  }

  vtkSmartPointer<vtkAbstractArray> result;
  result.TakeReference(arrays[0]->NewInstance());
  result->CopyInformation(arrays[0]->GetInformation());
  result->SetName(arrays[0]->GetName());
  result->SetNumberOfComponents(arrays[0]->GetNumberOfComponents());
  result->SetNumberOfTuples(numTuples);
  vtkIdType offset = 0;
  for (auto& array : arrays)
  {
    const auto count = array->GetNumberOfTuples();
    result->InsertTuples(offset, count, 0, array);
    offset += count;
  }
  result->Modified();
  assert(offset == numTuples);
  return result;
}
}

class vtkIossReader::vtkInternals
{
  // it's okay to instantiate this multiple times.
  Ioss::Init::Initializer io;

  std::map<std::string, DatabaseParitionInfo> DatabaseNames;
  vtkTimeStamp DatabaseNamesMTime;

  std::map<std::string, std::set<double>> DatabaseTimes;
  std::vector<double> TimestepValues;
  vtkTimeStamp TimestepValuesMTime;

  // a collection of names for blocks and sets in the file(s).
  std::array<std::set<vtkIossUtilities::EntityNameType>, vtkIossReader::NUMBER_OF_ENTITY_TYPES>
    EntityNames;
  vtkTimeStamp SelectionsMTime;

  std::map<DatabaseHandle, std::shared_ptr<Ioss::Region>> RegionMap;

  vtkIossUtilities::Cache Cache;

public:
  std::set<std::string> FileNames;
  vtkTimeStamp FileNamesMTime;

  const std::vector<double>& GetTimeSteps() const { return this->TimestepValues; }

  //@{
  /**
   * Cache related API.
   */
  void ClearCache() { this->Cache.Clear(); }
  void ResetCacheAccessCounts() { this->Cache.ResetAccessCounts(); }
  void ClearCacheUnused() { this->Cache.ClearUnused(); }
  //@}

  /**
   * Processes filenames to populate names for Ioss databases to read.
   *
   * A file collection representing files partitioned across ranks where each
   * rank generate a separate file (spatial partitioning) are all represented
   * by a single Ioss database.
   *
   * Multiple Ioss databases are generated when the files are a temporal
   * in nature or represent restarts.
   *
   * This method simply uses the filenames to determine what type of files we
   * are encountering. For spatial partitions, the filenames must end with
   * '{processor-count}.{rank}'.
   *
   * @returns `false` to indicate failure.
   */
  bool UpdateDatabaseNames(vtkIossReader* self);

  /**
   * Read Ioss databases to generate information about timesteps / times
   * in the databases.
   *
   * This is called after successful call to `UpdateDatabaseNames` which should
   * populate the list of Ioss databases. This method iterates over all
   * databases and gathers informations about timesteps available in those
   * databases. When running in parallel, only the root node opens the Ioss
   * databases and reads the time information. That information is then
   * exchanged with all ranks thus at the end of this method all ranks should
   * have their time information updated.
   *
   * @returns `false` on failure.
   */
  bool UpdateTimeInformation(vtkIossReader* self);

  /**
   * Populates various `vtkDataArraySelection` objects on the vtkIossReader with
   * names for entity-blocks, -sets, and fields defined on them.
   */
  bool UpdateEntityAndFieldSelections(vtkIossReader* self);

  /**
   * Fills up the output data-structure based on the entity blocks/sets chosen
   * and those available.
   */
  bool GenerateOutput(vtkPartitionedDataSetCollection* output, vtkIossReader* self);

  /**
   * Adds geometry (points) and topology (cell) information to the grid for the
   * entity block or set chosen using the name (`blockname`) and type
   * (`vtk_entity_type`).
   *
   * `handle` is the database / file handle for the current piece / rank
   * obtained by calling `GetDatabaseHandles`.
   *
   * If `remove_unused_points` is true, any points that are not used by the
   * cells are removed. When that is done, an array called
   * `__vtk_mesh_original_pt_ids__` is added to the cache for the entity
   * which can be used to identify which points were passed through.
   */
  bool GetMesh(vtkUnstructuredGrid* grid, const std::string& blockname,
    vtkIossReader::EntityType vtk_entity_type, const DatabaseHandle& handle,
    bool remove_unused_points);

  /**
   * Reads selected field arrays for the given entity block or set.
   * If `read_ioss_ids` is true, then element ids are read as applicable.
   *
   * `ids_to_extract`, when specified, is a `vtkIdTypeArray` identifying the
   * subset of indices to produce in the output. This is used for point data fields
   * when the mesh was generated with `remove_unused_points` on. This ensures
   * that point data arrays match the points. When `ids_to_extract` is provided,
   * for the caching to work correctly, the `cache_key_suffix` must be set to
   * the name of the entity block (or set) which provided the cells to determine
   * which points to extract.
   *
   * Returns true on success.
   *
   * On error, `std::runtime_error` is thrown.
   */
  bool GetFields(vtkDataSetAttributes* dsa, vtkDataArraySelection* selection,
    const std::string& blockname, vtkIossReader::EntityType vtk_entity_type,
    const DatabaseHandle& handle, int timestep, bool read_ioss_ids,
    vtkIdTypeArray* ids_to_extract = nullptr, const std::string& cache_key_suffix = std::string());

  /**
   * This reads node fields for an entity block or set.
   *
   * Internally calls `GetFields()` with correct values for `ids_to_extract` and
   * `cache_key_suffix`.
   *
   */
  bool GetNodeFields(vtkDataSetAttributes* dsa, vtkDataArraySelection* selection,
    const std::string& blockname, vtkIossReader::EntityType vtk_entity_type,
    const DatabaseHandle& handle, int timestep, bool read_ioss_ids);

  /**
   * Reads node block array with displacements and then transforms
   * the points in the grid using those displacements.
   */
  bool ApplyDisplacements(vtkUnstructuredGrid* grid, const std::string& blockname,
    vtkIossReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep);

  /**
   * Adds 'file_id' array to indicate which file the dataset was read from.
   */
  bool GenerateFileId(vtkUnstructuredGrid* grid, const std::string& blockname,
    vtkIossReader::EntityType vtk_entity_type, const DatabaseHandle& handle);

  /**
   * Read quality assurance and information data from the file.
   */
  bool GetQAAndInformationRecords(vtkFieldData* fd, const DatabaseHandle& handle);

  /**
   * Read global fields.
   */
  bool GetGlobalFields(vtkFieldData* fd, const DatabaseHandle& handle, int timestep);

  /**
   * Returns the list of fileids, if any to be read for a given "piece" for the
   * chosen timestep.
   */
  std::vector<DatabaseHandle> GetDatabaseHandles(int piece, int npieces, int timestep) const;

  /**
   * Useful for printing error messages etc.
   */
  std::string GetRawFileName(const DatabaseHandle& handle, bool shortname = false) const
  {
    auto iter = this->DatabaseNames.find(handle.first);
    if (iter == this->DatabaseNames.end())
    {
      throw std::runtime_error("bad database handle!");
    }

    const int& fileid = handle.second;
    const auto dbasename =
      shortname ? vtksys::SystemTools::GetFilenameName(handle.first) : handle.first;

    auto& dinfo = iter->second;
    if (dinfo.ProcessCount > 0)
    {
      return Ioss::Utils::decode_filename(
        dbasename, dinfo.ProcessCount, *std::next(dinfo.Ranks.begin(), fileid));
    }
    return dbasename;
  }

  /**
   * For spatially partitioned files, this returns the partition identifier for
   * the file identified by the handle.
   */
  int GetFileProcessor(const DatabaseHandle& handle) const
  {
    auto iter = this->DatabaseNames.find(handle.first);
    if (iter == this->DatabaseNames.end())
    {
      throw std::runtime_error("bad database handle!");
    }
    const int& fileid = handle.second;
    auto& dinfo = iter->second;
    if (dinfo.ProcessCount > 0)
    {
      return *std::next(dinfo.Ranks.begin(), fileid);
    }

    // this is not a spatially partitioned file; just return 0.
    return 0;
  }

private:
  std::vector<int> GetFileIds(const std::string& dbasename, int myrank, int numRanks) const;
  Ioss::Region* GetRegion(const std::string& dbasename, int fileid);
  Ioss::Region* GetRegion(const DatabaseHandle& handle)
  {
    return this->GetRegion(handle.first, handle.second);
  }

  //@{
  /**
   * Reads a field with name `fieldname` from entity block or set with chosen name
   * (`blockname`) and type (`vtk_entity_type`). Field may be a result
   * field which can be time-varying. In that case, `timestep` is used to
   * identify the timestep to read.
   *
   * Returns non-null array on success. Returns nullptr if block or field is
   * missing (which is not an error condition).
   *
   * On error, `std::runtime_error` is thrown.
   */
  vtkSmartPointer<vtkAbstractArray> GetField(const std::string& fieldname,
    const std::string& blockname, vtkIossReader::EntityType vtk_entity_type,
    const DatabaseHandle& handle, int timestep, vtkIdTypeArray* ids_to_extract = nullptr,
    const std::string& cache_key_suffix = std::string());
  vtkSmartPointer<vtkAbstractArray> GetField(const std::string& fieldname, Ioss::Region* region,
    Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle, int timestep,
    vtkIdTypeArray* ids_to_extract = nullptr, const std::string& cache_key_suffix = std::string());
  //@}

  /**
   * Fill up the `grid` with connectivity information for the entity block (or
   * set) with the given name (`blockname`) and type (vtk_entity_type).
   *
   * `handle` is the database / file handle for the current piece / rank
   * obtained by calling `GetDatabaseHandles`.
   *
   * Returns true on success. `false` will be returned when the handle doesn't
   * have the chosen blockname/entity.
   *
   * On file reading error, `std::runtime_error` is thrown.
   */
  bool GetTopology(vtkUnstructuredGrid* grid, const std::string& blockname,
    vtkIossReader::EntityType vtk_entity_type, const DatabaseHandle& handle);

  /**
   * Fill up `grid` with point coordinates aka geometry read from the block
   * with the given name (`blockname`). The point coordinates are always
   * read from a block of type NODEBLOCK.
   *
   * `handle` is the database / file handle for the current piece / rank
   * obtained by calling `GetDatabaseHandles`.
   *
   * Returns true on success.
   *
   * On file reading error, `std::runtime_error` is thrown.
   */
  bool GetGeometry(vtkPointSet* grid, const std::string& blockname, const DatabaseHandle& handle);

  /**
   * Fields like "ids" have to be vtkIdTypeArray in VTK. This method does the
   * conversion if needed.
   */
  vtkSmartPointer<vtkAbstractArray> ConvertFieldForVTK(vtkAbstractArray* array)
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
};

//----------------------------------------------------------------------------
std::vector<int> vtkIossReader::vtkInternals::GetFileIds(
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
  // continguous chunks.
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

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::UpdateDatabaseNames(vtkIossReader* self)
{
  if (this->DatabaseNamesMTime > this->FileNamesMTime)
  {
    return (!this->DatabaseNames.empty());
  }

  // Clear cache since we're updating the databases, old caches no longer makes
  // sense.
  this->Cache.Clear();

  // Clear old Ioss::Region's since they may not be correct anymore.
  this->RegionMap.clear();

  auto filenames = this->FileNames;
  auto controller = self->GetController();
  const int myrank = controller ? controller->GetLocalProcessId() : 0;

  if (myrank == 0)
  {
    if (filenames.size() == 1 && vtkIossFilesScanner::IsMetaFile(*filenames.begin()))
    {
      filenames = vtkIossFilesScanner::GetFilesFromMetaFile(*filenames.begin());
    }
    else if (self->GetScanForRelatedFiles())
    {
      filenames = vtkIossFilesScanner::GetRelatedFiles(filenames);
    }
  }

  if (!::Broadcast(controller, filenames, 0))
  {
    return false;
  }

  if (filenames.size() == 0)
  {
    vtkErrorWithObjectMacro(self, "No filename specified.");
    return false;
  }

  // process filename to determine the base-name and the `processor_count`, and
  // `my_processor` values.
  // clang-format off
  vtksys::RegularExpression regEx(R"(^(.*)\.([0-9]+)\.([0-9]+)$)");
  // clang-format on

  decltype(this->DatabaseNames) databases;
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
      databases.insert(std::make_pair(fname, DatabaseParitionInfo()));
    }
  }

  this->DatabaseNames.swap(databases);
  this->DatabaseNamesMTime.Modified();

  if (vtkLogger::GetCurrentVerbosityCutoff() >= vtkLogger::VERBOSITY_TRACE)
  {
    // let's log.
    vtkLogF(TRACE, "Found Ioss databases (%d)", static_cast<int>(this->DatabaseNames.size()));
    std::ostringstream str;
    for (auto& pair : this->DatabaseNames)
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
  return (this->DatabaseNames.size() > 0);
}

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::UpdateTimeInformation(vtkIossReader* self)
{
  if (this->TimestepValuesMTime > this->DatabaseNamesMTime)
  {
    return true;
  }

  auto controller = self->GetController();
  const auto rank = controller ? controller->GetLocalProcessId() : 0;
  const auto numRanks = controller ? controller->GetNumberOfProcesses() : 1;

  int success = 1;
  if (rank == 0)
  {
    // time values for each database.
    std::map<std::string, std::set<double>> dbase_times;

    // read all databases to collect timestep information.
    for (const auto& pair : this->DatabaseNames)
    {
      assert(pair.second.ProcessCount == 0 || pair.second.Ranks.size() > 0);
      const auto fileids = this->GetFileIds(pair.first, rank, numRanks);
      if (fileids.size() == 0)
      {
        continue;
      }
      try
      {
        auto region = this->GetRegion(pair.first, fileids.front());
        dbase_times[pair.first] = vtkIossUtilities::GetTimeValues(region);
      }
      catch (std::runtime_error& e)
      {
        vtkErrorWithObjectMacro(self, "Error in UpdateTimeInformation: \n" << e.what());
        success = 0;
        dbase_times.clear();
        break;
      }
    }

    this->DatabaseTimes.swap(dbase_times);
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
  }

  // Fillup TimestepValues for ease of use later.
  std::set<double> times_set;
  for (auto& pair : this->DatabaseTimes)
  {
    std::copy(pair.second.begin(), pair.second.end(), std::inserter(times_set, times_set.end()));
  }
  this->TimestepValues.resize(times_set.size());
  std::copy(times_set.begin(), times_set.end(), this->TimestepValues.begin());
  this->TimestepValuesMTime.Modified();
  return (success == 1);
}

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::UpdateEntityAndFieldSelections(vtkIossReader* self)
{
  if (this->SelectionsMTime > this->DatabaseNamesMTime)
  {
    return true;
  }

  auto controller = self->GetController();
  const auto rank = controller ? controller->GetLocalProcessId() : 0;
  const auto numRanks = controller ? controller->GetNumberOfProcesses() : 1;

  // This has to be done all all ranks since not all files in a database have
  // all the blocks consequently need not have all the fields.
  std::array<std::set<vtkIossUtilities::EntityNameType>, vtkIossReader::NUMBER_OF_ENTITY_TYPES>
    entity_names;
  std::array<std::set<std::string>, vtkIossReader::NUMBER_OF_ENTITY_TYPES> field_names;

  for (const auto& pair : this->DatabaseNames)
  {
    const auto fileids = this->GetFileIds(pair.first, rank, numRanks);
    for (const auto& fileid : fileids)
    {
      if (auto region = this->GetRegion(pair.first, fileid))
      {
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_node_blocks(),
          entity_names[vtkIossReader::NODEBLOCK], field_names[vtkIossReader::NODEBLOCK]);
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_edge_blocks(),
          entity_names[vtkIossReader::EDGEBLOCK], field_names[vtkIossReader::EDGEBLOCK]);
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_face_blocks(),
          entity_names[vtkIossReader::FACEBLOCK], field_names[vtkIossReader::FACEBLOCK]);
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_element_blocks(),
          entity_names[vtkIossReader::ELEMENTBLOCK], field_names[vtkIossReader::ELEMENTBLOCK]);
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_nodesets(),
          entity_names[vtkIossReader::NODESET], field_names[vtkIossReader::NODESET]);
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_edgesets(),
          entity_names[vtkIossReader::EDGESET], field_names[vtkIossReader::EDGESET]);
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_facesets(),
          entity_names[vtkIossReader::FACESET], field_names[vtkIossReader::FACESET]);
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_elementsets(),
          entity_names[vtkIossReader::ELEMENTSET], field_names[vtkIossReader::ELEMENTSET]);
        vtkIossUtilities::GetEntityAndFieldNames(region, region->get_sidesets(),
          entity_names[vtkIossReader::SIDESET], field_names[vtkIossReader::SIDESET]);
      }
    }
  }

  if (numRanks > 1)
  {
    //// sync selections across all ranks.
    ::Synchronize(controller, entity_names, entity_names);
    ::Synchronize(controller, field_names, field_names);
  }

  // update known block/set names.
  this->EntityNames = entity_names;
  for (int cc = ENTITY_START; cc < ENTITY_END; ++cc)
  {
    auto entitySelection = self->GetEntitySelection(cc);
    for (auto& name : entity_names[cc])
    {
      entitySelection->AddArray(name.second.c_str(), vtkIossReader::GetEntityTypeIsBlock(cc));
    }

    auto fieldSelection = self->GetFieldSelection(cc);
    for (auto& name : field_names[cc])
    {
      fieldSelection->AddArray(name.c_str(), vtkIossReader::GetEntityTypeIsBlock(cc));
    }
  }
  this->SelectionsMTime.Modified();
  return true;
}

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GenerateOutput(
  vtkPartitionedDataSetCollection* output, vtkIossReader* self)
{
  // we skip NODEBLOCK since we never put out NODEBLOCK in the output by itself.
  vtkNew<vtkDataAssembly> assembly;
  assembly->SetRootNodeName("Ioss");
  output->SetDataAssembly(assembly);

  for (int etype = vtkIossReader::NODEBLOCK + 1; etype < vtkIossReader::ENTITY_END; ++etype)
  {
    std::set<std::string> enabled_entities;
    auto selection = self->GetEntitySelection(etype);
    for (int idx = 0, max = selection->GetNumberOfArrays(); idx < max; ++idx)
    {
      auto name = selection->GetArrayName(idx);
      if (name != nullptr && selection->GetArraySetting(idx) != 0)
      {
        enabled_entities.insert(name);
      }
    }

    // we delay creating a node for this entity-type until one is needed
    int entity_node = -1;

    // EntityNames are sorted by their exodus "id".
    for (const auto& ename : this->EntityNames[etype])
    {
      if (enabled_entities.find(ename.second) != enabled_entities.end())
      {
        auto pdsIdx = output->GetNumberOfPartitionedDataSets();
        vtkNew<vtkPartitionedDataSet> parts;
        output->SetPartitionedDataSet(pdsIdx, parts);
        output->GetMetaData(pdsIdx)->Set(vtkCompositeDataSet::NAME(), ename.second.c_str());
        output->GetMetaData(pdsIdx)->Set(
          vtkIossReader::ENTITY_TYPE(), etype); // save for vtkIossReader use.
        if (entity_node == -1)
        {
          entity_node =
            assembly->AddNode(vtkIossReader::GetDataAssemblyNodeNameForEntityType(etype));
        }
        auto node = assembly->AddNode(ename.second.c_str(), entity_node);
        assembly->AddDataSetIndex(node, pdsIdx);
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
Ioss::Region* vtkIossReader::vtkInternals::GetRegion(const std::string& dbasename, int fileid)
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
    // fixme: should this be configurable? it won't really work if we made it
    // configurable since our vtkDataArraySelection object would need to purged
    // and refilled.
    properties.add(Ioss::Property("FIELD_SUFFIX_SEPARATOR", ""));
    auto dbase = std::unique_ptr<Ioss::DatabaseIO>(Ioss::IOFactory::create(
      "exodusII", dbasename, Ioss::READ_RESTART, MPI_COMM_WORLD, properties));
    if (dbase == nullptr || !dbase->ok(/*write_message=*/true))
    {
      throw std::runtime_error(
        "Failed to open database " + this->GetRawFileName(DatabaseHandle{ dbasename, fileid }));
    }
    dbase->set_surface_split_type(Ioss::SPLIT_BY_TOPOLOGIES);

    // note: `Ioss::Region` constructor may throw exception.
    auto region = std::make_shared<Ioss::Region>(dbase.get());

    // release the dbase ptr since region (if created successfully) takes over
    // the ownership and calls delete on it when done.
    dbase.release();

    riter =
      this->RegionMap.insert(std::make_pair(std::make_pair(dbasename, processor), region)).first;
  }
  return riter->second.get();
}

//----------------------------------------------------------------------------
std::vector<DatabaseHandle> vtkIossReader::vtkInternals::GetDatabaseHandles(
  int piece, int npieces, int timestep) const
{
  std::string dbasename;
  if (timestep >= 0 && timestep < static_cast<int>(this->TimestepValues.size()))
  {
    const double time = this->TimestepValues[timestep];

    // find the right database in a set of restarts;
    for (const auto& pair : this->DatabaseTimes)
    {
      if (pair.second.find(time) != pair.second.end())
      {
        // if multiple databases provide the same timestep, we opt to choose
        // the one with a newer end timestep. this follows from the fact that
        // often a restart may be started after "rewinding" a bit to overcome
        // some bad timesteps.
        if (dbasename.empty() ||
          (*this->DatabaseTimes.at(dbasename).rbegin() < *pair.second.rbegin()))
        {
          dbasename = pair.first;
        }
      }
    }
  }
  else if (timestep <= 0 && this->TimestepValues.size() == 0)
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

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GetMesh(vtkUnstructuredGrid* dataset,
  const std::string& blockname, vtkIossReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, bool remove_unused_points)
{
  auto ioss_entity_type = vtkIossUtilities::GetIossEntityType(vtk_entity_type);
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

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GetTopology(vtkUnstructuredGrid* grid,
  const std::string& blockname, vtkIossReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle)
{
  auto ioss_entity_type = vtkIossUtilities::GetIossEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockname, ioss_entity_type);
  if (!group_entity)
  {
    return false;
  }

  vtkLogScopeF(TRACE, "GetTopology (%s)[file=%s]", blockname.c_str(),
    this->GetRawFileName(handle, true).c_str());
  if (ioss_entity_type == Ioss::EntityType::SIDESET)
  {
    // for side set, the topology is stored in nested elements called
    // SideBlocks. Since we split side sets by topologies, each sideblock can be
    // treated as a regular entity block.
    assert(group_entity->get_database()->get_surface_split_type() == Ioss::SPLIT_BY_TOPOLOGIES);
    std::vector<std::pair<int, vtkSmartPointer<vtkCellArray>>> sideblock_cells;
    auto sideSet = static_cast<Ioss::SideSet*>(group_entity);
    vtkIdType numCells = 0, connectivitySize = 0;
    for (auto sideBlock : sideSet->get_side_blocks())
    {
      int cell_type = VTK_EMPTY_CELL;
      auto cellarray = vtkIossUtilities::GetConnectivity(sideBlock, cell_type, &this->Cache);
      if (cellarray != nullptr && cell_type != VTK_EMPTY_CELL)
      {
        numCells += cellarray->GetNumberOfCells();
        sideblock_cells.push_back(std::make_pair(cell_type, cellarray));
      }
    }
    if (sideblock_cells.size() == 1)
    {
      grid->SetCells(sideblock_cells.front().first, sideblock_cells.front().second);
      return true;
    }
    else if (sideblock_cells.size() > 1)
    {
      // this happens when side block has mixed topological elements.
      vtkNew<vtkCellArray> appendedCellArray;
      appendedCellArray->AllocateExact(numCells, connectivitySize);
      vtkNew<vtkUnsignedCharArray> cellTypesArray;
      cellTypesArray->SetNumberOfTuples(numCells);
      auto ptr = cellTypesArray->GetPointer(0);
      for (auto& pair : sideblock_cells)
      {
        appendedCellArray->Append(pair.second);
        ptr =
          std::fill_n(ptr, pair.second->GetNumberOfCells(), static_cast<unsigned char>(pair.first));
      }
      grid->SetCells(cellTypesArray, appendedCellArray);
      return true;
    }
  }
  else
  {
    int cell_type = VTK_EMPTY_CELL;
    auto cellarray = vtkIossUtilities::GetConnectivity(group_entity, cell_type, &this->Cache);
    if (cell_type != VTK_EMPTY_CELL && cellarray != nullptr)
    {
      grid->SetCells(cell_type, cellarray);
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GetGeometry(
  vtkPointSet* grid, const std::string& blockname, const DatabaseHandle& handle)
{
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockname, Ioss::EntityType::NODEBLOCK);
  if (!group_entity)
  {
    return false;
  }

  vtkLogScopeF(TRACE, "GetGeometry(%s)[file=%s]", blockname.c_str(),
    this->GetRawFileName(handle, true).c_str());
  auto pts = vtkIossUtilities::GetMeshModelCoordinates(group_entity, &this->Cache);
  grid->SetPoints(pts);
  return true;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> vtkIossReader::vtkInternals::GetField(
  const std::string& fieldname, const std::string& blockname,
  vtkIossReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
  vtkIdTypeArray* ids_to_extract, const std::string& cache_key_suffix)
{
  const auto ioss_entity_type = vtkIossUtilities::GetIossEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle.first, handle.second);
  if (!region)
  {
    return nullptr;
  }

  auto group_entity = region->get_entity(blockname, ioss_entity_type);
  if (!group_entity)
  {
    return nullptr;
  }

  return this->GetField(
    fieldname, region, group_entity, handle, timestep, ids_to_extract, cache_key_suffix);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> vtkIossReader::vtkInternals::GetField(
  const std::string& fieldname, Ioss::Region* region, Ioss::GroupingEntity* group_entity,
  const DatabaseHandle& vtkNotUsed(handle), int timestep, vtkIdTypeArray* ids_to_extract,
  const std::string& cache_key_suffix)
{
  const auto get_field = [&fieldname, &region, &timestep, this](
                           Ioss::GroupingEntity* entity) -> vtkSmartPointer<vtkAbstractArray> {
    if (!entity->field_exists(fieldname))
    {
      return nullptr;
    }

    if (!vtkIossUtilities::IsFieldTransient(entity, fieldname))
    {
      // non-time dependent field.
      return vtkIossUtilities::GetData(entity, fieldname, /*transform=*/nullptr, &this->Cache);
    }

    // determine state for transient data.
    const auto min = region->get_min_time();
    const auto max = region->get_max_time();
    int state = -1;
    for (int cc = min.first; cc <= max.first; ++cc)
    {
      if (region->get_state_time(cc) == this->TimestepValues[timestep])
      {
        state = cc;
        break;
      }
    }
    if (state == -1)
    {
      throw std::runtime_error("Invalid timestep chosen: " + std::to_string(timestep));
    }
    region->begin_state(state);
    try
    {
      const std::string key = "__vtk_transient_" + fieldname + "_" + std::to_string(state) + "__";
      auto f =
        vtkIossUtilities::GetData(entity, fieldname, /*transform=*/nullptr, &this->Cache, key);
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
    (vtkIossUtilities::IsFieldTransient(group_entity, fieldname)
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

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GetFields(vtkDataSetAttributes* dsa,
  vtkDataArraySelection* selection, const std::string& blockname,
  vtkIossReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
  bool read_ioss_ids, vtkIdTypeArray* ids_to_extract /*=nullptr*/,
  const std::string& cache_key_suffix /*= std::string()*/)
{
  std::vector<std::string> fieldnames;
  if (read_ioss_ids)
  {
    switch (vtk_entity_type)
    {
      case vtkIossReader::NODEBLOCK:
      case vtkIossReader::EDGEBLOCK:
      case vtkIossReader::FACEBLOCK:
      case vtkIossReader::ELEMENTBLOCK:
      case vtkIossReader::NODESET:
        fieldnames.push_back("ids");
        break;

      case vtkIossReader::EDGESET:
      case vtkIossReader::FACESET:
      case vtkIossReader::ELEMENTSET:
      case vtkIossReader::SIDESET:
        fieldnames.push_back("element_side");
        break;

      default:
        break;
    }
  }
  for (int cc = 0; cc < selection->GetNumberOfArrays(); ++cc)
  {
    if (selection->GetArraySetting(cc))
    {
      fieldnames.push_back(selection->GetArrayName(cc));
    }
  }
  for (const auto& fieldname : fieldnames)
  {
    if (auto array = this->GetField(fieldname, blockname, vtk_entity_type, handle, timestep,
          ids_to_extract, cache_key_suffix))
    {
      if (fieldname == "ids")
      {
        dsa->SetGlobalIds(vtkDataArray::SafeDownCast(array));
      }
      else
      {
        dsa->AddArray(array);
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GetNodeFields(vtkDataSetAttributes* dsa,
  vtkDataArraySelection* selection, const std::string& blockname,
  vtkIossReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
  bool read_ioss_ids)
{
  const auto ioss_entity_type = vtkIossUtilities::GetIossEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle.first, handle.second);
  auto group_entity = region ? region->get_entity(blockname, ioss_entity_type) : nullptr;
  if (!group_entity)
  {
    return false;
  }

  auto& cache = this->Cache;
  auto vtk_raw_ids_array =
    vtkIdTypeArray::SafeDownCast(cache.Find(group_entity, "__vtk_mesh_original_pt_ids__"));
  const std::string cache_key_suffix = vtk_raw_ids_array != nullptr ? blockname : std::string();
  return this->GetFields(dsa, selection, "nodeblock_1", vtkIossReader::NODEBLOCK, handle, timestep,
    read_ioss_ids, vtk_raw_ids_array, cache_key_suffix);
}

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GenerateFileId(vtkUnstructuredGrid* grid,
  const std::string& blockname, vtkIossReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle)
{
  const auto ioss_entity_type = vtkIossUtilities::GetIossEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle.first, handle.second);
  auto group_entity = region ? region->get_entity(blockname, ioss_entity_type) : nullptr;
  if (!group_entity)
  {
    return false;
  }

  auto& cache = this->Cache;
  if (auto file_ids = vtkDataArray::SafeDownCast(cache.Find(group_entity, "__vtk_file_ids__")))
  {
    assert(grid->GetNumberOfCells() == file_ids->GetNumberOfTuples());
    grid->GetCellData()->AddArray(file_ids);
    return true;
  }

  vtkNew<vtkIntArray> file_ids;
  file_ids->SetName("file_id");
  file_ids->SetNumberOfTuples(grid->GetNumberOfCells());
  std::fill(
    file_ids->GetPointer(0), file_ids->GetPointer(0) + grid->GetNumberOfCells(), handle.second);
  cache.Insert(group_entity, "__vtk_file_ids__", file_ids.GetPointer());
  grid->GetCellData()->AddArray(file_ids);
  return true;
}

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::ApplyDisplacements(vtkUnstructuredGrid* grid,
  const std::string& blockname, vtkIossReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep)
{
  const auto ioss_entity_type = vtkIossUtilities::GetIossEntityType(vtk_entity_type);
  auto region = this->GetRegion(handle.first, handle.second);
  auto group_entity = region ? region->get_entity(blockname, ioss_entity_type) : nullptr;
  if (!group_entity)
  {
    return false;
  }

  auto& cache = this->Cache;
  const auto xformPtsCacheKey = "__vtk_xformed_pts_" + std::to_string(timestep);
  if (auto xformedPts = vtkPoints::SafeDownCast(cache.Find(group_entity, xformPtsCacheKey)))
  {
    assert(xformedPts->GetNumberOfPoints() == grid->GetNumberOfPoints());
    grid->SetPoints(xformedPts);
    return true;
  }

  auto displ_array_name = vtkIossUtilities::GetDisplacementFieldName(
    region->get_entity("nodeblock_1", Ioss::EntityType::NODEBLOCK));
  if (displ_array_name.empty())
  {
    return false;
  }

  auto vtk_raw_ids_array =
    vtkIdTypeArray::SafeDownCast(cache.Find(group_entity, "__vtk_mesh_original_pt_ids__"));
  const std::string cache_key_suffix = vtk_raw_ids_array != nullptr ? blockname : std::string();
  auto array = vtkDataArray::SafeDownCast(this->GetField(displ_array_name, "nodeblock_1",
    vtkIossReader::NODEBLOCK, handle, timestep, vtk_raw_ids_array, cache_key_suffix));
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
      xformedPts->SetPoint(cc, (coords + displ).GetData());
    }

    grid->SetPoints(xformedPts);
    cache.Insert(group_entity, xformPtsCacheKey, xformedPts);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GetQAAndInformationRecords(
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

//----------------------------------------------------------------------------
bool vtkIossReader::vtkInternals::GetGlobalFields(
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

//============================================================================
vtkStandardNewMacro(vtkIossReader);
vtkCxxSetObjectMacro(vtkIossReader, Controller, vtkMultiProcessController);
vtkInformationKeyMacro(vtkIossReader, ENTITY_TYPE, Integer);
//----------------------------------------------------------------------------
vtkIossReader::vtkIossReader()
  : Controller(nullptr)
  , GenerateFileId(false)
  , ScanForRelatedFiles(true)
  , ReadIds(true)
  , RemoveUnusedPoints(true)
  , ApplyDisplacements(true)
  , ReadGlobalFields(true)
  , ReadQAAndInformationRecords(true)
  , Internals(new vtkIossReader::vtkInternals())
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkIossReader::~vtkIossReader()
{
  this->SetController(nullptr);
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkIossReader::CreateDefaultExecutive()
{
  return vtkReaderExecutive::New();
}

//----------------------------------------------------------------------------
int vtkIossReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
void vtkIossReader::SetScanForRelatedFiles(bool val)
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
void vtkIossReader::SetFileName(const char* fname)
{
  auto& internals = (*this->Internals);
  if (fname == nullptr)
  {
    if (internals.FileNames.size() != 0)
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
void vtkIossReader::AddFileName(const char* fname)
{
  auto& internals = (*this->Internals);
  if (fname != nullptr && !internals.FileNames.insert(fname).second)
  {
    internals.FileNamesMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIossReader::ClearFileNames()
{
  auto& internals = (*this->Internals);
  if (internals.FileNames.size() > 0)
  {
    internals.FileNames.clear();
    internals.FileNamesMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* vtkIossReader::GetFileName(int index) const
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
int vtkIossReader::GetNumberOfFileNames() const
{
  auto& internals = (*this->Internals);
  return static_cast<int>(internals.FileNames.size());
}

//----------------------------------------------------------------------------
int vtkIossReader::ReadMetaData(vtkInformation* metadata)
{
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
    const auto timesteps = internals.GetTimeSteps();
    if (timesteps.size() > 0)
    {
      metadata->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timesteps[0],
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

  metadata->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkIossReader::ReadMesh(
  int piece, int npieces, int vtkNotUsed(nghosts), int timestep, vtkDataObject* output)
{
  auto& internals = (*this->Internals);

  if (!internals.UpdateDatabaseNames(this))
  {
    // this should not be necessary. ReadMetaData returns false when
    // `UpdateDatabaseNames` fails. At which point vtkReaderExecutive should
    // never call `RequestData` leading to a call to this method. However, it
    // does, for some reason. Hence adding this check here.
    // ref: paraview/paraview#19951.
    return 0;
  }

  // This is the first method that gets called when generating data.
  // Reset internal cache counters so we can flush fields not accessed.
  internals.ResetCacheAccessCounts();

  auto collection = vtkPartitionedDataSetCollection::SafeDownCast(output);

  // setup output based on the block/set selections (and those available in the
  // database).
  if (!internals.GenerateOutput(collection, this))
  {
    vtkErrorMacro("Failed to generate output.");
    return 0;
  }

  // dbaseHandles are handles for individual files this instance will to read to
  // satisfy the request. Can be >= 0.
  const auto dbaseHandles = internals.GetDatabaseHandles(piece, npieces, timestep);
  for (unsigned int pdsIdx = 0; pdsIdx < collection->GetNumberOfPartitionedDataSets(); ++pdsIdx)
  {
    const std::string blockname(collection->GetMetaData(pdsIdx)->Get(vtkCompositeDataSet::NAME()));
    const auto vtk_entity_type =
      static_cast<vtkIossReader::EntityType>(collection->GetMetaData(pdsIdx)->Get(ENTITY_TYPE()));

    auto pds = collection->GetPartitionedDataSet(pdsIdx);
    assert(pds != nullptr);
    pds->SetNumberOfPartitions(static_cast<unsigned int>(dbaseHandles.size()));
    for (unsigned int cc = 0; cc < static_cast<unsigned int>(dbaseHandles.size()); ++cc)
    {
      const auto& handle = dbaseHandles[cc];
      try
      {
        // TODO: make this configurable to add support for CGNS; this may
        // be a structured grid, in that case.
        vtkNew<vtkUnstructuredGrid> dataset;
        if (internals.GetMesh(
              dataset, blockname, vtk_entity_type, handle, this->RemoveUnusedPoints))
        {
          // let's read arrays.
          auto fieldSelection = this->GetFieldSelection(vtk_entity_type);
          assert(fieldSelection);
          internals.GetFields(dataset->GetCellData(), fieldSelection, blockname, vtk_entity_type,
            handle, timestep, this->ReadIds);

          auto nodeFieldSelection = this->GetNodeBlockFieldSelection();
          assert(nodeFieldSelection);
          internals.GetNodeFields(dataset->GetPointData(), nodeFieldSelection, blockname,
            vtk_entity_type, handle, timestep, this->ReadIds);

          if (this->ApplyDisplacements)
          {
            internals.ApplyDisplacements(dataset, blockname, vtk_entity_type, handle, timestep);
          }

          if (this->GenerateFileId)
          {
            internals.GenerateFileId(dataset, blockname, vtk_entity_type, handle);
          }
          pds->SetPartition(cc, dataset);
        }
      }
      catch (const std::runtime_error& e)
      {
        vtkLogF(ERROR,
          "Error reading entity block (or set) named '%s' from '%s'; skipping. Details: %s",
          blockname.c_str(), internals.GetRawFileName(handle).c_str(), e.what());
      }
    }
  }

  if (dbaseHandles.size())
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
  }

  internals.ClearCacheUnused();

  return 1;
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkIossReader::GetEntitySelection(int type)
{
  if (type < 0 || type >= NUMBER_OF_ENTITY_TYPES)
  {
    vtkErrorMacro("Invalid type '" << type
                                   << "'. Supported values are "
                                      "vtkIossReader::NODEBLOCK (0), ... vtkIossReader::SIDESET ("
                                   << vtkIossReader::SIDESET << ").");
    return nullptr;
  }
  return this->EntitySelection[type];
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkIossReader::GetFieldSelection(int type)
{
  if (type < 0 || type >= NUMBER_OF_ENTITY_TYPES)
  {
    vtkErrorMacro("Invalid type '" << type
                                   << "'. Supported values are "
                                      "vtkIossReader::NODEBLOCK (0), ... vtkIossReader::SIDESET ("
                                   << vtkIossReader::SIDESET << ").");
    return nullptr;
  }
  return this->EntityFieldSelection[type];
}

//----------------------------------------------------------------------------
vtkMTimeType vtkIossReader::GetMTime()
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
void vtkIossReader::RemoveAllEntitySelections()
{
  for (int cc = ENTITY_START; cc < ENTITY_END; ++cc)
  {
    this->GetEntitySelection(cc)->RemoveAllArrays();
  }
}

//----------------------------------------------------------------------------
void vtkIossReader::RemoveAllFieldSelections()
{
  for (int cc = ENTITY_START; cc < ENTITY_END; ++cc)
  {
    this->GetFieldSelection(cc)->RemoveAllArrays();
  }
}

//----------------------------------------------------------------------------
void vtkIossReader::SetRemoveUnusedPoints(bool val)
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
const char* vtkIossReader::GetDataAssemblyNodeNameForEntityType(int type)
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
void vtkIossReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GenerateFileId: " << this->GenerateFileId << endl;
  os << indent << "ScanForRelatedFiles: " << this->ScanForRelatedFiles << endl;
  os << indent << "ReadIds: " << this->ReadIds << endl;
  os << indent << "RemoveUnusedPoints: " << this->RemoveUnusedPoints << endl;
  os << indent << "ApplyDisplacements: " << this->ApplyDisplacements << endl;
  os << indent << "ReadGlobalFields: " << this->ReadGlobalFields << endl;
  os << indent << "ReadQAAndInformationRecords: " << this->ReadQAAndInformationRecords << endl;
  os << indent << "NodeBlockSelection: " << endl;
  this->GetNodeBlockSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeBlockSelection: " << endl;
  this->GetEdgeBlockSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "FaceBlockSelection: " << endl;
  this->GetFaceBlockSelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ElementBlockSelection: " << endl;
  this->GetElementBlockSelection()->PrintSelf(os, indent.GetNextIndent());
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
  os << indent << "NodeSetFieldSelection: " << endl;
  this->GetNodeSetFieldSelection()->PrintSelf(os, indent.GetNextIndent());
}
