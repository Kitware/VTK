/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.cxx
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
#include "vtkRenderWindowInteractor.h"
#include "vtkPropPicker.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkGraphicsFactory.h"
#include "vtkMath.h"
#include "vtkOldStyleCallbackCommand.h"

vtkCxxRevisionMacro(vtkRenderWindowInteractor, "1.90");

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

  this->EventSize[0] = 0;
  this->EventSize[1] = 0;

  this->Size[0] = 0;
  this->Size[1] = 0;
  
  this->NumberOfFlyFrames = 15;
  this->Dolly = 0.30;
  
  this->ControlKey = 0;
  this->ShiftKey = 0;
  this->KeyCode = 0;
  this->RepeatCount = 0;
  this->KeySym = 0;
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
  if ( this->KeySym )
    {
    delete [] this->KeySym;
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

void vtkRenderWindowInteractor::SetInteractorStyle(vtkInteractorObserver *style)
{
  if (this->InteractorStyle != style)
    {
    // to avoid destructor recursion
    vtkInteractorObserver *temp = this->InteractorStyle;
    this->InteractorStyle = style;
    if (temp != NULL)
      {
      temp->SetInteractor(0);
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
void vtkRenderWindowInteractor::SetStartPickMethod(void (*f)(void *), 
                                                   void *arg)
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

// Specify a method to be executed after the pick operation.
void vtkRenderWindowInteractor::SetEndPickMethod(void (*f)(void *), void *arg)
{
  if ( this->EndPickTag )
    {
    this->RemoveObserver(this->EndPickTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->EndPickTag = this->AddObserver(vtkCommand::EndPickEvent,cbc);
    cbc->Delete();
    }
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
  if ( this->UserTag )
    {
    this->RemoveObserver(this->UserTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->UserTag = this->AddObserver(vtkCommand::UserEvent,cbc);
    cbc->Delete();
    }
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
  if ( this->ExitTag )
    {
    this->RemoveObserver(this->ExitTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->ExitTag = this->AddObserver(vtkCommand::ExitEvent,cbc);
    cbc->Delete();
    }
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

void vtkRenderWindowInteractor::FlyToImage(vtkRenderer *ren, float x, float y)
{
  float flyFrom[3], flyTo[3];
  float d[3], focalPt[3], position[3], positionFrom[3];
  int i, j;

  flyTo[0]=x; flyTo[1]=y;
  ren->GetActiveCamera()->GetFocalPoint(flyFrom);  flyTo[2] = flyFrom[2];
  ren->GetActiveCamera()->GetPosition(positionFrom);
  for (i=0; i<2; i++)
    {
    d[i] = flyTo[i] - flyFrom[i];
    }
  d[2] = 0.0;
  float distance = vtkMath::Normalize(d);
  float delta = distance/this->NumberOfFlyFrames;
  
  for (i=1; i<=NumberOfFlyFrames; i++)
    {
    for (j=0; j<3; j++)
      {
      focalPt[j] = flyFrom[j] + d[j]*i*delta;
      position[j] = positionFrom[j] + d[j]*i*delta;
      }
    ren->GetActiveCamera()->SetFocalPoint(focalPt);
    ren->GetActiveCamera()->SetPosition(position);
    ren->GetActiveCamera()->Dolly(this->Dolly/this->NumberOfFlyFrames + 1.0);
    ren->ResetCameraClippingRange();
    this->RenderWindow->Render();
    }
}

//----------------------------------------------------------------------------
vtkRenderer* vtkRenderWindowInteractor::FindPokedRenderer(int x,int y) 
{
  vtkRendererCollection *rc;
  vtkRenderer *aren;
  vtkRenderer *currentRenderer=NULL, *interactiveren=NULL, *viewportren=NULL;
  int numRens, i;

  rc = this->RenderWindow->GetRenderers();
  numRens = rc->GetNumberOfItems();
  
  for (i = numRens -1; (i >= 0) && !currentRenderer; i--) 
    {
    aren = (vtkRenderer *)rc->GetItemAsObject(i);
    if (aren->IsInViewport(x,y) && aren->GetInteractive()) 
      {
      currentRenderer = aren;
      }

    if (interactiveren == NULL && aren->GetInteractive())
      {
      // Save this renderer in case we can't find one in the viewport that
      // is interactive.
      interactiveren = aren;
      }
    if (viewportren == NULL && aren->IsInViewport(x, y))
      {
      // Save this renderer in case we can't find one in the viewport that 
      // is interactive.
      viewportren = aren;
      }
    }//for all renderers
  
  // We must have a value.  If we found an interactive renderer before, that's
  // better than a non-interactive renderer.
  if ( currentRenderer == NULL )
    {
    currentRenderer = interactiveren;
    }
  
  // We must have a value.  If we found a renderer that is in the viewport,
  // that is better than any old viewport (but not as good as an interactive
  // one).
  if ( currentRenderer == NULL )
    {
    currentRenderer = viewportren;
    }

  // We must have a value - take anything.
  if ( currentRenderer == NULL) 
    {
    rc->InitTraversal();
    aren = rc->GetNextItem();
    currentRenderer = aren;
    }

  return currentRenderer;
}

//----------------------------------------------------------------------------
vtkCamera* vtkRenderWindowInteractor::FindPokedCamera(int x,int y) 
{
  vtkRenderer *aren = this->FindPokedRenderer(x,y);
  if (aren != NULL) 
    {
    return aren->GetActiveCamera();
    }
  return NULL;
}

void vtkRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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
  os << indent << "EventSize: " << "( " << this->EventSize[0] <<
    ", " << this->EventSize[1] << " )\n";
  os << indent << "Viewport Size: " << "( " << this->Size[0] <<
    ", " << this->Size[1] << " )\n";
  os << indent << "Number of Fly Frames: " << this->NumberOfFlyFrames <<"\n";
  os << indent << "Dolly: " << this->Dolly <<"\n";
  os << indent << "ControlKey: " << this->ControlKey << "\n";
  os << indent << "ShiftKey: " << this->ShiftKey << "\n";
  os << indent << "KeyCode: " << this->KeyCode << "\n";
  os << indent << "KeySym: " << (this->KeySym ? this->KeySym : "(null)") 
     << "\n";
  os << indent << "RepeatCount: " << this->RepeatCount << "\n";
}











