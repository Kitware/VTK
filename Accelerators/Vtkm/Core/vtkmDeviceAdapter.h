// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkmDeviceAdapter
 * @brief   Choose which Viskores devices VTK's accelerated filters run on.
 *
 * VTK built with several Viskores devices -- CUDA and TBB, say -- runs each
 * Viskores filter on the best one available. vtkmDeviceAdapter lets that
 * choice be made explicitly: force one device, disable or re-enable a
 * device, or go back to the default.
 *
 * Devices are named as Viskores names them, ignoring case: "Serial", "Cuda",
 * "TBB", "OpenMP", "Kokkos". A request naming a device Viskores does not
 * know, or one this build or machine cannot run, is refused with a warning
 * and leaves the selection as it was.
 *
 * @warning The selection is per thread, as Viskores' runtime device tracker
 * is: it applies to filters executed on the thread that made it. A thread
 * starts from a copy of the first thread to use Viskores -- usually the main
 * thread -- taken when it first runs a Viskores filter, so a selection made
 * on the main thread before then is inherited, and later changes are not.
 */

#ifndef vtkmDeviceAdapter_h
#define vtkmDeviceAdapter_h

#include "vtkAcceleratorsVTKmCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKACCELERATORSVTKMCORE_EXPORT vtkmDeviceAdapter : public vtkObject
{
public:
  static vtkmDeviceAdapter* New();
  vtkTypeMacro(vtkmDeviceAdapter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Run on @a name only, disabling every other device. Returns false if the
   * device is unknown or cannot run here.
   */
  static bool ForceDevice(const char* name);

  /**
   * Stop running on @a name; other devices are unaffected. Returns false if
   * the device is unknown.
   */
  static bool DisableDevice(const char* name);

  /**
   * Undo ForceDevice() or DisableDevice() for @a name, making it available
   * again if it can run here. Returns false if the device is unknown.
   */
  static bool EnableDevice(const char* name);

  /**
   * Make every device this build and machine can run available again. This
   * also undoes a device forced through the VISKORES_DEVICE environment
   * variable.
   */
  static void Reset();

  /**
   * The devices filters can currently run on, separated by spaces.
   */
  static const char* GetAvailableDevices();

protected:
  vtkmDeviceAdapter();
  ~vtkmDeviceAdapter() override;

private:
  vtkmDeviceAdapter(const vtkmDeviceAdapter&) = delete;
  void operator=(const vtkmDeviceAdapter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmDeviceAdapter_h
