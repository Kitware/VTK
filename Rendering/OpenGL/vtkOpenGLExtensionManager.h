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

/**
 * @class   vtkOpenGLExtensionManager
 * @brief   Interface class for querying and using OpenGL extensions.
 *
 *
 *
 * vtkOpenGLExtensionManager acts as an interface to OpenGL extensions.  It
 * provides methods to query OpenGL extensions on the current or a given
 * render window and to load extension function pointers.  Currently does
 * not support GLU extensions since the GLU library is not linked to VTK.
 *
 * Before using vtkOpenGLExtensionManager, an OpenGL context must be created.
 * This is generally done with a vtkRenderWindow.  Note that simply creating
 * the vtkRenderWindow is not sufficient.  Usually you have to call Render
 * before the actual OpenGL context is created.  You can specify the
 * RenderWindow with the SetRenderWindow method.
 * \code
 * vtkOpenGLExtensionManager *extensions = vtkOpenGLExtensionManager::New();
 * extensions->SetRenderWindow(renwin);
 * \endcode
 * If no vtkRenderWindow is specified, the current OpenGL context (if any)
 * is used.
 *
 * Generally speaking, when using OpenGL extensions, you will need an
 * vtkOpenGLExtensionManager and the prototypes defined in vtkgl.h.
 * \code
*/

#include "vtkRenderingOpenGLModule.h" // For export macro
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

#ifndef vtkOpenGLExtensionManager_h
#define vtkOpenGLExtensionManager_h

#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
#include <string> // needed for std::string

class vtkRenderWindow;

