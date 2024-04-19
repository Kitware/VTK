// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCommonInformationKeyManager
 * @brief   Manages key types in vtkCommon.
 *
 * vtkCommonInformationKeyManager is included in the header of any
 * subclass of vtkInformationKey defined in the vtkCommon library.
 * It makes sure that the table of keys is created before and
 * destroyed after it is used.
 */

#ifndef vtkCommonInformationKeyManager_h
#define vtkCommonInformationKeyManager_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

#include "vtkDebugLeaksManager.h" // DebugLeaks exists longer than info keys.

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationKey;

class VTKCOMMONCORE_EXPORT vtkCommonInformationKeyManager
{
public:
  vtkCommonInformationKeyManager();
  ~vtkCommonInformationKeyManager();

  /**
   * Called by constructors of vtkInformationKey subclasses defined in
   * vtkCommon to register themselves with the manager.  The
   * instances will be deleted when vtkCommon is unloaded on
   * program exit.
   */
  static void Register(vtkInformationKey* key);

private:
  // Unimplemented
  vtkCommonInformationKeyManager(const vtkCommonInformationKeyManager&) = delete;
  vtkCommonInformationKeyManager& operator=(const vtkCommonInformationKeyManager&) = delete;

  static void ClassInitialize();
  static void ClassFinalize();
};

// This instance will show up in any translation unit that uses key
// types defined in vtkCommon or that has a singleton.  It will
// make sure vtkCommonInformationKeyManager's vector of keys is
// initialized before and destroyed after it is used.
static vtkCommonInformationKeyManager vtkCommonInformationKeyManagerInstance;

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkCommonInformationKeyManager.h
