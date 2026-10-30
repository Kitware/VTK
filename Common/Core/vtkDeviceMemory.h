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
 * Level Zero reports itself unavailable.
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
   * `destination`. Work already queued on the device is finished first, so the
   * copy sees it. Returns false, with the reason in `why`, if it could not.
   */
  static bool CopyToHost(vtkDataArray::MemorySpace space, void* destination, const void* source,
    std::size_t bytes, std::string* why = nullptr);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkDeviceMemory.h