extern "C" {
#ifdef _WIN32
#include "vtkOpenGL.h"  // Needed for WINAPI
  typedef int (WINAPI *vtkOpenGLExtensionManagerFunctionPointer)(void);
#else
  typedef void (*vtkOpenGLExtensionManagerFunctionPointer)(void);
#endif
}

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLExtensionManager : public vtkObject
{
public:
  vtkTypeMacro(vtkOpenGLExtensionManager, vtkObject);
  static vtkOpenGLExtensionManager *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  //@{
  /**
   * Set/Get the render window to query extensions on.  If set to null,
   * justs queries the current render window.
   */
  vtkRenderWindow* GetRenderWindow();
  virtual void SetRenderWindow(vtkRenderWindow *renwin);
  //@}

  /**
   * Updates the extensions string.
   */
  virtual void Update();

  //@{
  /**
   * Returns a string listing all available extensions.  Call Update first
   * to validate this string.
   */
  vtkGetStringMacro(ExtensionsString);
  //@}

  /**
   * Returns true if the extension is supported, false otherwise.
   */
  virtual int ExtensionSupported(const char *name);

  /**
   * Returns a function pointer to the OpenGL extension function with the
   * given name.  Returns NULL if the function could not be retrieved.
   */
  virtual vtkOpenGLExtensionManagerFunctionPointer GetProcAddress(
    const char *fname);

  /**
   * Loads all the functions associated with the given extension into the
   * appropriate static members of vtkgl. This method emits a warning if the
   * requested extension is not supported. It emits an error if the extension
   * does not load successfully.
   */
  virtual void LoadExtension(const char *name);

  /**
   * Returns true if the extension is supported and loaded successfully,
   * false otherwise. This method will "fail silently/gracefully" if the
   * extension is not supported or does not load properly. It emits neither
   * warnings nor errors. It is up to the caller to determine if the
   * extension loaded properly by paying attention to the return value.
   */
  virtual int LoadSupportedExtension(const char *name);


  /**
   * Loads all the functions associated with the given core-promoted extension
   * into the appropriate static members of vtkgl associated with the OpenGL
   * version that promoted the extension as a core feature. This method emits a
   * warning if the requested extension is not supported. It emits an error if
   * the extension does not load successfully.

   * For instance, extension GL_ARB_multitexture was promoted as a core
   * feature into OpenGL 1.3. An implementation that uses this
   * feature has to (IN THIS ORDER), check if OpenGL 1.3 is supported
   * with ExtensionSupported("GL_VERSION_1_3"), if true, load the extension
   * with LoadExtension("GL_VERSION_1_3"). If false, test for the extension
   * with ExtensionSupported("GL_ARB_multitexture"),if true load the extension
   * with this method LoadCorePromotedExtension("GL_ARB_multitexture").
   * If any of those loading stage succeeded, use vtgl::ActiveTexture() in
   * any case, NOT vtgl::ActiveTextureARB().
   * This method avoids the use of if statements everywhere in implementations
   * using core-promoted extensions.
   * Without this method, the implementation code should look like:
   * \code
   * int opengl_1_3=extensions->ExtensionSupported("GL_VERSION_1_3");
   * if(opengl_1_3)
   * {
   * extensions->LoadExtension("GL_VERSION_1_3");
   * }
   * else
   * {
   * if(extensions->ExtensionSupported("GL_ARB_multitexture"))
   * {
   * extensions->LoadCorePromotedExtension("GL_ARB_multitexture");
   * }
   * else
   * {
   * vtkErrorMacro("Required multitexture feature is not supported!");
   * }
   * }
   * ...
   * if(opengl_1_3)
   * {
   * vtkgl::ActiveTexture(vtkgl::TEXTURE0)
   * }
   * else
   * {
   * vtkgl::ActiveTextureARB(vtkgl::TEXTURE0_ARB)
   * }
   * \endcode
   * Thanks to this method, the code looks like:
   * \code
   * int opengl_1_3=extensions->ExtensionSupported("GL_VERSION_1_3");
   * if(opengl_1_3)
   * {
   * extensions->LoadExtension("GL_VERSION_1_3");
   * }
   * else
   * {
   * if(extensions->ExtensionSupported("GL_ARB_multitexture"))
   * {
   * extensions->LoadCorePromotedExtension("GL_ARB_multitexture");
   * }
   * else
   * {
   * vtkErrorMacro("Required multitexture feature is not supported!");
   * }
   * }
   * ...
   * vtkgl::ActiveTexture(vtkgl::TEXTURE0);
   * \endcode
   */
  virtual void LoadCorePromotedExtension(const char *name);

  /**
   * Similar to LoadCorePromotedExtension().
   * It loads an EXT extension into the pointers of its ARB equivalent.
   */
  virtual void LoadAsARBExtension(const char *name);

  /**
   * Return the driver's version parts. This may be used for
   * fine grained feature testing.
   */
  virtual int GetDriverVersionMajor(){ return this->DriverVersionMajor; }
  virtual int GetDriverVersionMinor(){ return this->DriverVersionMinor; }
  virtual int GetDriverVersionPatch(){ return this->DriverVersionPatch; }

  /**
   * Get GL API version that the driver provides. This is
   * often different than the GL version that VTK recognizes
   * so only use this for identifying a specific driver.
   */
  virtual int GetDriverGLVersionMajor(){ return this->DriverGLVersionMajor; }
  virtual int GetDriverGLVersionMinor(){ return this->DriverGLVersionMinor; }
  virtual int GetDriverGLVersionPatch(){ return this->DriverGLVersionPatch; }

  //@{
  /**
   * Test's for common implementors of rendering drivers. This may be used for
   * fine grained feature testing. Note: DriverIsMesa succeeds for OS Mesa,
   * use DriverGLRendererIsOSMessa to differentiate.
   */
  virtual bool DriverIsATI();
  virtual bool DriverIsNvidia();
  virtual bool DriverIsIntel();
  virtual bool DriverIsMesa();
  virtual bool DriverIsMicrosoft();
  //@}

  //@{
  /**
   * Test for a specific driver version.
   */
  virtual bool DriverVersionIs(int major);
  virtual bool DriverVersionIs(int major, int minor);
  virtual bool DriverVersionIs(int major, int minor, int patch);
  //@}

  //@{
  /**
   * Test for driver version greater than or equal
   * to the named version.
   */
  virtual bool DriverVersionAtLeast(int major);
  virtual bool DriverVersionAtLeast(int major, int minor);
  virtual bool DriverVersionAtLeast(int major, int minor, int patch);
  //@}

  //@{
  /**
   * Test for the driver's GL version as reported in
   * its GL_VERSION string. This is intended for driver
   * identification only, use ExtensionSuppported
   * to test for VTK support of a specific GL version.
   */
  virtual bool DriverGLVersionIs(int major, int minor, int patch);
  virtual bool DriverGLVersionIs(int major, int minor);
  //@}

  //@{
  /**
   * Test for a specific renderer. This could be used
   * in some cases to identify the graphics card or
   * specific driver. Use HasToken to prevent false
   * matches eg. avoid GeForce4 matching GeForce400
   */
  virtual bool DriverGLRendererIs(const char *str);
  virtual bool DriverGLRendererHas(const char *str);
  virtual bool DriverGLRendererHasToken(const char *str);
  //@}

  /**
   * Test for Mesa's offscreen renderer.
   */
  virtual bool DriverGLRendererIsOSMesa();

  /**
   * Get the OpenGL version, vendor and renderer strings. These can
   * be used to idnetify a specific driver.
   */
  virtual const char *GetDriverGLVendor(){ return this->DriverGLVendor.c_str(); }
  virtual const char *GetDriverGLVersion(){ return this->DriverGLVersion.c_str(); }
  virtual const char *GetDriverGLRenderer(){ return this->DriverGLRenderer.c_str(); }

  //@{
  /**
   * When set known driver bugs are ignored during driver feature
   * detection. This is used to evaluate the status of a new driver
   * release to see if the bugs have been fixed. The function takes
   * a description argument which, is sent to VTK's warning stream
   * when the ignore flag is set. This makes the test output searchable
   * for tests which have problems with certain drivers. The CMakeLists
   * variable VTK_IGNORE_GLDRIVER_BUGS can be used to set this at
   * build time. Default OFF.
   */
  bool GetIgnoreDriverBugs(const char *description);
  vtkSetMacro(IgnoreDriverBugs, bool);
  vtkBooleanMacro(IgnoreDriverBugs, bool);
  //@}

protected:
  vtkOpenGLExtensionManager();
  virtual ~vtkOpenGLExtensionManager();

  int OwnRenderWindow;
  char *ExtensionsString;

  vtkTimeStamp BuildTime;

  // driver specific info
  std::string DriverGLVersion;
  int DriverGLVersionMajor;
  int DriverGLVersionMinor;
  int DriverGLVersionPatch;
  std::string DriverGLVendor;
  std::string DriverGLRenderer;
  int DriverVersionMajor;
  int DriverVersionMinor;
  int DriverVersionPatch;
  enum DriverGLVendorIdType
  {
    DRIVER_VENDOR_UNKNOWN=0,
    DRIVER_VENDOR_ATI,
    DRIVER_VENDOR_NVIDIA,
    DRIVER_VENDOR_INTEL,
    DRIVER_VENDOR_MESA,
    DRIVER_VENDOR_MICROSOFT
  };
  DriverGLVendorIdType DriverGLVendorId;
  bool IgnoreDriverBugs;

  virtual void InitializeDriverInformation();

  virtual void ReadOpenGLExtensions();

  /**
   * Wrap around the generated vtkgl::LoadExtension to deal with OpenGL 1.2
   * and its optional part GL_ARB_imaging. Also functions like
   * glBlendEquation() or glBlendColor() are optional in OpenGL 1.2 or 1.3 and
   * provided by the GL_ARB_imaging but there are core features in OpenGL 1.4.
   */
  virtual int SafeLoadExtension(const char *name);

private:
  vtkOpenGLExtensionManager(const vtkOpenGLExtensionManager&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLExtensionManager&) VTK_DELETE_FUNCTION;

  vtkWeakPointer<vtkRenderWindow> RenderWindow;

};

#endif // vtkOpenGLExtensionManager_h
