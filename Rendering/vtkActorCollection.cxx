/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActorCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActorCollection.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkCxxRevisionMacro(vtkActorCollection, "1.11");
vtkStandardNewMacro(vtkActorCollection);

void vtkActorCollection::ApplyProperties(vtkProperty *p)
{
  vtkActor *actor;
  
  if ( p == NULL )
    {
    return;
    }
  
  for ( this->InitTraversal(); (actor=this->GetNextActor()); )
    {
    actor->GetProperty()->DeepCopy(p);
    }
}

//----------------------------------------------------------------------------
void vtkActorCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
