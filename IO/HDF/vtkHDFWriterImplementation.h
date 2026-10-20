// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHDFWriterImplementation
 * @brief   Implementation class for vtkHDFWriter
 *
 * Opens, closes and writes information to a VTK HDF file.
 */

#ifndef vtkHDFWriterImplementation_h
#define vtkHDFWriterImplementation_h

#include "vtkAbstractArray.h"
#include "vtkCellArray.h"
#include "vtkHDF5ScopedHandle.h"
#include "vtkHDFWriter.h"
#include "vtkType.h"
#include "vtk_hdf5.h"

#include <array>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

class vtkHDFWriter::Implementation
{
public:
  ///@{
  /**
   * Create/Open/Close file.
   * Set File and Root groups, close them when the file is closed.
   */
  bool CreateFile(bool overwrite, const std::string& filename);
  bool OpenFile(const std::string& filename);
  void CloseFile();
  ///@}

  /**
   * Access root group.
   */
  hid_t GetRoot() { return this->Root; }

  ///@{
  /**
   * Create/Get the steps group in the given group.
   */
  bool CreateStepsGroup(hid_t group);
  hid_t GetStepsGroup(hid_t currentGroup);
  ///@}

  ///@{
  /**
   * Global properties
   */
  void SetChunkSize(int chunkSize) { this->ChunkSize = chunkSize; };
  void SetCompressionLevel(int level) { this->CompressionLevel = level; };
  ///@}

  ///@{
  /**
   * Use distributed HDF5 MPI I/O for writing.
   * When enabled, all operations will be done collectively.
   * GetUseDistributedWriting will always return false if HDF5_ENABLE_PARALLEL is not enabled at
   * compile time
   */
  void SetUseDistributedWriting(bool distributedFile);
  bool GetUseDistributedWriting() const { return this->UseDistributedWriting; }
  ///@}

  /**
   * Write version and type attributes to the root group
   * A root must be open for the operation to succeed
   * Returns whether the operation was successful
   * If the operation fails, some attributes may have been written
   */
  bool WriteHeader(hid_t group, const char* hdfType);

  //--------------- Distributed ---------------
  /**
   * Collect offsets from all distributed partitions in the object using collective operartions.
   */
  bool CollectPartitionOffsets(vtkDataObject* input);

  ///@{
  /**
   * Open subfile where data has already been written, and needs to be referenced by the main file
   * using virtual datasets.
   * Return false if the subfile cannot be opened.
   */
  bool OpenSubfile(const std::string& filename);

  ///@{
  /**
   * Inform the implementation that all the data has been written in subfiles,
   * and that the virtual datasets can now be created from them.
   * This mechanism is used when writing a meta-file for temporal and/or multi-piece data.
   */
  void SetSubFilesReady(bool status) { this->SubFilesReady = status; }
  bool GetSubFilesReady() { return this->SubFilesReady; }
  ///@}

  //--------------- Attributes ---------------
  /**
   * Create a scalar integer attribute in the given group.
   * Noop if the attribute already exists.
   */
  vtkHDF::ScopedH5AHandle CreateScalarAttribute(hid_t group, const char* name, int value);

  /**
   * Create a vector attribute (1D array) with arbitrary type and size in the given group.
   * Noop if the attribute already exists.
   * Returned scoped handle may be invalid.
   */
  vtkHDF::ScopedH5AHandle CreateVectorAttribute(
    hid_t group, const char* name, hid_t type, hsize_t size, const void* data);

  /**
   * Create a string attribute in the given group.
   * Noop if the attribute already exists.
   */
  vtkHDF::ScopedH5AHandle CreateStringAttribute(
    hid_t group, const char* name, const std::string& value);

  //--------------- Groups ---------------
  /**
   * Retrieve group if it exists, create it if needed.
   * Returned scoped handle may be invalid when group could not be created
   */
  vtkHDF::ScopedH5GHandle CreateHdfGroup(hid_t group, const char* name);

  /**
   * Retrieve or create a group that keeps track of link creation order
   * Returned scoped handle may be invalid when group could not be created
   */
  vtkHDF::ScopedH5GHandle CreateHdfGroupWithLinkOrder(hid_t group, const char* name);

