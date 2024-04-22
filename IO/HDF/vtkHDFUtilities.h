// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @namespace vtkHDFUtilities
 * @brief Common utility variables and functions for reader and writer of vtkHDF
 *
 */

#ifndef vtkHDFUtilities_h
#define vtkHDFUtilities_h

#include "vtkDataArray.h"
#include "vtkHDF5ScopedHandle.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"

VTK_ABI_NAMESPACE_BEGIN

namespace vtkHDFUtilities
{
const std::string VTKHDF_ROOT_PATH = "/VTKHDF";

/*
 * The number of PolyData topologies saved in vtkHDF format
 */
constexpr std::size_t NUM_POLY_DATA_TOPOS = 4;

/*
 * A vector of the topology names that are saved in vtkHDF
 * Can be used for the name of the HDF group only
 */
const std::vector<std::string> POLY_DATA_TOPOS{ "Vertices", "Lines", "Polygons", "Strips" };

/*
 * Attribute tag used in the cache storage to indicate arrays related to the geometry of the data
 * set and not fields of the data set
 */
constexpr int GEOMETRY_ATTRIBUTE_TAG = -42;

/*
 * How many attribute types we have. This returns 3: point, cell and field
 * attribute types.
 */
constexpr static int GetNumberOfAttributeTypes()
{
  return 3;
}

/*
 * Returns the id to a HDF datatype (H5T) from a VTK datatype
 * Returns H5I_INVALID_HID if no corresponding type is found
 */
inline hid_t getH5TypeFromVtkType(const int dataType)
{
  switch (dataType)
  {
    case VTK_DOUBLE:
      return H5T_NATIVE_DOUBLE;

    case VTK_FLOAT:
      return H5T_NATIVE_FLOAT;

#if VTK_ID_TYPE_IMPL == VTK_LONG_LONG
    case VTK_ID_TYPE:
#endif
    case VTK_LONG_LONG:
      return H5T_NATIVE_LLONG;

    case VTK_UNSIGNED_LONG_LONG:
      return H5T_NATIVE_ULLONG;

#if VTK_ID_TYPE_IMPL == VTK_LONG
    case VTK_ID_TYPE:
#endif
    case VTK_LONG:
      return H5T_NATIVE_LONG;

    case VTK_UNSIGNED_LONG:
      return H5T_NATIVE_ULONG;

#if VTK_ID_TYPE_IMPL == VTK_INT
    case VTK_ID_TYPE:
#endif
    case VTK_INT:
      return H5T_NATIVE_INT;

    case VTK_UNSIGNED_INT:
      return H5T_NATIVE_UINT;

    case VTK_SHORT:
      return H5T_NATIVE_SHORT;

    case VTK_UNSIGNED_SHORT:
      return H5T_NATIVE_USHORT;

    case VTK_CHAR:
      return H5T_NATIVE_CHAR;

    case VTK_SIGNED_CHAR:
      return H5T_NATIVE_SCHAR;

    case VTK_UNSIGNED_CHAR:
      return H5T_NATIVE_UCHAR;

    default:
      return H5I_INVALID_HID;
  }
}

VTK_DEPRECATED_IN_9_4_0("Please use TemporalGeometryOffsets struct instead.")
struct TransientGeometryOffsets
{
public:
  bool Success = true;
  vtkIdType PartOffset = 0;
  vtkIdType PointOffset = 0;
  std::vector<vtkIdType> CellOffsets;
  std::vector<vtkIdType> ConnectivityOffsets;

  template <class T>
  TransientGeometryOffsets(T* impl, vtkIdType step)
  {
    auto recupMultiOffset = [&](std::string path, std::vector<vtkIdType>& val) {
      val = impl->GetMetadata(path.c_str(), 1, step);
      if (val.empty())
      {
        vtkErrorWithObjectMacro(
          nullptr, << path.c_str() << " array cannot be empty when there is temporal data");
        return false;
      }
      return true;
    };
    auto recupSingleOffset = [&](std::string path, vtkIdType& val) {
      std::vector<vtkIdType> buffer;
      if (!recupMultiOffset(path, buffer))
      {
        return false;
      }
      val = buffer[0];
      return true;
    };
    if (!recupSingleOffset("Steps/PartOffsets", this->PartOffset))
    {
      this->Success = false;
      return;
    }
    if (!recupSingleOffset("Steps/PointOffsets", this->PointOffset))
    {
      this->Success = false;
      return;
    }
    if (!recupMultiOffset("Steps/CellOffsets", this->CellOffsets))
    {
      this->Success = false;
      return;
    }
    if (!recupMultiOffset("Steps/ConnectivityIdOffsets", this->ConnectivityOffsets))
    {
      this->Success = false;
      return;
    }
  }
};

/*
 * @struct TemporalGeometryOffsets
 * @brief Use to get the offsets for temporal vtkHDF.
 *
 * To use it, create an object using the templated constructor of this struct.
 * It will fill the object with data that can be then retrieved.
 */
struct TemporalGeometryOffsets
{
public:
  bool Success = true;
  vtkIdType PartOffset = 0;
  vtkIdType PointOffset = 0;
  std::vector<vtkIdType> CellOffsets;
  std::vector<vtkIdType> ConnectivityOffsets;

  template <class T>
  TemporalGeometryOffsets(T* impl, vtkIdType step)
  {
    auto recupMultiOffset = [&](std::string path, std::vector<vtkIdType>& val) {
      val = impl->GetMetadata(path.c_str(), 1, step);
      if (val.empty())
      {
        vtkErrorWithObjectMacro(
          nullptr, << path.c_str() << " array cannot be empty when there is temporal data");
        return false;
      }
      return true;
    };
    auto recupSingleOffset = [&](std::string path, vtkIdType& val) {
      std::vector<vtkIdType> buffer;
      if (!recupMultiOffset(path, buffer))
      {
        return false;
      }
      val = buffer[0];
      return true;
    };
    if (!recupSingleOffset("Steps/PartOffsets", this->PartOffset))
    {
      this->Success = false;
      return;
    }
    if (!recupSingleOffset("Steps/PointOffsets", this->PointOffset))
    {
      this->Success = false;
      return;
    }
    if (!recupMultiOffset("Steps/CellOffsets", this->CellOffsets))
    {
      this->Success = false;
      return;
    }
    if (!recupMultiOffset("Steps/ConnectivityIdOffsets", this->ConnectivityOffsets))
    {
      this->Success = false;
      return;
    }
  }
};

}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHDFUtilities.h
