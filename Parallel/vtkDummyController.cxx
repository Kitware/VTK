/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDummyCommunicator.h"
#include "vtkDummyController.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkDummyController, "1.4");
vtkStandardNewMacro(vtkDummyController);


//----------------------------------------------------------------------------
vtkDummyController::vtkDummyController()
{
  this->Communicator = vtkDummyCommunicator::New();
  this->RMICommunicator = vtkDummyCommunicator::New();
}

vtkDummyController::~vtkDummyController()
{
  this->Communicator->Delete();
  this->RMICommunicator->Delete();
}

void vtkDummyController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkDummyController::SingleMethodExecute()
{
  if (this->SingleMethod)
    {
    // Should we wet the global controller here?  I'm going to say no since
    // we are not really a parallel job.

    (this->SingleMethod)(this, this->SingleData);
    }
  else
    {
    vtkWarningMacro("SingleMethod not set.");
    }
}

//-----------------------------------------------------------------------------
void vtkDummyController::MultipleMethodExecute()
{
  if (this->MultipleMethod[0])
    {
    // Should we wet the global controller here?  I'm going to say no since
    // we are not really a parallel job.

    (this->MultipleMethod[0])(this, this->MultipleData[0]);
    }
  else
    {
    vtkWarningMacro("MultipleMethod 0 not set.");
    }
}
