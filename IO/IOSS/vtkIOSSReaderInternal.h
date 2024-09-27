// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIOSSReader.h"    // For enums
#include "vtkIOSSUtilities.h" // For enums

#include <vtk_ioss.h>

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

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkCellData;
class vtkDataAssembly;
class vtkDataSetAttributes;
class vtkFieldData;
class vtkIdTypeArray;
class vtkPartitionedDataSetCollection;
class vtkPointData;
class vtkPointSet;
class vtkStructuredGrid;
class vtkUnsignedCharArray;
class vtkUnstructuredGrid;

struct DatabasePartitionInfo
{
  int ProcessCount = 0;
  std::set<int> Ranks;

  bool operator==(const DatabasePartitionInfo& other) const
  {
    return this->ProcessCount == other.ProcessCount && this->Ranks == other.Ranks;
  }
};

// Opaque handle used to identify a specific Region
using DatabaseHandle = std::pair<std::string, int>;

/**
 * @class vtkIOSSReaderInternal
 * @brief Internal methods and state for the IOSS reader.
 *
 * Note that this class is not part of the public API of VTK and thus
 * has no export macros. It has been put in a separate file so that a
 * subclass of the reader local to this module (vtkIOSSCellGridReader)
 * can access it and so it can be subclassed.
 */
class vtkIOSSReaderInternal
{
protected:
  // It's okay to instantiate this multiple times.
  Ioss::Init::Initializer io;

  double DisplacementMagnitude = 1.;

  using DatabaseNamesType = std::map<std::string, DatabasePartitionInfo>;
  DatabaseNamesType UnfilteredDatabaseNames;
  DatabaseNamesType DatabaseNames;
  vtkTimeStamp DatabaseNamesMTime;

  std::map<std::string, std::vector<std::pair<int, double>>> DatabaseTimes;
  std::vector<double> TimestepValues;
  vtkTimeStamp TimestepValuesMTime;

  // a collection of names for blocks and sets in the file(s).
  std::array<std::set<vtkIOSSUtilities::EntityNameType>, vtkIOSSReader::NUMBER_OF_ENTITY_TYPES>
    EntityNames;
  vtkTimeStamp SelectionsMTime;

  // Keeps track of idx of a partitioned dataset in the output.
  std::map<std::pair<Ioss::EntityType, std::string>, unsigned int> DatasetIndexMap;

  std::map<DatabaseHandle, std::shared_ptr<Ioss::Region>> RegionMap;

  vtkIOSSUtilities::Cache Cache;

  vtkIOSSUtilities::DatabaseFormatType Format = vtkIOSSUtilities::DatabaseFormatType::UNKNOWN;
  vtkIOSSReader* IOSSReader = nullptr;

  vtkSmartPointer<vtkDataAssembly> Assembly;
  vtkTimeStamp AssemblyMTime;

public:
  using EntityType = vtkIOSSReader::EntityType;

  vtkIOSSReaderInternal(vtkIOSSReader* reader)
    : IOSSReader(reader)
  {
  }
  virtual ~vtkIOSSReaderInternal() = default; // Force polymorphism

  Ioss::PropertyManager DatabaseProperties;
  std::set<std::string> FileNames;
  vtkTimeStamp FileNamesMTime;

  std::set<std::string> Selectors;

  const std::vector<double>& GetTimeSteps() const { return this->TimestepValues; }
  vtkIOSSUtilities::DatabaseFormatType GetFormat() const { return this->Format; }

  void SetDisplacementMagnitude(double s) { this->DisplacementMagnitude = s; }
  double GetDisplacementMagnitude() { return this->DisplacementMagnitude; }

  ///@{
  /**
   * Cache related API.
   */
  void ClearCache() { this->Cache.Clear(); }
  void ResetCacheAccessCounts() { this->Cache.ResetAccessCounts(); }
  void ClearCacheUnused() { this->Cache.ClearUnused(); }
  ///@}

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
  bool UpdateDatabaseNames(vtkIOSSReader* self);

  /**
   * Read Ioss databases to generate information about timesteps / times
   * in the databases.
   *
   * This is called after successful call to `UpdateDatabaseNames` which should
   * populate the list of Ioss databases. This method iterates over all
   * databases and gathers information about timesteps available in those
   * databases. When running in parallel, only the root node opens the Ioss
   * databases and reads the time information. That information is then
   * exchanged with all ranks thus at the end of this method all ranks should
   * have their time information updated.
   *
   * @returns `false` on failure.
   */
  bool UpdateTimeInformation(vtkIOSSReader* self);

  /**
   * Checks if the entity and field selections have changed.
   */
  bool NeedToUpdateEntityAndFieldSelections(
    vtkIOSSReader* self, const std::vector<DatabaseHandle>& dbaseHandles);

  /**
   * Populates various `vtkDataArraySelection` objects on the vtkIOSSReader with
   * names for entity-blocks, -sets, and fields defined on them.
   */
  bool UpdateEntityAndFieldSelections(vtkIOSSReader* self);

