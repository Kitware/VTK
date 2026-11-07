// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDeviceMemory
 * @brief   reads GPU memory through the vendor's runtime, loaded when first needed
 *
 * vtkDeviceDataArray holds memory another library allocated on a GPU. To read
 * it on the host it needs a copy, and the copy needs the vendor's runtime. VTK
 * does not link against one: this class loads it the first time it is asked,
 * so a VTK built without any GPU toolkit can still hold and read GPU memory on
 * a machine that has the driver.
 *
 * CUDA through the driver API (`libcuda.so.1`, `nvcuda.dll`), which every
 * NVIDIA driver installs. HIP through ROCm's runtime (`libamdhip64.so.7`,
 * `amdhip64_7.dll`, or version 6), which every HIP program runs on: the one the
 * process has loaded already, if any, since two copies of it share nothing.
 * ROCm 5's runtime is refused: it numbers memory types differently.
 * Level Zero through its loader (`libze_loader.so.1`, `ze_loader.dll`), which
 * Intel's GPU driver installs.
 *
 * A Level Zero pointer means something only in the context that allocated it,
 * so VTK keeps one Level Zero context for the process: one a producer gave it
 * (SetLevelZeroContext), or else one it makes on the first GPU the loader
 * reports, the first time it needs one. Pointers are checked and copied in it.
 *
 * Nothing here allocates or frees: the memory belongs to whoever made it.
 */

#ifndef vtkDeviceMemory_h
#define vtkDeviceMemory_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"        // For vtkDataArray::MemorySpace

#include <cstddef>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkDeviceMemory
{
public:
  /**
   * The memory space a vtkMemoryDescriptor names ("cuda", "hip",
   * "level_zero"). Returns false for "host" and for names it does not know.
   */
  static bool SpaceFromName(const char* name, vtkDataArray::MemorySpace& space);

  /**
   * The name of a device memory space, as vtkMemoryDescriptor spells it.
   */
  static const char* NameOf(vtkDataArray::MemorySpace space);

  /**
   * Whether the runtime for `space` could be loaded. When it could not, `why`
   * (if given) says why.
   */
  static bool IsAvailable(vtkDataArray::MemorySpace space, std::string* why = nullptr);

  /**
   * Whether `pointer` is memory of `space` that the runtime knows: device or
   * managed memory, not ordinary host memory.
   */
  static bool IsDevicePointer(
    vtkDataArray::MemorySpace space, const void* pointer, std::string* why = nullptr);

  /**
   * Copy `bytes` from device memory at `source` to host memory at
   * `destination`. Returns false, with the reason in `why`, if it could not.
   *
   * For CUDA and HIP, work already queued on the memory's device is finished
   * first, so the copy sees it. Level Zero has no device-wide synchronize:
   * the copy runs on VTK's own command list and does not wait for work a
   * producer queued elsewhere (a SYCL queue, say), so the producer must
   * finish writing before VTK reads.
   */
  static bool CopyToHost(vtkDataArray::MemorySpace space, void* destination, const void* source,
    std::size_t bytes, std::string* why = nullptr);

  /**
   * The number of the device `pointer` is on, as the vendor's runtime counts
   * devices (and DLPack's device_id does): CUDA's and HIP's device ordinal, or
   * the device's index among its Level Zero driver's devices. -1, with the
   * reason in `why`, if it cannot be told.
   */
  static int DeviceOrdinal(
    vtkDataArray::MemorySpace space, const void* pointer, std::string* why = nullptr);

  ///@{
  /**
   * VTK's Level Zero context and the driver and device it is on (the
   * `ze_*_handle_t`s). SetLevelZeroContext makes VTK use another library's;
   * set it before any Level Zero array is made, since arrays copy in whichever
   * context is current. A null context forgets it (a context VTK made is kept
   * alive, not destroyed). HasLevelZeroContext says whether there is one
   * without making one; GetLevelZeroContext makes one if there is none, and
   * returns false, with the reason in `why`, if it cannot.
   */
  static void SetLevelZeroContext(void* driver, void* device, void* context);
  static bool HasLevelZeroContext();
  static bool GetLevelZeroContext(
    void*& driver, void*& device, void*& context, std::string* why = nullptr);
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkDeviceMemory.h
