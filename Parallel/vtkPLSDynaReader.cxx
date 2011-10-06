/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLSDynaReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// NOTE TO DEVELOPERS: ========================================================
//
// This is a parallel version of the LSDynaReader.
// Its primary tasks are to determine which parts should be read on each process
// and to send the relevant information from the master node to all slave nodes


#include "vtkPLSDynaReader.h"

#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPLSDynaReader);

//-----------------------------------------------------------------------------
vtkPLSDynaReader::vtkPLSDynaReader()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkPLSDynaReader::~vtkPLSDynaReader()
{
  this->SetController(NULL);
}

void vtkPLSDynaReader::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
void vtkPLSDynaReader::SetController(vtkMultiProcessController *c)
{
  if ((c == NULL) || (c->GetNumberOfProcesses() == 0))
    {
    this->NumProcesses = 1;
    this->MyId = 0;
    }

  if (this->Controller == c)
    {
    return;
    }

  this->Modified();

  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }

  if (c == NULL)
    {
    return;
    }

  this->Controller = c;

  c->Register(this);
  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyId = c->GetLocalProcessId();
}

//-----------------------------------------------------------------------------
int vtkPLSDynaReader::RequestInformation( vtkInformation* request,
                                         vtkInformationVector** iinfo,
                                         vtkInformationVector* oinfo )
{
  return this->Superclass::RequestInformation(request,iinfo,oinfo);
}

//-----------------------------------------------------------------------------
int vtkPLSDynaReader::RequestData(vtkInformation* request,
  vtkInformationVector** iinfo,
  vtkInformationVector* oinfo )
{
  return this->Superclass::RequestData(request,iinfo,oinfo);
}
