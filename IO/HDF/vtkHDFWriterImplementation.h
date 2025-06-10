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
#include "vtkHDFUtilities.h"
#include "vtkHDFWriter.h"
#include "vtkType.h"

#include <array>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

class vtkHDFWriter::Implementation
{
public:
  hid_t GetRoot() { return this->Root; }
  hid_t GetFile() { return this->File; }
  hid_t GetStepsGroup(hid_t currentGroup);

  /**
   * Write version and type attributes to the root group
   * A root must be open for the operation to succeed
   * Returns whether the operation was successful
   * If the operation fails, some attributes may have been written
   */
  bool WriteHeader(hid_t group, const char* hdfType);

  /**
   * Create the file from the filename and create the root VTKHDF group.
   * This file is closed on object destruction.
   * Overwrite the file if it exists by default
   * Returns true if the operation was successful
   * If the operation fails, the file may have been created
   */
  bool CreateFile(bool overwrite, const std::string& filename);

  /**
   * Open existing VTKHDF file and set Root and File members.
   * This file is closed on object destruction.
   */
  bool OpenFile();

  /**
   * Close currently handled file, open using CreateFile or OpenFile.
   * This does only need to be called when we want to close the file early; the file and open groups
   * are closed automatically on object destruction.
   */
  void CloseFile();

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

  /**
   * Create the steps group in the given group. It can be retrieved later using `GetStepsGroup`
   */
  bool CreateStepsGroup(hid_t group);

  /**
   * @struct PolyDataTopos
   * @brief Stores a group name and the corresponding cell array.
   *
   * Use this structure to avoid maintaining two arrays which is error prone (AOS instead of SOA)
   */
  typedef struct
  {
    const char* hdfGroupName;
    vtkCellArray* cellArray;
  } PolyDataTopos;

  /**
   * Get the cell array for the POLY_DATA_TOPOS
   */
  std::vector<PolyDataTopos> GetCellArraysForTopos(vtkPolyData* polydata);

  // Creation utility functions

  /**
   * Create a dataset in the given group with the given parameters and write data to it
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateAndWriteHdfDataset(hid_t group, hid_t type, hid_t source_type,
    const char* name, int rank, std::vector<hsize_t> dimensions, const void* data);

  /**
   * Create a HDF dataspace
   * It is simple (not scalar or null) which means that it is an array of elements
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5SHandle CreateSimpleDataspace(int rank, const hsize_t dimensions[]);

  /**
   * Create a scalar integer attribute in the given group.
   * Noop if the attribute already exists.
   */
  vtkHDF::ScopedH5AHandle CreateScalarAttribute(hid_t group, const char* name, int value);

  /**
   * Create an unlimited HDF dataspace with a dimension of `0 * numCols`.
   * This dataspace can be attached to a chunked dataset and extended afterwards.
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5SHandle CreateUnlimitedSimpleDataspace(hsize_t numCols);

  /**
   * Create a group in the given group from a dataspace.
   * Returned scoped handle may be invalid.
   */
  vtkHDF::ScopedH5GHandle CreateHdfGroup(hid_t group, const char* name);

  /**
   * Create a group that keeps track of link creation order
   * Returned scoped handle may be invalid.
   */
  vtkHDF::ScopedH5GHandle CreateHdfGroupWithLinkOrder(hid_t group, const char* name);

  /**
   * Create a soft link to the real group containing the block dataset.
   * Return true if the operation succeeded.
   */
  bool CreateSoftLink(hid_t group, const char* groupName, const char* targetLink);

  /**
   * Create an external link to the real group containing the block dataset.
   * Return true if the operation succeeded.
   */
  bool CreateExternalLink(
    hid_t group, const char* filename, const char* source, const char* targetLink);

  /**
   * Open and return an existing group thanks to id and a relative or absolute path to this group.
   */
  vtkHDF::ScopedH5GHandle OpenExistingGroup(hid_t group, const char* name);

  /**
   * Open and return an existing dataset using its group id and dataset name.
   */
  vtkHDF::ScopedH5DHandle OpenDataset(hid_t group, const char* name);

  /**
   * Return the name of a group given its id
   */
  std::string GetGroupName(hid_t group);

  /**
   * Create a dataset in the given group from a dataspace
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateHdfDataset(
    hid_t group, const char* name, hid_t type, hid_t dataspace);

  /**
   * Create a dataset in the given group
   * It internally creates a dataspace from a rank and dimensions
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateHdfDataset(
    hid_t group, const char* name, hid_t type, int rank, const hsize_t dimensions[]);

  /**
   * Create a virtual dataset from all the subfiles that have been added.
   * This virtual dataset references the datasets with the same name in subfiles,
   * and its first dimension is the sum of all subfiles datasets'.
   * the number of components must be the same in every subfile.
   * Return true iff the operation completed successfully
   */
  bool CreateVirtualDataset(hid_t group, const char* name, hid_t type, int numComp);

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
   * Create a chunked dataset in the given group from a dataspace.
   * Chunked datasets are used to append data iteratively
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateChunkedHdfDataset(hid_t group, const char* name, hid_t type,
    hid_t dataspace, hsize_t numCols, hsize_t chunkSize[], int compressionLevel = 0);

