/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDynamicLoader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to William A. Hoffman who developed this class


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
// .NAME vtkDynamicLoader - class interface to system dynamic libraries
// .SECTION Description
// vtkDynamicLoader provides a portable interface to loading dynamic 
// libraries into a process.  


#ifndef __vtkDynamicLoader_h
#define __vtkDynamicLoader_h

//BTX
// Ugly stuff for library handles
// They are different on several different OS's
#if defined(__hpux)
# include <dl.h>
  typedef shl_t vtkLibHandle;
#elif defined(_WIN32)
# include "windows.h"
  typedef HMODULE vtkLibHandle;
#elif defined(__powerpc)
  typedef ConnectionID vtkLibHandle;
#else
  typedef void* vtkLibHandle;
#endif
//ETX

#include "vtkObject.h"


class VTK_EXPORT vtkDynamicLoader : public vtkObject
{
public:
  static vtkDynamicLoader *New() {return new vtkDynamicLoader;};
  const char *GetClassName() {return "vtkDynamicLoader";};

  // Description:
  // Load a dynamic library into the current process.
  // The returned vtkLibHandle can be used to access the symbols in the 
  // library.
  static vtkLibHandle OpenLibrary(const char*);

  // Description:
  // Attempt to detach a dynamic library from the
  // process.  A value of true is returned if it is sucessful.
  static int CloseLibrary(vtkLibHandle);

  // Description:
  // Find the address of the symbol in the given library
  static void* GetSymbolAddress(vtkLibHandle, const char*);

  // Description:
  // Return the library prefix for the given architecture
  static const char* LibPrefix();

  // Description:
  // Return the library extension for the given architecture
  static const char* LibExtension();

  // Description:
  // Return the last error produced from a calls made on this class.
  static const char* LastError();
  
protected:
  vtkDynamicLoader() {};
  ~vtkDynamicLoader() {};
  vtkDynamicLoader(const vtkDynamicLoader&) {};
  void operator=(const vtkDynamicLoader&) {};

  
};

#endif