  /**
   * Open and return an existing group thanks to id and a relative or absolute path to this group.
   */
  vtkHDF::ScopedH5GHandle OpenExistingGroup(hid_t group, const char* name);

  /**
   * Return the name of a group given its id
   */
  std::string GetGroupName(hid_t group);

  //--------------- Links ---------------
  /**
   * Create a soft link to the real group containing the block dataset.
   * Return true if the operation succeeded or if it already exists.
   */
  bool CreateSoftLink(hid_t group, const char* groupName, const char* targetLink);

  /**
   * Create an external link to the real group containing the block dataset.
   * Return true if the operation succeeded.
   */
  bool CreateExternalLink(
    hid_t group, const char* filename, const char* source, const char* targetLink);

  //--------------- Datasets ---------------
  ///@{
  /**
   * Create a chunked dataset with an empty extendable dataspace using chunking
   * Return true if the operation was successful.
   */
  bool InitDynamicDataset(
    hid_t group, const char* name, hid_t type, hsize_t cols, const std::vector<hsize_t>& chunkSize);
  bool InitDynamicDataset(hid_t group, const char* name, hid_t type,
    const std::vector<hsize_t>& dims, const std::vector<hsize_t>& chunkSize);
  ///@}

  /**
   * Open and return an existing dataset using its group id and dataset name.
   */
  vtkHDF::ScopedH5DHandle OpenDataset(hid_t group, const char* name);

  /**
   * Get size of 1D dataset. Return 0 if it does not exist.
   *
   * @warning: expects a 1D dataset.
   */
  hsize_t GetDataSetSize(hid_t group, const char* name);

  //--------------- Data writing ---------------
  /**
   * Creates a dataset in the given group from a dataArray and write data to it
   * dims provides the dimensions of the structured dataset. If dims is empty, the dataspace shape
   * is [nTuples] (or [nTuples, nComp] when nComp > 1). Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateHdfDatasetFromDataArray(hid_t group, const char* name, hid_t type,
    vtkAbstractArray* dataArray, const std::vector<hsize_t>& dims = {});

  /**
   * Append the given array to the dataset with the given `name`, creating it if it does not exist
   * yet. If the dataset/dataspace already exists, array types must match.
   * dims provides the dimensions of the structured dataset. Used only for ImageData for now.
   * Return true if the operation was successful.
   */
  bool AddOrCreateDataset(hid_t group, unsigned int partId, const char* name, hid_t type,
    vtkAbstractArray* dataArray, const std::vector<hsize_t>& dims = {});

  /**
   * Append a single row of integer values to the dataset with name `name` in `group` group.
   * Create the dataset and dataspace if it does not exist yet.
   * When offset is true, the value written to the dataset is offset by the previous value of the
   * dataspace.
   * Return true if the operation is successful.
   */
  bool AddOrCreateSingleRowDataset(hid_t group, unsigned int partId, const char* name,
    const std::vector<vtkIdType>& value, bool offset = false, bool trim = false);

  //--------------- Subfile tools ---------------
  ///@{
  /**
   * For temporal multi-piece meta-files, write the dataset `name` in group `group`,
   * which must be the "steps" group or a child of it as the running sum of all registered sub-files
   * datasets in the same location.
   * The `PolyData` version does the same operation in 2 dimensions, for offsets array of size
   * nbTimeSteps*nbPrimitives.
   */
  bool WriteSumSteps(hid_t group, const char* name);
  bool WriteSumStepsPolyData(hid_t group, const char* name);
  ///@}

  /**
   * Find the first non null part for the given path in all subfiles.
   */
  vtkHDF::ScopedH5GHandle GetSubfileNonNullPart(const std::string& blockPath, int& type);

  /**
   * Initialize empty data object array structures from a base group.
   * Used to get meta information for composite subfiles when all subfiles do not have non-null
   * data.
   */
  void CreateArraysFromNonNullPart(hid_t group, vtkDataObject* data);

