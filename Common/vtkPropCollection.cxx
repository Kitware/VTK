/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPropCollection.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPropCollection, "1.10");
vtkStandardNewMacro(vtkPropCollection);

int vtkPropCollection::GetNumberOfPaths()
{
  int numPaths=0;
  vtkProp *aProp;

  for ( this->InitTraversal(); (aProp=this->GetNextProp()); )
    {
    numPaths += aProp->GetNumberOfPaths();
    }
  return numPaths;
}
