// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHDFReaderImplementation
 * @brief   Implementation class for vtkHDFReader
 *
 */

#ifndef vtkHDFReaderImplementation_h
#define vtkHDFReaderImplementation_h

#include "vtkAMRBox.h"
#include "vtkDataSetAttributes.h"
#include "vtkHDFReader.h"
#include "vtk_hdf5.h"

#include <array>
#include <map>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkBitArray;
class vtkDataArray;
class vtkDataAssembly;
class vtkMemoryResourceStream;
class vtkResourceStream;
class vtkStringArray;

/**
 * Implementation for the vtkHDFReader. Opens, closes and
 * reads information from a VTK HDF file.
 */
class vtkHDFReader::Implementation
{
public:
  Implementation(vtkHDFReader* reader);
  virtual ~Implementation();

  /**
   * Opens this VTK HDF file and checks if it is valid.
   */
  bool Open(VTK_FILEPATH const char* fileName, bool quiet = false);

  /**
   * Opens this VTK HDF stream and checks if it is valid.
   */
  bool Open(vtkResourceStream* stream, bool quiet = false);

  /**
   * Closes the VTK HDF file and releases any allocated resources.
   */
  void Close();
  /**
   * Type of vtkDataSet stored by the HDF file, such as VTK_IMAGE_DATA or
   * VTK_UNSTRUCTURED_GRID, from vtkTypes.h
   */
  int GetDataSetType() { return this->DataSetType; }
  /**
   * Return the reader this impl is associated to
   */
  vtkHDFReader* GetReader() { return this->Reader; }
  /**
   * Returns the version of the VTK HDF implementation.
   */
  const std::array<int, 2>& GetVersion() { return this->Version; }
  /**
   * Reads an attribute from the /VTKHDF group
   */
  template <typename T>
  bool GetAttribute(const char* attributeName, size_t numberOfElements, T* value);
  /**
   * Return true if the attribute exists in the specified group
   */
  bool HasAttribute(const char* groupName, const char* attributeName);
  /**
   * Return true if the dataset exists in the specified group root group
   */
  bool HasDataset(const char* datasetName);
  /**
   * Returns the number of partitions for this dataset at the time step
   * `step` if applicable.
   */
  int GetNumberOfPieces(vtkIdType step = -1);
  /**
   * For an ImageData, sets the extent for 'partitionIndex'. Returns
   * true for success and false otherwise.
   */
  bool GetPartitionExtent(hsize_t partitionIndex, int* extent);
  /**
   * Returns the names of arrays for 'attributeType' (point or cell).
   */
  std::vector<std::string> GetArrayNames(int attributeType);
  /**
   * Return the name of all children of an HDF group given its path
   */
  std::vector<std::string> GetOrderedChildrenOfGroup(const std::string& path);
  ///@{
  /**
   * Reads and returns a new vtkDataArray. The actual type of the array
   * depends on the type of the HDF array. The array is read from the PointData
   * or CellData groups depending on the 'attributeType' parameter.
   * There are two versions: a first one that reads from a 3D array using a fileExtent,
   * and a second one that reads from a linear array using an offset and size.
   * The array has to be deleted by the user.
   */
  vtkDataArray* NewArray(
    int attributeType, const char* name, const std::vector<hsize_t>& fileExtent);
  vtkDataArray* NewArray(int attributeType, const char* name, hsize_t offset, hsize_t size);
  vtkAbstractArray* NewFieldArray(
    const char* name, vtkIdType offset = -1, vtkIdType size = -1, vtkIdType dimMaxSize = -1);
  ///@}

  /**
   * Given a named data array and an attribute type,
   * read from the file the special array attribute (scalars, global ids, etc.) if any,
   * and set in in the provided vtkDataSetAttributes object.
   */
  void AttachDatasetAttributeToArray(
    int attributeType, vtkDataArray* array, vtkDataSetAttributes* attributes);

  ///@{
  /**
   * Reads a 1D metadata array in a DataArray or a vector of vtkIdType.
   * We read either the whole array for the vector version or a slice
   * specified with (offset, size). For an error we return nullptr or an
   * empty vector.
   */
  vtkDataArray* NewMetadataArray(const char* name, hsize_t offset, hsize_t size);
  std::vector<vtkIdType> GetMetadata(const char* name, hsize_t size, hsize_t offset = 0);
  ///@}
  /**
   * Returns the dimensions of a HDF dataset.
   */
  std::vector<hsize_t> GetDimensions(const char* dataset);

  /**
   * Return true if current root path is a soft link
   */
  bool IsPathSoftLink(const std::string& path);

  ///@{
  /**
   * Fills the given Assembly with the content of the opened HDF file.
   * Return true on success, false if the HDF File isn't a composite or the 'Assembly' is missing.
   */
  bool FillAssembly(vtkDataAssembly* data);
  bool FillAssembly(vtkDataAssembly* data, hid_t assemblyHandle, int assemblyID, std::string path);
  ///@}

  /**
   * Read the number of steps from the opened file
   */
  std::size_t GetNumberOfSteps();

  ///@{
  /**
   * Read the values of the steps from the open file
   */
  vtkDataArray* GetStepValues();
  vtkDataArray* GetStepValues(hid_t group);
  ///@}

  /**
   * Methods to query for array offsets when steps are present
   */
  vtkIdType GetArrayOffset(vtkIdType step, int attributeType, std::string name);

  /**
   * Return the field array size (components, tuples) for the current step.
   * By default it returns {-1,1} which means to have as many components as necessary
   * and one tuple per step.
   */
  std::array<vtkIdType, 2> GetFieldArraySize(vtkIdType step, std::string name);

