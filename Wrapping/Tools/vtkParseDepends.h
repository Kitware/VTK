// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWrappingToolsModule.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Initialize the global dependency tracking structure for a given output
   * file.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_InitDependencyTracking(const char* output);

  /**
   * Add a dependency to the output.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_AddDependency(const char* dep);

  /**
   * Write dependency tracking information to a file.
   *
   * Returns non-zero on error.
   */
  VTKWRAPPINGTOOLS_EXPORT
  int vtkParse_WriteDependencyFile(const char* fname);

  /**
   * Finalize the dependency tracking structure.
   */
  VTKWRAPPINGTOOLS_EXPORT
  void vtkParse_FinalizeDependencyTracking(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
/* VTK-HeaderTest-Exclude: vtkParseDepends.h */
