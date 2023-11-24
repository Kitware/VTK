// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHDFWriter
 * @brief   Write a vtkPolyData into VTKHDF file.
 *
 */

#ifndef vtkHDFWriter_h
#define vtkHDFWriter_h

#include "vtkIOHDFModule.h" // For export macro
#include "vtkWriter.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class vtkUnstructuredGrid;
class vtkPointSet;
class vtkDataSet;
class vtkCellArray;

typedef int64_t hid_t;

/**
 * Writes Dataset input to the VTK HDF format. Currently only
 * supports serial processing and a single time step of vtkPolyData or vtkUnstructuredGrid
 *
 * File format specification is here:
 * https://docs.vtk.org/en/latest/design_documents/VTKFileFormats.html#hdf-file-formats
 *
 */
class VTKIOHDF_EXPORT vtkHDFWriter : public vtkWriter
{

private:
  vtkHDFWriter(const vtkHDFWriter&) = delete;
  void operator=(const vtkHDFWriter&) = delete;

public:
  static vtkHDFWriter* New();
  vtkTypeMacro(vtkHDFWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the file name of the vtkHDF file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Get/set the flag to Overwrite the file if true or fail when the file already exists if false.
   * Default is true.
   */
  vtkSetMacro(Overwrite, bool);
  vtkGetMacro(Overwrite, bool);

  /*
   * Write the dataset from the input in the file specified by the filename to the vtkHDF format.
   */
  void WriteData() override;

protected:
  int FillInputPortInformation(int port, vtkInformation* info) override;
  vtkHDFWriter();
  ~vtkHDFWriter() override;

private:
  class Implementation;
  std::unique_ptr<Implementation> Impl;
  char* FileName = nullptr;
  bool Overwrite = true;

  ///@{
  /**
   * Write the data to the current FileName in vtkHDF format.
   * returns true if the writing operation completes successfully.
   */
  bool WriteDatasetToFile(vtkPolyData* input);
  bool WriteDatasetToFile(vtkUnstructuredGrid* input);
  ///@}

  /**
   * Add the number of points to the file
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendNumberOfPoints(hid_t group, vtkPointSet* input);

  /**
   * Add the points of the point set to the file
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendPoints(hid_t group, vtkPointSet* input);

  /**
   * Add the number of cells to the file.
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendNumberOfCells(hid_t group, vtkCellArray* input);

  /**
   * Add the number of connectivity Ids to the file.
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendNumberOfConnectivityIds(hid_t group, vtkCellArray* input);

  /**
   * Add the unstrucutred grid cell types to the file.
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendCellTypes(hid_t group, vtkUnstructuredGrid* input);

  /**
   * Add the offsets to the file.
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendOffsets(hid_t group, vtkCellArray* input);

  /**
   * Add the connectivity array to the file.
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendConnectivity(hid_t group, vtkCellArray* input);

  /**
   * Add the cells of the polydata to the file
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendPrimitiveCells(hid_t baseGroup, vtkPolyData* input);

  /**
   * Add the data arrays of the object to the file
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendDataArrays(hid_t group, vtkDataObject* input);
};
VTK_ABI_NAMESPACE_END
#endif
