/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFilteringInformationKeyManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFilteringInformationKeyManager - Manages key types in vtkFiltering.
// .SECTION Description
// vtkFilteringInformationKeyManager is included in the header of any
// subclass of vtkInformationKey defined in the vtkFiltering library.
// It makes sure that the table of keys is created before and
// destroyed after it is used.

#ifndef vtkFilteringInformationKeyManager_h
#define vtkFilteringInformationKeyManager_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkSystemIncludes.h"

#include "vtkDebugLeaksManager.h" // DebugLeaks exists longer than info keys.

class vtkInformationKey;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkFilteringInformationKeyManager
{
public:
  vtkFilteringInformationKeyManager();
  ~vtkFilteringInformationKeyManager();

  // Description:
  // Called by constructors of vtkInformationKey subclasses defined in
  // vtkFiltering to register themselves with the manager.  The
  // instances will be deleted when vtkFiltering is unloaded on
  // program exit.
  static void Register(vtkInformationKey* key);

private:
  static void ClassInitialize();
  static void ClassFinalize();
};

// This instance will show up in any translation unit that uses key
// types defined in vtkFiltering or that has a singleton.  It will
// make sure vtkFilteringInformationKeyManager's vector of keys is
// initialized before and destroyed after it is used.
static vtkFilteringInformationKeyManager vtkFilteringInformationKeyManagerInstance;

#endif
// VTK-HeaderTest-Exclude: vtkFilteringInformationKeyManager.h
