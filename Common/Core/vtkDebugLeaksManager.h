// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDebugLeaksManager
 * @brief   Manages the vtkDebugLeaks singleton.
 *
 * vtkDebugLeaksManager should be included in any translation unit
 * that will use vtkDebugLeaks or that implements the singleton
 * pattern.  It makes sure that the vtkDebugLeaks singleton is created
 * before and destroyed after all other singletons in VTK.
 */

#ifndef vtkDebugLeaksManager_h
#define vtkDebugLeaksManager_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkDebugLeaksManager
{
public:
  vtkDebugLeaksManager();
  ~vtkDebugLeaksManager();

private:
  vtkDebugLeaksManager(const vtkDebugLeaksManager&) = delete;
  vtkDebugLeaksManager& operator=(const vtkDebugLeaksManager&) = delete;
};

// This instance will show up in any translation unit that uses
// vtkDebugLeaks or that has a singleton.  It will make sure
// vtkDebugLeaks is initialized before it is used and is the last
// static object destroyed.
static vtkDebugLeaksManager vtkDebugLeaksManagerInstance;

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkDebugLeaksManager.h
