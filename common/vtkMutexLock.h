/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutexLock.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkMutexLock - mutual exclusion locking class
// .SECTION Description
// vtkMutexLock allows the locking of variables which are accessed 
// through different threads

#ifndef __vtkMutexVariable_h
#define __vtkMutexVariable_h

#include "vtkObject.h"

//BTX

#ifdef VTK_USE_SPROC
#include <abi_mutex.h>
typedef abilock_t vtkMutexType;
#endif

#ifdef VTK_USE_PTHREADS
#include <pthread.h>
typedef pthread_mutex_t vtkMutexType;
#endif
 
#ifdef _WIN32
#include <winbase.h>
typedef HANDLE vtkMutexType;
#endif

#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
#ifndef _WIN32
typedef int vtkMutexType;
#endif
#endif
#endif

//ETX

class VTK_EXPORT vtkMutexLock : public vtkObject
{
public:
  vtkMutexLock();
  ~vtkMutexLock();
  static vtkMutexLock *New() {return new vtkMutexLock;};
  const char *GetClassName() {return "vtkMutexLock";};

  // Description:
  // Print method for vtkMutexLock
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Lock the vtkMutexLock
  void Lock( void );

  // Description:
  // Unlock the vtkMutexLock
  void Unlock( void );

protected:
  vtkMutexType   MutexLock;
};

#endif
