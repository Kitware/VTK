// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLExtensionManager.cxx

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

#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLExtensionManagerConfigure.h"
#include "vtkgl.h"

#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"

#include <string.h>

#include <vtkstd/string>

#include <assert.h>

#ifdef VTK_DEFINE_GLX_GET_PROC_ADDRESS_PROTOTYPE
extern "C" vtkglX::__GLXextFuncPtr glXGetProcAddressARB(const GLubyte *);
#endif //VTK_DEFINE_GLX_GET_PROC_ADDRESS_PROTOTYPE

#ifdef VTK_USE_VTK_DYNAMIC_LOADER
#include "vtkDynamicLoader.h"
#include <vtkstd/string>
#include <vtkstd/list>
#endif

#ifdef VTK_USE_APPLE_LOADER
#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
#include <dlfcn.h>
#else
#include <mach-o/dyld.h>
#endif
#endif //VTK_USE_APPLE_LOADER

// GLU is currently not linked in VTK.  We do not support it here.
#define GLU_SUPPORTED   0

vtkCxxRevisionMacro(vtkOpenGLExtensionManager, "1.23");
vtkStandardNewMacro(vtkOpenGLExtensionManager);

namespace vtkgl
{
// Description:
// Set the OpenGL function pointers with the function pointers
// of the core-promoted extension.
int LoadCorePromotedExtension(const char *name,
                              vtkOpenGLExtensionManager *manager);
}

vtkOpenGLExtensionManager::vtkOpenGLExtensionManager()
{
  this->RenderWindow = NULL;
  this->ExtensionsString = NULL;

  this->Modified();
}

vtkOpenGLExtensionManager::~vtkOpenGLExtensionManager()
{
  this->SetRenderWindow(NULL);
  if (this->ExtensionsString)
    {
    delete[] this->ExtensionsString;
    }
}

void vtkOpenGLExtensionManager::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RenderWindow: (" << this->RenderWindow << ")" << endl;
  os << indent << "BuildTime: " << this->BuildTime << endl;
  os << indent << "ExtensionsString: "
     << (this->ExtensionsString ? this->ExtensionsString : "(NULL)") << endl;
}

void vtkOpenGLExtensionManager::SetRenderWindow(vtkRenderWindow *renwin)
{
  if (renwin == this->RenderWindow)
    {
    return;
    }

  vtkDebugMacro("Setting RenderWindow to " << renwin);

  if (this->RenderWindow)
    {
    this->RenderWindow->UnRegister(this);
    }

  this->RenderWindow = renwin;
  if (this->RenderWindow)
    {
    this->RenderWindow->Register(this);
    }

  this->Modified();
}

void vtkOpenGLExtensionManager::Update()
{
  if (this->BuildTime > this->MTime)
    {
    return;
    }

  vtkDebugMacro("Update");

  if (this->ExtensionsString)
    {
    delete[] this->ExtensionsString;
    this->ExtensionsString = NULL;
    }

  this->ReadOpenGLExtensions();

  this->BuildTime.Modified();
}

int vtkOpenGLExtensionManager::ExtensionSupported(const char *name)
{
  this->Update();

  const char *p = this->ExtensionsString;
  int NameLen = strlen(name);
  int result = 0;

  for(;;)
    {
    int n;
    while (*p == ' ') p++;
    if (*p == '\0')
      {
      result = 0;
      break;
      }
    n = strcspn(p, " ");
    if ((NameLen == n) && (strncmp(name, p, n) == 0))
      {
      result = 1;
      break;
      }
    p += n;
    }
  return result;
}

vtkOpenGLExtensionManagerFunctionPointer
vtkOpenGLExtensionManager::GetProcAddress(const char *fname)
{
  vtkDebugMacro(<< "Trying to load OpenGL function " << fname);

#ifdef VTK_USE_WGL_GET_PROC_ADDRESS
  return reinterpret_cast<vtkOpenGLExtensionManagerFunctionPointer>(wglGetProcAddress(fname));
#endif //VTK_USE_WGL_GET_PROC_ADDRESS


#ifdef VTK_USE_APPLE_LOADER

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3

  void* globalsymbolobject = dlopen(NULL, RTLD_GLOBAL);
  if(globalsymbolobject)
    {
    void* dlsymbol = dlsym(globalsymbolobject, fname);
    dlclose(globalsymbolobject);
    if(!dlsymbol)
      {
      vtkDebugMacro("Could not load " << fname);
      }
    return (vtkOpenGLExtensionManagerFunctionPointer)(dlsymbol);
    }
  else
    {
    vtkDebugMacro("Could not load " << fname);
    return NULL;
    }

#else
  
  NSSymbol symbol = NULL;
  char *mangled_fname = new char[strlen(fname)+2];
  // Prepend a '_' to the function name.
  strcpy(mangled_fname+1, fname);
  mangled_fname[0] = '_';
  if (NSIsSymbolNameDefined(mangled_fname))
    {
    symbol = NSLookupAndBindSymbol(mangled_fname);
    }
  else
    {
    vtkDebugMacro("Could not load " << mangled_fname);
    }
  delete[] mangled_fname;
  if (symbol)
    {
    return (vtkOpenGLExtensionManagerFunctionPointer)NSAddressOfSymbol(symbol);
    }
  else
    {
    vtkDebugMacro("Could not load " << mangled_fname);
    return NULL;
    }

#endif //MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
#endif //VTK_USE_APPLE_LOADER


#ifdef VTK_USE_GLX_GET_PROC_ADDRESS
  return (vtkOpenGLExtensionManagerFunctionPointer)glXGetProcAddress((const GLubyte *)fname);
#endif //VTK_USE_GLX_GET_PROC_ADDRESS
#ifdef VTK_USE_GLX_GET_PROC_ADDRESS_ARB
  return (vtkOpenGLExtensionManagerFunctionPointer)glXGetProcAddressARB((const GLubyte *)fname);
#endif //VTK_USE_GLX_GET_PROC_ADDRESS_ARB


#ifdef VTK_USE_VTK_DYNAMIC_LOADER
  // If the GLX implementation cannot load procedures for us, load them
  // directly from the dynamic libraries.
  static vtkstd::list<vtkstd::string> ogl_libraries;

  if (ogl_libraries.empty())
    {
    const char *ext = vtkDynamicLoader::LibExtension();
    vtkstd::string::size_type ext_size = strlen(ext);
    // Must be the first function we tried to load.  Fill this list with
    // the OpenGL libraries we linked against.
    vtkstd::string l(OPENGL_LIBRARIES);
    vtkstd::string::size_type filename_start = 0;
    while (1)
      {
      vtkstd::string::size_type filename_end = l.find(';', filename_start);
      if (filename_end == vtkstd::string::npos)
        {
        break;
        }
      vtkstd::string possible_file = l.substr(filename_start,
                                              filename_end-filename_start);
      // Make sure this is actually a library.  Do this by making sure it
      // has an appropriate extension.  This is by no means definitive, but
      // it1 should do.
      if (   (possible_file.length() > ext_size)
          && (possible_file.substr(possible_file.length()-ext_size) == ext) )
        {
        ogl_libraries.push_back(possible_file);
        }

      filename_start = filename_end + 1;
      }
    }

  // Look for the function in each library.
  for (vtkstd::list<vtkstd::string>::iterator i = ogl_libraries.begin();
       i != ogl_libraries.end(); i++)
    {
    vtkLibHandle lh = vtkDynamicLoader::OpenLibrary((*i).c_str());
    void *f = vtkDynamicLoader::GetSymbolAddress(lh, fname);
    vtkDynamicLoader::CloseLibrary(lh);
    if (f) return (vtkOpenGLExtensionManagerFunctionPointer)f;
    }

  // Could not find the function.
  return NULL;
#endif //VTK_USE_VTK_DYNAMIC_LOADER

#ifdef VTK_NO_EXTENSION_LOADING
  return NULL;
#endif //VTK_NO_EXTENSION_LOADING
}

void vtkOpenGLExtensionManager::LoadExtension(const char *name)
{
  if (!this->ExtensionSupported(name))
    {
    vtkWarningMacro("Attempting to load " << name
                    << ", which is not supported.");
    }

  int success = vtkgl::LoadExtension(name, this);

  if (!success)
    {
    vtkErrorMacro("Extension " << name << " could not be loaded.");
    }
}

int vtkOpenGLExtensionManager::LoadSupportedExtension(const char *name)
{
  int supported = 0;
  int loaded = 0;

  supported = this->ExtensionSupported(name);

  if (supported)
    {
    loaded = vtkgl::LoadExtension(name, this);
    }

  vtkDebugMacro(
    << "vtkOpenGLExtensionManager::LoadSupportedExtension" << endl
    << "  name: " << name << endl
    << "  supported: " << supported << endl
    << "  loaded: " << loaded << endl
    );

  return supported && loaded;
}

void vtkOpenGLExtensionManager::LoadCorePromotedExtension(const char *name)
{
  if (!this->ExtensionSupported(name))
    {
    vtkWarningMacro("Attempting to load " << name
                    << ", which is not supported.");
    }
  int success = vtkgl::LoadCorePromotedExtension(name, this);
  
  if (!success)
    {
    vtkErrorMacro("Extension " << name << " could not be loaded.");
    }
}