  Implementation(vtkHDFWriter* writer);
  virtual ~Implementation();

private:
  /**
   * Store write offsets for a partition in the current time step.
   * This is used to allocate HDF5 dataset sizes and collective writes.
   */
  struct UnstructuredGridOffset
  {
    vtkIdType PartOffset;
    vtkIdType PointOffset;
    vtkIdType CellOffset;

    vtkIdType PointDataOffset;
    vtkIdType CellDataOffset;
    vtkIdType ConnectivityOffset;

    // Polyhedron support (to be completed)
    vtkIdType FaceOffset;
    vtkIdType FacePartOffset;
    vtkIdType FaceConnectivityOffset;
    vtkIdType PolyhedronOffset;
    vtkIdType PolyhedronToFaceOffset;
  };
  struct PartitionedOffsets
  {
    std::vector<vtkIdType> PartOffsets;            // size = nRanks + 1
    std::vector<UnstructuredGridOffset> UgOffsets; // size = nParts + 1
  };
  /**
   * Collect partitioned information for the current time step, append to existing temporal data
   */
  bool CollectPartitionOffsets(vtkDataObject* input, const UnstructuredGridOffset& temporalOffsets);

  /**
   * Print offsets as debug info.
   */
  void PrintOffsetInformation(PartitionedOffsets& offsets);

  /**
   * Return the dataset transfer property list to use for reads and writes.
   * In distributed single-file mode, use collective operations.
   */
  hid_t GetTransferPropertyList();

  bool UseDistributedWriting = false;
  vtkHDFWriter* Writer;
  vtkHDF::ScopedH5FHandle File;
  vtkHDF::ScopedH5GHandle Root;
  vtkHDF::ScopedH5GHandle StepsGroup;
  std::vector<vtkHDF::ScopedH5FHandle> Subfiles;
  std::vector<std::string> SubfileNames;
  bool SubFilesReady = false;
  vtkHDF::ScopedH5PHandle TransferPropertyList;
  PartitionedOffsets UgOffsets;

  // Forwarded constant properties from the writer
  int ChunkSize = 25000;
  int CompressionLevel = 0;

  //--------------- Datasets ---------------
  ///@{
  /**
   * Create a dataset in the given group from a dataspace or dimensions
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateHdfDataset(
    hid_t group, const char* name, hid_t type, hid_t dataspace);
  vtkHDF::ScopedH5DHandle CreateHdfDataset(
    hid_t group, const char* name, hid_t type, const std::vector<hsize_t>& dimensions);

  /**
   * Create a HDF dataspace
   * It is simple (not scalar or null) which means that it is an array of elements
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5SHandle CreateSimpleDataspace(const std::vector<hsize_t>& dimensions);

  /**
   * Creates a dataspace to the exact array dimensions
   * dims provides the dimensions of the structured dataset. If dims is empty, the dataspace shape
   * is [nTuples] (or [nTuples, nComp] when nComp > 1). Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5SHandle CreateDataspaceFromArray(
    vtkAbstractArray* dataArray, const std::vector<hsize_t>& dims = {});

  /**
   * Set a single row of the dataset
   * Return true if the write operation was successful.
   */
  bool WriteOneRowDataset(hid_t dataset, const std::vector<vtkIdType>& value, hsize_t rowOffset,
    bool offset, bool trim = false);

  /**
   * Write a contiguous buffer into an existing dataset selection, without resizing.
   */
  bool WriteDataset(hid_t dataset, hid_t memoryType, const std::vector<hsize_t>& start,
    const std::vector<hsize_t>& count, const void* data);

  /**
   * Append a full data array at the end of an existing infinite dataspace.
   * It can also overwrite the last elements using the `trim` parameter.
   * When `trim` is positive, it will overwrite the number of array defined
   * by the parameter starting from the end of the dataset. When `trim` is non positive
   * it appends data array at the end of the dataset.
   * Return true if the write operation was successful.
   */
  bool AddArrayToDataset(hid_t dataset, vtkAbstractArray* dataArray, hsize_t rowOffset,
    const std::vector<hsize_t>& dims = {});

