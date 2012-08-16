/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphicsFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectFactory.h"

#include "vtkGraphicsFactory.h"
#include "vtkToolkits.h"
#include "vtkDebugLeaks.h"


#include "vtkCriticalSection.h"

#include "stdlib.h"

static vtkSimpleCriticalSection vtkUseMesaClassesCriticalSection;
static vtkSimpleCriticalSection vtkOffScreenOnlyModeCriticalSection;
int vtkGraphicsFactory::UseMesaClasses = 0;

#ifdef VTK_USE_OFFSCREEN
int vtkGraphicsFactory::OffScreenOnlyMode = 1;
#else
int vtkGraphicsFactory::OffScreenOnlyMode = 0;
#endif

vtkStandardNewMacro(vtkGraphicsFactory);

const char *vtkGraphicsFactory::GetRenderLibrary()
{
  const char *temp;

  // first check the environment variable
  temp = getenv("VTK_RENDERER");

  // Backward compatibility
  if ( temp )
    {
    if (!strcmp("oglr",temp))
      {
      temp = "OpenGL";
      }
    else if (!strcmp("woglr",temp))
      {
      temp = "Win32OpenGL";
      }
    else if (strcmp("OpenGL",temp) &&
             strcmp("Win32OpenGL",temp))
      {
      vtkGenericWarningMacro(<<"VTK_RENDERER set to unsupported type:" << temp);
      temp = NULL;
      }
    }

  // if nothing is set then work down the list of possible renderers
  if ( !temp )
    {
#if defined(VTK_DISPLAY_X11_OGL) || defined(VTK_USE_OSMESA)
    temp = "OpenGL";
#endif
#ifdef VTK_DISPLAY_WIN32_OGL
    temp = "Win32OpenGL";
#endif
#ifdef VTK_DISPLAY_CARBON
    temp = "CarbonOpenGL";
#endif
#ifdef VTK_DISPLAY_COCOA
    temp = "CocoaOpenGL";
#endif
    }

  return temp;
}

vtkObject* vtkGraphicsFactory::CreateInstance(const char* vtkclassname )
{
  // first check the object factory
  vtkObject *ret = vtkObjectFactory::CreateInstance(vtkclassname);
  if (ret)
    {
    return ret;
    }
  // if the factory failed to create the object,
  // then destroy it now, as vtkDebugLeaks::ConstructClass was called
  // with vtkclassname, and not the real name of the class
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::DestructClass(vtkclassname);
#endif
  return 0;
}

//----------------------------------------------------------------------------
void vtkGraphicsFactory::SetUseMesaClasses(int use)
{
  vtkUseMesaClassesCriticalSection.Lock();
  vtkGraphicsFactory::UseMesaClasses = use;
  vtkUseMesaClassesCriticalSection.Unlock();
}

//----------------------------------------------------------------------------
int vtkGraphicsFactory::GetUseMesaClasses()
{
  return vtkGraphicsFactory::UseMesaClasses;
}

//----------------------------------------------------------------------------
void vtkGraphicsFactory::SetOffScreenOnlyMode(int use)
{
  vtkOffScreenOnlyModeCriticalSection.Lock();
  vtkGraphicsFactory::OffScreenOnlyMode = use;
  vtkOffScreenOnlyModeCriticalSection.Unlock();
}

//----------------------------------------------------------------------------
int vtkGraphicsFactory::GetOffScreenOnlyMode()
{
  return vtkGraphicsFactory::OffScreenOnlyMode;
}

//----------------------------------------------------------------------------
void vtkGraphicsFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