void vtkOpenGLExtensionManager::ReadOpenGLExtensions()
{
  vtkDebugMacro("ReadOpenGLExtensions");

#ifdef VTK_NO_EXTENSION_LOADING

  this->ExtensionsString = new char[1];
  this->ExtensionsString[0] = '\0';
  return;

#else //!VTK_NO_EXTENSION_LOADING

  if (this->RenderWindow)
    {
    if (!this->RenderWindow->IsA("vtkOpenGLRenderWindow"))
      {
      // If the render window is not OpenGL, then it obviously has no
      // extensions.
      this->ExtensionsString = new char[1];
      this->ExtensionsString[0] = '\0';
      return;
      }
    if (this->RenderWindow->GetNeverRendered())
      {
      this->RenderWindow->Render();
      }
    this->RenderWindow->MakeCurrent();
    }

  vtkstd::string extensions_string;

  const char *gl_extensions;
  const char *glu_extensions = "";
  const char *win_extensions;

  gl_extensions = (const char *)glGetString(GL_EXTENSIONS);

  if (gl_extensions == NULL)
    {
    gl_extensions = "";
    }

  if (!this->RenderWindow && (gl_extensions[0] == '\0'))
    {
    vtkDebugMacro("No window active?  Attaching default render window.");
    vtkRenderWindow *renwin = vtkRenderWindow::New();
    renwin->SetSize(1, 1);
    this->SetRenderWindow(renwin);
    renwin->Delete();
    this->ReadOpenGLExtensions();
    return;
    }

  extensions_string = gl_extensions;

#if GLU_SUPPORTED
  glu_extensions = (const char *)gluGetString(GLU_EXTENSIONS);
#endif
  if (glu_extensions != NULL)
    {
    extensions_string += " ";
    extensions_string += glu_extensions;
    }

#if defined(WIN32)
  // Don't use this->LoadExtension or we will go into an infinite loop.
  vtkgl::LoadExtension("WGL_ARB_extensions_string", this);
  if (vtkwgl::GetExtensionsStringARB)
    {
    win_extensions = vtkwgl::GetExtensionsStringARB(wglGetCurrentDC());
    }
  else
    {
    //vtkWarningMacro("Could not query WGL extensions.");
    win_extensions = "";
    }
#elif defined(__APPLE__)
//   vtkWarningMacro("Does APPLE have a windows extension string?");
  win_extensions = "";
#else
  win_extensions = glXGetClientString(glXGetCurrentDisplay(),
                                      GLX_EXTENSIONS);
#endif

  if (win_extensions != NULL)
    {
    extensions_string += " ";
    extensions_string += win_extensions;
    }

  // We build special extension identifiers for OpenGL versions.  Check to
  // see which are supported.
  vtkstd::string version_extensions;
  vtkstd::string::size_type beginpos, endpos;

  const char *version = (const char *)glGetString(GL_VERSION);
  int driverMajor, driverMinor;
  sscanf(version, "%d.%d", &driverMajor, &driverMinor);

  version_extensions = vtkgl::GLVersionExtensionsString();
  endpos = 0;
  while (endpos != vtkstd::string::npos)
    {
    beginpos = version_extensions.find_first_not_of(' ', endpos);
    if (beginpos == vtkstd::string::npos) break;
    endpos = version_extensions.find_first_of(' ', beginpos);

    vtkstd::string ve = version_extensions.substr(beginpos, endpos-beginpos);
    int tryMajor, tryMinor;
    sscanf(ve.c_str(), "GL_VERSION_%d_%d", &tryMajor, &tryMinor);
    if (   (driverMajor > tryMajor)
        || ((driverMajor == tryMajor) && (driverMinor >= tryMinor)) )
      {
      // OpenGL version supported.
      extensions_string += " ";
      extensions_string += ve;
      }
    }

#ifdef VTK_USE_X
  Display *display = NULL;
  int closeDisplay = 0;
  if (this->RenderWindow)
    {
    // Try getting the display of the window we are doing the queries on.
    display = (Display *)this->RenderWindow->GetGenericDisplayId();
    }
  if (!display)
    {
    // Try opening my own display.
    display = XOpenDisplay(NULL);
    closeDisplay = 1;
    }

  if (!display)
    {
    // If we could not find a display, silently fail to query the glX
    // extensions.  It could be that there is no glX (for example if using Mesa
    // offscreen).
    vtkDebugMacro(<< "Could not get a Display to query GLX extensions.");
    }
  else
    {
    glXQueryExtension(display, &driverMajor, &driverMinor);

    version_extensions = vtkgl::GLXVersionExtensionsString();
    endpos = 0;
    while (endpos != vtkstd::string::npos)
      {
      beginpos = version_extensions.find_first_not_of(' ', endpos);
      if (beginpos == vtkstd::string::npos) break;
      endpos = version_extensions.find_first_of(' ', beginpos);
      
      vtkstd::string ve = version_extensions.substr(beginpos, endpos-beginpos);
      int tryMajor, tryMinor;
      sscanf(ve.c_str(), "GLX_VERSION_%d_%d", &tryMajor, &tryMinor);
      if (   (driverMajor > tryMajor)
          || ((driverMajor == tryMajor) && (driverMinor >= tryMinor)) )
        {
        extensions_string += " ";
        extensions_string += ve;
        }
      }

    if (closeDisplay)
      {
      XCloseDisplay(display);
      }
    }
#endif //VTK_USE_X

  // Store extensions string.
  this->ExtensionsString = new char[extensions_string.length()+1];
  strcpy(this->ExtensionsString, extensions_string.c_str());

#endif //!VTK_NO_EXTENSION_LOADING
}

// Those two functions are part of OpenGL2.0 but don't have direct
// translation in the GL_ARB_shader_objects extension
GLboolean IsProgramFromARBToPromoted(GLuint program)
{
  GLint param;
  vtkgl::GetObjectParameterivARB(program, vtkgl::OBJECT_TYPE_ARB, &param);
  return param==static_cast<GLint>(vtkgl::PROGRAM_OBJECT_ARB);
}

GLboolean IsShaderFromARBToPromoted(GLuint shader)
{
  GLint param;
  vtkgl::GetObjectParameterivARB(shader, vtkgl::OBJECT_TYPE_ARB, &param);
  return param==static_cast<GLint>(vtkgl::SHADER_OBJECT_ARB);
}

// This function was implemented in the following way:
// 1. Compile VTK in a build directory in order to generate Rendering/vtkgl.h
// from glext.h, glxext.h and wglext.h
// 2. From the OpenGL specification 2.1
// ( http://www.opengl.org/registry/doc/glspec21.20061201.pdf ), go through
// Appendix C to Appendix J to identify the extensions promoted as core OpenGL
// features and in which version of OpenGL.

// For instance, for OpenGL 1.5 extensions, look for GL_VERSION_1_5 section.
// Each section is divided into three parts: the first part defines macros
// (const GLenum ....)
// the second part defines function pointer types (typedef ...) .
// the third part defines the function pointers.

// 3. Copy the function pointers part into this file.
// Group function pointers by extensions by looking at the Appendix in the
// OpenGL specifications and the description of the extension in the
// OpenGL extension registry at http://www.opengl.org/registry.

// 4. For each function pointer, get the address of the function defined in the
// extension and cast it as a core function pointer.

