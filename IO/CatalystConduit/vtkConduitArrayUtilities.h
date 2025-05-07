// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConduitArrayUtilities
 * @brief helper to convert Conduit arrays to VTK arrays.
 * @ingroup Insitu
 *
 * vtkConduitArrayUtilities is intended to convert Conduit nodes satisfying the
 * `mcarray` protocol to VTK arrays. It uses zero-copy when possible otherwise
 * it uses deep copy. If arrays are stored on acceleration devices and VTK
 * is not compiled with appropriate options (Viskores and appropriate acceleration
 * device turned on) the conversion fails.
 *
 * This is primarily designed for use by vtkConduitSource.
 */

#ifndef vtkConduitArrayUtilities_h
#define vtkConduitArrayUtilities_h

#include "vtkDeprecation.h"             // for VTK_DEPRECATED_IN_9_5_0
#include "vtkIOCatalystConduitModule.h" // for exports
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include "conduit.h" // for conduit_node

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;

class VTKIOCATALYSTCONDUIT_EXPORT vtkConduitArrayUtilities : public vtkObject
{
public:
  static vtkConduitArrayUtilities* New();
  vtkTypeMacro(vtkConduitArrayUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@{
  /**
   * Returns true if p is a device pointer,
   *         false if it is a host pointer.
   * id is the Viskores DeviceAdapterTag such as VTK_DEVICE_ADAPTER_CUDA
   * The 'working' parameter is set to true
   * if Viskores has the runtime needed for the 'id' device
   *         false otherwise
   */
  static bool IsDevicePointer(const void* p, int8_t& id, bool& working);
  ///@}

  ///@{
  /**
   * Returns a vtkDataArray from a conduit node in the conduit mcarray protocol.
   */
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKArray(const conduit_node* mcarray);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKArray(
    const conduit_node* mcarray, const std::string& arrayname);
  ///@}

  ///@{
  /**
   * Returns a vtkDataArray from a conduit node in the conduit mcarray protocol
   * that is a conduit ghost array named ascent_ghosts.
   *
   * @deprecated: Instead of using this function, use the state/metadata/vtk_fields
   * to define the attribute_type, values_to_replace and replacement_values instead for
   * the ghost array.
   */
  VTK_DEPRECATED_IN_9_5_0("This function is deprecated, because in the future "
                          "state/metadata/vtk_fields will only be used.")
  static vtkSmartPointer<vtkDataArray> MCGhostArrayToVTKGhostArray(
    const conduit_node* mcarray, bool is_cell_data);
  ///@}

  /**
   * Converts an mcarray to vtkCellArray.
   *
   * This may reinterpret unsigned array as signed arrays to avoid deep-copying
   * of data to match data type expected by vtkCellArray API.
   */
  VTK_DEPRECATED_IN_9_4_0("Version with additional `numberOfPoints` parameter needed with "
                          "zero-copy arrays stored on acceleration devices such as CUDA")
  static vtkSmartPointer<vtkCellArray> MCArrayToVTKCellArray(
    int cellType, vtkIdType cellSize, const conduit_node* mcarray);
  static vtkSmartPointer<vtkCellArray> MCArrayToVTKCellArray(
    vtkIdType numberOfPoints, int cellType, vtkIdType cellSize, const conduit_node* mcarray);

  /**
   * If the number of components in the array does not match the target, a new
   * array is created.
   */
  static vtkSmartPointer<vtkDataArray> SetNumberOfComponents(
    vtkDataArray* array, int num_components);

  /**
   * Read a O2MRelation element
   */
  VTK_DEPRECATED_IN_9_4_0("Version with additional `numberOfPoints` parameter needed with "
                          "zero-copy arrays stored on acceleration devices such as CUDA. "
                          "`leafname` is always connectivity, so it is removed in the new version.")
  static vtkSmartPointer<vtkCellArray> O2MRelationToVTKCellArray(
    const conduit_node* o2mrelation, const std::string& leafname);
  static vtkSmartPointer<vtkCellArray> O2MRelationToVTKCellArray(
    vtkIdType numberOfPoints, const conduit_node* o2mrelation);

protected:
  vtkConduitArrayUtilities();
  ~vtkConduitArrayUtilities() override;

  static bool IsDevicePointer(const void* p, int8_t& id);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKArrayImpl(
    const conduit_node* mcarray, bool force_signed);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKAOSArray(
    const conduit_node* mcarray, bool force_signed);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKSOAArray(
    const conduit_node* mcarray, bool force_signed);

private:
  vtkConduitArrayUtilities(const vtkConduitArrayUtilities&) = delete;
  void operator=(const vtkConduitArrayUtilities&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
