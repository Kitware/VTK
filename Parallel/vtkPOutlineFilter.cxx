/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOutlineFilter.cxx
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
#include "vtkMultiProcessController.h"
#include "vtkPOutlineFilter.h"
#include "vtkOutlineSource.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPOutlineFilter, "1.1");
vtkStandardNewMacro(vtkPOutlineFilter);
vtkCxxSetObjectMacro(vtkPOutlineFilter, Controller, vtkMultiProcessController);

vtkPOutlineFilter::vtkPOutlineFilter ()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->OutlineSource = vtkOutlineSource::New();
}

vtkPOutlineFilter::~vtkPOutlineFilter ()
{
  this->SetController(0);
  if (this->OutlineSource != NULL)
    {
    this->OutlineSource->Delete ();
    this->OutlineSource = NULL;
    }
}

void vtkPOutlineFilter::Execute()
{
  vtkPolyData *output = this->GetOutput();
  float bds[6];

  if ( !this->Controller )
    {
    vtkErrorMacro("Controller not set");
    return;
    }

  this->GetInput()->GetBounds(bds);

  int procid = this->Controller->GetLocalProcessId();
  if ( procid )
    {
    // Satellite node
    this->Controller->Send(bds, 6, 0, 1970);
    }
  else
    {
    int numProcs = this->Controller->GetNumberOfProcesses();
    int idx;
    float tmp[6];

    for (idx = 1; idx < numProcs; ++idx)
      {
      this->Controller->Receive(tmp, 6, idx, 792390);
      if (tmp[0] < bds[0])
        {
        bds[0] = tmp[0];
        }
      if (tmp[1] < bds[1])
        {
        bds[1] = tmp[1];
        }
      if (tmp[2] < bds[2])
        {
        bds[2] = tmp[2];
        }
      if (tmp[3] < bds[3])
        {
        bds[3] = tmp[3];
        }
      if (tmp[4] < bds[4])
        {
        bds[4] = tmp[4];
        }
      if (tmp[5] < bds[5])
        {
        bds[5] = tmp[5];
        }
      }
    // only output in process 0.
    this->OutlineSource->SetBounds(bds);          
    this->OutlineSource->Update();
    output->CopyStructure(this->OutlineSource->GetOutput());
    }
}


void vtkPOutlineFilter::ExecuteInformation()
{
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}


void vtkPOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
}


