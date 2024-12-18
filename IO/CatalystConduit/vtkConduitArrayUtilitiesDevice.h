// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConduitArrayUtilitiesDevice
 * @brief helper to convert Conduit arrays to VTK arrays.
 * @ingroup Insitu
 *
 * vtkConduitArrayUtilitiesDevice is intended to convert Conduit nodes satisfying the
 * `mcarray` protocol to VTK arrays. It uses zero-copy, as much as possible.
 * Currently implementation fails if zero-copy is not possible. In future, that
 * may be changed to do a deep-copy (with appropriate warnings) if necessary.
 *
 * This is primarily designed for use by vtkConduitSource.
 */

#ifndef vtkConduitArrayUtilitiesDevice_h
#define vtkConduitArrayUtilitiesDevice_h

#include "vtkIOCatalystConduitModule.h" // for exports
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include "conduit.h" // for conduit_node

#include "vtkm/cont/DeviceAdapterTag.h" // for vtkm::cont::DeviceAdapterId
#include <string>                       // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;

class VTKIOCATALYSTCONDUIT_EXPORT vtkConduitArrayUtilitiesDevice : public vtkObject
{
public:
  static vtkConduitArrayUtilitiesDevice* New();
  vtkTypeMacro(vtkConduitArrayUtilitiesDevice, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkSmartPointer<vtkDataArray> MCArrayToVTKmAOSArray(const conduit_node* mcarray,
    bool force_signed, const vtkm::cont::DeviceAdapterId& deviceAdapterId);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKmSOAArray(const conduit_node* mcarray,
    bool force_signed, const vtkm::cont::DeviceAdapterId& deviceAdapterId);
  static bool IfVTKmConvertVTKMonoShapedCellArray(vtkIdType numberOfPoints, int cellType,
    vtkIdType cellSize, vtkDataArray* connectivity, vtkCellArray* cellArray);
  static bool IfVTKmConvertVTKMixedCellArray(vtkIdType numberOfPoints, vtkDataArray* offsets,
    vtkDataArray* shapes, vtkDataArray* elements, vtkCellArray* cellArray);
  static bool CanRunOn(const vtkm::cont::DeviceAdapterId& deviceAdapterId);

protected:
  vtkConduitArrayUtilitiesDevice();
  ~vtkConduitArrayUtilitiesDevice() override;

private:
  vtkConduitArrayUtilitiesDevice(const vtkConduitArrayUtilitiesDevice&) = delete;
  void operator=(const vtkConduitArrayUtilitiesDevice&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