  /**
   * Return true if the given dataset exists in the given existing group.
   */
  bool DatasetAndGroupExist(const std::string& dataset, hid_t group);

  //--------------- Meta-file tools ---------------
  /**
   * Look into subfile `subfileId` and return the number of cells in part `part`.
   * Supports UnstructuredGrid and PolyData subfiles.
   */
  hsize_t GetNumberOfCellsSubfile(const std::string& basePath, std::size_t subfileId, hsize_t part,
    bool isPolyData, const std::string& groupName);

  /**
   * Retrieve a single value from the 1-dimensional (usually meta-data)
   * group `name` in a given subfile `subfileId`.
   * `part` indicates the line (dimension 0) offset to read in the group.
   * `primitive` is the column offset to use when reading into a 2-D meta-data array for Poly Data.
   * Unless `primitive` is specified, assume that the array is 1-D.
   */
  hsize_t GetSubfileNumberOf(const std::string& base, const std::string_view& qualifier,
    std::size_t subfileId, hsize_t part, char primitive = 0xff);

  /**
   * Create a virtual dataset from all the subfiles that have been added.
   * This virtual dataset references the datasets with the same name in subfiles,
   * and its first dimension is the sum of all subfiles datasets'.
   * the number of components must be the same in every subfile.
   * Return true iff the operation completed successfully
   */
  bool CreateVirtualDataset(hid_t group, const char* name, hid_t type, int numComp);
  /**
   * Set `totalSize` as the the sum of the subfiles dataset's size given a path to the dataset.
   * Return false on failure (dataset does not exist on every subfile). `totalSize` value should not
   * be used in this case.
   */
  bool GetSubFilesDatasetSize(
    const std::string& datasetPath, const std::string& groupName, hsize_t& totalSize);

  /**
   * Copy the datatype from dataset in subfile
   */
  vtkHDF::ScopedH5THandle GetSubfileDataType(const std::string& datasetPath);

  //--------------- Distributed tools ---------------
  // Possible indexing mode of VTKHDF datasets. See `GetDatasetIndexationMode`
  enum class IndexingMode
  {
    Points,
    Cells,
    Connectivity,
    TreeDepths,
    Trees,
    Descriptor,
    HTGCoords,
    MetaData,
    Undefined,
  };

  /**
   * Return the total number of rows for the given dataset.
   * This is computed using collected offsets from all ranks.
   */
  hsize_t GetDistributedDatasetSize(const std::string& datasetPath);

  /**
   * Resize an existing dataset using local rows or gathered distributed totals.
   */
  vtkHDF::ScopedH5DHandle AllocateDataset(
    hid_t group, const char* name, std::vector<hsize_t> dimensions);

  /**
   * Create a dataset and allocate it
   */
  vtkHDF::ScopedH5DHandle CreateAndAllocateDataSet(hid_t group, const char* name, hid_t type,
    unsigned int partId, const std::vector<hsize_t>& dimensions,
    const std::vector<hsize_t>& chunkSize, hsize_t& rowOffset, bool trim = false);

  /**
   * Return true if a given path can use distributed offsets and writing. Temporary utility while
   * everything does not support distributed writing.
   */
  bool UsesDistributedOffsets(const std::string& path);

  /**
   * Return the write offset for a given part of a dataset, writing <localRows> rows
   */
  hsize_t GetDatasetWriteOffset(
    hid_t dataset, unsigned int partId, hsize_t localRows, bool trim = false);

  /**
   * Get offset value from a file-absolute partition number and path, using computed offsets from
   * the collective offset pass.
   */
  hsize_t GetDatasetValueOffset(
    std::size_t partition, const std::string& datasetPath, IndexingMode mode) const;

  /**
   * Return the indexation mode of dataset at the given path: when the dataset adds 1 component for
   * every new time step or part, return `Single`. If we add a number of values equivalent to the
   * number of points of the dataset every step/part, return `Points`. The same goes for `Cells` and
   * `Connectivity`. This is used when creating virtual datasets from different parts, to know how
   * to interleave virtual mappings.
   */
  IndexingMode GetDatasetIndexationMode(const std::string& path);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHDFWriterImplementation.h
