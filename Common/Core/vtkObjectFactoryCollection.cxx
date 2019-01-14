/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectFactoryCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectFactoryCollection.h"

#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"

vtkObjectFactoryCollection* vtkObjectFactoryCollection::New()
{
  // Don't use the object factory macros. Creating an object factory here
  // will cause an infinite loop.
  vtkObjectFactoryCollection *ret = new vtkObjectFactoryCollection;
  ret->InitializeObjectBase();
  return ret;
}
