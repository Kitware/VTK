/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCriticalSection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCriticalSection
 * @brief   Critical section locking class
 *
 * vtkCriticalSection allows the locking of variables which are accessed
 * through different threads.  This header file also defines
 * vtkSimpleCriticalSection which is not a subclass of vtkObject.
 * The API is identical to that of vtkMutexLock, and the behavior is
 * identical as well, except on Windows 9x/NT platforms. The only difference
 * on these platforms is that vtkMutexLock is more flexible, in that
 * it works across processes as well as across threads, but also costs
 * more, in that it evokes a 600-cycle x86 ring transition. The
 * vtkCriticalSection provides a higher-performance equivalent (on
 * Windows) but won't work across processes. Since it is unclear how,
 * in vtk, an object at the vtk level can be shared across processes
 * in the first place, one should use vtkCriticalSection unless one has
 * a very good reason to use vtkMutexLock. If higher-performance equivalents
 * for non-Windows platforms (Irix, SunOS, etc) are discovered, they
 * should replace the implementations in this class
*/

#ifndef vtkCriticalSection_h
#define vtkCriticalSection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSimpleCriticalSection.h" // For simple critical section

class VTKCOMMONCORE_EXPORT vtkCriticalSection : public vtkObject
{
public:
  static vtkCriticalSection *New();

  vtkTypeMacro(vtkCriticalSection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Lock the vtkCriticalSection
   */
  void Lock();

  /**
   * Unlock the vtkCriticalSection
   */
  void Unlock();

protected:
  vtkSimpleCriticalSection SimpleCriticalSection;
  vtkCriticalSection() {}
  ~vtkCriticalSection() VTK_OVERRIDE {}

private:
  vtkCriticalSection(const vtkCriticalSection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCriticalSection&) VTK_DELETE_FUNCTION;
};


inline void vtkCriticalSection::Lock()
{
  this->SimpleCriticalSection.Lock();
}

inline void vtkCriticalSection::Unlock()
{
  this->SimpleCriticalSection.Unlock();
}

#endif
