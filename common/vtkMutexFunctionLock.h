/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutexFunctionLock.h
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
// .NAME vtkMutexFunctionLock - represents a function which may lock using mutual exclusion
// .SECTION Description
// vtkMutexFunctionLock is an object that allows any function or C++ commands
// to be run using mutual exclusion. The macro vtkMutexLockFuncMacro should
// be used when a command should be run between a Lock and Unlock. This macro
// accepts the vtkMutexFunctionLock as the first argument and the C++
// commands as the second argument.

#ifndef __vtkMutexFunctionLock_h
#define __vtkMutexFunctionLock_h

#include "vtkObject.h"
#include "vtkMutexLock.h"

//BTX
#define vtkMutexLockFuncMacro(mutfunc_obj,func) \
{ \
    (mutfunc_obj)->StartLock(); \
    func; \
    (mutfunc_obj)->EndLock(); \
}
//ETX

class VTK_EXPORT vtkMutexFunctionLock : public vtkObject 
{
public:
  vtkMutexFunctionLock();
  ~vtkMutexFunctionLock();
  static vtkMutexFunctionLock *New() {return new vtkMutexFunctionLock;};
  const char *GetClassName() {return "vtkMutexFunctionLock";};

  // Description:
  // Print method for vtkMutexFunctionLock
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Lock method for vtkMutexFunctionLock
  void StartLock(void);

  // Description:
  // Unlock method for vtkMutexFunctionLock
  void EndLock(void);

protected:
  vtkMutexLock *MutexVar;

};

#endif
