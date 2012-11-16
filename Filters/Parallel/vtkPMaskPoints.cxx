/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMaskPoints.h"

#include "vtkDummyController.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPMaskPoints);

//----------------------------------------------------------------------------
vtkPMaskPoints::vtkPMaskPoints()
{
  this->Controller = 0;

  vtkSmartPointer<vtkMultiProcessController> controller =
    vtkMultiProcessController::GetGlobalController();
  if (!controller)
    {
    controller = vtkSmartPointer<vtkDummyController>::New();
    }
  this->SetController(controller.GetPointer());
}

vtkPMaskPoints::~vtkPMaskPoints()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
void vtkPMaskPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Controller)
    {
    os << indent << "Controller: " << this->Controller << endl;
    }
  else
    {
    os << indent << "Controller: (null)\n";
    }
}

void vtkPMaskPoints::SetController(vtkMultiProcessController *c)
{
  if(this->Controller == c)
    {
    return;
    }

  this->Modified();

  if(this->Controller != 0)
    {
    this->Controller->UnRegister(this);
    this->Controller = 0;
    }

  if(c == 0)
    {
    return;
    }

  this->Controller = c;
  c->Register(this);
}

vtkMultiProcessController* vtkPMaskPoints::GetController()
{
  return (vtkMultiProcessController*)this->Controller;
}

void vtkPMaskPoints::InternalScatter
(unsigned long* a, unsigned long * b, int c, int d)
{
  this->Controller->Scatter(a, b, c, d);
}

void vtkPMaskPoints::InternalGather
(unsigned long* a, unsigned long* b, int c, int d)
{
  this->Controller->Gather(a, b, c, d);
}

int vtkPMaskPoints::InternalGetNumberOfProcesses()
{
  return this->Controller->GetNumberOfProcesses();
}

int vtkPMaskPoints::InternalGetLocalProcessId()
{
  return this->Controller->GetLocalProcessId();
}

void vtkPMaskPoints::InternalBarrier()
{
  this->Controller->Barrier();
}
