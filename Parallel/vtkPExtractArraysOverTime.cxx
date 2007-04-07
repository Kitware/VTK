/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractArraysOverTime.h"

#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkPExtractArraysOverTime, "1.1");
vtkStandardNewMacro(vtkPExtractArraysOverTime);

vtkCxxSetObjectMacro(vtkPExtractArraysOverTime, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPExtractArraysOverTime::vtkPExtractArraysOverTime()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPExtractArraysOverTime::~vtkPExtractArraysOverTime()
{
  this->SetController(0);
}
//----------------------------------------------------------------------------
void vtkPExtractArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPExtractArraysOverTime::PostExecute(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkRectilinearGrid *output = vtkRectilinearGrid::GetData(outInfo);

  int procid = 0;
  int numProcs = 1;
  if ( this->Controller )
    {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }

  if (numProcs > 1)
    {
    if (procid == 0)
      {
      for (int i = 1; i < numProcs; i++)
        {
        vtkRectilinearGrid* remoteOutput = vtkRectilinearGrid::New();
        this->Controller->Receive(remoteOutput, i, EXCHANGE_DATA);
        this->AddRemoteData(remoteOutput, output);
        remoteOutput->Delete();
        }
      }
    else
      {
      this->Controller->Send(output, 0, EXCHANGE_DATA);
      }
    }
  this->Superclass::PostExecute(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkPExtractArraysOverTime::AddRemoteData(vtkRectilinearGrid* routput,
                                              vtkRectilinearGrid* output)
{
  int* rDims = routput->GetDimensions();
  int* dims = output->GetDimensions();
  if (dims[0] != rDims[0])
    {
    vtkWarningMacro("Tried to add remote dataset of different length. "
                    "Skipping");
    return;
    }
  
  vtkUnsignedCharArray* rValidPts = vtkUnsignedCharArray::SafeDownCast(
    routput->GetPointData()->GetArray("vtkEAOTValidity"));

  if (rValidPts)
    {
    for (int i=0; i<dims[0]; i++)
      {
      if (rValidPts->GetValue(i))
        {
        output->GetPointData()->CopyData(routput->GetPointData(), i, i);
        }
      }
    }
}
