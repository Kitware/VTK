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
// wglext.h files.  Snapshots of these files are distributed with VTK,
// but you can also set CMake options to use other files.
//
// To use an OpenGL extension, you first need to make an instance of
// vtkOpenGLExtensionManager and give it a vtkRenderWindow.  You can then
// query the vtkOpenGLExtensionManager to see if the extension is supported
// with the ExtensionSupported method.  Valid names for extensions are
// given in the OpenGL extension registry at
// http://www.opengl.org/registry/ .
// You can also grep vtkgl.h (which will be in the binary build directory
// if VTK is not installed) for appropriate names.  There are also
// special extensions GL_VERSION_X_X (where X_X is replaced with a major
// and minor version, respectively) which contain all the constants and
// functions for OpenGL versions for which the gl.h header file is of an
// older version than the driver.
//
// \code
// if (   !extensions->ExtensionSupported("GL_VERSION_1_2")
//     || !extensions->ExtensionSupported("GL_ARB_multitexture") ) {
//   {
//   vtkErrorMacro("Required extensions not supported!");
//   }
// \endcode
//
// Once you have verified that the extensions you want exist, before you
// use them you have to load them with the LoadExtension method.
//
// \code
// extensions->LoadExtension("GL_VERSION_1_2");
// extensions->LoadExtension("GL_ARB_multitexture");
// \endcode
//
// Alternatively, you can use the LoadSupportedExtension method, which checks
// whether the requested extension is supported and, if so, loads it. The
// LoadSupportedExtension method will not raise any errors or warnings if it
// fails, so it is important for callers to pay attention to the return value.
//
// \code
// if (   extensions->LoadSupportedExtension("GL_VERSION_1_2")
//     && extensions->LoadSupportedExtension("GL_ARB_multitexture") ) {
//   {
//   vtkgl::ActiveTexture(vtkgl::TEXTURE0_ARB);
//   }
// else
//   {
//   vtkErrorMacro("Required extensions could not be loaded!");
//   }
// \endcode
//
// Once you have queried and loaded all of the extensions you need, you can
// delete the vtkOpenGLExtensionManager.  To use a constant of an extension,
// simply replace the "GL_" prefix with "vtkgl::".  Likewise, replace the
// "gl" prefix of functions with "vtkgl::".  In rare cases, an extension will
// add a type. In this case, add vtkgl:: to the type (i.e. vtkgl::GLchar).
//
// \code
// extensions->Delete();
// ...
// vtkgl::ActiveTexture(vtkgl::TEXTURE0_ARB);
// \endcode
//
// For wgl extensions, replace the "WGL_" and "wgl" prefixes with
// "vtkwgl::".  For glX extensions, replace the "GLX_" and "glX" prefixes
// with "vtkglX::".
//

#ifndef __vtkOpenGLExtensionManager_h
#define __vtkOpenGLExtensionManager_h

#include <vtkObject.h>
#include <vtkWeakPointer.h> // needed for vtkWeakPointer.

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
  vtkTypeMacro(vtkOpenGLExtensionManager, vtkObject);
  static vtkOpenGLExtensionManager *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/Get the render window to query extensions on.  If set to null,
  // justs queries the current render window.
  vtkRenderWindow* GetRenderWindow();
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
  virtual vtkOpenGLExtensionManagerFunctionPointer GetProcAddress(
    const char *fname);
