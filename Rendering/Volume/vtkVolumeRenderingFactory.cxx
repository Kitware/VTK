/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRenderingFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectFactory.h"

#include "vtkVolumeRenderingFactory.h"
#include "vtkToolkits.h"
#include "vtkDebugLeaks.h"

vtkStandardNewMacro(vtkVolumeRenderingFactory);


vtkObject* vtkVolumeRenderingFactory::CreateInstance(const char* vtkclassname )
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
void vtkVolumeRenderingFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