  /**
   * Open a sub group of the current file and consider it as the new root file.
   */
  bool OpenGroupAsVTKGroup(const std::string& groupPath);

  /**
   * Initialize meta information of the implementation based on root name specified.
   */
  bool RetrieveHDFInformation(const std::string& rootName, const std::string& groupPrefix = "");

  /**
   * Retrieve ImageData attributes and store them.
   * Return false on failure.
   */
  bool GetImageAttributes(int WholeExtent[6], double Origin[3], double Spacing[3]);

  ///@{
  /**
   * Specific public API for AMR support.
   */
  /**
   * Return the number of level in an AMR file
   */
  unsigned int GetAMRNumberOfLevels();

  /**
   * Retrieve for each required level AMRBlocks size and position.
   * Set maxLevel to the number of level in the file
   * Return true in case of success, false otherwise.
   */
  bool ComputeAMRBlocksPerLevels(unsigned int nLevels);

  /**
   * Retrieve offset for AMRBox, point/cell/field arrays for each level.
   * Set maxLevel to the number of level in the file
   * Return true in case of success, false otherwise.
   */
  bool ComputeAMROffsetsPerLevels(
    vtkDataArraySelection* dataArraySelection[3], vtkIdType step, unsigned int nLevels);

  /**
   * Read the AMR topology based on offset data on AMRBlocks.
   */
  bool ReadAMRTopology(
    vtkOverlappingAMR* data, unsigned int maxLevel, double origin[3], bool isTemporalData);

  /**
   * Get the temporal offset for a specific attribute
   * Return true if the offset arg was set, false is it was not for any reason
   */
  bool GetAMRTemporalOffsetForAttributeType(
    unsigned int level, int attributeType, const std::string& name, hsize_t& offset);
  ///@}

  /**
   * Create a new dataset given its type and the number of pieces.
   * Create a vtkPartitionedDataSet when the number of pieces is more than 1.
   */
  vtkSmartPointer<vtkDataObject> GetNewDataSet(int dataSetType, int numPieces);

  /**
   * Read data and build the HyperTreeGrid from descriptors, mask information and cell data array in
   * the file, reading from the offsets specified as arguments.
   * Return false on failure.
   */
  bool ReadHyperTreeGridData(vtkHyperTreeGrid* htg, const vtkDataArraySelection* arraySelection,
    vtkIdType cellOffset, vtkIdType treeIdsOffset, vtkIdType depthOffset,
    vtkIdType descriptorOffset, vtkIdType maskOffset, vtkIdType partOffset,
    vtkIdType verticesPerDepthOffset, vtkIdType depthLimit, vtkIdType step);

  /**
   * Read HTG meta-information stored in attributes
   */
  bool ReadHyperTreeGridMetaInfo(vtkHyperTreeGrid* htg);

  /**
   * Read HTG dimensions and coordinates
   */
  bool ReadHyperTreeGridDimensions(vtkHyperTreeGrid* htg);

  /**
   * Initialize selected Cell arrays for HyperTreeGrid
   */
  bool CreateHyperTreeGridCellArrays(vtkHyperTreeGrid* htg,
    std::vector<vtkSmartPointer<vtkAbstractArray>>& cellArrays,
    const vtkDataArraySelection* arraySelection, vtkIdType cellCount);

  /**
   * Read & add cell data for the tree currently processed.
   */
  bool AppendCellDataForHyperTree(std::vector<vtkSmartPointer<vtkAbstractArray>>& cellArrays,
    vtkIdType cellOffset, vtkIdType inputCellOffset, vtkIdType step, vtkIdType readableTreeSize);

  /**
   * Read & add mask data for the current tree
   */
  bool AppendMaskForHyperTree(vtkHyperTreeGrid* htg, vtkIdType inputCellOffset,
    vtkIdType maskOffset, vtkIdType readableTreeSize);

private:
  std::string FileName;
  vtkResourceStream* Stream = nullptr;
  vtkNew<vtkMemoryResourceStream> LocalMemStream;
  hid_t File;
  hid_t VTKGroup;
  // in the same order as vtkDataObject::AttributeTypes: POINT, CELL, FIELD
  std::array<hid_t, 3> AttributeDataGroup;
  int DataSetType;
  int NumberOfPieces;
  std::array<int, 2> Version;
  vtkHDFReader* Reader;

  ///@{
  /**
   * Specific methods and structure of AMR support.
   */
  struct AMRBlocksInformation
  {
    std::vector<int> BlocksPerLevel;
    std::vector<vtkIdType> BlockOffsetsPerLevel;
    std::map<std::string, std::vector<vtkIdType>> CellOffsetsPerLevel;
    std::map<std::string, std::vector<vtkIdType>> PointOffsetsPerLevel;

    void Clear()
    {
      this->BlocksPerLevel.clear();
      this->BlockOffsetsPerLevel.clear();
      this->PointOffsetsPerLevel.clear();
      this->CellOffsetsPerLevel.clear();
    }
  };

  AMRBlocksInformation AMRInformation;

  bool ReadLevelSpacing(hid_t levelGroupID, double* spacing);
  bool ReadAMRBoxRawValues(
    hid_t levelGroupID, std::vector<int>& amrBoxRawData, int level, bool isTemporalData);
  bool ReadLevelTopology(unsigned int level, const std::string& levelGroupName,
    vtkOverlappingAMR* data, double origin[3], bool isTemporalData);
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHDFReaderImplementation.h
