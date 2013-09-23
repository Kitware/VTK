/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInstantiator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
//   vtkCommon          - vtkCommonInstantiator.h
//   vtkFiltering       - vtkFilteringInstantiator.h
//   vtkIO              - vtkIOInstantiator.h
//   vtkImaging         - vtkImagingInstantiator.h
//   vtkGraphics        - vtkGraphicsInstantiator.h
//   vtkRendering       - vtkRenderingInstantiator.h
//   vtkVolumeRendering - vtkVolumeRenderingInstantiator.h
//   vtkHybrid          - vtkHybridInstantiator.h
//   vtkParallel        - vtkParallelInstantiator.h
//
// The VTK_MAKE_INSTANTIATOR() command in CMake is used to automatically
// generate the creator registration for each VTK library.  It can also
// be used to create registration code for VTK-style user libraries
// that are linked to vtkCommon.  After using this command to register
// classes from a new library, the generated header must be included.
//

#ifndef __vtkInstantiator_h
#define __vtkInstantiator_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

// The vtkDebugLeaks singleton must be initialized before and
// destroyed after the vtkInstantiator singleton.
#include "vtkDebugLeaksManager.h" // Needed for proper singleton initialization

class vtkInstantiatorInitialize;
class vtkInstantiatorHashTable;

class VTKCOMMONCORE_EXPORT vtkInstantiator : public vtkObject
{
public:
  static vtkInstantiator* New();
  vtkTypeMacro(vtkInstantiator,vtkObject);
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
class VTKCOMMONCORE_EXPORT vtkInstantiatorInitialize
{
public:
  vtkInstantiatorInitialize();
  ~vtkInstantiatorInitialize();
private:
  static unsigned int Count;
private:
  vtkInstantiatorInitialize(const vtkInstantiatorInitialize& other); // no copy constructor
  vtkInstantiatorInitialize& operator=(const vtkInstantiatorInitialize& rhs); // no copy assignment
};

// This instance will show up in any translation unit that uses
// vtkInstantiator.  It will make sure vtkInstantiator is initialized
// before it is used.
static vtkInstantiatorInitialize vtkInstantiatorInitializer;
//ETX

#endif
