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
  ///@}

  ///@{
  /**
   * Get/set the flag to write all timesteps from the input dataset.
   * When turned OFF, only write the first timestep.
   */
  vtkSetMacro(WriteAllTimeSteps, bool);
  vtkGetMacro(WriteAllTimeSteps, bool);
  ///@}

  ///@{
  /**
   * Configurable chunk size for transient (time-dependent) data, where arrays resized every
   * timestep, hence requiring chunking. Read more about chunks and chunk size here :
   * https://support.hdfgroup.org/HDF5/doc/Advanced/Chunking/
   * Defaults to 100.
   */
  vtkSetMacro(ChunkSize, int);
  vtkGetMacro(ChunkSize, int);
  ///@}

  /**
   * Write the dataset from the input in the file specified by the filename to the vtkHDF format.
   */
  void WriteData() override;

protected:
  /**
   * Override vtkWriter's ProcessRequest method, in order to dispatch the request
   * not only to RequestData as vtkWriter does, but to RequestInformation and RequestUpdateExtent as
   * well to handle timesteps properly.
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkHDFWriter();
  ~vtkHDFWriter() override;

private:
  ///@{
  /**
   * Write the data to the current FileName in vtkHDF format.
   * returns true if the writing operation completes successfully.
   */
  bool WriteDatasetToFile(vtkPolyData* input);
  bool WriteDatasetToFile(vtkUnstructuredGrid* input);
  ///@}

  ///@{
  /**
   * For transient data, update the steps group with information relevant to the current timestep.
   */
  bool UpdateStepsGroup(vtkUnstructuredGrid* input);
  bool UpdateStepsGroup(vtkPolyData* input);
  ///@}

  ///@{
  /**
   * Initialize the `Steps` group for transient data, and extendable datasets where needed.
   * This way, the other functions will append to existing datasets every step.
   */
  bool InitializeTransientData(vtkUnstructuredGrid* input);
  bool InitializeTransientData(vtkPolyData* input);
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

  /**
   * Append the offset data in the steps group for the current array for transient data
   */
  bool AppendTransientDataArray(hid_t arrayGroup, vtkAbstractArray* array, const char* arrayName,
    const char* offsetsGroupName, hid_t dataType);

  /**
   * Write the NSteps attribute and the Value dataset to group for transient writing.
   */
  bool AppendTimeValues(hid_t group);

  class Implementation;
  std::unique_ptr<Implementation> Impl;
  char* FileName = nullptr;
  bool Overwrite = true;

  // Transient-related configuration and variables
  double* timeSteps = nullptr;
  bool WriteAllTimeSteps = true;
  bool IsTransient = false;
  int CurrentTimeIndex = 0;
  int NumberOfTimeSteps = 0;
  int ChunkSize = 100;
};
VTK_ABI_NAMESPACE_END
#endif