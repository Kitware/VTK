/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHDFReaderImplementation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
   * Returns the number of partitions for this dataset.
   */
  int GetNumberOfPieces() { return this->NumberOfPieces; }
  /**
   * For an ImageData, sets the extent for 'partitionIndex'. Returns
   * true for success and false otherwise.
   */
  bool GetPartitionExtent(hsize_t partitionIndex, int* extent);
  /**
   * Returns the names of arrays for 'attributeType' (point or cell).
   */
  std::vector<std::string> GetArrayNames(int attributeType);
  //@{
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
  vtkAbstractArray* NewFieldArray(const char* name);
  //@}

  //@{
  /**
   * Reads a 1D metadata array in a DataArray or a vector of vtkIdType.
   * We read either the whole array for the vector version or a slice
   * specified with (offset, size). For an error we return nullptr or an
   * empty vector.
   */
  vtkDataArray* NewMetadataArray(const char* name, hsize_t offset, hsize_t size);
  std::vector<vtkIdType> GetMetadata(const char* name, hsize_t size);
  //@}
  /**
   * Returns the dimensions of a HDF dataset.
   */
  std::vector<hsize_t> GetDimensions(const char* dataset);

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

protected:
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

  //@{
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
  vtkDataArray* NewArray(hid_t group, const char* name, const std::vector<hsize_t>& fileExtent);
  template <typename T>
  vtkDataArray* NewArray(
    hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents);
  template <typename T>
  bool NewArray(
    hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents, T* data);
  vtkStringArray* NewStringArray(hid_t dataset, hsize_t size);
  //@}
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
};

//------------------------------------------------------------------------------
// explicit template instantiation declaration
extern template bool vtkHDFReader::Implementation::GetAttribute<int>(
  const char* attributeName, size_t dim, int* value);
extern template bool vtkHDFReader::Implementation::GetAttribute<double>(
  const char* attributeName, size_t dim, double* value);

#endif
// VTK-HeaderTest-Exclude: vtkHDFReaderImplementation.h
