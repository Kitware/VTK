/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectFactory.h
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
// .NAME vtkObjectFactory - abstract base class for vtkObjectFactories
// .SECTION Description
// vtkObjectFactory is used to create vtk objects.   The base class
// vtkObjectFactory contains a static method CreateInstance which is used
// to create vtk objects from the list of registerd vtkObjectFactory 
// sub-classes.   The first time CreateInstance is called, all dll's or shared
// libraries in the environment variable VTK_AUTOLOAD_PATH are loaded into
// the current process.   The C function vtkLoad is called on each dll.  
// vtkLoad should return an instance of the factory sub-class implemented
// in the shared library. VTK_AUTOLOAD_PATH is an environment variable 
// containing a colon separated (semi-colon on win32) list of paths.
//
// This can be use to overide the creation of any object in VTK.  
//



#ifndef __vtkObjectFactory_h
#define __vtkObjectFactory_h




#include "vtkObject.h"

class vtkObjectFactoryCollection;

class VTK_EXPORT vtkObjectFactory : public vtkObject
{
public:  
  // Methods from vtkObject
  virtual const char *GetClassName() {return "vtkObjectFactory";};
  // Description:
  // Print ObjectFactor to stream.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Class Methods used to interface with the registered factories
  
  // Description:
  // Create and return an instance of the named vtk object.
  // Each loaded vtkObjectFactory will be asked in the order
  // the factory was in the VTK_AUTOLOAD_PATH.  After the
  // first factory returns the object no other factories are asked.
  static vtkObject* CreateInstance(const char* vtkclassname);
  // Description:
  // Re-check the VTK_AUTOLOAD_PATH for new factory libraries.
  // This calls UnRegisterAll before re-loading
  static void ReHash(); 
  // Description:
  // Register a factory so it can be used to create vtk objects
  static void RegisterFactory(vtkObjectFactory* );
  // Description:
  // Remove a factory from the list of registered factories
  static void UnRegisterFactory(vtkObjectFactory*);
  // Description:
  // Unregister all factories
  static void UnRegisterAllFactories();
  
protected:
  // Description:
  // This method is provioded by sub-classes of vtkObjectFactory.
  // It should create the named vtk object or return 0 if that object
  // is not supported by the factory implementation.
  virtual vtkObject* CreateObject(const char* vtkclassname ) = 0;
  
  
  vtkObjectFactory();
  ~vtkObjectFactory();
  vtkObjectFactory(const vtkObjectFactory&) {};
  void operator=(const vtkObjectFactory&) {};
private:
  // Description:
  // Initialize the static members of vtkObjectFactory.   RegisterDefaults
  // is called here.
  static void Init();
  // Description:
  // Register default factories which are not loaded at run time.
  static void RegisterDefaults();
  // Description:
  // Load dynamic factories from the VTK_AUTOLOAD_PATH
  static void LoadDynamicFactories();
  // Description:
  // Load all dynamic libraries in the given path
  static void LoadLibrariesInPath( const char*);
  
  // list of registered factories
  static vtkObjectFactoryCollection* RegisterdFactories; 
  
  // member variables for a factory set by the base class
  // at load or register time
  void* LibraryHandle;
  unsigned long LibraryDate;
  char* LibraryPath;
};

#endif
