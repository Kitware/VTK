/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInstantiator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkInstantiator - create an instance of any VTK class from its name.
// .SECTION Description
// vtkInstantiator provides an interface to create an instance of any
// VTK class from its name.  Instances are created through registered
// pointers to functions returning the objects.  New classes can also be
// registered with the creator.  VTK libraries automatically register
// their classes with the creator when they are loaded.  Instances are
// created using the static New() method, so the normal vtkObjectFactory
// mechanism is still invoked.
//
// When using this class from language wrappers (Tcl, Python, or Java),
// the vtkInstantiator should be able to create any class from any kit
// that has been loaded.
//
// In C++ code, one should include the header for each kit from which
// one wishes to create instances through vtkInstantiator.  This is
// necessary to ensure proper linking when building static libraries.
// Be careful, though, because including each kit's header means every
// class from that kit will be linked into your executable whether or
// not the class is used.  The headers are:
//
//   vtkCommon    - vtkCommonInstantiator.h
//   vtkFiltering - vtkFilteringInstantiator.h
//   vtkIO        - vtkIOInstantiator.h
//   vtkImaging   - vtkImagingInstantiator.h
//   vtkGraphics  - vtkGraphicsInstantiator.h
//   vtkRendering - vtkRenderingInstantiator.h
//   vtkHybrid    - vtkHybridInstantiator.h
//   vtkParallel  - vtkParallelInstantiator.h
//   vtkPatented  - vtkPatentedInstantiator.h
//
// The VTK_MAKE_INSTANTIATOR() command in CMake is used to automatically
// generate the creator registration for each VTK library.  It can also
// be used to create registration code for VTK-style user libraries
// that are linked to vtkCommon.  After using this command to register
// classes from a new library, the generated header must be included.
//

#ifndef __vtkInstantiator_h
#define __vtkInstantiator_h

#include "vtkObject.h"

class vtkInstantiatorInitialize;
class vtkInstantiatorHashTable;

class VTK_COMMON_EXPORT vtkInstantiator : public vtkObject
{
public:
  static vtkInstantiator* New();
  vtkTypeRevisionMacro(vtkInstantiator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create an instance of the class whose name is given.  If creation
  // fails, a NULL pointer is returned.
  static vtkObject* CreateInstance(const char* className);
  
  //BTX
  typedef vtkObject* (*CreateFunction)();

  // Description:
  // Register a function to create instances of the class whose name
  // is given.  This allows more than one create function to be
  // registered for the same class.  The first one registered is used
  // until it is unregistered.
  static void RegisterInstantiator(const char* className,
                                   CreateFunction createFunction);

  // Description:
  // Unregister the instance creation of the class whose name is
  // given.  This will unregister the function given, but any other
  // function registered for the same class will be left untouched.
  static void UnRegisterInstantiator(const char* className,
                                     CreateFunction createFunction);
  //ETX
  
protected:
  vtkInstantiator();
  ~vtkInstantiator();
  
  // Internal storage for registered creation functions.
  static vtkInstantiatorHashTable* CreatorTable;
  
  static void ClassInitialize();
  static void ClassFinalize();
  
  //BTX
  friend class vtkInstantiatorInitialize;
  //ETX
  
private:
  vtkInstantiator(const vtkInstantiator&);  // Not implemented.
  void operator=(const vtkInstantiator&);  // Not implemented.
};

//BTX
// Utility class to make sure vtkInstantiator is initialized before it
// is used.
class VTK_COMMON_EXPORT vtkInstantiatorInitialize
{
public:
  vtkInstantiatorInitialize();
  ~vtkInstantiatorInitialize();
private:
  static unsigned int Count;
};

// This instance will show up in any translation unit that uses
// vtkInstantiator.  It will make sure vtkInstantiator is initialized
// before it is used.
static vtkInstantiatorInitialize vtkInstantiatorInitializer;
//ETX

#endif
