/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPicker.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkAbstractPicker.h"
#include "vtkObjectFactory.h"


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
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->StartPickTag);
  this->StartPickTag = this->AddObserver(vtkCommand::StartPickEvent,cbc);
}

// Specify function to be called when something is picked.
void vtkAbstractPicker::SetPickMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->StartPickTag);
  this->StartPickTag = this->AddObserver(vtkCommand::PickEvent,cbc);
}

// Specify function to be called after all picking operations have been
// performed.
void vtkAbstractPicker::SetEndPickMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->StartPickTag);
  this->StartPickTag = this->AddObserver(vtkCommand::EndPickEvent,cbc);
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
  this->vtkObject::PrintSelf(os,indent);

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
