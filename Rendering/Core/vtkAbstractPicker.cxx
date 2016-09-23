/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractPicker.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"


// Construct object with initial tolerance of 1/40th of window. There are no
// pick methods and picking is performed from the renderer's actors.
vtkAbstractPicker::vtkAbstractPicker()
{
  this->Renderer = NULL;

  this->SelectionPoint[0] = 0.0;
  this->SelectionPoint[1] = 0.0;
  this->SelectionPoint[2] = 0.0;

  this->PickPosition[0] = 0.0;
  this->PickPosition[1] = 0.0;
  this->PickPosition[2] = 0.0;

  this->PickFromList = 0;
  this->PickList = vtkPropCollection::New();
}

vtkAbstractPicker::~vtkAbstractPicker()
{
  this->PickList->Delete();
}

// Initialize the picking process.
void vtkAbstractPicker::Initialize()
{
  this->Renderer = NULL;

  this->SelectionPoint[0] = 0.0;
  this->SelectionPoint[1] = 0.0;
  this->SelectionPoint[2] = 0.0;

  this->PickPosition[0] = 0.0;
  this->PickPosition[1] = 0.0;
  this->PickPosition[2] = 0.0;
}

// Initialize list of actors in pick list.
void vtkAbstractPicker::InitializePickList()
{
  this->Modified();
  this->PickList->RemoveAllItems();
}

// Add an actor to the pick list.
void vtkAbstractPicker::AddPickList(vtkProp *a)
{
  this->Modified();
  this->PickList->AddItem(a);
}

// Delete an actor from the pick list.
void vtkAbstractPicker::DeletePickList(vtkProp *a)
{
  this->Modified();
  this->PickList->RemoveItem(a);
}

void vtkAbstractPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->PickFromList )
  {
    os << indent << "Picking from list\n";
  }
  else
  {
    os << indent << "Picking from renderer's prop list\n";
  }

  os << indent << "Renderer: " << this->Renderer << "\n";

  os << indent << "Selection Point: (" <<  this->SelectionPoint[0] << ","
     << this->SelectionPoint[1] << ","
     << this->SelectionPoint[2] << ")\n";

  os << indent << "Pick Position: (" <<  this->PickPosition[0] << ","
     << this->PickPosition[1] << ","
     << this->PickPosition[2] << ")\n";
}
