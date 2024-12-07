// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkObjectFactory.h"

#include "vtkDebugLeaks.h"
#include "vtkGraphicsFactory.h"

#include <cstdlib>
#include <mutex>

VTK_ABI_NAMESPACE_BEGIN
static std::mutex vtkUseMesaClassesCriticalSection;
static std::mutex vtkOffScreenOnlyModeCriticalSection;
int vtkGraphicsFactory::UseMesaClasses = 0;

#ifdef VTK_USE_OFFSCREEN
int vtkGraphicsFactory::OffScreenOnlyMode = 1;
#else
int vtkGraphicsFactory::OffScreenOnlyMode = 0;
#endif

vtkStandardNewMacro(vtkGraphicsFactory);

const char* vtkGraphicsFactory::GetRenderLibrary()
{
  const char* temp;

  // first check the environment variable
  temp = getenv("VTK_RENDERER");

  // Backward compatibility
  if (temp)
  {
    if (!strcmp("oglr", temp))
    {
      temp = "OpenGL";
    }
    else if (!strcmp("woglr", temp))
    {
      temp = "Win32OpenGL";
    }
    else if (strcmp("OpenGL", temp) != 0 && strcmp("Win32OpenGL", temp) != 0)
    {
      vtkGenericWarningMacro(<< "VTK_RENDERER set to unsupported type:" << temp);
      temp = nullptr;
    }
  }

  // if nothing is set then work down the list of possible renderers
  if (!temp)
  {
#if defined(VTK_DISPLAY_X11_OGL)
    temp = "OpenGL";
#endif
#ifdef VTK_DISPLAY_WIN32_OGL
    temp = "Win32OpenGL";
#endif
#ifdef VTK_DISPLAY_COCOA
    temp = "CocoaOpenGL";
#endif
  }

  return temp;
}

vtkObject* vtkGraphicsFactory::CreateInstance(const char* vtkclassname)
{
  // first check the object factory
  vtkObject* ret = vtkObjectFactory::CreateInstance(vtkclassname);
  if (ret)
  {
    return ret;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkGraphicsFactory::SetUseMesaClasses(int use)
{
  vtkUseMesaClassesCriticalSection.lock();
  vtkGraphicsFactory::UseMesaClasses = use;
  vtkUseMesaClassesCriticalSection.unlock();
}

//------------------------------------------------------------------------------
int vtkGraphicsFactory::GetUseMesaClasses()
{
  return vtkGraphicsFactory::UseMesaClasses;
}

//------------------------------------------------------------------------------
void vtkGraphicsFactory::SetOffScreenOnlyMode(int use)
{
  vtkOffScreenOnlyModeCriticalSection.lock();
  vtkGraphicsFactory::OffScreenOnlyMode = use;
  vtkOffScreenOnlyModeCriticalSection.unlock();
}

//------------------------------------------------------------------------------
int vtkGraphicsFactory::GetOffScreenOnlyMode()
{
  return vtkGraphicsFactory::OffScreenOnlyMode;
}

//------------------------------------------------------------------------------
void vtkGraphicsFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
