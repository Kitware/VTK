/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareObjectFactory.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKitwareObjectFactory.h"
#include "vtkKitwareContourFilter.h"
#include "vtkVersion.h"

vtkCxxRevisionMacro(vtkKitwareObjectFactory, "1.8");

void vtkKitwareObjectFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Kitware object factory" << endl;
}


vtkObject* vtkKitwareObjectFactory::CreateObject(const char* vtkclassname )
{
  // This should be a hash table, but for now use strcmp
  if(strcmp(vtkclassname, "vtkContourFilter") == 0)
    {
    return vtkKitwareContourFilter::New();
    }
  return 0;
}

const char* vtkKitwareObjectFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}


extern "C" vtkObjectFactory* vtkLoad()
{
  return vtkKitwareObjectFactory::New();
}