  /**
   * Populates the vtkDataAssembly used for block/set selection.
   */
  bool UpdateAssembly(vtkIOSSReader* self, int* tag);

  vtkDataAssembly* GetAssembly() const;

  /**
   * Fills up the output data-structure based on the entity blocks/sets chosen
   * and those available.
   */
  bool GenerateOutput(vtkPartitionedDataSetCollection* output, vtkIOSSReader* self);

  /**
   * Fills up the vtkDataAssembly with ioss-assemblies, if present.
   */
  bool ReadAssemblies(vtkPartitionedDataSetCollection* output, const DatabaseHandle& handle);

  /**
   * Reads datasets (meshes and fields) for the given block.
   */
  std::vector<vtkSmartPointer<vtkDataSet>> GetDataSets(const std::string& blockname,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
    vtkIOSSReader* self);

  /**
   * Reads datasets (meshes and fields) for the given exodus entity.
   *
   * This method is only invoked when MergeExodusEntityBlocks is true
   * (which is not the default).
   */
  vtkSmartPointer<vtkDataSet> GetExodusEntityDataSet(const std::vector<std::string>& blockNames,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
    vtkIOSSReader* self);

  /**
   * Read quality assurance and information data from the file.
   */
  bool GetQAAndInformationRecords(vtkFieldData* fd, const DatabaseHandle& handle);

  /**
   * Read global fields.
   */
  bool GetGlobalFields(vtkFieldData* fd, const DatabaseHandle& handle, int timestep);

  /**
   * Get if there are restart files available.
   */
  bool HaveRestartFiles() const { return this->DatabaseTimes.size() > 1; }

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
    auto dbasename = shortname ? vtksys::SystemTools::GetFilenameName(handle.first) : handle.first;

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

  /**
   * Returns if the given database handles have regions already created.
   */
  bool HaveCreatedRegions(const std::vector<DatabaseHandle>& dbaseHandles)
  {
    if (this->RegionMap.empty())
    {
      return false;
    }
    const bool allHandlesAreNew = std::all_of(dbaseHandles.begin(), dbaseHandles.end(),
      [&](const DatabaseHandle& handle)
      { return this->RegionMap.find(handle) == this->RegionMap.end(); });
    return !allHandlesAreNew;
  }

  /**
   * Releases any open file handles.
   */
  void ReleaseHandles()
  {
    // RegionMap is where all the handles are kept. All we need to do is release
    // them.
    for (const auto& pair : this->RegionMap)
    {
      pair.second->get_database()->closeDatabase();
    }
  }

  /**
   * Little more aggressive than `ReleaseHandles` but less intense than `Reset`,
   * releases all IOSS regions and thus all the meta-data IOSS may have cached
   * as well.
   */
  void ReleaseRegions() { this->RegionMap.clear(); }

  /**
   * Clear all regions, databases etc.
   */
  void Reset()
  {
    this->Cache.Clear();
    this->RegionMap.clear();
    this->DatabaseNames.clear();
    this->IOSSReader->RemoveAllSelections();
    this->DatabaseNamesMTime = vtkTimeStamp();
    this->SelectionsMTime = vtkTimeStamp();
    this->TimestepValuesMTime = vtkTimeStamp();
  }

  void ResetDatabaseNamesMTime() { this->DatabaseNamesMTime = vtkTimeStamp(); }

protected:
  std::vector<int> GetFileIds(const std::string& dbasename, int myrank, int numRanks) const;
  Ioss::Region* GetRegion(const std::string& dbasename, int fileid);
  Ioss::Region* GetRegion(const DatabaseHandle& handle)
  {
    return this->GetRegion(handle.first, handle.second);
  }

  ///@{
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
  vtkSmartPointer<vtkAbstractArray> GetField(const std::string& fieldname, Ioss::Region* region,
    const Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle, int timestep,
    vtkIdTypeArray* ids_to_extract = nullptr, const std::string& cache_key_suffix = std::string());
  ///@}

  /**
   * Get a vector of cell arrays and their cell type for the entity block (or set) with the
   * given name (`blockname`) and type (vtk_entity_type).
   *
   * `handle` is the database / file handle for the current piece / rank
   * obtained by calling `GetDatabaseHandles`.
   *
   * Returns a vector of cell arrays and their cell type.
   *
   * On file reading error, `std::runtime_error` is thrown.
   */
  std::vector<std::pair<int, vtkSmartPointer<vtkCellArray>>> GetTopology(
    const std::string& blockname, vtkIOSSReader::EntityType vtk_entity_type,
    const DatabaseHandle& handle);