  /**
   * Creates a dataspace to the exact array dimensions
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5SHandle CreateDataspaceFromArray(vtkAbstractArray* dataArray);

  /**
   * Creates a dataset in the given group from a dataArray and write data to it
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateDatasetFromDataArray(
    hid_t group, const char* name, hid_t type, vtkAbstractArray* dataArray);

  ///@{
  /**
   * Creates a dataset and write a value to it.
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateSingleValueDataset(hid_t group, const char* name, vtkIdType value);
  ///@}

  /**
   * Create a chunked dataset with an empty extendable dataspace using chunking and set the desired
   * level of compression.
   * Return true if the operation was successful.
   */
  bool InitDynamicDataset(hid_t group, const char* name, hid_t type, hsize_t cols,
    hsize_t chunkSize[], int compressionLevel = 0);

  /**
   * Add a single value of integer type to an existing dataspace.
   * The trim parameter allows to overwrite the last data instead
   * of appending it to the dataset.
   * Return true if the write operation was successful.
   */
  bool AddSingleValueToDataset(hid_t dataset, vtkIdType value, bool offset, bool trim = false);

  /**
   * Add a 2D value of integer type to an existing dataspace which represents the FieldDataSize.
   * Return true if the write operation was successful.
   */
  bool AddFieldDataSizeValueToDataset(hid_t dataset, vtkIdType* value, vtkIdType size, bool offset);

  /**
   * Append a full data array at the end of an existing infinite dataspace.
   * It can also overwrite the last elements using the `trim` parameter.
   * When `trim` is positive, it will overwrite the number of array defined
   * by the parameter starting from the end of the dataset. When `trim` is non positive
   * it appends data array at the end of the dataset.
   * Return true if the write operation was successful.
   */
  bool AddArrayToDataset(hid_t dataset, vtkAbstractArray* dataArray, int trim = 0);

  /**
   * Append the given array to the dataset with the given `name`, creating it if it does not exist
   * yet. If the dataset/dataspace already exists, array types much match.
   * Return true if the operation was successful.
   */
  bool AddOrCreateDataset(hid_t group, const char* name, hid_t type, vtkAbstractArray* dataArray);

  /**
   * Append a single integer value to the dataset with name `name` in `group` group.
   * Create the dataset and dataspace if it does not exist yet.
   * When offset is true, the value written to the dataset is offset by the previous value of the
   * dataspace.
   * Return true if the operation is successful.
   */
  bool AddOrCreateSingleValueDataset(
    hid_t group, const char* name, vtkIdType value, bool offset = false, bool trim = false);

  /**
   * Append a 2D integer value to the dataset with name `FieldDataSize`.
   * Create the dataset and dataspace if it does not exist yet.
   * When offset is true, the value written to the dataset is offset by the previous value of the
   * dataspace.
   * Return true if the operation is successful.
   */
  bool AddOrCreateFieldDataSizeValueDataset(
    hid_t group, const char* name, vtkIdType* value, vtkIdType size, bool offset = false);

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
  vtkHDFWriter* Writer;
  vtkHDF::ScopedH5FHandle File;
  vtkHDF::ScopedH5GHandle Root;
  vtkHDF::ScopedH5GHandle StepsGroup;
  std::vector<vtkHDF::ScopedH5FHandle> Subfiles;
  std::vector<std::string> SubfileNames;
  bool SubFilesReady = false;

  const std::array<std::string, 4> PrimitiveNames = { { "Vertices", "Lines", "Polygons",
    "Strips" } };

  /**
   * Look into subfile `subfileId` and return the number of cells in part `part`.
   * Supports UnstructuredGrid and PolyData subfiles.
   */
  hsize_t GetNumberOfCellsSubfile(const std::string& basePath, std::size_t subfileId, hsize_t part,
    bool isPolyData, const std::string& groupName);

  /**
   * Return the digit between 0 and 4 in the order of `PrimitiveNames`,
   * representing the primitive associated to `group`.
   * Return -1 if group is not a polydata primitive group.
   */
  char GetPrimitive(hid_t group);

  /**
   * Retrieve a single value from the 1-dimensional (usually meta-data)
   * group `name` in a given subfile `subfileId`.
   * `part` indicates the line (dimension 0) offset to read in the group.
   * `primitive` is the column offset to use when reading into a 2-D meta-data array for Poly Data.
   * Unless `primitive` is specified, assume that the array is 1-D.
   */
  hsize_t GetSubfileNumberOf(const std::string& base, const std::string& qualifier,
    std::size_t subfileId, hsize_t part, char primitive = 0xff);

  std::string GetBasePath(const std::string& fullPath);

  /**
   * Return true if the given dataset exists in the given existing group.
   */
  bool DatasetAndGroupExist(const std::string& dataset, hid_t group);

  /**
   * Set `totalSize` as the the sum of the subfiles dataset's size given a path to the dataset.
   * Return false on failure (dataset does not exist on every subfile). `totalSize` value should not
   * be used in this case.
   */
  bool GetSubFilesDatasetSize(
    const std::string& datasetPath, const std::string& groupName, hsize_t& totalSize);

  // Possible indexing mode of VTKHDF datasets. See `GetDatasetIndexationMode`
  enum class IndexingMode
  {
    Points,
    Cells,
    Connectivity,
    MetaData,
    Undefined
  };

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
