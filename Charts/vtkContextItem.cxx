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
  this->Opacity = 1.0;
}

//-----------------------------------------------------------------------------
vtkContextItem::~vtkContextItem()
{
}

//-----------------------------------------------------------------------------
void vtkContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