  /**
   * Combine a vector cell types, cell arrays pairs into a single
   * vtkUnsignedCharArray of cell types and a vtkCellArray.
   */
  std::pair<vtkSmartPointer<vtkUnsignedCharArray>, vtkSmartPointer<vtkCellArray>> CombineTopologies(
    const std::vector<std::pair<int, vtkSmartPointer<vtkCellArray>>>& topologies);

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
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle);

  /**
   * Get with point coordinates aka geometry read from the block
   * with the given name (`blockname`). The point coordinates are always
   * read from a block of type NODEBLOCK.
   *
   * `handle` is the database / file handle for the current piece / rank
   * obtained by calling `GetDatabaseHandles`.
   *
   * Returns points on success.
   *
   * On file reading error, `std::runtime_error` is thrown.
   */
  vtkSmartPointer<vtkPoints> GetGeometry(
    const std::string& blockname, const DatabaseHandle& handle);

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
  bool GetGeometry(
    vtkUnstructuredGrid* grid, const std::string& blockname, const DatabaseHandle& handle);

  /**
   * GetGeometry for vtkStructuredGrid i.e. CGNS.
   */
  bool GetGeometry(vtkStructuredGrid* grid, const Ioss::StructuredBlock* groupEntity);

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
   *
   * This method is only invoked when MergeExodusEntityBlocks is false
   * (which is the default).
   */
  bool GetMesh(vtkUnstructuredGrid* grid, const std::string& blockname,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle,
    bool remove_unused_points);

  /**
   * Adds geometry (points) and topology (cell) information to the grid for all the
   * entity blocks or sets chosen using the names (`blockNames`) and type
   * (`vtk_entity_type`).
   *
   * `handle` is the database / file handle for the current piece / rank
   * obtained by calling `GetDatabaseHandles`.
   *
   * This method is only invoked when MergeExodusEntityBlocks is true
   * (which is not the default).
   */
  bool GetEntityMesh(vtkUnstructuredGrid* grid, const std::vector<std::string>& blockNames,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle);

  /**
   * Reads a structured block. vtk_entity_type must be
   * `vtkIOSSReader::STRUCTUREDBLOCK`.
   *
   * This method is only invoked when MergeExodusEntityBlocks is false
   * (which is the default).
   */
  bool GetMesh(vtkStructuredGrid* grid, const std::string& blockname,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle);

  /**
   * Add "id" array to the dataset using the id for the grouping entity, if
   * any. The array named "object_id" is added as a cell-data array to follow
   * the pattern used by vtkExodusIIReader.
   */
  bool GenerateEntityIdArray(vtkCellData* cd, vtkIdType numberOfCells, const std::string& blockname,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle);

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
  bool GetFields(vtkDataSetAttributes* dsa, vtkDataArraySelection* selection, Ioss::Region* region,
    Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle, int timestep,
    bool read_ioss_ids, vtkIdTypeArray* ids_to_extract = nullptr,
    const std::string& cache_key_suffix = std::string());

  /**
   * This reads node fields for an entity block or set.
   *
   * Internally calls `GetFields()` with correct values for `ids_to_extract` and
   * `cache_key_suffix`.
   *
   */
  bool GetNodeFields(vtkDataSetAttributes* dsa, vtkDataArraySelection* selection,
    Ioss::Region* region, Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle,
    int timestep, bool read_ioss_ids, bool mergeExodusEntityBlocks = false);

  /**
   * Reads node block array with displacements and then transforms
   * the points in the grid using those displacements.
   */
  bool ApplyDisplacements(vtkPointSet* grid, Ioss::Region* region,
    Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle, int timestep,
    bool mergeExodusEntityBlocks = false);

  /**
   * Adds 'file_id' array to indicate which file the dataset was read from.
   */
  bool GenerateFileId(vtkDataSetAttributes* cellData, vtkIdType numberOfCells,
    Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle);

  /**
   * Fields like "ids" have to be vtkIdTypeArray in VTK. This method does the
   * conversion if needed.
   */
  vtkSmartPointer<vtkAbstractArray> ConvertFieldForVTK(vtkAbstractArray* array);

  unsigned int GetDataSetIndexForEntity(const Ioss::GroupingEntity* entity) const
  {
    return this->DatasetIndexMap.at(std::make_pair(entity->type(), entity->name()));
  }

  /// Add field-data arrays holding side-set specifications
  /// (i.e., (cell-id, side-id) tuples) for use by the
  /// UnstructuredGridToCellGrid conversion filter.
  void GenerateElementAndSideIds(vtkDataSet* dataset, Ioss::SideSet* sideSet,
    const DatabaseHandle& handle, const std::string& blockname,
    vtkIOSSReader::EntityType vtk_entity_type);

  ///@{
  /**
   * Called by `GetDataSets` to process each type of dataset.
   * There's slight difference in how they are handled and hence these separate methods.
   */
  std::vector<vtkSmartPointer<vtkDataSet>> GetExodusDataSets(const std::string& blockname,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
    vtkIOSSReader* self);

  std::vector<vtkSmartPointer<vtkDataSet>> GetCGNSDataSets(const std::string& blockname,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
    vtkIOSSReader* self);
  ///@}

  bool BuildAssembly(Ioss::Region* region, vtkDataAssembly* assembly, int root, bool add_leaves);

  /**
   * Generate a subset based the readers current settings for FileRange and
   * FileStride.
   */
  DatabaseNamesType GenerateSubset(const DatabaseNamesType& databases, vtkIOSSReader* self);
};

VTK_ABI_NAMESPACE_END
