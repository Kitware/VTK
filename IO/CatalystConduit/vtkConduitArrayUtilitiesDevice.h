// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConduitArrayUtilitiesDevice
 * @brief helper to convert Conduit arrays stored on acceleration devices to Viskores arrays.
 * @ingroup Insitu
 *
 * vtkConduitArrayUtilitiesDevice is intended to convert Conduit nodes satisfying the
 * `mcarray` protocol, with memory allocated on acceleration devices, to Viskores arrays. It uses
 * zero-copy, when possible otherwise it uses deep copy.
 *
 * This is primarily designed for use by vtkConduitSource.
 */

#ifndef vtkConduitArrayUtilitiesDevice_h
#define vtkConduitArrayUtilitiesDevice_h

#include "vtkIOCatalystConduitModule.h" // for exports
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include "conduit.h" // for conduit_node

#include "viskores/cont/DeviceAdapterTag.h" // for viskores::cont::DeviceAdapterId
#include <string>                           // for std::string

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
    bool force_signed, const viskores::cont::DeviceAdapterId& deviceAdapterId);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKmSOAArray(const conduit_node* mcarray,
    bool force_signed, const viskores::cont::DeviceAdapterId& deviceAdapterId);
  static bool IfVTKmConvertVTKMonoShapedCellArray(vtkIdType numberOfPoints, int cellType,
    vtkIdType cellSize, vtkDataArray* connectivity, vtkCellArray* cellArray);
  static bool IfVTKmConvertVTKMixedCellArray(vtkIdType numberOfPoints, vtkDataArray* offsets,
    vtkDataArray* shapes, vtkDataArray* elements, vtkCellArray* cellArray);
  static bool CanRunOn(const viskores::cont::DeviceAdapterId& deviceAdapterId);

protected:
  vtkConduitArrayUtilitiesDevice();
  ~vtkConduitArrayUtilitiesDevice() override;

private:
  vtkConduitArrayUtilitiesDevice(const vtkConduitArrayUtilitiesDevice&) = delete;
  void operator=(const vtkConduitArrayUtilitiesDevice&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
