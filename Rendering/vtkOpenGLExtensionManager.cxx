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

vtkStandardNewMacro(vtkOpenGLExtensionManager);

namespace vtkgl
{
// Description:
// Set the OpenGL function pointers with the function pointers
// of the core-promoted extension.
int LoadCorePromotedExtension(const char *name,
                              vtkOpenGLExtensionManager *manager);
// Description:
// Set the ARB function pointers with the function pointers
// of a EXT extension.
int LoadAsARBExtension(const char *name,
                       vtkOpenGLExtensionManager *manager);
}

vtkOpenGLExtensionManager::vtkOpenGLExtensionManager()
{
  this->OwnRenderWindow = 0;
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


vtkRenderWindow* vtkOpenGLExtensionManager::GetRenderWindow()
{
  return this->RenderWindow;
}

void vtkOpenGLExtensionManager::SetRenderWindow(vtkRenderWindow *renwin)
{
  if (renwin == this->RenderWindow)
    {
    return;
    }

  if (this->OwnRenderWindow && this->RenderWindow)
    {
    this->RenderWindow->UnRegister(this);
    this->RenderWindow = 0;
    }

  vtkDebugMacro("Setting RenderWindow to " << renwin);
  this->OwnRenderWindow = 0;
  this->RenderWindow = renwin;
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
  size_t NameLen = strlen(name);
  int result = 0;

  for(;;)
    {
    size_t n;
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
  
  // Woraround for a nVidia bug in indirect/remote rendering mode (ssh -X)
  // The version returns is not the one actually supported.
  // For example, the version returns is greater or equal to 2.1
  // but where PBO (which are core in 2.1) are not actually supported.
  // In this case, force the version to be 1.1 (minimal). Anything above
  // will be requested only through extensions.
  // See ParaView bug 
  if(result && !this->RenderWindow->IsDirect())
    {
    if (result && strncmp(name, "GL_VERSION_",11) == 0)
      {
      // whatever is the OpenGL version, return false.
      // (nobody asks for GL_VERSION_1_1)
      result=0;
      }
    }
  
  
  // Workaround for a bug on Mac PowerPC G5 with nVidia GeForce FX 5200
  // Mac OS 10.3.9 and driver 1.5 NVIDIA-1.3.42. It reports it supports
  // OpenGL>=1.4 but querying for glPointParameteri and glPointParameteriv
  // return null pointers. So it does not actually supports fully OpenGL 1.4.
  // It will make this method return false with "GL_VERSION_1_4" and true
  // with "GL_VERSION_1_5".
  if (result && strcmp(name, "GL_VERSION_1_4") == 0)
    {
    result=this->GetProcAddress("glPointParameteri")!=0 &&
      this->GetProcAddress("glPointParameteriv")!=0;
    }
  
  const char *gl_renderer=
    reinterpret_cast<const char *>(glGetString(GL_RENDERER));

  // Workaround for a bug on renderer string="Quadro4 900 XGL/AGP/SSE2"
  // version string="1.5.8 NVIDIA 96.43.01" or "1.5.6 NVIDIA 87.56"
  // The driver reports it supports 1.5 but the 1.4 core promoted extension
  // GL_EXT_blend_func_separate is implemented in software (poor performance).
  // All the NV2x chipsets are probably affected. NV2x chipsets are used
  // in GeForce4 and Quadro4.
  // It will make this method return false with "GL_VERSION_1_4" and true
  // with "GL_VERSION_1_5".
  if (result && strcmp(name, "GL_VERSION_1_4") == 0)
    {
    result=strstr(gl_renderer,"Quadro4")==0 &&
      strstr(gl_renderer,"GeForce4")==0;
    }
  
  const char *gl_version=
    reinterpret_cast<const char *>(glGetString(GL_VERSION));
  const char *gl_vendor=
    reinterpret_cast<const char *>(glGetString(GL_VENDOR));

  // Workaround for a bug on renderer string="ATI Radeon X1600 OpenGL Engine"
  // version string="2.0 ATI-1.4.58" vendor string="ATI Technologies Inc."
  // It happens on a Apple iMac Intel Core Duo (early 2006) with Mac OS X
  // 10.4.11 (Tiger) and an ATI Radeon X1600 128MB.
  // The driver reports it supports 2.0 (where GL_ARB_texture_non_power_of_two
  // extension has been promoted to core) and that it supports extension
  // GL_ARB_texture_non_power_of_two. Reality is that non power of two
  // textures just don't work in this OS/driver/card.
  // It will make this method returns false with "GL_VERSION_2_0" and true
  // with "GL_VERSION_2_1".
  // It will make this method returns false with
  // "GL_ARB_texture_non_power_of_two".
  if (result && strcmp(name, "GL_VERSION_2_0") == 0)
    {
    result=!(strcmp(gl_renderer,"ATI Radeon X1600 OpenGL Engine")==0 &&
             strcmp(gl_version,"2.0 ATI-1.4.58")==0 &&
             strcmp(gl_vendor,"ATI Technologies Inc.")==0);
    }
  if (result && strcmp(name, "GL_ARB_texture_non_power_of_two") == 0)
    {
    result=!(strcmp(gl_renderer,"ATI Radeon X1600 OpenGL Engine")==0 &&
             strcmp(gl_version,"2.0 ATI-1.4.58")==0 &&
             strcmp(gl_vendor,"ATI Technologies Inc.")==0);
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

#ifdef VTK_USE_X
 #ifdef VTK_USE_GLX_GET_PROC_ADDRESS
  // In a perfect world, it should be 
  // return static_cast<vtkOpenGLExtensionManagerFunctionPointer>(glXGetProcAddress(reinterpret_cast<const GLubyte *>(fname)));
  // but glx.h of Solaris 10 has line 209 wrong: it is
  // extern void (*glXGetProcAddress(const GLubyte *procname))();
  // when it should be:
  // extern void (*glXGetProcAddress(const GLubyte *procname))(void);
  return reinterpret_cast<vtkOpenGLExtensionManagerFunctionPointer>(glXGetProcAddress(reinterpret_cast<const GLubyte *>(fname)));
 #endif //VTK_USE_GLX_GET_PROC_ADDRESS
 #ifdef VTK_USE_GLX_GET_PROC_ADDRESS_ARB
  return reinterpret_cast<vtkOpenGLExtensionManagerFunctionPointer>(glXGetProcAddressARB(reinterpret_cast<const GLubyte *>(fname)));
 #endif //VTK_USE_GLX_GET_PROC_ADDRESS_ARB
#endif

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

  int success = this->SafeLoadExtension(name);

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
    loaded = this->SafeLoadExtension(name);
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

void vtkOpenGLExtensionManager::LoadAsARBExtension(const char *name)
{
  if (!this->ExtensionSupported(name))
    {
    vtkWarningMacro("Attempting to load " << name
                    << ", which is not supported.");
    }
  int success = vtkgl::LoadAsARBExtension(name, this);

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
    this->RenderWindow->MakeCurrent();
    if (!this->RenderWindow->IsCurrent())
      {
      // Really should create a method in the render window to create
      // the graphics context instead of forcing a full render.
      this->RenderWindow->Render();
      }
    if (!this->RenderWindow->IsCurrent())
      {
      // this case happens with a headless Mac: a mac with a graphics card
      // with no monitor attached to it, connected to it with "Screen Sharing"
      // (VNC-like feature added in Mac OS 10.5)
      // see bug 8554.
      this->ExtensionsString = new char[1];
      this->ExtensionsString[0] = '\0';
      return;
      }
    }

  vtkstd::string extensions_string;

  const char *gl_extensions;
  const char *glu_extensions = "";
  const char *win_extensions;

  gl_extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

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
    renwin->Register(this);
    this->OwnRenderWindow = 1;
    renwin->Delete();
    this->ReadOpenGLExtensions();
    return;
    }

  extensions_string = gl_extensions;

#if GLU_SUPPORTED
  glu_extensions =
    reinterpret_cast<const char *>(gluGetString(GLU_EXTENSIONS));
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

  const char *version =
    reinterpret_cast<const char *>(glGetString(GL_VERSION));
  int driverMajor = 0;
  int driverMinor = 0;
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
    display =
      static_cast<Display *>(this->RenderWindow->GetGenericDisplayId());
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

// ----------------------------------------------------------------------------
// Description:
// Wrap around the generated vtkgl::LoadExtension to deal with OpenGL 1.2
// and its optional part GL_ARB_imaging. Also functions like glBlendEquation()
// or glBlendColor are optional in OpenGL 1.2 or 1.3 and provided by the
// GL_ARB_imaging but there are core features in OpenGL 1.4.
int vtkOpenGLExtensionManager::SafeLoadExtension(const char *name)
{
  if (strcmp(name, "GL_VERSION_1_2") == 0)
    {
    vtkgl::DrawRangeElements = reinterpret_cast<vtkgl::PFNGLDRAWRANGEELEMENTSPROC>(this->GetProcAddress("glDrawRangeElements"));
    vtkgl::TexImage3D = reinterpret_cast<vtkgl::PFNGLTEXIMAGE3DPROC>(this->GetProcAddress("glTexImage3D"));
    vtkgl::TexSubImage3D = reinterpret_cast<vtkgl::PFNGLTEXSUBIMAGE3DPROC>(this->GetProcAddress("glTexSubImage3D"));
    vtkgl::CopyTexSubImage3D = reinterpret_cast<vtkgl::PFNGLCOPYTEXSUBIMAGE3DPROC>(this->GetProcAddress("glCopyTexSubImage3D"));
    
    // rely on the generated function for most of the OpenGL 1.2 functions.
    int success=vtkgl::LoadExtension(name, this);
    success=success && vtkgl::LoadExtension("GL_VERSION_1_2_DEPRECATED", this);
    
    return success && (vtkgl::DrawRangeElements != NULL) && (vtkgl::TexImage3D != NULL) && (vtkgl::TexSubImage3D != NULL) && (vtkgl::CopyTexSubImage3D != NULL);
    }
  if (strcmp(name, "GL_ARB_imaging") == 0)
    {
    vtkgl::BlendColor = reinterpret_cast<vtkgl::PFNGLBLENDCOLORPROC>(this->GetProcAddress("glBlendColor"));
    vtkgl::BlendEquation = reinterpret_cast<vtkgl::PFNGLBLENDEQUATIONPROC>(this->GetProcAddress("glBlendEquation"));
    vtkgl::ColorTable = reinterpret_cast<vtkgl::PFNGLCOLORTABLEPROC>(this->GetProcAddress("glColorTable"));
    vtkgl::ColorTableParameterfv = reinterpret_cast<vtkgl::PFNGLCOLORTABLEPARAMETERFVPROC>(this->GetProcAddress("glColorTableParameterfv"));
    vtkgl::ColorTableParameteriv = reinterpret_cast<vtkgl::PFNGLCOLORTABLEPARAMETERIVPROC>(this->GetProcAddress("glColorTableParameteriv"));
    vtkgl::CopyColorTable = reinterpret_cast<vtkgl::PFNGLCOPYCOLORTABLEPROC>(this->GetProcAddress("glCopyColorTable"));
    vtkgl::GetColorTable = reinterpret_cast<vtkgl::PFNGLGETCOLORTABLEPROC>(this->GetProcAddress("glGetColorTable"));
    vtkgl::GetColorTableParameterfv = reinterpret_cast<vtkgl::PFNGLGETCOLORTABLEPARAMETERFVPROC>(this->GetProcAddress("glGetColorTableParameterfv"));
    vtkgl::GetColorTableParameteriv = reinterpret_cast<vtkgl::PFNGLGETCOLORTABLEPARAMETERIVPROC>(this->GetProcAddress("glGetColorTableParameteriv"));
    vtkgl::ColorSubTable = reinterpret_cast<vtkgl::PFNGLCOLORSUBTABLEPROC>(this->GetProcAddress("glColorSubTable"));
    vtkgl::CopyColorSubTable = reinterpret_cast<vtkgl::PFNGLCOPYCOLORSUBTABLEPROC>(this->GetProcAddress("glCopyColorSubTable"));
    vtkgl::ConvolutionFilter1D = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONFILTER1DPROC>(this->GetProcAddress("glConvolutionFilter1D"));
    vtkgl::ConvolutionFilter2D = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONFILTER2DPROC>(this->GetProcAddress("glConvolutionFilter2D"));
    vtkgl::ConvolutionParameterf = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONPARAMETERFPROC>(this->GetProcAddress("glConvolutionParameterf"));
    vtkgl::ConvolutionParameterfv = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONPARAMETERFVPROC>(this->GetProcAddress("glConvolutionParameterfv"));
    vtkgl::ConvolutionParameteri = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONPARAMETERIPROC>(this->GetProcAddress("glConvolutionParameteri"));
    vtkgl::ConvolutionParameteriv = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONPARAMETERIVPROC>(this->GetProcAddress("glConvolutionParameteriv"));
    vtkgl::CopyConvolutionFilter1D = reinterpret_cast<vtkgl::PFNGLCOPYCONVOLUTIONFILTER1DPROC>(this->GetProcAddress("glCopyConvolutionFilter1D"));
    vtkgl::CopyConvolutionFilter2D = reinterpret_cast<vtkgl::PFNGLCOPYCONVOLUTIONFILTER2DPROC>(this->GetProcAddress("glCopyConvolutionFilter2D"));
    vtkgl::GetConvolutionFilter = reinterpret_cast<vtkgl::PFNGLGETCONVOLUTIONFILTERPROC>(this->GetProcAddress("glGetConvolutionFilter"));
    vtkgl::GetConvolutionParameterfv = reinterpret_cast<vtkgl::PFNGLGETCONVOLUTIONPARAMETERFVPROC>(this->GetProcAddress("glGetConvolutionParameterfv"));
    vtkgl::GetConvolutionParameteriv = reinterpret_cast<vtkgl::PFNGLGETCONVOLUTIONPARAMETERIVPROC>(this->GetProcAddress("glGetConvolutionParameteriv"));
    vtkgl::GetSeparableFilter = reinterpret_cast<vtkgl::PFNGLGETSEPARABLEFILTERPROC>(this->GetProcAddress("glGetSeparableFilter"));
    vtkgl::SeparableFilter2D = reinterpret_cast<vtkgl::PFNGLSEPARABLEFILTER2DPROC>(this->GetProcAddress("glSeparableFilter2D"));
    vtkgl::GetHistogram = reinterpret_cast<vtkgl::PFNGLGETHISTOGRAMPROC>(this->GetProcAddress("glGetHistogram"));
    vtkgl::GetHistogramParameterfv = reinterpret_cast<vtkgl::PFNGLGETHISTOGRAMPARAMETERFVPROC>(this->GetProcAddress("glGetHistogramParameterfv"));
    vtkgl::GetHistogramParameteriv = reinterpret_cast<vtkgl::PFNGLGETHISTOGRAMPARAMETERIVPROC>(this->GetProcAddress("glGetHistogramParameteriv"));
    vtkgl::GetMinmax = reinterpret_cast<vtkgl::PFNGLGETMINMAXPROC>(this->GetProcAddress("glGetMinmax"));
    vtkgl::GetMinmaxParameterfv = reinterpret_cast<vtkgl::PFNGLGETMINMAXPARAMETERFVPROC>(this->GetProcAddress("glGetMinmaxParameterfv"));
    vtkgl::GetMinmaxParameteriv = reinterpret_cast<vtkgl::PFNGLGETMINMAXPARAMETERIVPROC>(this->GetProcAddress("glGetMinmaxParameteriv"));
    vtkgl::Histogram = reinterpret_cast<vtkgl::PFNGLHISTOGRAMPROC>(this->GetProcAddress("glHistogram"));
    vtkgl::Minmax = reinterpret_cast<vtkgl::PFNGLMINMAXPROC>(this->GetProcAddress("glMinmax"));
    vtkgl::ResetHistogram = reinterpret_cast<vtkgl::PFNGLRESETHISTOGRAMPROC>(this->GetProcAddress("glResetHistogram"));
    vtkgl::ResetMinmax = reinterpret_cast<vtkgl::PFNGLRESETMINMAXPROC>(this->GetProcAddress("glResetMinmax"));
    return (vtkgl::BlendColor != NULL) && (vtkgl::BlendEquation != NULL) && (vtkgl::ColorTable != NULL) && (vtkgl::ColorTableParameterfv != NULL) && (vtkgl::ColorTableParameteriv != NULL) && (vtkgl::CopyColorTable != NULL) && (vtkgl::GetColorTable != NULL) && (vtkgl::GetColorTableParameterfv != NULL) && (vtkgl::GetColorTableParameteriv != NULL) && (vtkgl::ColorSubTable != NULL) && (vtkgl::CopyColorSubTable != NULL) && (vtkgl::ConvolutionFilter1D != NULL) && (vtkgl::ConvolutionFilter2D != NULL) && (vtkgl::ConvolutionParameterf != NULL) && (vtkgl::ConvolutionParameterfv != NULL) && (vtkgl::ConvolutionParameteri != NULL) && (vtkgl::ConvolutionParameteriv != NULL) && (vtkgl::CopyConvolutionFilter1D != NULL) && (vtkgl::CopyConvolutionFilter2D != NULL) && (vtkgl::GetConvolutionFilter != NULL) && (vtkgl::GetConvolutionParameterfv != NULL) && (vtkgl::GetConvolutionParameteriv != NULL) && (vtkgl::GetSeparableFilter != NULL) && (vtkgl::SeparableFilter2D != NULL) && (vtkgl::GetHistogram != NULL) && (vtkgl::GetHistogramParameterfv != NULL) && (vtkgl::GetHistogramParameteriv != NULL) && (vtkgl::GetMinmax != NULL) && (vtkgl::GetMinmaxParameterfv != NULL) && (vtkgl::GetMinmaxParameteriv != NULL) && (vtkgl::Histogram != NULL) && (vtkgl::Minmax != NULL) && (vtkgl::ResetHistogram != NULL) && (vtkgl::ResetMinmax != NULL);
    }
  
  if (strcmp(name, "GL_VERSION_1_3") == 0)
    {
    int success=vtkgl::LoadExtension(name, this);
    return success && vtkgl::LoadExtension("GL_VERSION_1_3_DEPRECATED", this);
    }
  if (strcmp(name, "GL_VERSION_1_4") == 0)
    {
    // rely on the generated function for most of the OpenGL 1.4 functions.
    int success=vtkgl::LoadExtension(name, this);
    
    success=success && vtkgl::LoadExtension("GL_VERSION_1_4_DEPRECATED", this);
    
    // The following functions that used to be optional in OpenGL 1.2 and 1.3
    // and only available through GL_ARB_imaging are now core features in
    // OpenGL 1.4.
    // See Appendix G.3 Changes to the imaging Subset.
    vtkgl::BlendColor = reinterpret_cast<vtkgl::PFNGLBLENDCOLORPROC>(this->GetProcAddress("glBlendColor"));
    vtkgl::BlendEquation = reinterpret_cast<vtkgl::PFNGLBLENDEQUATIONPROC>(this->GetProcAddress("glBlendEquation"));
    return success && (vtkgl::BlendColor != NULL) && (vtkgl::BlendEquation != NULL);
    }
  if (strcmp(name, "GL_VERSION_1_5") == 0)
    {
    int success=vtkgl::LoadExtension(name, this);
    return success && vtkgl::LoadExtension("GL_VERSION_1_5_DEPRECATED", this);
    }
  if (strcmp(name, "GL_VERSION_2_0") == 0)
    {
    int success=vtkgl::LoadExtension(name, this);
    return success && vtkgl::LoadExtension("GL_VERSION_2_0_DEPRECATED", this);
    }
  if (strcmp(name, "GL_VERSION_2_1") == 0)
    {
    int success=vtkgl::LoadExtension(name, this);
    return success && vtkgl::LoadExtension("GL_VERSION_2_1_DEPRECATED", this);
    }
  if (strcmp(name, "GL_VERSION_3_0") == 0)
    {
    int success=vtkgl::LoadExtension(name, this);
    return success && vtkgl::LoadExtension("GL_VERSION_3_0_DEPRECATED", this);
    }
   if (strcmp(name, "GL_ARB_framebuffer_object") == 0)
    {
    int success=vtkgl::LoadExtension(name, this);
    return success &&
      vtkgl::LoadExtension("GL_ARB_framebuffer_object_DEPRECATED", this);
    }
  
  // For all other cases, rely on the generated function.
  int result=vtkgl::LoadExtension(name, this);
  return result;
}

// Those two functions are part of OpenGL2.0 but don't have direct
// translation in the GL_ARB_shader_objects extension
GLboolean IsProgramFromARBToPromoted(GLuint program)
{
  GLint param;
  // in this case, vtkgl::GetProgramiv has been initialized with the pointer to
  // "GetObjectParameterivARB" by LoadCorePromotedExtension()
  // but vtkgl::GetObjectParameterivARB hasn't been initialized.
  vtkgl::GetProgramiv(program, vtkgl::OBJECT_TYPE_ARB, &param);
  return param==static_cast<GLint>(vtkgl::PROGRAM_OBJECT_ARB);
}

GLboolean IsShaderFromARBToPromoted(GLuint shader)
{
  GLint param;
  // in this case, vtkgl::GetShaderiv has been initialized with the pointer to
  // "GetObjectParameterivARB" by LoadCorePromotedExtension()
  // but vtkgl::GetObjectParameterivARB hasn't been initialized.
  vtkgl::GetShaderiv(shader, vtkgl::OBJECT_TYPE_ARB, &param);
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
    vtkgl::TexImage3D = reinterpret_cast<vtkgl::PFNGLTEXIMAGE3DPROC>(manager->GetProcAddress("glTexImage3DEXT"));
    vtkgl::TexSubImage3D = reinterpret_cast<vtkgl::PFNGLTEXSUBIMAGE3DPROC>(manager->GetProcAddress("glTexSubImage3DEXT"));
    vtkgl::CopyTexSubImage3D = reinterpret_cast<vtkgl::PFNGLCOPYTEXSUBIMAGE3DPROC>(manager->GetProcAddress("glCopyTexSubImage3DEXT"));
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
    vtkgl::DrawRangeElements = reinterpret_cast<vtkgl::PFNGLDRAWRANGEELEMENTSPROC>(manager->GetProcAddress("glDrawRangeElementsEXT"));
    return 1 && (vtkgl::DrawRangeElements != NULL);
    }
  
  if (strcmp(name, "GL_SGI_color_table") == 0)
    {
    // OpenGL Spec talks about GL_EXT_color_table but reality is
    // GL_SGI_color_table is used. Also GL_EXT_color_table is not listed
    // on the registry website.
    vtkgl::ColorTable = reinterpret_cast<vtkgl::PFNGLCOLORTABLESGIPROC>(manager->GetProcAddress("glColorTableSGI"));
    vtkgl::ColorTableParameterfv = reinterpret_cast<vtkgl::PFNGLCOLORTABLEPARAMETERFVSGIPROC>(manager->GetProcAddress("glColorTableParameterfvSGI"));
    vtkgl::ColorTableParameteriv = reinterpret_cast<vtkgl::PFNGLCOLORTABLEPARAMETERIVSGIPROC>(manager->GetProcAddress("glColorTableParameterivSGI"));
    vtkgl::CopyColorTable = reinterpret_cast<vtkgl::PFNGLCOPYCOLORTABLESGIPROC>(manager->GetProcAddress("glCopyColorTableSGI"));
    vtkgl::GetColorTable = reinterpret_cast<vtkgl::PFNGLGETCOLORTABLESGIPROC>(manager->GetProcAddress("glGetColorTableSGI"));
    vtkgl::GetColorTableParameterfv = reinterpret_cast<vtkgl::PFNGLGETCOLORTABLEPARAMETERFVSGIPROC>(manager->GetProcAddress("glGetColorTableParameterfvSGI"));
    vtkgl::GetColorTableParameteriv = reinterpret_cast<vtkgl::PFNGLGETCOLORTABLEPARAMETERIVSGIPROC>(manager->GetProcAddress("glGetColorTableParameterivSGI"));
    return 1 && (vtkgl::ColorTable != NULL) && (vtkgl::ColorTableParameterfv != NULL) && (vtkgl::ColorTableParameteriv != NULL) && (vtkgl::CopyColorTable != NULL) && (vtkgl::GetColorTable != NULL) && (vtkgl::GetColorTableParameterfv != NULL) && (vtkgl::GetColorTableParameteriv != NULL);
    }
  
  if (strcmp(name, "GL_EXT_color_subtable") == 0)
    {
    vtkgl::ColorSubTable = reinterpret_cast<vtkgl::PFNGLCOLORSUBTABLEPROC>(manager->GetProcAddress("glColorSubTableEXT"));
    vtkgl::CopyColorSubTable = reinterpret_cast<vtkgl::PFNGLCOPYCOLORSUBTABLEPROC>(manager->GetProcAddress("glCopyColorSubTableEXT"));
    return 1 && (vtkgl::ColorSubTable != NULL) && (vtkgl::CopyColorSubTable != NULL);
    }
  
  if (strcmp(name, "GL_EXT_convolution") == 0)
    {
    vtkgl::ConvolutionFilter1D = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONFILTER1DPROC>(manager->GetProcAddress("glConvolutionFilter1DEXT"));
    vtkgl::ConvolutionFilter2D = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONFILTER2DPROC>(manager->GetProcAddress("glConvolutionFilter2DEXT"));
    vtkgl::ConvolutionParameterf = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONPARAMETERFPROC>(manager->GetProcAddress("glConvolutionParameterfEXT"));
    vtkgl::ConvolutionParameterfv = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONPARAMETERFVPROC>(manager->GetProcAddress("glConvolutionParameterfvEXT"));
    vtkgl::ConvolutionParameteri = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONPARAMETERIPROC>(manager->GetProcAddress("glConvolutionParameteriEXT"));
    vtkgl::ConvolutionParameteriv = reinterpret_cast<vtkgl::PFNGLCONVOLUTIONPARAMETERIVPROC>(manager->GetProcAddress("glConvolutionParameterivEXT"));
    vtkgl::CopyConvolutionFilter1D = reinterpret_cast<vtkgl::PFNGLCOPYCONVOLUTIONFILTER1DPROC>(manager->GetProcAddress("glCopyConvolutionFilter1DEXT"));
    vtkgl::CopyConvolutionFilter2D = reinterpret_cast<vtkgl::PFNGLCOPYCONVOLUTIONFILTER2DPROC>(manager->GetProcAddress("glCopyConvolutionFilter2DEXT"));
    vtkgl::GetConvolutionFilter = reinterpret_cast<vtkgl::PFNGLGETCONVOLUTIONFILTERPROC>(manager->GetProcAddress("glGetConvolutionFilterEXT"));
    vtkgl::GetConvolutionParameterfv = reinterpret_cast<vtkgl::PFNGLGETCONVOLUTIONPARAMETERFVPROC>(manager->GetProcAddress("glGetConvolutionParameterfvEXT"));
    vtkgl::GetConvolutionParameteriv = reinterpret_cast<vtkgl::PFNGLGETCONVOLUTIONPARAMETERIVPROC>(manager->GetProcAddress("glGetConvolutionParameterivEXT"));
    vtkgl::GetSeparableFilter = reinterpret_cast<vtkgl::PFNGLGETSEPARABLEFILTERPROC>(manager->GetProcAddress("glGetSeparableFilterEXT"));
    vtkgl::SeparableFilter2D = reinterpret_cast<vtkgl::PFNGLSEPARABLEFILTER2DPROC>(manager->GetProcAddress("glSeparableFilter2DEXT"));
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
    vtkgl::GetHistogram = reinterpret_cast<vtkgl::PFNGLGETHISTOGRAMPROC>(manager->GetProcAddress("glGetHistogramEXT"));
    vtkgl::GetHistogramParameterfv = reinterpret_cast<vtkgl::PFNGLGETHISTOGRAMPARAMETERFVPROC>(manager->GetProcAddress("glGetHistogramParameterfvEXT"));
    vtkgl::GetHistogramParameteriv = reinterpret_cast<vtkgl::PFNGLGETHISTOGRAMPARAMETERIVPROC>(manager->GetProcAddress("glGetHistogramParameterivEXT"));
    vtkgl::GetMinmax = reinterpret_cast<vtkgl::PFNGLGETMINMAXPROC>(manager->GetProcAddress("glGetMinmaxEXT"));
    vtkgl::GetMinmaxParameterfv = reinterpret_cast<vtkgl::PFNGLGETMINMAXPARAMETERFVPROC>(manager->GetProcAddress("glGetMinmaxParameterfvEXT"));
    vtkgl::GetMinmaxParameteriv = reinterpret_cast<vtkgl::PFNGLGETMINMAXPARAMETERIVPROC>(manager->GetProcAddress("glGetMinmaxParameterivEXT"));
    vtkgl::Histogram = reinterpret_cast<vtkgl::PFNGLHISTOGRAMPROC>(manager->GetProcAddress("glHistogramEXT"));
    vtkgl::Minmax = reinterpret_cast<vtkgl::PFNGLMINMAXPROC>(manager->GetProcAddress("glMinmaxEXT"));
    vtkgl::ResetHistogram = reinterpret_cast<vtkgl::PFNGLRESETHISTOGRAMPROC>(manager->GetProcAddress("glResetHistogramEXT"));
    vtkgl::ResetMinmax = reinterpret_cast<vtkgl::PFNGLRESETMINMAXPROC>(manager->GetProcAddress("glResetMinmaxEXT"));
    return 1 && (vtkgl::GetHistogram != NULL) && (vtkgl::GetHistogramParameterfv != NULL) && (vtkgl::GetHistogramParameteriv != NULL) && (vtkgl::GetMinmax != NULL) && (vtkgl::GetMinmaxParameterfv != NULL) && (vtkgl::GetMinmaxParameteriv != NULL) && (vtkgl::Histogram != NULL) && (vtkgl::Minmax != NULL) && (vtkgl::ResetHistogram != NULL) && (vtkgl::ResetMinmax != NULL);
    }

  if (strcmp(name, "GL_EXT_blend_color") == 0)
    {
    vtkgl::BlendColor = reinterpret_cast<vtkgl::PFNGLBLENDCOLORPROC>(manager->GetProcAddress("glBlendColorEXT"));
    return 1 && (vtkgl::BlendColor != NULL);
    }
  
  if (strcmp(name, "GL_EXT_blend_minmax") == 0)
    {
    vtkgl::BlendEquation = reinterpret_cast<vtkgl::PFNGLBLENDEQUATIONPROC>(manager->GetProcAddress("glBlendEquationEXT"));
    return 1 && (vtkgl::BlendEquation != NULL);
    }
  if (strcmp(name, "GL_EXT_blend_subtract") == 0)
    {
    return 1;
    }

  // OpenGL 1.3
  
  if (strcmp(name, "GL_ARB_texture_compression") == 0)
    {
    vtkgl::CompressedTexImage3D = reinterpret_cast<vtkgl::PFNGLCOMPRESSEDTEXIMAGE3DPROC>(manager->GetProcAddress("glCompressedTexImage3DARB"));
    vtkgl::CompressedTexImage2D = reinterpret_cast<vtkgl::PFNGLCOMPRESSEDTEXIMAGE2DPROC>(manager->GetProcAddress("glCompressedTexImage2DARB"));
    vtkgl::CompressedTexImage1D = reinterpret_cast<vtkgl::PFNGLCOMPRESSEDTEXIMAGE1DPROC>(manager->GetProcAddress("glCompressedTexImage1DARB"));
    vtkgl::CompressedTexSubImage3D = reinterpret_cast<vtkgl::PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC>(manager->GetProcAddress("glCompressedTexSubImage3DARB"));
    vtkgl::CompressedTexSubImage2D = reinterpret_cast<vtkgl::PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC>(manager->GetProcAddress("glCompressedTexSubImage2DARB"));
    vtkgl::CompressedTexSubImage1D = reinterpret_cast<vtkgl::PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC>(manager->GetProcAddress("glCompressedTexSubImage1DARB"));
    vtkgl::GetCompressedTexImage = reinterpret_cast<vtkgl::PFNGLGETCOMPRESSEDTEXIMAGEPROC>(manager->GetProcAddress("glGetCompressedTexImageARB"));
    return 1 && (vtkgl::CompressedTexImage3D != NULL) && (vtkgl::CompressedTexImage2D != NULL) && (vtkgl::CompressedTexImage1D != NULL) && (vtkgl::CompressedTexSubImage3D != NULL) && (vtkgl::CompressedTexSubImage2D != NULL) && (vtkgl::CompressedTexSubImage1D != NULL) && (vtkgl::GetCompressedTexImage != NULL);
    }

  if (strcmp(name, "GL_ARB_texture_cube_map") == 0)
    {
    return 1;
    }
  
  if (strcmp(name, "GL_ARB_multisample") == 0)
    {
    vtkgl::SampleCoverage = reinterpret_cast<vtkgl::PFNGLSAMPLECOVERAGEPROC>(manager->GetProcAddress("glSampleCoverageARB"));
    return 1 && (vtkgl::SampleCoverage != NULL);
    }
  
  if (strcmp(name, "GL_ARB_multitexture") == 0)
    {
    vtkgl::ActiveTexture = reinterpret_cast<vtkgl::PFNGLACTIVETEXTUREPROC>(manager->GetProcAddress("glActiveTextureARB"));
    vtkgl::ClientActiveTexture = reinterpret_cast<vtkgl::PFNGLCLIENTACTIVETEXTUREPROC>(manager->GetProcAddress("glClientActiveTextureARB"));
    vtkgl::MultiTexCoord1d = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD1DPROC>(manager->GetProcAddress("glMultiTexCoord1dARB"));
    vtkgl::MultiTexCoord1dv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD1DVPROC>(manager->GetProcAddress("glMultiTexCoord1dvARB"));
    vtkgl::MultiTexCoord1f = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD1FPROC>(manager->GetProcAddress("glMultiTexCoord1fARB"));
    vtkgl::MultiTexCoord1fv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD1FVPROC>(manager->GetProcAddress("glMultiTexCoord1fvARB"));
    vtkgl::MultiTexCoord1i = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD1IPROC>(manager->GetProcAddress("glMultiTexCoord1iARB"));
    vtkgl::MultiTexCoord1iv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD1IVPROC>(manager->GetProcAddress("glMultiTexCoord1ivARB"));
    vtkgl::MultiTexCoord1s = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD1SPROC>(manager->GetProcAddress("glMultiTexCoord1sARB"));
    vtkgl::MultiTexCoord1sv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD1SVPROC>(manager->GetProcAddress("glMultiTexCoord1svARB"));
    vtkgl::MultiTexCoord2d = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD2DPROC>(manager->GetProcAddress("glMultiTexCoord2dARB"));
    vtkgl::MultiTexCoord2dv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD2DVPROC>(manager->GetProcAddress("glMultiTexCoord2dvARB"));
    vtkgl::MultiTexCoord2f = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD2FPROC>(manager->GetProcAddress("glMultiTexCoord2fARB"));
    vtkgl::MultiTexCoord2fv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD2FVPROC>(manager->GetProcAddress("glMultiTexCoord2fvARB"));
    vtkgl::MultiTexCoord2i = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD2IPROC>(manager->GetProcAddress("glMultiTexCoord2iARB"));
    vtkgl::MultiTexCoord2iv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD2IVPROC>(manager->GetProcAddress("glMultiTexCoord2ivARB"));
    vtkgl::MultiTexCoord2s = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD2SPROC>(manager->GetProcAddress("glMultiTexCoord2sARB"));
    vtkgl::MultiTexCoord2sv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD2SVPROC>(manager->GetProcAddress("glMultiTexCoord2svARB"));
    vtkgl::MultiTexCoord3d = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD3DPROC>(manager->GetProcAddress("glMultiTexCoord3dARB"));
    vtkgl::MultiTexCoord3dv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD3DVPROC>(manager->GetProcAddress("glMultiTexCoord3dvARB"));
    vtkgl::MultiTexCoord3f = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD3FPROC>(manager->GetProcAddress("glMultiTexCoord3fARB"));
    vtkgl::MultiTexCoord3fv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD3FVPROC>(manager->GetProcAddress("glMultiTexCoord3fvARB"));
    vtkgl::MultiTexCoord3i = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD3IPROC>(manager->GetProcAddress("glMultiTexCoord3iARB"));
    vtkgl::MultiTexCoord3iv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD3IVPROC>(manager->GetProcAddress("glMultiTexCoord3ivARB"));
    vtkgl::MultiTexCoord3s = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD3SPROC>(manager->GetProcAddress("glMultiTexCoord3sARB"));
    vtkgl::MultiTexCoord3sv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD3SVPROC>(manager->GetProcAddress("glMultiTexCoord3svARB"));
    vtkgl::MultiTexCoord4d = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD4DPROC>(manager->GetProcAddress("glMultiTexCoord4dARB"));
    vtkgl::MultiTexCoord4dv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD4DVPROC>(manager->GetProcAddress("glMultiTexCoord4dvARB"));
    vtkgl::MultiTexCoord4f = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD4FPROC>(manager->GetProcAddress("glMultiTexCoord4fARB"));
    vtkgl::MultiTexCoord4fv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD4FVPROC>(manager->GetProcAddress("glMultiTexCoord4fvARB"));
    vtkgl::MultiTexCoord4i = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD4IPROC>(manager->GetProcAddress("glMultiTexCoord4iARB"));
    vtkgl::MultiTexCoord4iv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD4IVPROC>(manager->GetProcAddress("glMultiTexCoord4ivARB"));
    vtkgl::MultiTexCoord4s = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD4SPROC>(manager->GetProcAddress("glMultiTexCoord4sARB"));
    vtkgl::MultiTexCoord4sv = reinterpret_cast<vtkgl::PFNGLMULTITEXCOORD4SVPROC>(manager->GetProcAddress("glMultiTexCoord4svARB"));
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
    vtkgl::LoadTransposeMatrixf = reinterpret_cast<vtkgl::PFNGLLOADTRANSPOSEMATRIXFPROC>(manager->GetProcAddress("glLoadTransposeMatrixfARB"));
    vtkgl::LoadTransposeMatrixd = reinterpret_cast<vtkgl::PFNGLLOADTRANSPOSEMATRIXDPROC>(manager->GetProcAddress("glLoadTransposeMatrixdARB"));
    vtkgl::MultTransposeMatrixf = reinterpret_cast<vtkgl::PFNGLMULTTRANSPOSEMATRIXFPROC>(manager->GetProcAddress("glMultTransposeMatrixfARB"));
    vtkgl::MultTransposeMatrixd = reinterpret_cast<vtkgl::PFNGLMULTTRANSPOSEMATRIXDPROC>(manager->GetProcAddress("glMultTransposeMatrixdARB"));
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
    vtkgl::FogCoordf = reinterpret_cast<vtkgl::PFNGLFOGCOORDFPROC>(manager->GetProcAddress("glFogCoordfEXT"));
    vtkgl::FogCoordfv = reinterpret_cast<vtkgl::PFNGLFOGCOORDFVPROC>(manager->GetProcAddress("glFogCoordfvEXT"));
    vtkgl::FogCoordd = reinterpret_cast<vtkgl::PFNGLFOGCOORDDPROC>(manager->GetProcAddress("glFogCoorddEXT"));
    vtkgl::FogCoorddv = reinterpret_cast<vtkgl::PFNGLFOGCOORDDVPROC>(manager->GetProcAddress("glFogCoorddvEXT"));
    vtkgl::FogCoordPointer = reinterpret_cast<vtkgl::PFNGLFOGCOORDPOINTERPROC>(manager->GetProcAddress("glFogCoordPointerEXT"));
    return 1 && (vtkgl::FogCoordf != NULL) && (vtkgl::FogCoordfv != NULL) && (vtkgl::FogCoordd != NULL) && (vtkgl::FogCoorddv != NULL) && (vtkgl::FogCoordPointer != NULL);
    }

  if (strcmp(name, "GL_EXT_multi_draw_arrays") == 0)
    {
    vtkgl::MultiDrawArrays = reinterpret_cast<vtkgl::PFNGLMULTIDRAWARRAYSPROC>(manager->GetProcAddress("glMultiDrawArraysEXT"));
    vtkgl::MultiDrawElements = reinterpret_cast<vtkgl::PFNGLMULTIDRAWELEMENTSPROC>(manager->GetProcAddress("glMultiDrawElementsEXT"));
    return 1 && (vtkgl::MultiDrawArrays != NULL) && (vtkgl::MultiDrawElements != NULL);
    }

  if (strcmp(name, "GL_ARB_point_parameters") == 0)
    {
    vtkgl::PointParameterf = reinterpret_cast<vtkgl::PFNGLPOINTPARAMETERFPROC>(manager->GetProcAddress("glPointParameterfARB"));
    vtkgl::PointParameterfv = reinterpret_cast<vtkgl::PFNGLPOINTPARAMETERFVPROC>(manager->GetProcAddress("glPointParameterfvARB"));
    return 1 && (vtkgl::PointParameterf != NULL) && (vtkgl::PointParameterfv != NULL);
    }

  
  if (strcmp(name, "GL_EXT_secondary_color") == 0)
    {
    vtkgl::SecondaryColor3b = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3BPROC>(manager->GetProcAddress("glSecondaryColor3bEXT"));
    vtkgl::SecondaryColor3bv = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3BVPROC>(manager->GetProcAddress("glSecondaryColor3bvEXT"));
    vtkgl::SecondaryColor3d = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3DPROC>(manager->GetProcAddress("glSecondaryColor3dEXT"));
    vtkgl::SecondaryColor3dv = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3DVPROC>(manager->GetProcAddress("glSecondaryColor3dvEXT"));
    vtkgl::SecondaryColor3f = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3FPROC>(manager->GetProcAddress("glSecondaryColor3fEXT"));
    vtkgl::SecondaryColor3fv = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3FVPROC>(manager->GetProcAddress("glSecondaryColor3fvEXT"));
    vtkgl::SecondaryColor3i = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3IPROC>(manager->GetProcAddress("glSecondaryColor3iEXT"));
    vtkgl::SecondaryColor3iv = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3IVPROC>(manager->GetProcAddress("glSecondaryColor3ivEXT"));
    vtkgl::SecondaryColor3s = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3SPROC>(manager->GetProcAddress("glSecondaryColor3sEXT"));
    vtkgl::SecondaryColor3sv = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3SVPROC>(manager->GetProcAddress("glSecondaryColor3svEXT"));
    vtkgl::SecondaryColor3ub = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3UBPROC>(manager->GetProcAddress("glSecondaryColor3ubEXT"));
    vtkgl::SecondaryColor3ubv = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3UBVPROC>(manager->GetProcAddress("glSecondaryColor3ubvEXT"));
    vtkgl::SecondaryColor3ui = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3UIPROC>(manager->GetProcAddress("glSecondaryColor3uiEXT"));
    vtkgl::SecondaryColor3uiv = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3UIVPROC>(manager->GetProcAddress("glSecondaryColor3uivEXT"));
    vtkgl::SecondaryColor3us = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3USPROC>(manager->GetProcAddress("glSecondaryColor3usEXT"));
    vtkgl::SecondaryColor3usv = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLOR3USVPROC>(manager->GetProcAddress("glSecondaryColor3usvEXT"));
    vtkgl::SecondaryColorPointer = reinterpret_cast<vtkgl::PFNGLSECONDARYCOLORPOINTERPROC>(manager->GetProcAddress("glSecondaryColorPointerEXT"));
    return 1 && (vtkgl::SecondaryColor3b != NULL) && (vtkgl::SecondaryColor3bv != NULL) && (vtkgl::SecondaryColor3d != NULL) && (vtkgl::SecondaryColor3dv != NULL) && (vtkgl::SecondaryColor3f != NULL) && (vtkgl::SecondaryColor3fv != NULL) && (vtkgl::SecondaryColor3i != NULL) && (vtkgl::SecondaryColor3iv != NULL) && (vtkgl::SecondaryColor3s != NULL) && (vtkgl::SecondaryColor3sv != NULL) && (vtkgl::SecondaryColor3ub!= NULL) && (vtkgl::SecondaryColor3ubv != NULL) && (vtkgl::SecondaryColor3ui != NULL) && (vtkgl::SecondaryColor3uiv != NULL) && (vtkgl::SecondaryColor3us != NULL) && (vtkgl::SecondaryColor3usv != NULL) && (vtkgl::SecondaryColorPointer != NULL);
    }

  if (strcmp(name, "GL_EXT_blend_func_separate") == 0)
    {
    vtkgl::BlendFuncSeparate = reinterpret_cast<vtkgl::PFNGLBLENDFUNCSEPARATEPROC>(manager->GetProcAddress("glBlendFuncSeparateEXT"));
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
    vtkgl::WindowPos2d = reinterpret_cast<vtkgl::PFNGLWINDOWPOS2DPROC>(manager->GetProcAddress("glWindowPos2dARB"));
    vtkgl::WindowPos2dv = reinterpret_cast<vtkgl::PFNGLWINDOWPOS2DVPROC>(manager->GetProcAddress("glWindowPos2dvARB"));
    vtkgl::WindowPos2f = reinterpret_cast<vtkgl::PFNGLWINDOWPOS2FPROC>(manager->GetProcAddress("glWindowPos2fARB"));
    vtkgl::WindowPos2fv = reinterpret_cast<vtkgl::PFNGLWINDOWPOS2FVPROC>(manager->GetProcAddress("glWindowPos2fvARB"));
    vtkgl::WindowPos2i = reinterpret_cast<vtkgl::PFNGLWINDOWPOS2IPROC>(manager->GetProcAddress("glWindowPos2iARB"));
    vtkgl::WindowPos2iv = reinterpret_cast<vtkgl::PFNGLWINDOWPOS2IVPROC>(manager->GetProcAddress("glWindowPos2ivARB"));
    vtkgl::WindowPos2s = reinterpret_cast<vtkgl::PFNGLWINDOWPOS2SPROC>(manager->GetProcAddress("glWindowPos2sARB"));
    vtkgl::WindowPos2sv = reinterpret_cast<vtkgl::PFNGLWINDOWPOS2SVPROC>(manager->GetProcAddress("glWindowPos2svARB"));
    vtkgl::WindowPos3d = reinterpret_cast<vtkgl::PFNGLWINDOWPOS3DPROC>(manager->GetProcAddress("glWindowPos3dARB"));
    vtkgl::WindowPos3dv = reinterpret_cast<vtkgl::PFNGLWINDOWPOS3DVPROC>(manager->GetProcAddress("glWindowPos3dvARB"));
    vtkgl::WindowPos3f = reinterpret_cast<vtkgl::PFNGLWINDOWPOS3FPROC>(manager->GetProcAddress("glWindowPos3fARB"));
    vtkgl::WindowPos3fv = reinterpret_cast<vtkgl::PFNGLWINDOWPOS3FVPROC>(manager->GetProcAddress("glWindowPos3fvARB"));
    vtkgl::WindowPos3i = reinterpret_cast<vtkgl::PFNGLWINDOWPOS3IPROC>(manager->GetProcAddress("glWindowPos3iARB"));
    vtkgl::WindowPos3iv = reinterpret_cast<vtkgl::PFNGLWINDOWPOS3IVPROC>(manager->GetProcAddress("glWindowPos3ivARB"));
    vtkgl::WindowPos3s = reinterpret_cast<vtkgl::PFNGLWINDOWPOS3SPROC>(manager->GetProcAddress("glWindowPos3sARB"));
    vtkgl::WindowPos3sv = reinterpret_cast<vtkgl::PFNGLWINDOWPOS3SVPROC>(manager->GetProcAddress("glWindowPos3svARB"));
    return 1 && (vtkgl::WindowPos2d != NULL) && (vtkgl::WindowPos2dv != NULL) && (vtkgl::WindowPos2f != NULL) && (vtkgl::WindowPos2fv != NULL) && (vtkgl::WindowPos2i != NULL) && (vtkgl::WindowPos2iv != NULL) && (vtkgl::WindowPos2s != NULL) && (vtkgl::WindowPos2sv != NULL) && (vtkgl::WindowPos3d != NULL) && (vtkgl::WindowPos3dv != NULL) && (vtkgl::WindowPos3f != NULL) && (vtkgl::WindowPos3fv != NULL) && (vtkgl::WindowPos3i != NULL) && (vtkgl::WindowPos3iv != NULL) && (vtkgl::WindowPos3s != NULL) && (vtkgl::WindowPos3sv != NULL);
    }

  // OpenGL 1.5
   
  if (strcmp(name, "GL_ARB_vertex_buffer_object") == 0)
    {
    vtkgl::BindBuffer = reinterpret_cast<vtkgl::PFNGLBINDBUFFERPROC>(manager->GetProcAddress("glBindBufferARB"));
    vtkgl::DeleteBuffers = reinterpret_cast<vtkgl::PFNGLDELETEBUFFERSPROC>(manager->GetProcAddress("glDeleteBuffersARB"));
    vtkgl::GenBuffers = reinterpret_cast<vtkgl::PFNGLGENBUFFERSPROC>(manager->GetProcAddress("glGenBuffersARB"));
    vtkgl::IsBuffer = reinterpret_cast<vtkgl::PFNGLISBUFFERPROC>(manager->GetProcAddress("glIsBufferARB"));
    vtkgl::BufferData = reinterpret_cast<vtkgl::PFNGLBUFFERDATAPROC>(manager->GetProcAddress("glBufferDataARB"));
    vtkgl::BufferSubData = reinterpret_cast<vtkgl::PFNGLBUFFERSUBDATAPROC>(manager->GetProcAddress("glBufferSubDataARB"));
    vtkgl::GetBufferSubData = reinterpret_cast<vtkgl::PFNGLGETBUFFERSUBDATAPROC>(manager->GetProcAddress("glGetBufferSubDataARB"));
    vtkgl::MapBuffer = reinterpret_cast<vtkgl::PFNGLMAPBUFFERPROC>(manager->GetProcAddress("glMapBufferARB"));
    vtkgl::UnmapBuffer = reinterpret_cast<vtkgl::PFNGLUNMAPBUFFERPROC>(manager->GetProcAddress("glUnmapBufferARB"));
    vtkgl::GetBufferParameteriv = reinterpret_cast<vtkgl::PFNGLGETBUFFERPARAMETERIVPROC>(manager->GetProcAddress("glGetBufferParameterivARB"));
    vtkgl::GetBufferPointerv = reinterpret_cast<vtkgl::PFNGLGETBUFFERPOINTERVPROC>(manager->GetProcAddress("glGetBufferPointervARB"));
    return 1 && (vtkgl::BindBuffer != NULL) && (vtkgl::DeleteBuffers != NULL) && (vtkgl::GenBuffers != NULL) && (vtkgl::IsBuffer != NULL) && (vtkgl::BufferData != NULL) && (vtkgl::BufferSubData != NULL) && (vtkgl::GetBufferSubData != NULL) && (vtkgl::MapBuffer != NULL) && (vtkgl::UnmapBuffer != NULL) && (vtkgl::GetBufferParameteriv != NULL) && (vtkgl::GetBufferPointerv != NULL);
    }
   
  if (strcmp(name, "GL_ARB_occlusion_query") == 0)
    {
    vtkgl::GenQueries = reinterpret_cast<vtkgl::PFNGLGENQUERIESPROC>(manager->GetProcAddress("glGenQueriesARB"));
    vtkgl::DeleteQueries = reinterpret_cast<vtkgl::PFNGLDELETEQUERIESPROC>(manager->GetProcAddress("glDeleteQueriesARB"));
    vtkgl::IsQuery = reinterpret_cast<vtkgl::PFNGLISQUERYPROC>(manager->GetProcAddress("glIsQueryARB"));
    vtkgl::BeginQuery = reinterpret_cast<vtkgl::PFNGLBEGINQUERYPROC>(manager->GetProcAddress("glBeginQueryARB"));
    vtkgl::EndQuery = reinterpret_cast<vtkgl::PFNGLENDQUERYPROC>(manager->GetProcAddress("glEndQueryARB"));
    vtkgl::GetQueryiv = reinterpret_cast<vtkgl::PFNGLGETQUERYIVPROC>(manager->GetProcAddress("glGetQueryivARB"));
    vtkgl::GetQueryObjectiv = reinterpret_cast<vtkgl::PFNGLGETQUERYOBJECTIVPROC>(manager->GetProcAddress("glGetQueryObjectivARB"));
    vtkgl::GetQueryObjectuiv = reinterpret_cast<vtkgl::PFNGLGETQUERYOBJECTUIVPROC>(manager->GetProcAddress("glGetQueryObjectuivARB"));
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
     
    vtkgl::DeleteProgram = reinterpret_cast<vtkgl::PFNGLDELETEPROGRAMPROC>(manager->GetProcAddress("glDeleteObjectARB"));
    vtkgl::DeleteShader = reinterpret_cast<vtkgl::PFNGLDELETESHADERPROC>(manager->GetProcAddress("glDeleteObjectARB"));
     
    // There is no translation for GetHandle in OpenGL2.0.
   
    vtkgl::IsProgram = reinterpret_cast<vtkgl::PFNGLISPROGRAMPROC>(IsProgramFromARBToPromoted);
    vtkgl::IsShader = reinterpret_cast<vtkgl::PFNGLISSHADERPROC>(IsShaderFromARBToPromoted);
     
    vtkgl::DetachShader = reinterpret_cast<vtkgl::PFNGLDETACHSHADERPROC>(manager->GetProcAddress("glDetachObjectARB"));
    vtkgl::CreateShader = reinterpret_cast<vtkgl::PFNGLCREATESHADERPROC>(manager->GetProcAddress("glCreateShaderObjectARB"));
    vtkgl::ShaderSource = reinterpret_cast<vtkgl::PFNGLSHADERSOURCEPROC>(manager->GetProcAddress("glShaderSourceARB"));
    vtkgl::CompileShader = reinterpret_cast<vtkgl::PFNGLCOMPILESHADERPROC>(manager->GetProcAddress("glCompileShaderARB"));
    vtkgl::CreateProgram = reinterpret_cast<vtkgl::PFNGLCREATEPROGRAMPROC>(manager->GetProcAddress("glCreateProgramObjectARB"));
    
    vtkgl::AttachShader = reinterpret_cast<vtkgl::PFNGLATTACHSHADERPROC>(manager->GetProcAddress("glAttachObjectARB"));
    vtkgl::LinkProgram = reinterpret_cast<vtkgl::PFNGLLINKPROGRAMPROC>(manager->GetProcAddress("glLinkProgramARB"));
    vtkgl::UseProgram = reinterpret_cast<vtkgl::PFNGLUSEPROGRAMPROC>(manager->GetProcAddress("glUseProgramObjectARB"));
    vtkgl::ValidateProgram = reinterpret_cast<vtkgl::PFNGLVALIDATEPROGRAMPROC>(manager->GetProcAddress("glValidateProgramARB"));
    vtkgl::Uniform1f = reinterpret_cast<vtkgl::PFNGLUNIFORM1FPROC>(manager->GetProcAddress("glUniform1fARB"));
    vtkgl::Uniform2f = reinterpret_cast<vtkgl::PFNGLUNIFORM2FPROC>(manager->GetProcAddress("glUniform2fARB"));
    vtkgl::Uniform3f = reinterpret_cast<vtkgl::PFNGLUNIFORM3FPROC>(manager->GetProcAddress("glUniform3fARB"));
    vtkgl::Uniform4f = reinterpret_cast<vtkgl::PFNGLUNIFORM4FPROC>(manager->GetProcAddress("glUniform4fARB"));
    vtkgl::Uniform1i = reinterpret_cast<vtkgl::PFNGLUNIFORM1IPROC>(manager->GetProcAddress("glUniform1iARB"));
    vtkgl::Uniform2i = reinterpret_cast<vtkgl::PFNGLUNIFORM2IPROC>(manager->GetProcAddress("glUniform2iARB"));
    vtkgl::Uniform3i = reinterpret_cast<vtkgl::PFNGLUNIFORM3IPROC>(manager->GetProcAddress("glUniform3iARB"));
    vtkgl::Uniform4i = reinterpret_cast<vtkgl::PFNGLUNIFORM4IPROC>(manager->GetProcAddress("glUniform4iARB"));
    vtkgl::Uniform1fv = reinterpret_cast<vtkgl::PFNGLUNIFORM1FVPROC>(manager->GetProcAddress("glUniform1fvARB"));
    vtkgl::Uniform2fv = reinterpret_cast<vtkgl::PFNGLUNIFORM2FVPROC>(manager->GetProcAddress("glUniform2fvARB"));
    vtkgl::Uniform3fv = reinterpret_cast<vtkgl::PFNGLUNIFORM3FVPROC>(manager->GetProcAddress("glUniform3fvARB"));
    vtkgl::Uniform4fv = reinterpret_cast<vtkgl::PFNGLUNIFORM4FVPROC>(manager->GetProcAddress("glUniform4fvARB"));
    vtkgl::Uniform1iv = reinterpret_cast<vtkgl::PFNGLUNIFORM1IVPROC>(manager->GetProcAddress("glUniform1ivARB"));
    vtkgl::Uniform2iv = reinterpret_cast<vtkgl::PFNGLUNIFORM2IVPROC>(manager->GetProcAddress("glUniform2ivARB"));
    vtkgl::Uniform3iv = reinterpret_cast<vtkgl::PFNGLUNIFORM3IVPROC>(manager->GetProcAddress("glUniform3ivARB"));
    vtkgl::Uniform4iv = reinterpret_cast<vtkgl::PFNGLUNIFORM4IVPROC>(manager->GetProcAddress("glUniform4ivARB"));
    vtkgl::UniformMatrix2fv = reinterpret_cast<vtkgl::PFNGLUNIFORMMATRIX2FVPROC>(manager->GetProcAddress("glUniformMatrix2fvARB"));
    vtkgl::UniformMatrix3fv = reinterpret_cast<vtkgl::PFNGLUNIFORMMATRIX3FVPROC>(manager->GetProcAddress("glUniformMatrix3fvARB"));
    vtkgl::UniformMatrix4fv = reinterpret_cast<vtkgl::PFNGLUNIFORMMATRIX4FVPROC>(manager->GetProcAddress("glUniformMatrix4fvARB"));
    
    // GetObjectParameterf* don't have translation in OpenGL2.0
   
    // GetObjectParameter* translate both to GetProgram* and GetShader*
    vtkgl::GetProgramiv = reinterpret_cast<vtkgl::PFNGLGETPROGRAMIVPROC>(manager->GetProcAddress("glGetObjectParameterivARB"));
    vtkgl::GetShaderiv = reinterpret_cast<vtkgl::PFNGLGETSHADERIVPROC>(manager->GetProcAddress("glGetObjectParameterivARB"));
    
    // glGetInfoLogARB translates both to GetProgramInfoLog and
    // GetShaderInfoLog.
    
    vtkgl::GetProgramInfoLog = reinterpret_cast<vtkgl::PFNGLGETPROGRAMINFOLOGPROC>(manager->GetProcAddress("glGetInfoLogARB"));
    vtkgl::GetShaderInfoLog = reinterpret_cast<vtkgl::PFNGLGETSHADERINFOLOGPROC>(manager->GetProcAddress("glGetInfoLogARB"));
    
    
    vtkgl::GetAttachedShaders = reinterpret_cast<vtkgl::PFNGLGETATTACHEDSHADERSPROC>(manager->GetProcAddress("glGetAttachedObjectsARB"));
    vtkgl::GetUniformLocation = reinterpret_cast<vtkgl::PFNGLGETUNIFORMLOCATIONPROC>(manager->GetProcAddress("glGetUniformLocationARB"));
    vtkgl::GetActiveUniform = reinterpret_cast<vtkgl::PFNGLGETACTIVEUNIFORMPROC>(manager->GetProcAddress("glGetActiveUniformARB"));
    vtkgl::GetUniformfv = reinterpret_cast<vtkgl::PFNGLGETUNIFORMFVPROC>(manager->GetProcAddress("glGetUniformfvARB"));
    vtkgl::GetUniformiv = reinterpret_cast<vtkgl::PFNGLGETUNIFORMIVPROC>(manager->GetProcAddress("glGetUniformivARB"));
    vtkgl::GetShaderSource = reinterpret_cast<vtkgl::PFNGLGETSHADERSOURCEPROC>(manager->GetProcAddress("glGetShaderSourceARB"));
    return 1 && (vtkgl::DeleteProgram != NULL) && (vtkgl::DeleteShader != NULL) && (vtkgl::IsProgram != NULL) && (vtkgl::IsShader != NULL) && (vtkgl::DetachShader != NULL) && (vtkgl::CreateShader != NULL) && (vtkgl::ShaderSource != NULL) && (vtkgl::CompileShader != NULL) && (vtkgl::CreateProgram != NULL) && (vtkgl::AttachShader != NULL) && (vtkgl::LinkProgram != NULL) && (vtkgl::UseProgram != NULL) && (vtkgl::ValidateProgram != NULL) && (vtkgl::Uniform1f != NULL) && (vtkgl::Uniform2f != NULL) && (vtkgl::Uniform3f != NULL) && (vtkgl::Uniform4f != NULL) && (vtkgl::Uniform1i != NULL) && (vtkgl::Uniform2i != NULL) && (vtkgl::Uniform3i != NULL) && (vtkgl::Uniform4i != NULL) && (vtkgl::Uniform1fv != NULL) && (vtkgl::Uniform2fv != NULL) && (vtkgl::Uniform3fv != NULL) && (vtkgl::Uniform4fv != NULL) && (vtkgl::Uniform1iv != NULL) && (vtkgl::Uniform2iv != NULL) && (vtkgl::Uniform3iv != NULL) && (vtkgl::Uniform4iv != NULL) && (vtkgl::UniformMatrix2fv != NULL) && (vtkgl::UniformMatrix3fv != NULL) && (vtkgl::UniformMatrix4fv != NULL) && (vtkgl::GetProgramiv != NULL) && (vtkgl::GetShaderiv != NULL) && (vtkgl::GetProgramInfoLog != NULL) && (vtkgl::GetShaderInfoLog != NULL) && (vtkgl::GetAttachedShaders != NULL) && (vtkgl::GetUniformLocation != NULL) && (vtkgl::GetActiveUniform != NULL) && (vtkgl::GetUniformfv != NULL) && (vtkgl::GetUniformiv != NULL) && (vtkgl::GetShaderSource != NULL);
    }

  if (strcmp(name, "GL_ARB_vertex_shader") == 0)
    {
    vtkgl::BindAttribLocation = reinterpret_cast<vtkgl::PFNGLBINDATTRIBLOCATIONPROC>(manager->GetProcAddress("glBindAttribLocationARB"));
    vtkgl::GetActiveAttrib = reinterpret_cast<vtkgl::PFNGLGETACTIVEATTRIBPROC>(manager->GetProcAddress("glGetActiveAttribARB"));
    vtkgl::GetAttribLocation = reinterpret_cast<vtkgl::PFNGLGETATTRIBLOCATIONPROC>(manager->GetProcAddress("glGetAttribLocationARB"));
    
    // Defined both by GL_ARB_vertex_shader and GL_ARB_vertex_program
    vtkgl::DisableVertexAttribArray = reinterpret_cast<vtkgl::PFNGLDISABLEVERTEXATTRIBARRAYPROC>(manager->GetProcAddress("glDisableVertexAttribArrayARB"));
    vtkgl::EnableVertexAttribArray = reinterpret_cast<vtkgl::PFNGLENABLEVERTEXATTRIBARRAYPROC>(manager->GetProcAddress("glEnableVertexAttribArrayARB"));
    
    vtkgl::GetVertexAttribdv = reinterpret_cast<vtkgl::PFNGLGETVERTEXATTRIBDVPROC>(manager->GetProcAddress("glGetVertexAttribdvARB"));
    vtkgl::GetVertexAttribfv = reinterpret_cast<vtkgl::PFNGLGETVERTEXATTRIBFVPROC>(manager->GetProcAddress("glGetVertexAttribfvARB"));
    vtkgl::GetVertexAttribiv = reinterpret_cast<vtkgl::PFNGLGETVERTEXATTRIBIVPROC>(manager->GetProcAddress("glGetVertexAttribivARB"));
    vtkgl::GetVertexAttribPointerv = reinterpret_cast<vtkgl::PFNGLGETVERTEXATTRIBPOINTERVPROC>(manager->GetProcAddress("glGetVertexAttribPointervARB"));
    
    vtkgl::VertexAttrib1d = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB1DPROC>(manager->GetProcAddress("glVertexAttrib1dARB"));
    vtkgl::VertexAttrib1dv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB1DVPROC>(manager->GetProcAddress("glVertexAttrib1dvARB"));
    vtkgl::VertexAttrib1f = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB1FPROC>(manager->GetProcAddress("glVertexAttrib1fARB"));
    vtkgl::VertexAttrib1fv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB1FVPROC>(manager->GetProcAddress("glVertexAttrib1fvARB"));
    vtkgl::VertexAttrib1s = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB1SPROC>(manager->GetProcAddress("glVertexAttrib1sARB"));
    vtkgl::VertexAttrib1sv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB1SVPROC>(manager->GetProcAddress("glVertexAttrib1svARB"));
    vtkgl::VertexAttrib2d = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB2DPROC>(manager->GetProcAddress("glVertexAttrib2dARB"));
    vtkgl::VertexAttrib2dv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB2DVPROC>(manager->GetProcAddress("glVertexAttrib2dvARB"));
    vtkgl::VertexAttrib2f = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB2FPROC>(manager->GetProcAddress("glVertexAttrib2fARB"));
    vtkgl::VertexAttrib2fv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB2FVPROC>(manager->GetProcAddress("glVertexAttrib2fvARB"));
    vtkgl::VertexAttrib2s = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB2SPROC>(manager->GetProcAddress("glVertexAttrib2sARB"));
    vtkgl::VertexAttrib2sv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB2SVPROC>(manager->GetProcAddress("glVertexAttrib2svARB"));
    vtkgl::VertexAttrib3d = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB3DPROC>(manager->GetProcAddress("glVertexAttrib3dARB"));
    vtkgl::VertexAttrib3dv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB3DVPROC>(manager->GetProcAddress("glVertexAttrib3dvARB"));
    vtkgl::VertexAttrib3f = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB3FPROC>(manager->GetProcAddress("glVertexAttrib3fARB"));
    vtkgl::VertexAttrib3fv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB3FVPROC>(manager->GetProcAddress("glVertexAttrib3fvARB"));
    vtkgl::VertexAttrib3s = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB3SPROC>(manager->GetProcAddress("glVertexAttrib3sARB"));
    vtkgl::VertexAttrib3sv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB3SVPROC>(manager->GetProcAddress("glVertexAttrib3svARB"));
    vtkgl::VertexAttrib4Nbv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4NBVPROC>(manager->GetProcAddress("glVertexAttrib4NbvARB"));
    vtkgl::VertexAttrib4Niv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4NIVPROC>(manager->GetProcAddress("glVertexAttrib4NivARB"));
    vtkgl::VertexAttrib4Nsv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4NSVPROC>(manager->GetProcAddress("glVertexAttrib4NsvARB"));
    vtkgl::VertexAttrib4Nub = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4NUBPROC>(manager->GetProcAddress("glVertexAttrib4NubARB"));
    vtkgl::VertexAttrib4Nubv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4NUBVPROC>(manager->GetProcAddress("glVertexAttrib4NubvARB"));
    vtkgl::VertexAttrib4Nuiv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4NUIVPROC>(manager->GetProcAddress("glVertexAttrib4NuivARB"));
    vtkgl::VertexAttrib4Nusv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4NUSVPROC>(manager->GetProcAddress("glVertexAttrib4NusvARB"));
    vtkgl::VertexAttrib4bv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4BVPROC>(manager->GetProcAddress("glVertexAttrib4bvARB"));
    vtkgl::VertexAttrib4d = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4DPROC>(manager->GetProcAddress("glVertexAttrib4dARB"));
    vtkgl::VertexAttrib4dv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4DVPROC>(manager->GetProcAddress("glVertexAttrib4dvARB"));
    vtkgl::VertexAttrib4f = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4FPROC>(manager->GetProcAddress("glVertexAttrib4fARB"));
    vtkgl::VertexAttrib4fv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4FVPROC>(manager->GetProcAddress("glVertexAttrib4fvARB"));
    vtkgl::VertexAttrib4iv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4IVPROC>(manager->GetProcAddress("glVertexAttrib4ivARB"));
    vtkgl::VertexAttrib4s = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4SPROC>(manager->GetProcAddress("glVertexAttrib4sARB"));
    vtkgl::VertexAttrib4sv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4SVPROC>(manager->GetProcAddress("glVertexAttrib4svARB"));
    vtkgl::VertexAttrib4ubv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4UBVPROC>(manager->GetProcAddress("glVertexAttrib4ubvARB"));
    vtkgl::VertexAttrib4uiv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4UIVPROC>(manager->GetProcAddress("glVertexAttrib4uivARB"));
    vtkgl::VertexAttrib4usv = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIB4USVPROC>(manager->GetProcAddress("glVertexAttrib4usvARB"));
    vtkgl::VertexAttribPointer = reinterpret_cast<vtkgl::PFNGLVERTEXATTRIBPOINTERPROC>(manager->GetProcAddress("glVertexAttribPointerARB"));
    
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
    vtkgl::DrawBuffers = reinterpret_cast<vtkgl::PFNGLDRAWBUFFERSPROC>(manager->GetProcAddress("glDrawBuffersARB"));
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
    vtkgl::BlendEquationSeparate = reinterpret_cast<vtkgl::PFNGLBLENDEQUATIONSEPARATEPROC>(manager->GetProcAddress("glBlendEquationSeparateEXT"));
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
    vtkgl::StencilOpSeparate = reinterpret_cast<vtkgl::PFNGLSTENCILOPSEPARATEPROC>(manager->GetProcAddress("glStencilOpSeparateATI"));
    vtkgl::StencilFuncSeparate = reinterpret_cast<vtkgl::PFNGLSTENCILFUNCSEPARATEPROC>(manager->GetProcAddress("glStencilFuncSeparateATI"));
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

// ----------------------------------------------------------------------------
int vtkgl::LoadAsARBExtension(const char *name,
                              vtkOpenGLExtensionManager *manager)
{
  assert("pre: name_exists" && name!=NULL);
  assert("pre: manager_exists" && manager!=NULL);

  if (strcmp(name, "GL_EXT_geometry_shader4") == 0)
    {
    vtkgl::ProgramParameteriARB = reinterpret_cast<vtkgl::PFNGLPROGRAMPARAMETERIARBPROC>(manager->GetProcAddress("glProgramParameteriEXT"));

    // FramebufferTextureEXT(), FramebufferTextureLayerEXT() and
    // FramebufferTextureFaceEXT() are also define by extension
    // GL_NV_geometry_program4. Weird. Spec mistake.

    vtkgl::FramebufferTextureARB = reinterpret_cast<vtkgl::PFNGLFRAMEBUFFERTEXTUREARBPROC>(manager->GetProcAddress("glFramebufferTextureEXT"));

    vtkgl::FramebufferTextureLayerARB = reinterpret_cast<vtkgl::PFNGLFRAMEBUFFERTEXTURELAYERARBPROC>(manager->GetProcAddress("glFramebufferTextureLayerEXT"));

    vtkgl::FramebufferTextureFaceARB = reinterpret_cast<vtkgl::PFNGLFRAMEBUFFERTEXTUREFACEARBPROC>(manager->GetProcAddress("glFramebufferTextureFaceEXT"));

    return 1;
    }
  return 0;
}
