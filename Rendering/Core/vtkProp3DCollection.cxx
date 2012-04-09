/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProp3DCollection.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkProp3DCollection);

//----------------------------------------------------------------------------
void vtkProp3DCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
