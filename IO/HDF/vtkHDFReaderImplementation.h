// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHDFReaderImplementation
 * @brief   Implementation class for vtkHDFReader
 *
 */

#ifndef vtkHDFReaderImplementation_h
#define vtkHDFReaderImplementation_h

#include "vtkHDFReader.h"
#include "vtkHDFUtilities.h"
#include "vtk_hdf5.h"
#include <array>
#include <map>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataArray;
class vtkStringArray;
class vtkDataAssembly;

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
  bool Open(VTK_FILEPATH const char* fileName);
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
   * Returns the version of the VTK HDF implementation.
   */
  const std::array<int, 2>& GetVersion() { return this->Version; }
  /**
   * Reads an attribute from the /VTKHDF group
   */
  template <typename T>
  bool GetAttribute(const char* attributeName, size_t numberOfElements, T* value);
  /**
   * Reads an attribute from the group passed to it
   */
  template <typename T>
  bool GetAttribute(hid_t group, const char* attributeName, size_t numberOfElements, T* value);
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

  ///@{
  /**
   * Read the number of steps from the opened file
   */
  std::size_t GetNumberOfSteps();
  std::size_t GetNumberOfSteps(hid_t group);
  ///@}

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
  bool RetrieveHDFInformation(const std::string& rootName);

  ///@{
  /**
   * Specific public API for AMR supports.
   */
  /**
   * Retrieve for each required level AMRBlocks size and position.
   */
  bool ComputeAMRBlocksPerLevels(unsigned int maxLevel);

  /**
   * Retrieve offset for AMRBox, point/cell/field arrays for each level.
   */
  bool ComputeAMROffsetsPerLevels(
    vtkDataArraySelection* dataArraySelection[3], vtkIdType step, unsigned int maxLevel);

  /**
   * Read the AMR topology based on offset data on AMRBlocks.
   */
  bool ReadAMRTopology(vtkOverlappingAMR* data, unsigned int level, unsigned int maxLevel,
    double origin[3], bool isTemporalData);

  /**
   * Read the AMR data based on offset on point/cell/field datas.
   */
  bool ReadAMRData(vtkOverlappingAMR* data, unsigned int level, unsigned int maxLevel,
    vtkDataArraySelection* dataArraySelection[3], bool isTemporalData);
  ///@}

protected:
  /**
   * Used to store HDF native types in a map
   */
  struct TypeDescription
  {
    int Class;
    size_t Size;
    int Sign;
    TypeDescription()
      : Class(H5T_NO_CLASS)
      , Size(0)
      , Sign(H5T_SGN_ERROR)
    {
    }
    bool operator<(const TypeDescription& other) const
    {
      return Class < other.Class || (Class == other.Class && Size < other.Size) ||
        (Class == other.Class && Size == other.Size && Sign < other.Sign);
    }
  };

  /**
   * Opens the hdf5 dataset given the 'group' and 'name'.
   * Returns the hdf dataset and sets 'nativeType' and 'dims'.
   * The caller needs to close the returned hid_t manually using H5Dclose or a Scoped Handle if it
   * is not an invalid hid.
   */
  hid_t OpenDataSet(hid_t group, const char* name, hid_t* nativeType, std::vector<hsize_t>& dims);
  /**
   * Convert C++ template type T to HDF5 native type
   * this can be constexpr in C++17 standard
   */
  template <typename T>
  hid_t TemplateTypeToHdfNativeType();
  /**
   * Create a vtkDataArray based on the C++ template type T.
   * For instance, for a float we create a vtkFloatArray.
   * this can be constexpr in C++17 standard
   */
  template <typename T>
  vtkDataArray* NewVtkDataArray();

  ///@{
  /**
   * Reads a vtkDataArray of type T from the attributeType, dataset
   * The array has type 'T' and 'numberOfComponents'. We are reading
   * fileExtent slab from the array. It returns the array or nullptr
   * in case of an error.
   * There are three cases for fileExtent:
   * fileExtent.size() == 0 - in this case we expect a 1D array and we read
   *                          the whole array. Used for field arrays.
   * fileExtent.size()>>1 == ndims - in this case we read a scalar
   * fileExtent.size()>>1 + 1 == ndims - in this case we read an array with
   *                           the number of components > 1.
   */
  vtkDataArray* NewArrayForGroup(
    hid_t group, const char* name, const std::vector<hsize_t>& fileExtent);
  vtkDataArray* NewArrayForGroup(hid_t dataset, hid_t nativeType, const std::vector<hsize_t>& dims,
    const std::vector<hsize_t>& fileExtent);
  template <typename T>
  vtkDataArray* NewArray(
    hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents);
  template <typename T>
  bool NewArray(
    hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents, T* data);
  vtkStringArray* NewStringArray(hid_t dataset, hsize_t size);
  ///@}
  /**
   * Builds a map between native types and GetArray routines for that type.
   */
  void BuildTypeReaderMap();
  /**
   * Associates a struct of three integers with HDF type. This can be used as
   * key in a map.
   */
  TypeDescription GetTypeDescription(hid_t type);

private:
  std::string FileName;
  hid_t File;
  hid_t VTKGroup;
  // in the same order as vtkDataObject::AttributeTypes: POINT, CELL, FIELD
  std::array<hid_t, 3> AttributeDataGroup;
  int DataSetType;
  int NumberOfPieces;
  std::array<int, 2> Version;
  vtkHDFReader* Reader;
  using ArrayReader = vtkDataArray* (vtkHDFReader::Implementation::*)(hid_t dataset,
    const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents);
  std::map<TypeDescription, ArrayReader> TypeReaderMap;

  bool ReadDataSetType();

  ///@{
  /**
   * Specific methods and structure of AMR support.
   */
  struct AMRBlocksInformation
  {
    std::vector<int> BlocksPerLevel;
    std::vector<int> BlockOffsetsPerLevel;
    std::map<std::string, std::vector<int>> CellOffsetsPerLevel;
    std::map<std::string, std::vector<int>> PointOffsetsPerLevel;
    std::map<std::string, std::vector<int>> FieldOffsetsPerLevel;
    std::map<std::string, std::vector<int>> FieldSizesPerLevel;

    void Clear()
    {
      this->BlocksPerLevel.clear();
      this->BlockOffsetsPerLevel.clear();
      this->PointOffsetsPerLevel.clear();
      this->CellOffsetsPerLevel.clear();
      this->FieldOffsetsPerLevel.clear();
      this->FieldSizesPerLevel.clear();
    }
  };

  AMRBlocksInformation AMRInformation;

  bool ReadLevelSpacing(hid_t levelGroupID, double* spacing);
  bool ReadAMRBoxRawValues(
    hid_t levelGroupID, std::vector<int>& amrBoxRawData, int level, bool isTemporalData);
  bool ReadLevelTopology(unsigned int level, const std::string& levelGroupName,
    vtkOverlappingAMR* data, double origin[3], bool isTemporalData);
  bool ReadLevelData(unsigned int level, const std::string& levelGroupName, vtkOverlappingAMR* data,
    vtkDataArraySelection* dataArraySelection[3], bool isTemporalData);
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHDFReaderImplementation.h
