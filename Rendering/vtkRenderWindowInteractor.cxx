/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkRenderWindowInteractor.h"
#include "vtkPropPicker.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkGraphicsFactory.h"
#include "vtkMath.h"
#include "vtkCommand.h"


// Construct object so that light follows camera motion.
vtkRenderWindowInteractor::vtkRenderWindowInteractor()
{
  this->RenderWindow    = NULL;
  this->InteractorStyle = NULL;
  this->SetInteractorStyle(vtkInteractorStyleSwitch::New()); 
  this->InteractorStyle->Delete();
  
  this->LightFollowCamera = 1;
  this->Initialized = 0;
  this->Enabled = 0;
  this->DesiredUpdateRate = 15;
  // default limit is 3 hours per frame
  this->StillUpdateRate = 0.0001;
  
  this->Picker = this->CreateDefaultPicker();
  this->Picker->Register(this);
  this->Picker->Delete();

  this->StartPickTag = 0;
  this->EndPickTag = 0;
  this->UserTag = 0;
  this->ExitTag = 0;

  this->EventPosition[0] = 0;
  this->EventPosition[1] = 0;

  this->Size[0] = 0;
  this->Size[1] = 0;
  
  this->NumberOfFlyFrames = 15;
  this->Dolly = 0.30;
}

vtkRenderWindowInteractor::~vtkRenderWindowInteractor()
{
  if (this->InteractorStyle != NULL)
    {
    this->InteractorStyle->UnRegister(this);
    }
  if ( this->Picker)
    {
    this->Picker->UnRegister(this);
    }
}

vtkRenderWindowInteractor *vtkRenderWindowInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkGraphicsFactory::CreateInstance("vtkRenderWindowInteractor");
  return (vtkRenderWindowInteractor *)ret;
}

void vtkRenderWindowInteractor::Render()
{
  if (this->RenderWindow)
    {
    this->RenderWindow->Render();
    }
}

// treat renderWindow and interactor as one object.
// it might be easier if the GetReference count method were redefined.
void vtkRenderWindowInteractor::UnRegister(vtkObject *o)
{
  if (this->RenderWindow && this->RenderWindow->GetInteractor() == this &&
      this->RenderWindow != o)
    {
    if (this->GetReferenceCount()+this->RenderWindow->GetReferenceCount() == 3)
      {
      this->RenderWindow->SetInteractor(NULL);
      this->SetRenderWindow(NULL);
      }
    }

  this->vtkObject::UnRegister(o);
}

void vtkRenderWindowInteractor::SetRenderWindow(vtkRenderWindow *aren)
{
  if (this->RenderWindow != aren)
    {
    // to avoid destructor recursion
    vtkRenderWindow *temp = this->RenderWindow;
    this->RenderWindow = aren;
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    if (this->RenderWindow != NULL)
      {
      this->RenderWindow->Register(this);
      if (this->RenderWindow->GetInteractor() != this)
	{
	this->RenderWindow->SetInteractor(this);
	}
      }
    }
}

void vtkRenderWindowInteractor::SetInteractorStyle(vtkInteractorStyle *aren)
{
  if (this->InteractorStyle != aren)
    {
    // to avoid destructor recursion
    vtkInteractorStyle *temp = this->InteractorStyle;
    this->InteractorStyle = aren;
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    if (this->InteractorStyle != NULL)
      {
      this->InteractorStyle->Register(this);
      if (this->InteractorStyle->GetInteractor() != this)
	{
	this->InteractorStyle->SetInteractor(this);
	}
      }
    }
}

void vtkRenderWindowInteractor::UpdateSize(int x,int y) 
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0])||(y != this->Size[1]))
    {
    this->Size[0] = x;
    this->Size[1] = y;
    this->RenderWindow->SetSize(x,y);
    }
}

// Specify a method to be executed prior to the pick operation.
void vtkRenderWindowInteractor::SetStartPickMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->StartPickTag);
  this->StartPickTag = this->AddObserver(vtkCommand::StartPickEvent,cbc);
}

// Specify a method to be executed after the pick operation.
void vtkRenderWindowInteractor::SetEndPickMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->EndPickTag);
  this->EndPickTag = this->AddObserver(vtkCommand::EndPickEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetStartPickMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->StartPickTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetEndPickMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->EndPickTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Creates an instance of vtkPropPicker by default
vtkAbstractPropPicker *vtkRenderWindowInteractor::CreateDefaultPicker()
{
  return vtkPropPicker::New();
}

// Set the user method. This method is invoked on a <u> keypress.
void vtkRenderWindowInteractor::SetUserMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->UserTag);
  this->UserTag = this->AddObserver(vtkCommand::UserEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetUserMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->UserTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set the exit method. This method is invoked on a <e> keypress.
void vtkRenderWindowInteractor::SetExitMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->ExitTag);
  this->ExitTag = this->AddObserver(vtkCommand::ExitEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetExitMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->ExitTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

void vtkRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
    {
    this->InvokeEvent(vtkCommand::ExitEvent,NULL);
    }
  else
    {
    this->TerminateApp();
    }
}

void vtkRenderWindowInteractor::UserCallback()
{
  this->InvokeEvent(vtkCommand::UserEvent,NULL);
}

void vtkRenderWindowInteractor::StartPickCallback()
{
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);
}

void vtkRenderWindowInteractor::EndPickCallback()
{
  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);
}

void vtkRenderWindowInteractor::FlyTo(vtkRenderer *ren, float x, float y, float z)
{
  float flyFrom[3], flyTo[3];
  float d[3], focalPt[3];
  int i, j;

  flyTo[0]=x; flyTo[1]=y; flyTo[2]=z;
  ren->GetActiveCamera()->GetFocalPoint(flyFrom);
  for (i=0; i<3; i++)
    {
    d[i] = flyTo[i] - flyFrom[i];
    }
  float distance = vtkMath::Normalize(d);
  float delta = distance/this->NumberOfFlyFrames;
  
  for (i=1; i<=NumberOfFlyFrames; i++)
    {
    for (j=0; j<3; j++)
      {
      focalPt[j] = flyFrom[j] + d[j]*i*delta;
      }
    ren->GetActiveCamera()->SetFocalPoint(focalPt);
    ren->GetActiveCamera()->Dolly(this->Dolly/this->NumberOfFlyFrames + 1.0);
    ren->ResetCameraClippingRange();
    this->RenderWindow->Render();
    }
}

void vtkRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "InteractorStyle:    " << this->InteractorStyle << "\n";
  os << indent << "RenderWindow:    " << this->RenderWindow << "\n";
  if ( this->Picker )
    {
    os << indent << "Picker: " << this->Picker << "\n";
    }
  else
    {
    os << indent << "Picker: (none)\n";
    }
  os << indent << "LightFollowCamera: " << (this->LightFollowCamera ? "On\n" : "Off\n");
  os << indent << "DesiredUpdateRate: " << this->DesiredUpdateRate << "\n";
  os << indent << "StillUpdateRate: " << this->StillUpdateRate << "\n";
  os << indent << "Initialized: " << this->Initialized << "\n";
  os << indent << "Enabled: " << this->Enabled << "\n";
  os << indent << "EventPosition: " << "( " << this->EventPosition[0] <<
    ", " << this->EventPosition[1] << " )\n";
  os << indent << "Viewport Size: " << "( " << this->Size[0] <<
    ", " << this->Size[1] << " )\n";
  os << indent << "Number of Fly Frames: " << this->NumberOfFlyFrames <<"\n";
  os << indent << "Dolly: " << this->Dolly <<"\n";

}











