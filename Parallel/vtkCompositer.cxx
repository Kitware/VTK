/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompositer.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"
#include "vtkDataArray.h"
#include "vtkMultiProcessController.h"

vtkCxxRevisionMacro(vtkCompositer, "1.7");
vtkStandardNewMacro(vtkCompositer);

//-------------------------------------------------------------------------
vtkCompositer::vtkCompositer()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->NumberOfProcesses = 1;
  if (this->Controller)
    {
    this->Controller->Register(this);
    this->NumberOfProcesses = this->Controller->GetNumberOfProcesses();
    }
}
  
//-------------------------------------------------------------------------
vtkCompositer::~vtkCompositer()
{
  this->SetController(NULL);
}


//-------------------------------------------------------------------------
void vtkCompositer::SetController(vtkMultiProcessController *mpc)
{
  if (this->Controller == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    this->NumberOfProcesses = mpc->GetNumberOfProcesses();
    }
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    }
  this->Controller = mpc;
}

//-------------------------------------------------------------------------
void vtkCompositer::CompositeBuffer(vtkDataArray *pBuf, vtkFloatArray *zBuf,
                                    vtkDataArray *pTmp, vtkFloatArray *zTmp)
{
  pBuf = pBuf;
  zBuf = zBuf;
  pTmp = pTmp;
  zTmp = zTmp;
}

//-------------------------------------------------------------------------
void vtkCompositer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}



