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

class vtkCellArray;
class vtkDataObjectTree;
class vtkDataSet;
class vtkPoints;
class vtkPointSet;
class vtkPolyData;
class vtkUnstructuredGrid;
class vtkPartitionedDataSet;
class vtkPartitionedDataSetCollection;
class vtkMultiBlockDataSet;

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
   * When turned OFF, only write the current timestep.
   */
  vtkSetMacro(WriteAllTimeSteps, bool);
  vtkGetMacro(WriteAllTimeSteps, bool);
  ///@}

  ///@{
  /**
   * Get/set the chunk size used for chunk storage layout. Chunked storage is required for
   * extensible/unlimited dimensions datasets (such as time-dependent data), and filters such as
   * compression. Read more about chunks and chunk size here :
   * https://support.hdfgroup.org/HDF5/doc/Advanced/Chunking/
   *
   * Regarding performance impact of chunking and how to find the optimal value depending on the
   * data, please check this documentation:
   * https://docs.hdfgroup.org/hdf5/develop/_l_b_dset_layout.html
   *
   * Defaults to 25000 (to fit with the default chunk cache of 1Mb of HDF5).
   */
  vtkSetMacro(ChunkSize, int);
  vtkGetMacro(ChunkSize, int);
  ///@}

  ///@{
  /**
   * Get/set the compression level used by hdf5.
   * The compression level is between 0 (no compression) and 9 (max compression level).
   *
   * @warning Compression level used can have a big performance impact for writing/reading data.
   * For reference, the default value used by HDF5 when we apply a compression is 4.
   *
   * @note Only points, cells and data arrays will be compressed. Other datas are considered to be
   * too small to be worth compressing.
   *
   * Default to 0.
   */
  vtkSetClampMacro(CompressionLevel, int, 0, 9);
  vtkGetMacro(CompressionLevel, int);
  ///@}

  ///@{
  /**
   * When set, write composite leaf blocks in different files,
   * named FileName_without_extension_BlockName.extension.
   * If FileName does not have an extension, blocks are named
   * FileName_BlockName.vtkhdf
   * These files are referenced by the main file using external links.
   * Default is false.
   */
  vtkSetMacro(UseExternalComposite, bool);
  vtkGetMacro(UseExternalComposite, bool);
  ///@}

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
  /**
   * Open destination file and write the input dataset to the file specified by the filename
   * attribute in vtkHDF format.
   */
  void WriteData() override;

  /**
   * Dispatch the input vtkDataObject to the right writing function, depending on its dynamic type.
   * Data will be written in the specified group, which must already exist.
   */
  void DispatchDataObject(hid_t group, vtkDataObject* input, unsigned int partId = 0);

  ///@{
  /**
   * Write the given dataset to the current FileName in vtkHDF format.
   * returns true if the writing operation completes successfully.
   */
  bool WriteDatasetToFile(hid_t group, vtkPolyData* input, unsigned int partId = 0);
  bool WriteDatasetToFile(hid_t group, vtkUnstructuredGrid* input, unsigned int partId = 0);
  bool WriteDatasetToFile(hid_t group, vtkPartitionedDataSet* input);
  bool WriteDatasetToFile(hid_t group, vtkDataObjectTree* input);
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
  bool InitializeTemporalPolyData();
  bool InitializeTemporalUnstructuredGrid();
  ///@}

  ///@{
  /**
   * Initialize empty dynamic chunked datasets where data will be appended.
   * These datasets will be extended when a new partition is written.
   */
  bool InitializeChunkedDatasets(hid_t group, vtkUnstructuredGrid* input);
  bool InitializeChunkedDatasets(hid_t group, vtkPolyData* input);
  bool InitializePointDatasets(hid_t group, vtkPoints* input);
  bool InitializePrimitiveDataset(hid_t group);
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
  bool AppendDataArrays(hid_t group, vtkDataObject* input, unsigned int partId = 0);

  ///@{
  /**
   * Append all available blocks of a given vtkPartitionedDataSetCollection to the same HDF5 group,
   * without hierarchy.
   */
  bool AppendBlocks(hid_t group, vtkPartitionedDataSetCollection* pdc);
  ///@}

  ///@{
  /**
   * Write block in a separate file whose name is derived to the block name,
   * and create an external link from VTKHDF/blockName to this file's content.
   * The block should be of non-composite type.
   */
  bool AppendExternalBlock(vtkDataObject* block, std::string& blockName);
  ///@}

  /**
   * Add the assembly associated to the given PDC to the specified group
   * Individual blocks need to be added to the file beforehand.
   */
  bool AppendAssembly(hid_t group, vtkPartitionedDataSetCollection* pdc);

  /**
   * Append assembly and blocks of a multiblock dataset to the selected HDF5 group (usually root).
   * datasetCount needs to be initialized to 0 beforehand. It is used to track the number of
   * datasets during recursion.
   */
  bool AppendMultiblock(hid_t group, vtkMultiBlockDataSet* mb);

  /**
   * Append the offset data in the steps group for the current array for transient data
   */
  bool AppendDataArrayOffset(
    vtkAbstractArray* array, const char* arrayName, const char* offsetsGroupName);

  /**
   * Write the NSteps attribute and the Value dataset to group for transient writing.
   */
  bool AppendTimeValues(hid_t group);

  /**
   * Check if the mesh geometry changed between this step and the last.
   */
  template <typename vtkStaticMeshDataSetT>
  bool HasGeometryChangedFromPreviousStep(vtkStaticMeshDataSetT* input);

  /**
   * Update the time value of the MeshMTime which wiil be used in the next time step
   */
  void UpdatePreviousStepMeshMTime(vtkDataObject* input);

  class Implementation;
  std::unique_ptr<Implementation> Impl;

  // Configurable properties
  char* FileName = nullptr;
  bool Overwrite = true;
  bool WriteAllTimeSteps = true;
  bool UseExternalComposite = false;
  int ChunkSize = 25000;
  int CompressionLevel = 0;

  // Temporal-related private variables
  double* timeSteps = nullptr;
  bool IsTemporal = false;
  int CurrentTimeIndex = 0;
  int NumberOfTimeSteps = 0;
  vtkMTimeType PreviousStepMeshMTime = 0;
};
VTK_ABI_NAMESPACE_END
#endif
