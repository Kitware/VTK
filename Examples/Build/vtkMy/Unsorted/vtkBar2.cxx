/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkBar2.cxx
Language:  C++
Date:      $Date$
Version:   $Revision$

=========================================================================*/
#include "vtkBar2.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkBar2* vtkBar2::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBar2");
  if(ret)
    {
    return (vtkBar2*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBar2;
}
