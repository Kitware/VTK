/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPicker.cxx
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
#include "vtkAbstractPicker.h"
#include "vtkObjectFactory.h"
#include "vtkOldStyleCallbackCommand.h"

vtkCxxRevisionMacro(vtkAbstractPicker, "1.8");

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

  this->StartPickTag = 0;
  this->PickTag = 0;
  this->EndPickTag = 0;

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

// Specify function to be called as picking operation begins.
void vtkAbstractPicker::SetStartPickMethod(void (*f)(void *), void *arg)
{
  if ( this->StartPickTag )
    {
    this->RemoveObserver(this->StartPickTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->StartPickTag = this->AddObserver(vtkCommand::StartPickEvent,cbc);
    cbc->Delete();
    }
}

// Specify function to be called when something is picked.
void vtkAbstractPicker::SetPickMethod(void (*f)(void *), void *arg)
{
  if ( this->StartPickTag )
    {
    this->RemoveObserver(this->StartPickTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->StartPickTag = this->AddObserver(vtkCommand::PickEvent,cbc);
    cbc->Delete();
    }
}

// Specify function to be called after all picking operations have been
// performed.
void vtkAbstractPicker::SetEndPickMethod(void (*f)(void *), void *arg)
{
  if ( this->StartPickTag )
    {
    this->RemoveObserver(this->StartPickTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->StartPickTag = this->AddObserver(vtkCommand::EndPickEvent,cbc);
    cbc->Delete();
    }
}


// Set a method to delete user arguments for StartPickMethod.
void vtkAbstractPicker::SetStartPickMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->StartPickTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set a method to delete user arguments for PickMethod.
void vtkAbstractPicker::SetPickMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->PickTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set a method to delete user arguments for EndPickMethod.
void vtkAbstractPicker::SetEndPickMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->EndPickTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
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
