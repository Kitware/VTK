// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtk_hdf5.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <class T>
vtkHDFUtilities::TransientGeometryOffsets::TransientGeometryOffsets(T* impl, vtkIdType step)
{
  auto recupMultiOffset = [&](std::string path, std::vector<vtkIdType>& val)
  {
    val = impl->GetMetadata(path.c_str(), 1, step);
    if (val.empty())
    {
      vtkErrorWithObjectMacro(
        nullptr, << path.c_str() << " array cannot be empty when there is temporal data");
      return false;
    }
    return true;
  };
  auto recupSingleOffset = [&](std::string path, vtkIdType& val)
  {
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

//------------------------------------------------------------------------------
template <class T>
vtkHDFUtilities::TemporalGeometryOffsets::TemporalGeometryOffsets(T* impl, vtkIdType step)
{
  auto getMultiOffset = [&](std::string path, std::vector<vtkIdType>& val)
  {
    val = impl->GetMetadata(path.c_str(), 1, step);
    if (val.empty())
    {
      vtkErrorWithObjectMacro(
        nullptr, << path.c_str() << " array cannot be empty when there is temporal data");
      return false;
    }
    return true;
  };
  auto getSingleOffset = [&](std::string path, vtkIdType& val)
  {
    std::vector<vtkIdType> buffer;
    if (!getMultiOffset(path, buffer))
    {
      return false;
    }
    val = buffer[0];
    return true;
  };
  if (!getSingleOffset("Steps/PartOffsets", this->PartOffset))
  {
    this->Success = false;
    return;
  }
  if (!getSingleOffset("Steps/PointOffsets", this->PointOffset))
  {
    this->Success = false;
    return;
  }
  if (!getMultiOffset("Steps/CellOffsets", this->CellOffsets))
  {
    this->Success = false;
    return;
  }
  if (!getMultiOffset("Steps/ConnectivityIdOffsets", this->ConnectivityOffsets))
  {
    this->Success = false;
    return;
  }
}

//------------------------------------------------------------------------------
template <class T>
vtkHDFUtilities::TemporalHyperTreeGridOffsets::TemporalHyperTreeGridOffsets(T* impl, vtkIdType step)
{
  // No time data to read
  if (step == -1)
  {
    return;
  }
  auto getSingleOffset = [&](std::string path, vtkIdType& val)
  {
    std::vector<vtkIdType> buffer = impl->GetMetadata(path.c_str(), 1, step);
    if (buffer.empty())
    {
      vtkErrorWithObjectMacro(
        nullptr, << path.c_str() << " array cannot be empty when there is temporal data");
      return false;
    }
    val = buffer[0];
    return true;
  };
  this->Success = getSingleOffset("Steps/TreeIdsOffsets", this->TreeIdsOffset) &&
    getSingleOffset("Steps/DepthPerTreeOffsets", this->DepthPerTreeOffset) &&
    getSingleOffset(
      "Steps/NumberOfCellsPerTreeDepthOffsets", this->NumberOfCellsPerTreeDepthOffset) &&
    getSingleOffset("Steps/DescriptorsOffsets", this->DescriptorsOffset) &&
    getSingleOffset("Steps/MaskOffsets", this->MaskOffset) &&
    getSingleOffset("Steps/XCoordinatesOffsets", this->XCoordinatesOffset) &&
    getSingleOffset("Steps/YCoordinatesOffsets", this->YCoordinatesOffset) &&
    getSingleOffset("Steps/ZCoordinatesOffsets", this->ZCoordinatesOffset) &&
    getSingleOffset("Steps/PartOffsets", this->PartOffset);
}

//------------------------------------------------------------------------------
template <typename T>
hid_t vtkHDFUtilities::TemplateTypeToHdfNativeType()
{
  if (std::is_same<T, char>::value)
  {
    return H5T_NATIVE_CHAR;
  }
  if (std::is_same<T, signed char>::value)
  {
    return H5T_NATIVE_SCHAR;
  }
  else if (std::is_same<T, unsigned char>::value)
  {
    return H5T_NATIVE_UCHAR;
  }
  else if (std::is_same<T, short>::value)
  {
    return H5T_NATIVE_SHORT;
  }
  else if (std::is_same<T, unsigned short>::value)
  {
    return H5T_NATIVE_USHORT;
  }
  else if (std::is_same<T, int>::value)
  {
    return H5T_NATIVE_INT;
  }
  else if (std::is_same<T, unsigned int>::value)
  {
    return H5T_NATIVE_UINT;
  }
  else if (std::is_same<T, long>::value)
  {
    return H5T_NATIVE_LONG;
  }
  else if (std::is_same<T, unsigned long>::value)
  {
    return H5T_NATIVE_ULONG;
  }
  else if (std::is_same<T, long long>::value)
  {
    return H5T_NATIVE_LLONG;
  }
  else if (std::is_same<T, unsigned long long>::value)
  {
    return H5T_NATIVE_ULLONG;
  }
  else if (std::is_same<T, float>::value)
  {
    return H5T_NATIVE_FLOAT;
  }
  else if (std::is_same<T, double>::value)
  {
    return H5T_NATIVE_DOUBLE;
  }
  vtkErrorWithObjectMacro(nullptr, "Invalid type: " << typeid(T).name());
  return -1;
}

//------------------------------------------------------------------------------
template <typename T>
bool vtkHDFUtilities::GetAttribute(
  hid_t group, const char* attributeName, size_t numberOfElements, T* value)
{
  vtkHDF::ScopedH5AHandle attr = H5Aopen_name(group, attributeName);
  if (attr < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string(attributeName) + " attribute not found");
    return false;
  }

  vtkHDF::ScopedH5SHandle space = H5Aget_space(attr);
  if (space < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string(attributeName) + " attribute: get_space error");
    return false;
  }
  int ndims = H5Sget_simple_extent_ndims(space);
  if (ndims < 0)
  {
    vtkErrorWithObjectMacro(
      nullptr, << std::string(attributeName) + " attribute: get_simple_extent_ndims error");
    return false;
  }

  if (ndims > 1)
  {
    vtkErrorWithObjectMacro(nullptr,
      << std::string(attributeName) + " attribute should have rank 1 or 0, it has rank " << ndims);
    return false;
  }

  if (ndims == 0 && numberOfElements != 1)
  {
    vtkErrorWithObjectMacro(nullptr,
      << std::string(attributeName) + " attribute should have rank 1, it has rank " << ndims);
    return false;
  }

  hsize_t ne = 0;
  if (H5Sget_simple_extent_dims(space, &ne, nullptr) < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string("Cannot find dimension for ") + attributeName);
    return false;
  }

  if (numberOfElements != 1 && ne != numberOfElements)
  {
    vtkErrorWithObjectMacro(
      nullptr, << attributeName << " attribute should have " << numberOfElements << " dimensions");
    return false;
  }
  hid_t hdfType = vtkHDFUtilities::TemplateTypeToHdfNativeType<T>();
  if (hdfType < 0)
  {
    vtkErrorWithObjectMacro(
      nullptr, << std::string("Native type not implemented: ") + typeid(T).name());
    return false;
  }

  if (H5Aread(attr, hdfType, value) < 0)
  {
    vtkErrorWithObjectMacro(
      nullptr, << std::string("Error reading ") + attributeName + " attribute");
    return false;
  }

  return true;
}

VTK_ABI_NAMESPACE_END
