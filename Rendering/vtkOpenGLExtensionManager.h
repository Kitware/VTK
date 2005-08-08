// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLExtensionManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

// .NAME vtkOpenGLExtensionManager - Interface class for querying and using OpenGL extensions.
//
// .SECTION Description
//
// vtkOpenGLExtensionManager acts as an interface to OpenGL extensions.  It
// provides methods to query OpenGL extensions on the current or a given
// render window and to load extension function pointers.  Currently does
// not support GLU extensions since the GLU library is not linked to VTK.
//
// Before using vtkOpenGLExtensionManager, an OpenGL context must be created.
// This is generally done with a vtkRenderWindow.  Note that simply creating
// the vtkRenderWindow is not sufficient.  Usually you have to call Render
// before the actual OpenGL context is created.  You can specify the
// RenderWindow with the SetRenderWindow method.
// \code
// vtkOpenGLExtensionManager *extensions = vtkOpenGLExtensionManager::New();
// extensions->SetRenderWindow(renwin);
// \endcode
// If no vtkRenderWindow is specified, the current OpenGL context (if any)
// is used.
//
// Generally speaking, when using OpenGL extensions, you will need an
// vtkOpenGLExtensionManager and the prototypes defined in vtkgl.h.
// \code
// #include "vtkOpenGLExtensionManager.h"
// #include "vtkgl.h"
// \endcode
// The vtkgl.h include file contains all the constants and function
// pointers required for using OpenGL extensions in a portable and
// namespace safe way.  vtkgl.h is built from parsed glext.h, glxext.h, and
// wglext.h files.  Snapshots of these files are distributed with vtkSNL,
// but you can also set CMake options to use other files.
//
// To use an OpenGL extension, you first need to make an instance of
// vtkOpenGLExtensionManager and give it a vtkRenderWindow.  You can then
// query the vtkOpenGLExtensionManager to see if the extension is supported
// with the ExtensionSupported method.  Valid names for extensions are
// given in the OpenGL extension registry at
// \ref http://oss.sgi.com/projects/ogl-sample/registry/ .
// You can also grep vtkgl.h (which will be in the binary build directory
// if vtkSNL is not installed) for appropriate names.  There are also
// special extensions GL_VERSION_X_X (where X_X is replaced with a major
// and minor version, respectively) which contain all the constants and
// functions for OpenGL versions for which the gl.h header file is of an
// older version than the driver.
//
// \code
// if (   !extensions->ExtensionSupported("GL_VERSION_1_2")
//     || !extensions->ExtensionSupported("GL_ARB_multitexture") ) {
//  {
//    vtkErrorMacro("Required extensions not supported!");
//  }
// \endcode
//
// Once you have verified that the extensions you want exist, before you
// use them you have to loaded them with the LoadExtension method.
//
// \code
// extensions->LoadExtension("GL_VERSION_1_2");
// extensions->LoadExtension("GL_ARB_multitexture");
// \endcode
//
// Once you have queried and loaded all of the extensions you need, you can
// delete the vtkExtensionManager.  To use a constant of an extension, simply
// replace the "GL_" prefix with "vtkgl::".  Likewise, replace the "gl" prefix
// of functions with "vtkgl::".  In rare cases, an extension will add a type.
// In this case, add vtkgl:: to the type (i.e. vtkgl::GLchar).
//
// \code
// extensions->Delete();
// ...
// vtkgl::ActiveTexture(vtkgl::TEXTURE0_ARB);
// \endcode
// For wgl extensions, replace the "WGL_" and "wgl" prefixes with
// "vtkwgl::".  For glX extensions, replace the "GLX_" and "glX" prefixes
// with "vtkglX::".
//

#ifndef __vtkOpenGLExtensionManager
#define __vtkOpenGLExtensionManager

#include <vtkObject.h>

class vtkRenderWindow;

//BTX
extern "C" {
#ifdef _WIN32
#include <vtkOpenGL.h>  // Needed for WINAPI
  typedef int (WINAPI *vtkOpenGLExtensionManagerFunctionPointer)(void);
#else
  typedef void (*vtkOpenGLExtensionManagerFunctionPointer)(void);
#endif
}
//ETX

class VTK_RENDERING_EXPORT vtkOpenGLExtensionManager : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkOpenGLExtensionManager, vtkObject);
  static vtkOpenGLExtensionManager *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/Get the render window to query extensions on.  If set to null,
  // justs queries the current render window.
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  virtual void SetRenderWindow(vtkRenderWindow *renwin);

  // Description:
  // Updates the extensions string.
  virtual void Update();

  // Description:
  // Returns a string listing all available extensions.  Call Update first
  // to validate this string.
  vtkGetStringMacro(ExtensionsString);

  // Description:
  // Returns true if the extension is supported, false otherwise.
  virtual int ExtensionSupported(const char *name);

//BTX
  // Description:
  // Returns a function pointer to the OpenGL extension function with the
  // given name.  Returns NULL if the function could not be retrieved.
  virtual vtkOpenGLExtensionManagerFunctionPointer GetProcAddress(const char *fname);
//ETX

  // Description:
  // Loads all the functions associated with the given extension into the
  // appropriate static members of vtkgl.
  virtual void LoadExtension(const char *name);

protected:
  vtkOpenGLExtensionManager();
  virtual ~vtkOpenGLExtensionManager();

  vtkRenderWindow *RenderWindow;

  char *ExtensionsString;

  vtkTimeStamp BuildTime;

  virtual void ReadOpenGLExtensions();

private:
  vtkOpenGLExtensionManager(const vtkOpenGLExtensionManager&); // Not implemented
  void operator=(const vtkOpenGLExtensionManager&); // Not implemented
};

#endif //__vtkOpenGLExtensionManager

