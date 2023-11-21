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
  const char* GetLastError() { return this->LastError; }

  /*
   * Write version and type attributes to the root group
   * A root must be open for the operation to succeed
   * Returns wether the operation was successful
   * If the operation fails, some attributes may have been written
   */
  bool WriteHeader(const char* hdfType);

  /*
   * Open the file from the filename and create the root VTKHDF group
   * Overwrite the file if it exists by default
   * This doesn't write any attribute or dataset to the file
   * This file is not closed until another root is opened or this object is destructed
   * Returns wether the operation was successful
   * If the operation fails, the file may have been created
   */
  bool OpenRoot(bool overwrite = true);

  /*
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

  /*
   * Get the cell array for the POLY_DATA_TOPOS
   */
  std::vector<PolyDataTopos> getCellArraysForTopos(vtkPolyData* polydata);

  // Creation utility functions

  /*
   * Create a dataset in the given group with the given parameters and write data to it
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle createAndWriteHdfDataset(hid_t group, hid_t type, hid_t source_type,
    const char* name, int rank, const hsize_t dimensions[], const void* data);

  /*
   * Create a HDF dataspace
   * It is simple (not scalar or null) which means that it is an array of elements
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5SHandle createSimpleDataspace(int rank, const hsize_t dimensions[]);

  /*
   * Create a dataset in the given group from a dataspace
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle createHdfDataset(
    hid_t group, const char* name, hid_t type, hid_t dataspace);

  /*
   * Create a dataset in the given group
   * It internally creates a dataspace from a rank and dimensions
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle createHdfDataset(
    hid_t group, const char* name, hid_t type, int rank, const hsize_t dimensions[]);

  /*
   * Creates a dataspace to the exact array dimensions
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5SHandle createDataspaceFromArray(vtkAbstractArray* dataArray);
  /*
   * Creates a dataset in the given group from a dataArray and write data to it
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle createDatasetFromDataArray(
    hid_t group, const char* name, hid_t type, vtkAbstractArray* dataArray);

  /*
   * Creates a single-value dataset and write a value to it.
   * Returned scoped handle may be invalid
   */
  vtkHDF::ScopedH5DHandle createSingleValueDataset(hid_t group, const char* name, int value);

  Implementation(vtkHDFWriter* writer);
  virtual ~Implementation();

private:
  vtkHDFWriter* Writer;
  const char* LastError;
  vtkHDF::ScopedH5FHandle File;
  vtkHDF::ScopedH5GHandle Root;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHDFWriterImplementation.h
