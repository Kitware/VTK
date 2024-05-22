// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariProfiling
 * @brief   Allows instrumenting of the VTK ANARI source code
 *
 * This class uses NVTX (NVIDIA Tools Extension Library) for annotating source code
 * to provide contextual information for further analysis and profiling. If NVTX
 * wasn't enabled during the build process, then the usage of the class will essentially
 * be a no-op.
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariProfiling_h
#define vtkAnariProfiling_h

#include "vtkRenderingAnariModule.h" // For export macro
#include <cstdint>                   // For ivars

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGANARI_EXPORT vtkAnariProfiling
{
public:
  vtkAnariProfiling();
  vtkAnariProfiling(const char* label, uint32_t color);
  ~vtkAnariProfiling();

  /**
   * 32-bit ARGB color. 0x[alpha][red][green][blue]
   */
  static constexpr uint32_t BROWN = 0xFF8B4513;
  static constexpr uint32_t RED = 0xFFFF0000;
  static constexpr uint32_t MAROON = 0xFF800000;
  static constexpr uint32_t YELLOW = 0xFFFFFF00;
  static constexpr uint32_t GOLD = 0xFFFFD700;
  static constexpr uint32_t GREEN = 0xFF008000;
  static constexpr uint32_t LIME = 0xFF00FF00;
  static constexpr uint32_t BLUE = 0xFF0000FF;
  static constexpr uint32_t AQUA = 0xFF00FFFF;

private:
  /**
   * Marks the start of a profiling range.
   * @param label the name given to the profiling range
   * @param colorName the color, by name, to use for this profiling range
   */
  void StartProfiling(const char* label, uint32_t color);

  /**
   * Marks the end of the profiling range.
   */
  void StopProfiling();
};

VTK_ABI_NAMESPACE_END
#endif