//ETX

  // Description:
  // Loads all the functions associated with the given extension into the
  // appropriate static members of vtkgl. This method emits a warning if the
  // requested extension is not supported. It emits an error if the extension
  // does not load successfully.
  virtual void LoadExtension(const char *name);

  // Description:
  // Returns true if the extension is supported and loaded successfully,
  // false otherwise. This method will "fail silently/gracefully" if the
  // extension is not supported or does not load properly. It emits neither
  // warnings nor errors. It is up to the caller to determine if the
  // extension loaded properly by paying attention to the return value.
  virtual int LoadSupportedExtension(const char *name);

  
  // Description:
  // Loads all the functions associated with the given core-promoted extension
  // into the appropriate static members of vtkgl associated with the OpenGL
  // version that promoted the extension as a core feature. This method emits a
  // warning if the requested extension is not supported. It emits an error if
  // the extension does not load successfully.
  //
  // For instance, extension GL_ARB_multitexture was promoted as a core
  // feature into OpenGL 1.3. An implementation that uses this
  // feature has to (IN THIS ORDER), check if OpenGL 1.3 is supported
  // with ExtensionSupported("GL_VERSION_1_3"), if true, load the extension
  // with LoadExtension("GL_VERSION_1_3"). If false, test for the extension
  // with ExtensionSupported("GL_ARB_multitexture"),if true load the extension
  // with this method LoadCorePromotedExtension("GL_ARB_multitexture").
  // If any of those loading stage succeeded, use vtgl::ActiveTexture() in
  // any case, NOT vtgl::ActiveTextureARB().
  // This method avoids the use of if statements everywhere in implementations
  // using core-promoted extensions.
  // Without this method, the implementation code should look like:
  // \code
  // int opengl_1_3=extensions->ExtensionSupported("GL_VERSION_1_3");
  // if(opengl_1_3)
  // {
  //   extensions->LoadExtension("GL_VERSION_1_3");
  // }
  // else
  // {
  //  if(extensions->ExtensionSupported("GL_ARB_multitexture"))
  //  {
  //   extensions->LoadCorePromotedExtension("GL_ARB_multitexture");
  //  }
  //  else
  //  {
  //   vtkErrorMacro("Required multitexture feature is not supported!");
  //  }
  // }
  // ...
  // if(opengl_1_3)
  // {
  //  vtkgl::ActiveTexture(vtkgl::TEXTURE0)
  // }
  // else
  // {
  //  vtkgl::ActiveTextureARB(vtkgl::TEXTURE0_ARB)
  // }
  // \endcode
  // Thanks to this method, the code looks like:
  // \code
  // int opengl_1_3=extensions->ExtensionSupported("GL_VERSION_1_3");
  // if(opengl_1_3)
  // {
  //   extensions->LoadExtension("GL_VERSION_1_3");
  // }
  // else
  // {
  //  if(extensions->ExtensionSupported("GL_ARB_multitexture"))
  //  {
  //   extensions->LoadCorePromotedExtension("GL_ARB_multitexture");
  //  }
  //  else
  //  {
  //   vtkErrorMacro("Required multitexture feature is not supported!");
  //  }
  // }
  // ...
  // vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  // \endcode
  virtual void LoadCorePromotedExtension(const char *name);

  // Description:
  // Similar to LoadCorePromotedExtension().
  // It loads an EXT extension into the pointers of its ARB equivalent.
  virtual void LoadAsARBExtension(const char *name);
//BTX
protected:
  vtkOpenGLExtensionManager();
  virtual ~vtkOpenGLExtensionManager();


  int OwnRenderWindow;
  char *ExtensionsString;

  vtkTimeStamp BuildTime;

  virtual void ReadOpenGLExtensions();
  
  // Description:
  // Wrap around the generated vtkgl::LoadExtension to deal with OpenGL 1.2
  // and its optional part GL_ARB_imaging. Also functions like
  // glBlendEquation() or glBlendColor() are optional in OpenGL 1.2 or 1.3 and
  // provided by the GL_ARB_imaging but there are core features in OpenGL 1.4.
  virtual int SafeLoadExtension(const char *name);

private:
  vtkOpenGLExtensionManager(const vtkOpenGLExtensionManager&); // Not implemented
  void operator=(const vtkOpenGLExtensionManager&); // Not implemented

  vtkWeakPointer<vtkRenderWindow> RenderWindow;
//ETX
};

#endif //__vtkOpenGLExtensionManager
