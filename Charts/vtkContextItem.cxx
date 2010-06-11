/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextItem.h"

// Get my new commands
#include "vtkCommand.h"

#include "vtkInteractorStyle.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkContextItem::vtkContextItem()
{
  this->Scene = NULL;
  this->Visible = true;
  this->Opacity = 1.0;
}

//-----------------------------------------------------------------------------
vtkContextItem::~vtkContextItem()
{
  this->SetScene(NULL);
}

//-----------------------------------------------------------------------------
void vtkContextItem::SetScene(vtkContextScene *scene)
{
  // Cannot have a reference counted pointer to the scene as this causes a
  // reference loop, where the scene and the item never get to a reference
  // count of zero.
  this->Scene = scene;
}

vtkContextScene* vtkContextItem::GetScene()
{
  // Return the underlying pointer
  return this->Scene.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
