// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHDFWriter
 * @brief   Write a data object to a VTKHDF file.
 *
 * This writer can handle vtkPolyData, vtkUnstructuredGrid, vtkPartitionedDataSet,
 * vtkMultiBlockDataSet and vtkPartitionedDataSetCollection data types,
 * as well as time-varying data.
 *
 * For temporal datasets with a constant MeshMTime, geometry will only be written once.
 *
 * This writer is compatible with MPI and multi-piece/partitioned datasets.
 *
 * When writing using multiple MPI processes, one file is written for each process.
 * When all processing are done writing all time steps, rank 0 will create the main file,
 * using HDF5 Virtual DataSets to link to the actual data written by each rank.
 * All individual files process files are also readable independently.
 *
 * Options are provided for data compression, and writing partitions, composite parts and time steps
 * in different files.
 * Reading performance and size on disk may be impacted by the chosen chunk size and compression
 * settings.
 *
 * To comply with the HDF5 and VTKHDF standard specification,
 * "/" and "." contained in field names will be replaced by "_".
 *
 * The full file format specification is here:
 * https://docs.vtk.org/en/latest/design_documents/VTKFileFormats.html#hdf-file-formats
 *
 */

#ifndef vtkHDFWriter_h
#define vtkHDFWriter_h

#include "vtkIOHDFModule.h" // For export macro
#include "vtkWriter.h"

#include <map>
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
class vtkMultiProcessController;
class vtkDataObjectTreeIterator;

typedef int64_t hid_t;

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
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

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
   * Default is true
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
   * named FileStem_BlockName.extension.
   * If FileName does not have an extension, blocks are named FileName_BlockName.vtkhdf
   * These files are referenced by the main file using external links.
   * Default is false.
   */
  vtkSetMacro(UseExternalComposite, bool);
  vtkGetMacro(UseExternalComposite, bool);
  ///@}

  ///@{
  /**
   * When set, write each time step in a different file.
   * These individual time files are referenced by the main file using the HDF5 virtual dataset
   * feature. This way, individual time step files can be opened by the reader as a non
   * time-dependent dataset, and the main file referencing those as a time-dependent file
   * seamlessly.
   *
   * Subfiles are named FileStem_X.extension, where X is the time step index.
   * extension defaults to .vtkhdf in case the base filename does not have one already.
   *
   * Note: this option does not support static meshes. Points and cells with be copied
   * across time step files.
   * Default is false.
   */
  vtkSetMacro(UseExternalTimeSteps, bool);
  vtkGetMacro(UseExternalTimeSteps, bool);
  ///@}

  ///@{
  /**
   * When set, write each partition of the input vtkPartitionedDataSet in a different file,
   * named FileStem_partX.extension, where X is the partition index.
   * If FileName does not have an extension, files are named FileName_partX.vtkhdf
   * These individual time files are referenced by the main file using the HDF5 virtual dataset
   * feature, just like the `UseExternalTimeSteps` does.
   * When applied to composite datasets, this option forces UseExternalComposite ON.
   * Default is false.
   */
  vtkSetMacro(UseExternalPartitions, bool);
  vtkGetMacro(UseExternalPartitions, bool);
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

  /**
   * For distributed datasets, write the meta-file referencing sub-files using Virtual Datasets.
   * This file is written only on process/piece 0
   */
  void WriteDistributedMetafile(vtkDataObject* input);

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
   * For temporal data, update the steps group with information relevant to the current timestep.
   */
  bool UpdateStepsGroup(hid_t group, vtkUnstructuredGrid* input);
  bool UpdateStepsGroup(hid_t group, vtkPolyData* input);
  ///@}

  ///@{
  /**
   * Initialize the `Steps` group for temporal data, and extendable datasets where needed.
   * This way, the other functions will append to existing datasets every step.
   */
  bool InitializeTemporalPolyData(hid_t group);
  bool InitializeTemporalUnstructuredGrid(hid_t group);
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

  ///@{
  /**
   * Add the data arrays of the object to the file
   * OpenRoot should succeed on this->Impl before calling this function
   */
  bool AppendDataArrays(hid_t group, vtkDataObject* input, unsigned int partId = 0);
  bool AppendDataSetAttributes(hid_t group, vtkDataObject* input, unsigned int partId = 0);
  bool AppendFieldDataArrays(hid_t group, vtkDataObject* input, unsigned int partId = 0);
  ///@}

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
  bool AppendExternalBlock(vtkDataObject* block, const std::string& blockName);
  ///@}

  /**
   * Add the assembly associated to the given PDC to the specified group
   * Individual blocks need to be added to the file beforehand.
   */
  bool AppendAssembly(hid_t group, vtkPartitionedDataSetCollection* pdc);

  /**
   * Append assembly and blocks of a multiblock dataset to the selected HDF5 group (usually root).
   * leafIndex needs to be initialized to 0 beforehand. It is used to track the number of
   * datasets during recursion.
   */
  bool AppendMultiblock(hid_t group, vtkMultiBlockDataSet* mb, int& leafIndex);

  /**
   * Write the current non-null composite block with given index to the root group with the given
   * unique name, properly setting MeshMTime for the block
   */
  void AppendIterDataObject(vtkDataObjectTreeIterator* treeIter, const int& leafIndex,
    const std::string& uniqueSubTreeName);

  /**
   * Write the composite dataset with given name as HDF virtual datasets using elements from
   * previously written subfiles in a distributed setting. This covers the case where the current
   * composite block is null for rank 0 but not for other ranks, and block characteristics (type,
   * arrays) need to be deducted from non-null ranks first.
   */
  void AppendCompositeSubfilesDataObject(const std::string& uniqueSubTreeName);

  ///@{
  /**
   * Append the offset data in the steps group for the current array for temporal data
   */
  bool AppendDataArrayOffset(hid_t baseGroup, vtkAbstractArray* array, const std::string& arrayName,
    const std::string& offsetsGroupName);
  bool AppendDataArraySizeOffset(hid_t baseGroup, vtkAbstractArray* array,
    const std::string& arrayName, const std::string& offsetsGroupName);
  ///@}

  /**
   * Write the NSteps attribute and the Value dataset to group for temporal writing.
   */
  bool AppendTimeValues(hid_t group);

  /**
   * Check if the mesh geometry changed between this step and the last.
   */
  bool HasGeometryChangedFromPreviousStep(vtkDataSet* input);

  /**
   * Update the time value of the MeshMTime which will be used in the next time step
   */
  void UpdatePreviousStepMeshMTime(vtkDataObject* input);

  class Implementation;
  std::unique_ptr<Implementation> Impl;

  // Configurable properties
  char* FileName = nullptr;
  bool Overwrite = true;
  bool WriteAllTimeSteps = true;
  bool UseExternalComposite = false;
  bool UseExternalTimeSteps = false;
  bool UseExternalPartitions = false;
  int ChunkSize = 25000;
  int CompressionLevel = 0;

  // Temporal-related private variables
  std::vector<double> timeSteps;
  bool IsTemporal = false;
  int CurrentTimeIndex = 0;
  int NumberOfTimeSteps = 1;
  vtkMTimeType PreviousStepMeshMTime = 0;
  std::map<vtkIdType, vtkMTimeType> CompositeMeshMTime;

  // Distributed-related variables
  vtkMultiProcessController* Controller = nullptr;
  int NbPieces = 1;
  int CurrentPiece = 0;
  bool UsesDummyController = false;
  std::vector<vtkIdType> PointOffsets;
  std::vector<vtkIdType> CellOffsets;
  std::vector<vtkIdType> ConnectivityIdOffsets;
};
VTK_ABI_NAMESPACE_END
#endif
