/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPHardwareSelector.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

class vtkPHardwareSelector::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New() { return new vtkObserver(); }
  virtual void Execute(vtkObject *, unsigned long eventId,
                       void *)
    {
    if (eventId == vtkCommand::StartEvent)
      {
      this->Target->StartRender();
      }
    else if (eventId == vtkCommand::EndEvent)
      {
      this->Target->EndRender();
      }
    }
  vtkPHardwareSelector* Target;
};

vtkStandardNewMacro(vtkPHardwareSelector);
//----------------------------------------------------------------------------
vtkPHardwareSelector::vtkPHardwareSelector()
{
  this->ProcessIsRoot = false;
  this->Observer = vtkObserver::New();
  this->Observer->Target = this;
}

//----------------------------------------------------------------------------
vtkPHardwareSelector::~vtkPHardwareSelector()
{
  this->Observer->Target = 0;
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
bool vtkPHardwareSelector::CaptureBuffers()
{
  if (this->ProcessIsRoot)
    {
    return this->Superclass::CaptureBuffers();
    }

  this->InvokeEvent(vtkCommand::StartEvent);
  this->BeginSelection();
  vtkRenderWindow* rwin = this->Renderer->GetRenderWindow();
  rwin->AddObserver(vtkCommand::StartEvent, this->Observer);
  rwin->AddObserver(vtkCommand::EndEvent, this->Observer);

  for (this->CurrentPass = MIN_KNOWN_PASS;
    this->CurrentPass < MAX_KNOWN_PASS; this->CurrentPass++)
    {
    if (this->PassRequired(this->CurrentPass))
      {
      break;
      }
    }

  if (this->CurrentPass == MAX_KNOWN_PASS)
    {
    this->EndRender();
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkPHardwareSelector::StartRender()
{
}

//----------------------------------------------------------------------------
void vtkPHardwareSelector::EndRender()
{

  this->CurrentPass++;
  for (; this->CurrentPass < MAX_KNOWN_PASS; this->CurrentPass++)
    {
    if (this->PassRequired(this->CurrentPass))
      {
      break;
      }
    }

  if (this->CurrentPass>=MAX_KNOWN_PASS)
    {
    vtkRenderWindow* rwin = this->Renderer->GetRenderWindow();
    rwin->RemoveObserver(this->Observer);
    this->EndSelection();
    this->InvokeEvent(vtkCommand::EndEvent);
    }
}

//----------------------------------------------------------------------------
void vtkPHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ProcessIsRoot: " << this->ProcessIsRoot << endl;
}
