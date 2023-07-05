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
#include "vtk_hdf5.h"
#include <array>
#include <map>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataArray;
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
  vtkAbstractArray* NewFieldArray(const char* name, vtkIdType offset = -1, vtkIdType size = -1);
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
   * Fills the given AMR data with the content of the opened HDF file.
   * The number of level to read is limited by the maximumLevelsToReadByDefault argument.
   * maximumLevelsToReadByDefault == 0 means to read all levels (no limit).
   * Only the selected data array in dataArraySelection are added to the AMR data.
   * Returns true on success.
   */
  bool FillAMR(vtkOverlappingAMR* data, unsigned int maximumLevelsToReadByDefault, double origin[3],
    vtkDataArraySelection* dataArraySelection[3]);

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
   * Opens the hdf5 dataset given the 'group'
   * and 'name'.
   * Returns the hdf dataset and sets 'nativeType' and 'dims'.
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
   * Associates a struc of three integers with HDF type. This can be used as
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
   * These methods are valid only with AMR data set type.
   */
  bool ComputeAMRBlocksPerLevels(std::vector<int>& levels);
  bool ReadLevelSpacing(hid_t levelGroupID, double* spacing);
  bool ReadAMRBoxRawValues(hid_t levelGroupID, std::vector<int>& amrBoxRawData);
  bool ReadLevelTopology(unsigned int level, const std::string& levelGroupName,
    vtkOverlappingAMR* data, double origin[3]);
  bool ReadLevelData(unsigned int level, const std::string& levelGroupName, vtkOverlappingAMR* data,
    vtkDataArraySelection* dataArraySelection[3]);
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHDFReaderImplementation.h
