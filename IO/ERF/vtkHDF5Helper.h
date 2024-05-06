// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkHDF5Helper_h
#define vtkHDF5Helper_h

#include "vtkIOERFModule.h" // For export macro

#define H5_USE_16_API
#include "vtk_hdf5.h" // For hid_t

#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkAbstractArray;

/**
 * @class vtkHDF5Helper
 *
 * Helper class used to read hdf5 file format.
 */
class VTKIOERF_EXPORT vtkHDF5Helper
{
public:
  /**
   * Get childs of a specific group name and id.
   */
  static std::vector<std::string> GetChildren(const hid_t& id, const std::string& name);

  /**
   * Get the relative path for a name based on the id
   */
  static std::string GetPathFromName(
    const hid_t& id, const std::string& path, const std::string& name);

  /**
   *  Check existence of array defined by pathName relative to fileId.
   */
  static bool ArrayExists(hid_t fileId, const char* pathName);

  /**
   *  Check existence of group defined by groupName relative to fileId.
   */
  static bool GroupExists(hid_t fileId, const char* groupName);

  /**
   *  Get length of array defined by arrayId.
   */
  static hsize_t GetDataLength(hid_t arrayId);

  /**
   *  Get dimension of array defined by arrayId.
   */
  static std::vector<hsize_t> GetDataDimensions(hid_t arrayId);

  ///@{
  /**
   *  Create an appropriate data array based on fileId and the array defined by pathName.
   */
  static vtkAbstractArray* CreateDataArray(const hid_t& fileId, const std::string& pathName);
  static vtkAbstractArray* CreateDataArray(
    const hid_t& fileId, const std::string& path, const std::string& dataSetName);
  ///@}

  /**
   * FileInfo() is a callback function for H5Giterate().
   */
  static herr_t FileInfoCallBack(hid_t /* loc_id */, const char* name, void* opdata);

private:
  vtkHDF5Helper(const vtkHDF5Helper&) = delete;
  void operator=(const vtkHDF5Helper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
