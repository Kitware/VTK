/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropCollection.cxx
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
#include "vtkPropCollection.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPropCollection, "1.9");
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