int vtkgl::LoadCorePromotedExtension(const char *name,
                                     vtkOpenGLExtensionManager *manager)
{
  assert("pre: name_exists" && name!=NULL);
  assert("pre: manager_exists" && manager!=NULL);
  
  // OpenGL 1.1
  
  // VTK supports at least OpenGL 1.1. There is no need to load promoted
  // extentions GL_EXT_subtexture and GL_EXT_copy_texture.
  // Just silently returns 1.
  
  if (strcmp(name, "GL_EXT_subtexture") == 0)
    {
    // GL_EXT_subtexture defines glTexSubImage1D and glTexSubImage2D()
    return 1;
    }
  if (strcmp(name, "GL_EXT_copy_texture") == 0)
    {
    // GL_EXT_copy_texture defines glCopyTexImage1D(), glCopyTexImage2D(),
    // glCopyTexSubImage1D()
    // and glCopyTexSubImage2D().
    // if both GL_EXT_copy_texture and GL_EXT_texture3D are supported,
    // it also defines vtkgl::CopyTexSubImage3D but we postpone that
    // in the GL_EXT_texture3D section.
    return 1;
    }
  
  // OpenGL 1.2
  if (strcmp(name, "GL_EXT_texture3D") == 0)
    {
    vtkgl::TexImage3D = (vtkgl::PFNGLTEXIMAGE3DPROC)manager->GetProcAddress("glTexImage3DEXT");
    vtkgl::TexSubImage3D = (vtkgl::PFNGLTEXSUBIMAGE3DPROC)manager->GetProcAddress("glTexSubImage3DEXT");
    vtkgl::CopyTexSubImage3D = (vtkgl::PFNGLCOPYTEXSUBIMAGE3DPROC)manager->GetProcAddress("glCopyTexSubImage3DEXT");
    return 1 && (vtkgl::TexImage3D != NULL) && (vtkgl::TexSubImage3D != NULL)
      && (vtkgl::CopyTexSubImage3D != NULL);
    }
  
  if (strcmp(name, "GL_EXT_bgra") == 0)
    {
    return 1;
    }
  if (strcmp(name, "GL_EXT_packed_pixels") == 0)
    {
    return 1;
    }
  if (strcmp(name, "GL_EXT_rescale_normal") == 0)
    {
    return 1;
    }
  if (strcmp(name, "GL_EXT_separate_specular_color") == 0)
    {
    return 1;
    }
  if (strcmp(name, "GL_SGIS_texture_edge_clamp") == 0)
    {
    return 1;
    }
  if (strcmp(name, "GL_EXT_draw_range_elements") == 0)
    {
    vtkgl::DrawRangeElements = (vtkgl::PFNGLDRAWRANGEELEMENTSPROC)manager->GetProcAddress("glDrawRangeElementsEXT");
    return 1 && (vtkgl::DrawRangeElements != NULL);
    }
  
  if (strcmp(name, "GL_SGI_color_table") == 0)
    {
    // OpenGL Spec talks about GL_EXT_color_table but reality is
    // GL_SGI_color_table is used. Also GL_EXT_color_table is not listed
    // on the registry website.
    vtkgl::ColorTable = (vtkgl::PFNGLCOLORTABLESGIPROC)manager->GetProcAddress("glColorTableSGI");
    vtkgl::ColorTableParameterfv = (vtkgl::PFNGLCOLORTABLEPARAMETERFVSGIPROC)manager->GetProcAddress("glColorTableParameterfvSGI");
    vtkgl::ColorTableParameteriv = (vtkgl::PFNGLCOLORTABLEPARAMETERIVSGIPROC)manager->GetProcAddress("glColorTableParameterivSGI");
    vtkgl::CopyColorTable = (vtkgl::PFNGLCOPYCOLORTABLESGIPROC)manager->GetProcAddress("glCopyColorTableSGI");
    vtkgl::GetColorTable = (vtkgl::PFNGLGETCOLORTABLESGIPROC)manager->GetProcAddress("glGetColorTableSGI");
    vtkgl::GetColorTableParameterfv = (vtkgl::PFNGLGETCOLORTABLEPARAMETERFVSGIPROC)manager->GetProcAddress("glGetColorTableParameterfvSGI");
    vtkgl::GetColorTableParameteriv = (vtkgl::PFNGLGETCOLORTABLEPARAMETERIVSGIPROC)manager->GetProcAddress("glGetColorTableParameterivSGI");
    return 1 && (vtkgl::ColorTable != NULL) && (vtkgl::ColorTableParameterfv != NULL) && (vtkgl::ColorTableParameteriv != NULL) && (vtkgl::CopyColorTable != NULL) && (vtkgl::GetColorTable != NULL) && (vtkgl::GetColorTableParameterfv != NULL) && (vtkgl::GetColorTableParameteriv != NULL);
    }
  
  if (strcmp(name, "GL_EXT_color_subtable") == 0)
    {
    vtkgl::ColorSubTable = (vtkgl::PFNGLCOLORSUBTABLEPROC)manager->GetProcAddress("glColorSubTableEXT");
    vtkgl::CopyColorSubTable = (vtkgl::PFNGLCOPYCOLORSUBTABLEPROC)manager->GetProcAddress("glCopyColorSubTableEXT");
    return 1 && (vtkgl::ColorSubTable != NULL) && (vtkgl::CopyColorSubTable != NULL);
    }
  
  if (strcmp(name, "GL_EXT_convolution") == 0)
    {
    vtkgl::ConvolutionFilter1D = (vtkgl::PFNGLCONVOLUTIONFILTER1DPROC)manager->GetProcAddress("glConvolutionFilter1DEXT");
    vtkgl::ConvolutionFilter2D = (vtkgl::PFNGLCONVOLUTIONFILTER2DPROC)manager->GetProcAddress("glConvolutionFilter2DEXT");
    vtkgl::ConvolutionParameterf = (vtkgl::PFNGLCONVOLUTIONPARAMETERFPROC)manager->GetProcAddress("glConvolutionParameterfEXT");
    vtkgl::ConvolutionParameterfv = (vtkgl::PFNGLCONVOLUTIONPARAMETERFVPROC)manager->GetProcAddress("glConvolutionParameterfvEXT");
    vtkgl::ConvolutionParameteri = (vtkgl::PFNGLCONVOLUTIONPARAMETERIPROC)manager->GetProcAddress("glConvolutionParameteriEXT");
    vtkgl::ConvolutionParameteriv = (vtkgl::PFNGLCONVOLUTIONPARAMETERIVPROC)manager->GetProcAddress("glConvolutionParameterivEXT");
    vtkgl::CopyConvolutionFilter1D = (vtkgl::PFNGLCOPYCONVOLUTIONFILTER1DPROC)manager->GetProcAddress("glCopyConvolutionFilter1DEXT");
    vtkgl::CopyConvolutionFilter2D = (vtkgl::PFNGLCOPYCONVOLUTIONFILTER2DPROC)manager->GetProcAddress("glCopyConvolutionFilter2DEXT");
    vtkgl::GetConvolutionFilter = (vtkgl::PFNGLGETCONVOLUTIONFILTERPROC)manager->GetProcAddress("glGetConvolutionFilterEXT");
    vtkgl::GetConvolutionParameterfv = (vtkgl::PFNGLGETCONVOLUTIONPARAMETERFVPROC)manager->GetProcAddress("glGetConvolutionParameterfvEXT");
    vtkgl::GetConvolutionParameteriv = (vtkgl::PFNGLGETCONVOLUTIONPARAMETERIVPROC)manager->GetProcAddress("glGetConvolutionParameterivEXT");
    vtkgl::GetSeparableFilter = (vtkgl::PFNGLGETSEPARABLEFILTERPROC)manager->GetProcAddress("glGetSeparableFilterEXT");
    vtkgl::SeparableFilter2D = (vtkgl::PFNGLSEPARABLEFILTER2DPROC)manager->GetProcAddress("glSeparableFilter2DEXT");
    return 1 && (vtkgl::ConvolutionFilter1D != NULL) && (vtkgl::ConvolutionFilter2D != NULL) && (vtkgl::ConvolutionParameterf != NULL) && (vtkgl::ConvolutionParameterfv != NULL) && (vtkgl::ConvolutionParameteri != NULL) && (vtkgl::ConvolutionParameteriv != NULL) && (vtkgl::CopyConvolutionFilter1D != NULL) && (vtkgl::CopyConvolutionFilter2D != NULL) && (vtkgl::GetConvolutionFilter != NULL) && (vtkgl::GetConvolutionParameterfv != NULL) && (vtkgl::GetConvolutionParameteriv != NULL) && (vtkgl::GetSeparableFilter != NULL) && (vtkgl::SeparableFilter2D != NULL);
    }
  
  if (strcmp(name, "GL_HP_convolution_border_modes") == 0)
    {
    return 1;
    }

  if (strcmp(name, "GL_SGI_color_matrix") == 0)
    {
    return 1;
    }
  
  if (strcmp(name, "GL_EXT_histogram") == 0)
    {
    vtkgl::GetHistogram = (vtkgl::PFNGLGETHISTOGRAMPROC)manager->GetProcAddress("glGetHistogramEXT");
    vtkgl::GetHistogramParameterfv = (vtkgl::PFNGLGETHISTOGRAMPARAMETERFVPROC)manager->GetProcAddress("glGetHistogramParameterfvEXT");
    vtkgl::GetHistogramParameteriv = (vtkgl::PFNGLGETHISTOGRAMPARAMETERIVPROC)manager->GetProcAddress("glGetHistogramParameterivEXT");
    vtkgl::GetMinmax = (vtkgl::PFNGLGETMINMAXPROC)manager->GetProcAddress("glGetMinmaxEXT");
    vtkgl::GetMinmaxParameterfv = (vtkgl::PFNGLGETMINMAXPARAMETERFVPROC)manager->GetProcAddress("glGetMinmaxParameterfvEXT");
    vtkgl::GetMinmaxParameteriv = (vtkgl::PFNGLGETMINMAXPARAMETERIVPROC)manager->GetProcAddress("glGetMinmaxParameterivEXT");
    vtkgl::Histogram = (vtkgl::PFNGLHISTOGRAMPROC)manager->GetProcAddress("glHistogramEXT");
    vtkgl::Minmax = (vtkgl::PFNGLMINMAXPROC)manager->GetProcAddress("glMinmaxEXT");
    vtkgl::ResetHistogram = (vtkgl::PFNGLRESETHISTOGRAMPROC)manager->GetProcAddress("glResetHistogramEXT");
    vtkgl::ResetMinmax = (vtkgl::PFNGLRESETMINMAXPROC)manager->GetProcAddress("glResetMinmaxEXT");
    return 1 && (vtkgl::GetHistogram != NULL) && (vtkgl::GetHistogramParameterfv != NULL) && (vtkgl::GetHistogramParameteriv != NULL) && (vtkgl::GetMinmax != NULL) && (vtkgl::GetMinmaxParameterfv != NULL) && (vtkgl::GetMinmaxParameteriv != NULL) && (vtkgl::Histogram != NULL) && (vtkgl::Minmax != NULL) && (vtkgl::ResetHistogram != NULL) && (vtkgl::ResetMinmax != NULL);
    }

  if (strcmp(name, "GL_EXT_blend_color") == 0)
    {
    vtkgl::BlendColor = (vtkgl::PFNGLBLENDCOLORPROC)manager->GetProcAddress("glBlendColorEXT");
    return 1 && (vtkgl::BlendColor != NULL);
    }
  
  if (strcmp(name, "GL_EXT_blend_minmax") == 0)
    {
    vtkgl::BlendEquation = (vtkgl::PFNGLBLENDEQUATIONPROC)manager->GetProcAddress("glBlendEquationEXT");
    return 1 && (vtkgl::BlendEquation != NULL);
    }
  if (strcmp(name, "GL_EXT_blend_subtract") == 0)
    {
    return 1;
    }

  // OpenGL 1.3
  
  if (strcmp(name, "GL_ARB_texture_compression") == 0)
    {
    vtkgl::CompressedTexImage3D = (vtkgl::PFNGLCOMPRESSEDTEXIMAGE3DPROC)manager->GetProcAddress("glCompressedTexImage3DARB");
    vtkgl::CompressedTexImage2D = (vtkgl::PFNGLCOMPRESSEDTEXIMAGE2DPROC)manager->GetProcAddress("glCompressedTexImage2DARB");
    vtkgl::CompressedTexImage1D = (vtkgl::PFNGLCOMPRESSEDTEXIMAGE1DPROC)manager->GetProcAddress("glCompressedTexImage1DARB");
    vtkgl::CompressedTexSubImage3D = (vtkgl::PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)manager->GetProcAddress("glCompressedTexSubImage3DARB");
    vtkgl::CompressedTexSubImage2D = (vtkgl::PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)manager->GetProcAddress("glCompressedTexSubImage2DARB");
    vtkgl::CompressedTexSubImage1D = (vtkgl::PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)manager->GetProcAddress("glCompressedTexSubImage1DARB");
    vtkgl::GetCompressedTexImage = (vtkgl::PFNGLGETCOMPRESSEDTEXIMAGEPROC)manager->GetProcAddress("glGetCompressedTexImageARB");
    return 1 && (vtkgl::CompressedTexImage3D != NULL) && (vtkgl::CompressedTexImage2D != NULL) && (vtkgl::CompressedTexImage1D != NULL) && (vtkgl::CompressedTexSubImage3D != NULL) && (vtkgl::CompressedTexSubImage2D != NULL) && (vtkgl::CompressedTexSubImage1D != NULL) && (vtkgl::GetCompressedTexImage != NULL);
    }

  if (strcmp(name, "GL_ARB_texture_cube_map") == 0)
    {
    return 1;
    }
  
  if (strcmp(name, "GL_ARB_multisample") == 0)
    {
    vtkgl::SampleCoverage = (vtkgl::PFNGLSAMPLECOVERAGEPROC)manager->GetProcAddress("glSampleCoverageARB");
    return 1 && (vtkgl::SampleCoverage != NULL);
    }
  
  if (strcmp(name, "GL_ARB_multitexture") == 0)
    {
    vtkgl::ActiveTexture = (vtkgl::PFNGLACTIVETEXTUREPROC)manager->GetProcAddress("glActiveTextureARB");
    vtkgl::ClientActiveTexture = (vtkgl::PFNGLCLIENTACTIVETEXTUREPROC)manager->GetProcAddress("glClientActiveTextureARB");
    vtkgl::MultiTexCoord1d = (vtkgl::PFNGLMULTITEXCOORD1DPROC)manager->GetProcAddress("glMultiTexCoord1dARB");
    vtkgl::MultiTexCoord1dv = (vtkgl::PFNGLMULTITEXCOORD1DVPROC)manager->GetProcAddress("glMultiTexCoord1dvARB");
    vtkgl::MultiTexCoord1f = (vtkgl::PFNGLMULTITEXCOORD1FPROC)manager->GetProcAddress("glMultiTexCoord1fARB");
    vtkgl::MultiTexCoord1fv = (vtkgl::PFNGLMULTITEXCOORD1FVPROC)manager->GetProcAddress("glMultiTexCoord1fvARB");
    vtkgl::MultiTexCoord1i = (vtkgl::PFNGLMULTITEXCOORD1IPROC)manager->GetProcAddress("glMultiTexCoord1iARB");
    vtkgl::MultiTexCoord1iv = (vtkgl::PFNGLMULTITEXCOORD1IVPROC)manager->GetProcAddress("glMultiTexCoord1ivARB");
    vtkgl::MultiTexCoord1s = (vtkgl::PFNGLMULTITEXCOORD1SPROC)manager->GetProcAddress("glMultiTexCoord1sARB");
    vtkgl::MultiTexCoord1sv = (vtkgl::PFNGLMULTITEXCOORD1SVPROC)manager->GetProcAddress("glMultiTexCoord1svARB");
    vtkgl::MultiTexCoord2d = (vtkgl::PFNGLMULTITEXCOORD2DPROC)manager->GetProcAddress("glMultiTexCoord2dARB");
    vtkgl::MultiTexCoord2dv = (vtkgl::PFNGLMULTITEXCOORD2DVPROC)manager->GetProcAddress("glMultiTexCoord2dvARB");
    vtkgl::MultiTexCoord2f = (vtkgl::PFNGLMULTITEXCOORD2FPROC)manager->GetProcAddress("glMultiTexCoord2fARB");
    vtkgl::MultiTexCoord2fv = (vtkgl::PFNGLMULTITEXCOORD2FVPROC)manager->GetProcAddress("glMultiTexCoord2fvARB");
    vtkgl::MultiTexCoord2i = (vtkgl::PFNGLMULTITEXCOORD2IPROC)manager->GetProcAddress("glMultiTexCoord2iARB");
    vtkgl::MultiTexCoord2iv = (vtkgl::PFNGLMULTITEXCOORD2IVPROC)manager->GetProcAddress("glMultiTexCoord2ivARB");
    vtkgl::MultiTexCoord2s = (vtkgl::PFNGLMULTITEXCOORD2SPROC)manager->GetProcAddress("glMultiTexCoord2sARB");
    vtkgl::MultiTexCoord2sv = (vtkgl::PFNGLMULTITEXCOORD2SVPROC)manager->GetProcAddress("glMultiTexCoord2svARB");
    vtkgl::MultiTexCoord3d = (vtkgl::PFNGLMULTITEXCOORD3DPROC)manager->GetProcAddress("glMultiTexCoord3dARB");
    vtkgl::MultiTexCoord3dv = (vtkgl::PFNGLMULTITEXCOORD3DVPROC)manager->GetProcAddress("glMultiTexCoord3dvARB");
    vtkgl::MultiTexCoord3f = (vtkgl::PFNGLMULTITEXCOORD3FPROC)manager->GetProcAddress("glMultiTexCoord3fARB");
    vtkgl::MultiTexCoord3fv = (vtkgl::PFNGLMULTITEXCOORD3FVPROC)manager->GetProcAddress("glMultiTexCoord3fvARB");
    vtkgl::MultiTexCoord3i = (vtkgl::PFNGLMULTITEXCOORD3IPROC)manager->GetProcAddress("glMultiTexCoord3iARB");
    vtkgl::MultiTexCoord3iv = (vtkgl::PFNGLMULTITEXCOORD3IVPROC)manager->GetProcAddress("glMultiTexCoord3ivARB");
    vtkgl::MultiTexCoord3s = (vtkgl::PFNGLMULTITEXCOORD3SPROC)manager->GetProcAddress("glMultiTexCoord3sARB");
    vtkgl::MultiTexCoord3sv = (vtkgl::PFNGLMULTITEXCOORD3SVPROC)manager->GetProcAddress("glMultiTexCoord3svARB");
    vtkgl::MultiTexCoord4d = (vtkgl::PFNGLMULTITEXCOORD4DPROC)manager->GetProcAddress("glMultiTexCoord4dARB");
    vtkgl::MultiTexCoord4dv = (vtkgl::PFNGLMULTITEXCOORD4DVPROC)manager->GetProcAddress("glMultiTexCoord4dvARB");
    vtkgl::MultiTexCoord4f = (vtkgl::PFNGLMULTITEXCOORD4FPROC)manager->GetProcAddress("glMultiTexCoord4fARB");
    vtkgl::MultiTexCoord4fv = (vtkgl::PFNGLMULTITEXCOORD4FVPROC)manager->GetProcAddress("glMultiTexCoord4fvARB");
    vtkgl::MultiTexCoord4i = (vtkgl::PFNGLMULTITEXCOORD4IPROC)manager->GetProcAddress("glMultiTexCoord4iARB");
    vtkgl::MultiTexCoord4iv = (vtkgl::PFNGLMULTITEXCOORD4IVPROC)manager->GetProcAddress("glMultiTexCoord4ivARB");
    vtkgl::MultiTexCoord4s = (vtkgl::PFNGLMULTITEXCOORD4SPROC)manager->GetProcAddress("glMultiTexCoord4sARB");
    vtkgl::MultiTexCoord4sv = (vtkgl::PFNGLMULTITEXCOORD4SVPROC)manager->GetProcAddress("glMultiTexCoord4svARB");
    return 1 && (vtkgl::ActiveTexture != NULL) && (vtkgl::ClientActiveTexture != NULL) && (vtkgl::MultiTexCoord1d != NULL) && (vtkgl::MultiTexCoord1dv != NULL) && (vtkgl::MultiTexCoord1f != NULL) && (vtkgl::MultiTexCoord1fv != NULL) && (vtkgl::MultiTexCoord1i != NULL) && (vtkgl::MultiTexCoord1iv != NULL) && (vtkgl::MultiTexCoord1s != NULL) && (vtkgl::MultiTexCoord1sv != NULL) && (vtkgl::MultiTexCoord2d != NULL) && (vtkgl::MultiTexCoord2dv != NULL) && (vtkgl::MultiTexCoord2f != NULL) && (vtkgl::MultiTexCoord2fv != NULL) && (vtkgl::MultiTexCoord2i != NULL) && (vtkgl::MultiTexCoord2iv != NULL) && (vtkgl::MultiTexCoord2s != NULL) && (vtkgl::MultiTexCoord2sv != NULL) && (vtkgl::MultiTexCoord3d != NULL) && (vtkgl::MultiTexCoord3dv != NULL) && (vtkgl::MultiTexCoord3f != NULL) && (vtkgl::MultiTexCoord3fv != NULL) && (vtkgl::MultiTexCoord3i != NULL) && (vtkgl::MultiTexCoord3iv != NULL) && (vtkgl::MultiTexCoord3s != NULL) && (vtkgl::MultiTexCoord3sv != NULL) && (vtkgl::MultiTexCoord4d != NULL) && (vtkgl::MultiTexCoord4dv != NULL) && (vtkgl::MultiTexCoord4f != NULL) && (vtkgl::MultiTexCoord4fv != NULL) && (vtkgl::MultiTexCoord4i != NULL) && (vtkgl::MultiTexCoord4iv != NULL) && (vtkgl::MultiTexCoord4s != NULL) && (vtkgl::MultiTexCoord4sv !=NULL);
    }
 
  if (strcmp(name, "GL_ARB_texture_env_add") == 0)
    {
    return 1;
    }
 
  if (strcmp(name, "GL_ARB_texture_env_combine") == 0)
    {
    return 1;
    }
 
  if (strcmp(name, "GL_ARB_texture_env_dot3") == 0)
    {
    return 1;
    }
 
  if (strcmp(name, "GL_ARB_texture_border_clamp") == 0)
    {
    return 1;
    }
 
  if (strcmp(name, "GL_ARB_transpose_matrix") == 0)
    {
    vtkgl::LoadTransposeMatrixf = (vtkgl::PFNGLLOADTRANSPOSEMATRIXFPROC)manager->GetProcAddress("glLoadTransposeMatrixfARB");
    vtkgl::LoadTransposeMatrixd = (vtkgl::PFNGLLOADTRANSPOSEMATRIXDPROC)manager->GetProcAddress("glLoadTransposeMatrixdARB");
    vtkgl::MultTransposeMatrixf = (vtkgl::PFNGLMULTTRANSPOSEMATRIXFPROC)manager->GetProcAddress("glMultTransposeMatrixfARB");
    vtkgl::MultTransposeMatrixd = (vtkgl::PFNGLMULTTRANSPOSEMATRIXDPROC)manager->GetProcAddress("glMultTransposeMatrixdARB");
    return 1 && (vtkgl::LoadTransposeMatrixf != NULL) && (vtkgl::LoadTransposeMatrixd != NULL) && (vtkgl::MultTransposeMatrixf != NULL) && (vtkgl::MultTransposeMatrixd != NULL);
    }

  // OpenGL 1.4
  
  if (strcmp(name, "GL_SGIS_generate_mipmap") == 0)
    {
    return 1;
    }
  
  if (strcmp(name, "GL_NV_blend_square") == 0)
    {
    return 1;
    }

  if (strcmp(name, "GL_ARB_depth_texture") == 0)
    {
    return 1;
    }
  
  if (strcmp(name, "GL_ARB_shadow") == 0)
    {
    return 1;
    }
  
  if (strcmp(name, "GL_EXT_fog_coord") == 0)
    {
    vtkgl::FogCoordf = (vtkgl::PFNGLFOGCOORDFPROC)manager->GetProcAddress("glFogCoordfEXT");
    vtkgl::FogCoordfv = (vtkgl::PFNGLFOGCOORDFVPROC)manager->GetProcAddress("glFogCoordfvEXT");
    vtkgl::FogCoordd = (vtkgl::PFNGLFOGCOORDDPROC)manager->GetProcAddress("glFogCoorddEXT");
    vtkgl::FogCoorddv = (vtkgl::PFNGLFOGCOORDDVPROC)manager->GetProcAddress("glFogCoorddvEXT");
    vtkgl::FogCoordPointer = (vtkgl::PFNGLFOGCOORDPOINTERPROC)manager->GetProcAddress("glFogCoordPointerEXT");
    return 1 && (vtkgl::FogCoordf != NULL) && (vtkgl::FogCoordfv != NULL) && (vtkgl::FogCoordd != NULL) && (vtkgl::FogCoorddv != NULL) && (vtkgl::FogCoordPointer != NULL);
    }

  if (strcmp(name, "GL_EXT_multi_draw_arrays") == 0)
    {
    vtkgl::MultiDrawArrays = (vtkgl::PFNGLMULTIDRAWARRAYSPROC)manager->GetProcAddress("glMultiDrawArraysEXT");
    vtkgl::MultiDrawElements = (vtkgl::PFNGLMULTIDRAWELEMENTSPROC)manager->GetProcAddress("glMultiDrawElementsEXT");
    return 1 && (vtkgl::MultiDrawArrays != NULL) && (vtkgl::MultiDrawElements != NULL);
    }

  if (strcmp(name, "GL_ARB_point_parameters") == 0)
    {
    vtkgl::PointParameterf = (vtkgl::PFNGLPOINTPARAMETERFPROC)manager->GetProcAddress("glPointParameterfARB");
    vtkgl::PointParameterfv = (vtkgl::PFNGLPOINTPARAMETERFVPROC)manager->GetProcAddress("glPointParameterfvARB");
    return 1 && (vtkgl::PointParameterf != NULL) && (vtkgl::PointParameterfv != NULL);
    }

  
  if (strcmp(name, "GL_EXT_secondary_color") == 0)
    {
    vtkgl::SecondaryColor3b = (vtkgl::PFNGLSECONDARYCOLOR3BPROC)manager->GetProcAddress("glSecondaryColor3bEXT");
    vtkgl::SecondaryColor3bv = (vtkgl::PFNGLSECONDARYCOLOR3BVPROC)manager->GetProcAddress("glSecondaryColor3bvEXT");
    vtkgl::SecondaryColor3d = (vtkgl::PFNGLSECONDARYCOLOR3DPROC)manager->GetProcAddress("glSecondaryColor3dEXT");
    vtkgl::SecondaryColor3dv = (vtkgl::PFNGLSECONDARYCOLOR3DVPROC)manager->GetProcAddress("glSecondaryColor3dvEXT");
    vtkgl::SecondaryColor3f = (vtkgl::PFNGLSECONDARYCOLOR3FPROC)manager->GetProcAddress("glSecondaryColor3fEXT");
    vtkgl::SecondaryColor3fv = (vtkgl::PFNGLSECONDARYCOLOR3FVPROC)manager->GetProcAddress("glSecondaryColor3fvEXT");
    vtkgl::SecondaryColor3i = (vtkgl::PFNGLSECONDARYCOLOR3IPROC)manager->GetProcAddress("glSecondaryColor3iEXT");
    vtkgl::SecondaryColor3iv = (vtkgl::PFNGLSECONDARYCOLOR3IVPROC)manager->GetProcAddress("glSecondaryColor3ivEXT");
    vtkgl::SecondaryColor3s = (vtkgl::PFNGLSECONDARYCOLOR3SPROC)manager->GetProcAddress("glSecondaryColor3sEXT");
    vtkgl::SecondaryColor3sv = (vtkgl::PFNGLSECONDARYCOLOR3SVPROC)manager->GetProcAddress("glSecondaryColor3svEXT");
    vtkgl::SecondaryColor3ub = (vtkgl::PFNGLSECONDARYCOLOR3UBPROC)manager->GetProcAddress("glSecondaryColor3ubEXT");
    vtkgl::SecondaryColor3ubv = (vtkgl::PFNGLSECONDARYCOLOR3UBVPROC)manager->GetProcAddress("glSecondaryColor3ubvEXT");
    vtkgl::SecondaryColor3ui = (vtkgl::PFNGLSECONDARYCOLOR3UIPROC)manager->GetProcAddress("glSecondaryColor3uiEXT");
    vtkgl::SecondaryColor3uiv = (vtkgl::PFNGLSECONDARYCOLOR3UIVPROC)manager->GetProcAddress("glSecondaryColor3uivEXT");
    vtkgl::SecondaryColor3us = (vtkgl::PFNGLSECONDARYCOLOR3USPROC)manager->GetProcAddress("glSecondaryColor3usEXT");
    vtkgl::SecondaryColor3usv = (vtkgl::PFNGLSECONDARYCOLOR3USVPROC)manager->GetProcAddress("glSecondaryColor3usvEXT");
    vtkgl::SecondaryColorPointer = (vtkgl::PFNGLSECONDARYCOLORPOINTERPROC)manager->GetProcAddress("glSecondaryColorPointerEXT");
    return 1 && (vtkgl::SecondaryColor3b != NULL) && (vtkgl::SecondaryColor3bv != NULL) && (vtkgl::SecondaryColor3d != NULL) && (vtkgl::SecondaryColor3dv != NULL) && (vtkgl::SecondaryColor3f != NULL) && (vtkgl::SecondaryColor3fv != NULL) && (vtkgl::SecondaryColor3i != NULL) && (vtkgl::SecondaryColor3iv != NULL) && (vtkgl::SecondaryColor3s != NULL) && (vtkgl::SecondaryColor3sv != NULL) && (vtkgl::SecondaryColor3ub!= NULL) && (vtkgl::SecondaryColor3ubv != NULL) && (vtkgl::SecondaryColor3ui != NULL) && (vtkgl::SecondaryColor3uiv != NULL) && (vtkgl::SecondaryColor3us != NULL) && (vtkgl::SecondaryColor3usv != NULL) && (vtkgl::SecondaryColorPointer != NULL);
    }

  if (strcmp(name, "GL_EXT_blend_func_separate") == 0)
    {
    vtkgl::BlendFuncSeparate = (vtkgl::PFNGLBLENDFUNCSEPARATEPROC)manager->GetProcAddress("glBlendFuncSeparateEXT");
    return 1 && (vtkgl::BlendFuncSeparate != NULL);
    }
   
  if (strcmp(name, "GL_EXT_stencil_wrap") == 0)
    {
    return 1;
    }

  if (strcmp(name, "GL_ARB_texture_env_crossbar") == 0)
    {
    return 1;
    }
   
  if (strcmp(name, "GL_EXT_texture_lod_bias") == 0)
    {
    return 1;
    }
   
  if (strcmp(name, "GL_ARB_texture_mirrored_repeat") == 0)
    {
    return 1;
    }

  if (strcmp(name, "GL_ARB_window_pos") == 0)
    {
    vtkgl::WindowPos2d = (vtkgl::PFNGLWINDOWPOS2DPROC)manager->GetProcAddress("glWindowPos2dARB");
    vtkgl::WindowPos2dv = (vtkgl::PFNGLWINDOWPOS2DVPROC)manager->GetProcAddress("glWindowPos2dvARB");
    vtkgl::WindowPos2f = (vtkgl::PFNGLWINDOWPOS2FPROC)manager->GetProcAddress("glWindowPos2fARB");
    vtkgl::WindowPos2fv = (vtkgl::PFNGLWINDOWPOS2FVPROC)manager->GetProcAddress("glWindowPos2fvARB");
    vtkgl::WindowPos2i = (vtkgl::PFNGLWINDOWPOS2IPROC)manager->GetProcAddress("glWindowPos2iARB");
    vtkgl::WindowPos2iv = (vtkgl::PFNGLWINDOWPOS2IVPROC)manager->GetProcAddress("glWindowPos2ivARB");
    vtkgl::WindowPos2s = (vtkgl::PFNGLWINDOWPOS2SPROC)manager->GetProcAddress("glWindowPos2sARB");
    vtkgl::WindowPos2sv = (vtkgl::PFNGLWINDOWPOS2SVPROC)manager->GetProcAddress("glWindowPos2svARB");
    vtkgl::WindowPos3d = (vtkgl::PFNGLWINDOWPOS3DPROC)manager->GetProcAddress("glWindowPos3dARB");
    vtkgl::WindowPos3dv = (vtkgl::PFNGLWINDOWPOS3DVPROC)manager->GetProcAddress("glWindowPos3dvARB");
    vtkgl::WindowPos3f = (vtkgl::PFNGLWINDOWPOS3FPROC)manager->GetProcAddress("glWindowPos3fARB");
    vtkgl::WindowPos3fv = (vtkgl::PFNGLWINDOWPOS3FVPROC)manager->GetProcAddress("glWindowPos3fvARB");
    vtkgl::WindowPos3i = (vtkgl::PFNGLWINDOWPOS3IPROC)manager->GetProcAddress("glWindowPos3iARB");
    vtkgl::WindowPos3iv = (vtkgl::PFNGLWINDOWPOS3IVPROC)manager->GetProcAddress("glWindowPos3ivARB");
    vtkgl::WindowPos3s = (vtkgl::PFNGLWINDOWPOS3SPROC)manager->GetProcAddress("glWindowPos3sARB");
    vtkgl::WindowPos3sv = (vtkgl::PFNGLWINDOWPOS3SVPROC)manager->GetProcAddress("glWindowPos3svARB");
    return 1 && (vtkgl::WindowPos2d != NULL) && (vtkgl::WindowPos2dv != NULL) && (vtkgl::WindowPos2f != NULL) && (vtkgl::WindowPos2fv != NULL) && (vtkgl::WindowPos2i != NULL) && (vtkgl::WindowPos2iv != NULL) && (vtkgl::WindowPos2s != NULL) && (vtkgl::WindowPos2sv != NULL) && (vtkgl::WindowPos3d != NULL) && (vtkgl::WindowPos3dv != NULL) && (vtkgl::WindowPos3f != NULL) && (vtkgl::WindowPos3fv != NULL) && (vtkgl::WindowPos3i != NULL) && (vtkgl::WindowPos3iv != NULL) && (vtkgl::WindowPos3s != NULL) && (vtkgl::WindowPos3sv != NULL);
    }

  // OpenGL 1.5
   
  if (strcmp(name, "GL_ARB_vertex_buffer_object") == 0)
    {
    vtkgl::BindBuffer = (vtkgl::PFNGLBINDBUFFERPROC)manager->GetProcAddress("glBindBufferARB");
    vtkgl::DeleteBuffers = (vtkgl::PFNGLDELETEBUFFERSPROC)manager->GetProcAddress("glDeleteBuffersARB");
    vtkgl::GenBuffers = (vtkgl::PFNGLGENBUFFERSPROC)manager->GetProcAddress("glGenBuffersARB");
    vtkgl::IsBuffer = (vtkgl::PFNGLISBUFFERPROC)manager->GetProcAddress("glIsBufferARB");
    vtkgl::BufferData = (vtkgl::PFNGLBUFFERDATAPROC)manager->GetProcAddress("glBufferDataARB");
    vtkgl::BufferSubData = (vtkgl::PFNGLBUFFERSUBDATAPROC)manager->GetProcAddress("glBufferSubDataARB");
    vtkgl::GetBufferSubData = (vtkgl::PFNGLGETBUFFERSUBDATAPROC)manager->GetProcAddress("glGetBufferSubDataARB");
    vtkgl::MapBuffer = (vtkgl::PFNGLMAPBUFFERPROC)manager->GetProcAddress("glMapBufferARB");
    vtkgl::UnmapBuffer = (vtkgl::PFNGLUNMAPBUFFERPROC)manager->GetProcAddress("glUnmapBufferARB");
    vtkgl::GetBufferParameteriv = (vtkgl::PFNGLGETBUFFERPARAMETERIVPROC)manager->GetProcAddress("glGetBufferParameterivARB");
    vtkgl::GetBufferPointerv = (vtkgl::PFNGLGETBUFFERPOINTERVPROC)manager->GetProcAddress("glGetBufferPointervARB");
    return 1 && (vtkgl::BindBuffer != NULL) && (vtkgl::DeleteBuffers != NULL) && (vtkgl::GenBuffers != NULL) && (vtkgl::IsBuffer != NULL) && (vtkgl::BufferData != NULL) && (vtkgl::BufferSubData != NULL) && (vtkgl::GetBufferSubData != NULL) && (vtkgl::MapBuffer != NULL) && (vtkgl::UnmapBuffer != NULL) && (vtkgl::GetBufferParameteriv != NULL) && (vtkgl::GetBufferPointerv != NULL);
    }
   
  if (strcmp(name, "GL_ARB_occlusion_query") == 0)
    {
    vtkgl::GenQueries = (vtkgl::PFNGLGENQUERIESPROC)manager->GetProcAddress("glGenQueriesARB");
    vtkgl::DeleteQueries = (vtkgl::PFNGLDELETEQUERIESPROC)manager->GetProcAddress("glDeleteQueriesARB");
    vtkgl::IsQuery = (vtkgl::PFNGLISQUERYPROC)manager->GetProcAddress("glIsQueryARB");
    vtkgl::BeginQuery = (vtkgl::PFNGLBEGINQUERYPROC)manager->GetProcAddress("glBeginQueryARB");
    vtkgl::EndQuery = (vtkgl::PFNGLENDQUERYPROC)manager->GetProcAddress("glEndQueryARB");
    vtkgl::GetQueryiv = (vtkgl::PFNGLGETQUERYIVPROC)manager->GetProcAddress("glGetQueryivARB");
    vtkgl::GetQueryObjectiv = (vtkgl::PFNGLGETQUERYOBJECTIVPROC)manager->GetProcAddress("glGetQueryObjectivARB");
    vtkgl::GetQueryObjectuiv = (vtkgl::PFNGLGETQUERYOBJECTUIVPROC)manager->GetProcAddress("glGetQueryObjectuivARB");
    return 1 && (vtkgl::GenQueries != NULL) && (vtkgl::DeleteQueries != NULL) && (vtkgl::IsQuery != NULL) && (vtkgl::BeginQuery != NULL) && (vtkgl::EndQuery != NULL) && (vtkgl::GetQueryiv != NULL) && (vtkgl::GetQueryObjectiv != NULL) && (vtkgl::GetQueryObjectuiv != NULL);
    }
   
  if (strcmp(name, "GL_EXT_shadow_funcs") == 0)
    {
    return 1;
    }
   
  // OpenGL 2.0
   
  if (strcmp(name, "GL_ARB_shader_objects") == 0)
    { 
    // glDeleteObjectARB translates both to DeleteProgram and DeleteShader.
     
    vtkgl::DeleteProgram = (vtkgl::PFNGLDELETEPROGRAMPROC)manager->GetProcAddress("glDeleteObjectARB");
    vtkgl::DeleteShader = (vtkgl::PFNGLDELETESHADERPROC)manager->GetProcAddress("glDeleteObjectARB");
     
    // There is no translation for GetHandle in OpenGL2.0.
   
    vtkgl::IsProgram = (vtkgl::PFNGLISPROGRAMPROC)IsProgramFromARBToPromoted;
    vtkgl::IsShader = (vtkgl::PFNGLISSHADERPROC)IsShaderFromARBToPromoted;
     
    vtkgl::DetachShader = (vtkgl::PFNGLDETACHSHADERPROC)manager->GetProcAddress("glDetachObjectARB");
    vtkgl::CreateShader = (vtkgl::PFNGLCREATESHADERPROC)manager->GetProcAddress("glCreateShaderObjectARB");
    vtkgl::ShaderSource = (vtkgl::PFNGLSHADERSOURCEPROC)manager->GetProcAddress("glShaderSourceARB");
    vtkgl::CompileShader = (vtkgl::PFNGLCOMPILESHADERPROC)manager->GetProcAddress("glCompileShaderARB");
    vtkgl::CreateProgram = (vtkgl::PFNGLCREATEPROGRAMPROC)manager->GetProcAddress("glCreateProgramObjectARB");
    
    vtkgl::AttachShader = (vtkgl::PFNGLATTACHSHADERPROC)manager->GetProcAddress("glAttachObjectARB");
    vtkgl::LinkProgram = (vtkgl::PFNGLLINKPROGRAMPROC)manager->GetProcAddress("glLinkProgramARB");
    vtkgl::UseProgram = (vtkgl::PFNGLUSEPROGRAMPROC)manager->GetProcAddress("glUseProgramObjectARB");
    vtkgl::ValidateProgram = (vtkgl::PFNGLVALIDATEPROGRAMPROC)manager->GetProcAddress("glValidateProgramARB");
    vtkgl::Uniform1f = (vtkgl::PFNGLUNIFORM1FPROC)manager->GetProcAddress("glUniform1fARB");
    vtkgl::Uniform2f = (vtkgl::PFNGLUNIFORM2FPROC)manager->GetProcAddress("glUniform2fARB");
    vtkgl::Uniform3f = (vtkgl::PFNGLUNIFORM3FPROC)manager->GetProcAddress("glUniform3fARB");
    vtkgl::Uniform4f = (vtkgl::PFNGLUNIFORM4FPROC)manager->GetProcAddress("glUniform4fARB");
    vtkgl::Uniform1i = (vtkgl::PFNGLUNIFORM1IPROC)manager->GetProcAddress("glUniform1iARB");
    vtkgl::Uniform2i = (vtkgl::PFNGLUNIFORM2IPROC)manager->GetProcAddress("glUniform2iARB");
    vtkgl::Uniform3i = (vtkgl::PFNGLUNIFORM3IPROC)manager->GetProcAddress("glUniform3iARB");
    vtkgl::Uniform4i = (vtkgl::PFNGLUNIFORM4IPROC)manager->GetProcAddress("glUniform4iARB");
    vtkgl::Uniform1fv = (vtkgl::PFNGLUNIFORM1FVPROC)manager->GetProcAddress("glUniform1fvARB");
    vtkgl::Uniform2fv = (vtkgl::PFNGLUNIFORM2FVPROC)manager->GetProcAddress("glUniform2fvARB");
    vtkgl::Uniform3fv = (vtkgl::PFNGLUNIFORM3FVPROC)manager->GetProcAddress("glUniform3fvARB");
    vtkgl::Uniform4fv = (vtkgl::PFNGLUNIFORM4FVPROC)manager->GetProcAddress("glUniform4fvARB");
    vtkgl::Uniform1iv = (vtkgl::PFNGLUNIFORM1IVPROC)manager->GetProcAddress("glUniform1ivARB");
    vtkgl::Uniform2iv = (vtkgl::PFNGLUNIFORM2IVPROC)manager->GetProcAddress("glUniform2ivARB");
    vtkgl::Uniform3iv = (vtkgl::PFNGLUNIFORM3IVPROC)manager->GetProcAddress("glUniform3ivARB");
    vtkgl::Uniform4iv = (vtkgl::PFNGLUNIFORM4IVPROC)manager->GetProcAddress("glUniform4ivARB");
    vtkgl::UniformMatrix2fv = (vtkgl::PFNGLUNIFORMMATRIX2FVPROC)manager->GetProcAddress("glUniformMatrix2fvARB");
    vtkgl::UniformMatrix3fv = (vtkgl::PFNGLUNIFORMMATRIX3FVPROC)manager->GetProcAddress("glUniformMatrix3fvARB");
    vtkgl::UniformMatrix4fv = (vtkgl::PFNGLUNIFORMMATRIX4FVPROC)manager->GetProcAddress("glUniformMatrix4fvARB");
    
    // GetObjectParameterf* don't have translation in OpenGL2.0
   
    // GetObjectParameter* translate both to GetProgram* and GetShader*
    vtkgl::GetProgramiv = (vtkgl::PFNGLGETPROGRAMIVPROC)manager->GetProcAddress("glGetObjectParameterivARB");
    vtkgl::GetShaderiv = (vtkgl::PFNGLGETSHADERIVPROC)manager->GetProcAddress("glGetObjectParameterivARB");
    
    // glGetInfoLogARB translates both to GetProgramInfoLog and
    // GetShaderInfoLog.
    
    vtkgl::GetProgramInfoLog = (vtkgl::PFNGLGETPROGRAMINFOLOGPROC)manager->GetProcAddress("glGetInfoLogARB");
    vtkgl::GetShaderInfoLog = (vtkgl::PFNGLGETSHADERINFOLOGPROC)manager->GetProcAddress("glGetInfoLogARB");
    
    
    vtkgl::GetAttachedShaders = (vtkgl::PFNGLGETATTACHEDSHADERSPROC)manager->GetProcAddress("glGetAttachedObjectsARB");
    vtkgl::GetUniformLocation = (vtkgl::PFNGLGETUNIFORMLOCATIONPROC)manager->GetProcAddress("glGetUniformLocationARB");
    vtkgl::GetActiveUniform = (vtkgl::PFNGLGETACTIVEUNIFORMPROC)manager->GetProcAddress("glGetActiveUniformARB");
    vtkgl::GetUniformfv = (vtkgl::PFNGLGETUNIFORMFVPROC)manager->GetProcAddress("glGetUniformfvARB");
    vtkgl::GetUniformiv = (vtkgl::PFNGLGETUNIFORMIVPROC)manager->GetProcAddress("glGetUniformivARB");
    vtkgl::GetShaderSource = (vtkgl::PFNGLGETSHADERSOURCEPROC)manager->GetProcAddress("glGetShaderSourceARB");
    return 1 && (vtkgl::DeleteProgram != NULL) && (vtkgl::DeleteShader != NULL) && (vtkgl::IsProgram != NULL) && (vtkgl::IsShader != NULL) && (vtkgl::DetachShader != NULL) && (vtkgl::CreateShader != NULL) && (vtkgl::ShaderSource != NULL) && (vtkgl::CompileShader != NULL) && (vtkgl::CreateProgram != NULL) && (vtkgl::AttachShader != NULL) && (vtkgl::LinkProgram != NULL) && (vtkgl::UseProgram != NULL) && (vtkgl::ValidateProgram != NULL) && (vtkgl::Uniform1f != NULL) && (vtkgl::Uniform2f != NULL) && (vtkgl::Uniform3f != NULL) && (vtkgl::Uniform4f != NULL) && (vtkgl::Uniform1i != NULL) && (vtkgl::Uniform2i != NULL) && (vtkgl::Uniform3i != NULL) && (vtkgl::Uniform4i != NULL) && (vtkgl::Uniform1fv != NULL) && (vtkgl::Uniform2fv != NULL) && (vtkgl::Uniform3fv != NULL) && (vtkgl::Uniform4fv != NULL) && (vtkgl::Uniform1iv != NULL) && (vtkgl::Uniform2iv != NULL) && (vtkgl::Uniform3iv != NULL) && (vtkgl::Uniform4iv != NULL) && (vtkgl::UniformMatrix2fv != NULL) && (vtkgl::UniformMatrix3fv != NULL) && (vtkgl::UniformMatrix4fv != NULL) && (vtkgl::GetProgramiv != NULL) && (vtkgl::GetShaderiv != NULL) && (vtkgl::GetProgramInfoLog != NULL) && (vtkgl::GetShaderInfoLog != NULL) && (vtkgl::GetAttachedShaders != NULL) && (vtkgl::GetUniformLocation != NULL) && (vtkgl::GetActiveUniform != NULL) && (vtkgl::GetUniformfv != NULL) && (vtkgl::GetUniformiv != NULL) && (vtkgl::GetShaderSource != NULL);
    }

  if (strcmp(name, "GL_ARB_vertex_shader") == 0)
    {
    vtkgl::BindAttribLocation = (vtkgl::PFNGLBINDATTRIBLOCATIONPROC)manager->GetProcAddress("glBindAttribLocationARB");
    vtkgl::GetActiveAttrib = (vtkgl::PFNGLGETACTIVEATTRIBPROC)manager->GetProcAddress("glGetActiveAttribARB");
    vtkgl::GetAttribLocation = (vtkgl::PFNGLGETATTRIBLOCATIONPROC)manager->GetProcAddress("glGetAttribLocationARB");
    
    // Defined both by GL_ARB_vertex_shader and GL_ARB_vertex_program
    vtkgl::DisableVertexAttribArray = (vtkgl::PFNGLDISABLEVERTEXATTRIBARRAYPROC)manager->GetProcAddress("glDisableVertexAttribArrayARB");
    vtkgl::EnableVertexAttribArray = (vtkgl::PFNGLENABLEVERTEXATTRIBARRAYPROC)manager->GetProcAddress("glEnableVertexAttribArrayARB");
    
    vtkgl::GetVertexAttribdv = (vtkgl::PFNGLGETVERTEXATTRIBDVPROC)manager->GetProcAddress("glGetVertexAttribdvARB");
    vtkgl::GetVertexAttribfv = (vtkgl::PFNGLGETVERTEXATTRIBFVPROC)manager->GetProcAddress("glGetVertexAttribfvARB");
    vtkgl::GetVertexAttribiv = (vtkgl::PFNGLGETVERTEXATTRIBIVPROC)manager->GetProcAddress("glGetVertexAttribivARB");
    vtkgl::GetVertexAttribPointerv = (vtkgl::PFNGLGETVERTEXATTRIBPOINTERVPROC)manager->GetProcAddress("glGetVertexAttribPointervARB");
    
    vtkgl::VertexAttrib1d = (vtkgl::PFNGLVERTEXATTRIB1DPROC)manager->GetProcAddress("glVertexAttrib1dARB");
    vtkgl::VertexAttrib1dv = (vtkgl::PFNGLVERTEXATTRIB1DVPROC)manager->GetProcAddress("glVertexAttrib1dvARB");
    vtkgl::VertexAttrib1f = (vtkgl::PFNGLVERTEXATTRIB1FPROC)manager->GetProcAddress("glVertexAttrib1fARB");
    vtkgl::VertexAttrib1fv = (vtkgl::PFNGLVERTEXATTRIB1FVPROC)manager->GetProcAddress("glVertexAttrib1fvARB");
    vtkgl::VertexAttrib1s = (vtkgl::PFNGLVERTEXATTRIB1SPROC)manager->GetProcAddress("glVertexAttrib1sARB");
    vtkgl::VertexAttrib1sv = (vtkgl::PFNGLVERTEXATTRIB1SVPROC)manager->GetProcAddress("glVertexAttrib1svARB");
    vtkgl::VertexAttrib2d = (vtkgl::PFNGLVERTEXATTRIB2DPROC)manager->GetProcAddress("glVertexAttrib2dARB");
    vtkgl::VertexAttrib2dv = (vtkgl::PFNGLVERTEXATTRIB2DVPROC)manager->GetProcAddress("glVertexAttrib2dvARB");
    vtkgl::VertexAttrib2f = (vtkgl::PFNGLVERTEXATTRIB2FPROC)manager->GetProcAddress("glVertexAttrib2fARB");
    vtkgl::VertexAttrib2fv = (vtkgl::PFNGLVERTEXATTRIB2FVPROC)manager->GetProcAddress("glVertexAttrib2fvARB");
    vtkgl::VertexAttrib2s = (vtkgl::PFNGLVERTEXATTRIB2SPROC)manager->GetProcAddress("glVertexAttrib2sARB");
    vtkgl::VertexAttrib2sv = (vtkgl::PFNGLVERTEXATTRIB2SVPROC)manager->GetProcAddress("glVertexAttrib2svARB");
    vtkgl::VertexAttrib3d = (vtkgl::PFNGLVERTEXATTRIB3DPROC)manager->GetProcAddress("glVertexAttrib3dARB");
    vtkgl::VertexAttrib3dv = (vtkgl::PFNGLVERTEXATTRIB3DVPROC)manager->GetProcAddress("glVertexAttrib3dvARB");
    vtkgl::VertexAttrib3f = (vtkgl::PFNGLVERTEXATTRIB3FPROC)manager->GetProcAddress("glVertexAttrib3fARB");
    vtkgl::VertexAttrib3fv = (vtkgl::PFNGLVERTEXATTRIB3FVPROC)manager->GetProcAddress("glVertexAttrib3fvARB");
    vtkgl::VertexAttrib3s = (vtkgl::PFNGLVERTEXATTRIB3SPROC)manager->GetProcAddress("glVertexAttrib3sARB");
    vtkgl::VertexAttrib3sv = (vtkgl::PFNGLVERTEXATTRIB3SVPROC)manager->GetProcAddress("glVertexAttrib3svARB");
    vtkgl::VertexAttrib4Nbv = (vtkgl::PFNGLVERTEXATTRIB4NBVPROC)manager->GetProcAddress("glVertexAttrib4NbvARB");
    vtkgl::VertexAttrib4Niv = (vtkgl::PFNGLVERTEXATTRIB4NIVPROC)manager->GetProcAddress("glVertexAttrib4NivARB");
    vtkgl::VertexAttrib4Nsv = (vtkgl::PFNGLVERTEXATTRIB4NSVPROC)manager->GetProcAddress("glVertexAttrib4NsvARB");
    vtkgl::VertexAttrib4Nub = (vtkgl::PFNGLVERTEXATTRIB4NUBPROC)manager->GetProcAddress("glVertexAttrib4NubARB");
    vtkgl::VertexAttrib4Nubv = (vtkgl::PFNGLVERTEXATTRIB4NUBVPROC)manager->GetProcAddress("glVertexAttrib4NubvARB");
    vtkgl::VertexAttrib4Nuiv = (vtkgl::PFNGLVERTEXATTRIB4NUIVPROC)manager->GetProcAddress("glVertexAttrib4NuivARB");
    vtkgl::VertexAttrib4Nusv = (vtkgl::PFNGLVERTEXATTRIB4NUSVPROC)manager->GetProcAddress("glVertexAttrib4NusvARB");
    vtkgl::VertexAttrib4bv = (vtkgl::PFNGLVERTEXATTRIB4BVPROC)manager->GetProcAddress("glVertexAttrib4bvARB");
    vtkgl::VertexAttrib4d = (vtkgl::PFNGLVERTEXATTRIB4DPROC)manager->GetProcAddress("glVertexAttrib4dARB");
    vtkgl::VertexAttrib4dv = (vtkgl::PFNGLVERTEXATTRIB4DVPROC)manager->GetProcAddress("glVertexAttrib4dvARB");
    vtkgl::VertexAttrib4f = (vtkgl::PFNGLVERTEXATTRIB4FPROC)manager->GetProcAddress("glVertexAttrib4fARB");
    vtkgl::VertexAttrib4fv = (vtkgl::PFNGLVERTEXATTRIB4FVPROC)manager->GetProcAddress("glVertexAttrib4fvARB");
    vtkgl::VertexAttrib4iv = (vtkgl::PFNGLVERTEXATTRIB4IVPROC)manager->GetProcAddress("glVertexAttrib4ivARB");
    vtkgl::VertexAttrib4s = (vtkgl::PFNGLVERTEXATTRIB4SPROC)manager->GetProcAddress("glVertexAttrib4sARB");
    vtkgl::VertexAttrib4sv = (vtkgl::PFNGLVERTEXATTRIB4SVPROC)manager->GetProcAddress("glVertexAttrib4svARB");
    vtkgl::VertexAttrib4ubv = (vtkgl::PFNGLVERTEXATTRIB4UBVPROC)manager->GetProcAddress("glVertexAttrib4ubvARB");
    vtkgl::VertexAttrib4uiv = (vtkgl::PFNGLVERTEXATTRIB4UIVPROC)manager->GetProcAddress("glVertexAttrib4uivARB");
    vtkgl::VertexAttrib4usv = (vtkgl::PFNGLVERTEXATTRIB4USVPROC)manager->GetProcAddress("glVertexAttrib4usvARB");
    vtkgl::VertexAttribPointer = (vtkgl::PFNGLVERTEXATTRIBPOINTERPROC)manager->GetProcAddress("glVertexAttribPointerARB");
    
    return 1 && (vtkgl::BindAttribLocation != NULL) && (vtkgl::GetActiveAttrib != NULL) && (vtkgl::GetAttribLocation != NULL) && (vtkgl::DisableVertexAttribArray != NULL) && (vtkgl::EnableVertexAttribArray != NULL) && (vtkgl::GetVertexAttribdv != NULL) && (vtkgl::GetVertexAttribfv != NULL) && (vtkgl::GetVertexAttribiv != NULL) && (vtkgl::GetVertexAttribPointerv != NULL) && (vtkgl::VertexAttrib1d != NULL) && (vtkgl::VertexAttrib1dv != NULL) && (vtkgl::VertexAttrib1f != NULL) && (vtkgl::VertexAttrib1fv != NULL) && (vtkgl::VertexAttrib1s != NULL) && (vtkgl::VertexAttrib1sv != NULL) && (vtkgl::VertexAttrib2d != NULL) && (vtkgl::VertexAttrib2dv != NULL) && (vtkgl::VertexAttrib2f != NULL) && (vtkgl::VertexAttrib2fv != NULL) && (vtkgl::VertexAttrib2s != NULL) && (vtkgl::VertexAttrib2sv != NULL) && (vtkgl::VertexAttrib3d != NULL) && (vtkgl::VertexAttrib3dv != NULL) && (vtkgl::VertexAttrib3f != NULL) && (vtkgl::VertexAttrib3fv != NULL) && (vtkgl::VertexAttrib3s != NULL) && (vtkgl::VertexAttrib3sv != NULL) && (vtkgl::VertexAttrib4Nbv != NULL) && (vtkgl::VertexAttrib4Niv != NULL) && (vtkgl::VertexAttrib4Nsv != NULL) && (vtkgl::VertexAttrib4Nub != NULL) && (vtkgl::VertexAttrib4Nubv != NULL) && (vtkgl::VertexAttrib4Nuiv != NULL) && (vtkgl::VertexAttrib4Nusv != NULL) && (vtkgl::VertexAttrib4bv != NULL) && (vtkgl::VertexAttrib4d != NULL) && (vtkgl::VertexAttrib4dv != NULL) && (vtkgl::VertexAttrib4f != NULL) && (vtkgl::VertexAttrib4fv != NULL) && (vtkgl::VertexAttrib4iv != NULL) && (vtkgl::VertexAttrib4s != NULL) && (vtkgl::VertexAttrib4sv != NULL) && (vtkgl::VertexAttrib4ubv != NULL) && (vtkgl::VertexAttrib4uiv != NULL) && (vtkgl::VertexAttrib4usv != NULL) && (vtkgl::VertexAttribPointer != NULL);
    
    // bug in the glext.h file:
    // the following method are in GL_ARB_vertex_program instead of
    // GL_ARB_vertex_shader
//    vtkgl::PFNGLENABLEVERTEXATTRIBARRAYARBPROC vtkgl::EnableVertexAttribArrayARB = NULL;
//vtkgl::PFNGLDISABLEVERTEXATTRIBARRAYARBPROC vtkgl::DisableVertexAttribArrayARB = NULL;
    }
 
  if (strcmp(name, "GL_ARB_fragment_shader") == 0)
    {
    return 1;
    }
 
  if (strcmp(name, "GL_ARB_shading_language_100") == 0)
    {
    return 1;
    }
 
  if (strcmp(name, "GL_ARB_draw_buffers") == 0)
    {
    vtkgl::DrawBuffers = (vtkgl::PFNGLDRAWBUFFERSPROC)manager->GetProcAddress("glDrawBuffersARB");
    return 1 && (vtkgl::DrawBuffers != NULL);
    }

  if (strcmp(name, "GL_ARB_texture_non_power_of_two") == 0)
    {
    return 1;
    }

  if (strcmp(name, "GL_ARB_point_sprite") == 0)
    {
    return 1;
    }
  
  if (strcmp(name, "GL_EXT_blend_equation_separate") == 0)
    {
    vtkgl::BlendEquationSeparate = (vtkgl::PFNGLBLENDEQUATIONSEPARATEPROC)manager->GetProcAddress("glBlendEquationSeparateEXT");
    return 1 && (vtkgl::BlendEquationSeparate != NULL);
    }
  
  if (strcmp(name, "GL_EXT_blend_logic_op") == 0)
    {
    return 1;
    }
  
  // Separate stencil was "based on" API of extension GL_ATI_separate_stencil
  // but this extension was not promoted to OpenGL2.0...
  
  if (strcmp(name, "GL_ATI_separate_stencil") == 0)
    {
    vtkgl::StencilOpSeparate = (vtkgl::PFNGLSTENCILOPSEPARATEPROC)manager->GetProcAddress("glStencilOpSeparateATI");
    vtkgl::StencilFuncSeparate = (vtkgl::PFNGLSTENCILFUNCSEPARATEPROC)manager->GetProcAddress("glStencilFuncSeparateATI");
    return 1 && (vtkgl::StencilOpSeparate != NULL) && (vtkgl::StencilFuncSeparate != NULL);
    // StencilMaskSeparate?
    }
  
  // No GL_EXT_stencil_two_side ? No ActiveStencilFace?
  
  // OpenGL 2.1
  if (strcmp(name, "GL_EXT_texture_sRGB") == 0)
    {
    return 1;
    }
  
  if (strcmp(name, "GL_ARB_pixel_buffer_object") == 0)
    {
    return 1;
    }
  
  return 0;
}
