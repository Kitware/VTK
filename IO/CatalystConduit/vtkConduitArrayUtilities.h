// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConduitArrayUtilities
 * @brief helper to convert Conduit arrays to VTK arrays.
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

  /**
   * Converts an mcarray to vtkCellArray.
   *
   * This may reinterpret unsigned array as signed arrays to avoid deep-copying
   * of data to match data type expected by vtkCellArray API.
   */
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
  static vtkSmartPointer<vtkCellArray> O2MRelationToVTKCellArray(
    vtkIdType numberOfPoints, const conduit_node* o2mrelation);

protected:
  vtkConduitArrayUtilities();
  ~vtkConduitArrayUtilities() override;

  static bool IsDevicePointer(const void* p, int8_t& id);
  VTK_DEPRECATED_IN_9_6_0("Use MCArrayToVTKArray.")
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKArrayImpl(const conduit_node* mcarray)
  {
    return MCArrayToVTKArray(mcarray);
  }
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKAOSArray(const conduit_node* mcarray);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKSOAArray(const conduit_node* mcarray);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKStridedArray(const conduit_node* mcarray);
  VTK_DEPRECATED_IN_9_6_0("Use the overload without force_signed parameter.")
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKArrayImpl(
    const conduit_node* mcarray, bool vtkNotUsed(force_signed))
  {
    return MCArrayToVTKArray(mcarray);
  }
  VTK_DEPRECATED_IN_9_6_0("Use the overload without force_signed parameter.")
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKAOSArray(
    const conduit_node* mcarray, bool vtkNotUsed(force_signed))
  {
    return MCArrayToVTKAOSArray(mcarray);
  }
  VTK_DEPRECATED_IN_9_6_0("Use the overload without force_signed parameter.")
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKSOAArray(
    const conduit_node* mcarray, bool vtkNotUsed(force_signed))
  {
    return MCArrayToVTKSOAArray(mcarray);
  }
  VTK_DEPRECATED_IN_9_6_0("Use the overload without force_signed parameter.")
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKStridedArray(
    const conduit_node* mcarray, bool vtkNotUsed(force_signed))
  {
    return MCArrayToVTKStridedArray(mcarray);
  }

private:
  vtkConduitArrayUtilities(const vtkConduitArrayUtilities&) = delete;
  void operator=(const vtkConduitArrayUtilities&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
