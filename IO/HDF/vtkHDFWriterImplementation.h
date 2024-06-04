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

VTK_ABI_NAMESPACE_BEGIN

class vtkHDFWriter::Implementation
{
public:
  hid_t GetRoot() { return this->Root; }
  hid_t GetFile() { return this->File; }
  hid_t GetStepsGroup() { return this->StepsGroup; }
  const char* GetLastError() { return this->LastError; }

  /**
   * Write version and type attributes to the root group
   * A root must be open for the operation to succeed
   * Returns wether the operation was successful
   * If the operation fails, some attributes may have been written
   */
  bool WriteHeader(hid_t group, const char* hdfType);

  /**
   * Open the file from the filename and create the root VTKHDF group
   * Overwrite the file if it exists by default
   * This doesn't write any attribute or dataset to the file
   * This file is not closed until another root is opened or this object is destructed
   * Returns wether the operation was successful
   * If the operation fails, the file may have been created
   */
  bool OpenFile(bool overwrite = true);

  /**
   * Create the steps group in the root group. Set a member variable to store the group, so it can
   * be retrieved later using `GetStepsGroup` function.
   */
  bool CreateStepsGroup();

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
    const char* name, int rank, const hsize_t dimensions[], const void* data);

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
   * Create a soft link to the real group containing the block datatset.
   */
  herr_t CreateSoftLink(hid_t group, const char* groupName, const char* targetLink);

  /**
   * Create an external link to the real group containing the block datatset.
   */
  herr_t CreateExternalLink(
    hid_t group, const char* filename, const char* source, const char* targetLink);

  /**
   * Open and return an existing group thanks to id and a relative or absolute path to this group.
   */
  vtkHDF::ScopedH5GHandle OpenExistingGroup(hid_t group, const char* name);

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

  /**
   * Creates a single-value dataset and write a value to it.
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle CreateSingleValueDataset(hid_t group, const char* name, int value);

  /**
   * Create a chunked dataset with an empty extendable dataspace using chunking and set the desired
   * level of compression. Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle InitDynamicDataset(hid_t group, const char* name, hid_t type,
    hsize_t cols, hsize_t chunkSize[], int compressionLevel = 0);

  /**
   * Add a single value of integer type to an existing dataspace.
   * The trim parameter allows to overwrite the last data instead
   * of appending it to the dataset.
   * Return true if the write operation was successful.
   */
  bool AddSingleValueToDataset(hid_t dataset, int value, bool offset, bool trim = false);

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
    hid_t group, const char* name, int value, bool offset = false, bool trim = false);

  Implementation(vtkHDFWriter* writer);
  virtual ~Implementation();

private:
  vtkHDFWriter* Writer;
  const char* LastError;
  vtkHDF::ScopedH5FHandle File;
  vtkHDF::ScopedH5GHandle Root;
  vtkHDF::ScopedH5GHandle StepsGroup;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHDFWriterImplementation.h
