/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDuplicatePolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDuplicatePolyData.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkDuplicatePolyData);

vtkCxxSetObjectMacro(vtkDuplicatePolyData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkDuplicatePolyData,SocketController, vtkSocketController);

//----------------------------------------------------------------------------
vtkDuplicatePolyData::vtkDuplicatePolyData()
{
  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->Synchronous = 1;

  this->Schedule = NULL;
  this->ScheduleLength = 0;
  this->NumberOfProcesses = 0;

  this->SocketController = NULL;
  this->ClientFlag = 0;
  this->MemorySize = 0;
}

//----------------------------------------------------------------------------
vtkDuplicatePolyData::~vtkDuplicatePolyData()
{
  this->SetController(0);
  // Free the schedule memory.
  this->InitializeSchedule(0);
}


#define vtkDPDPow2(j) (1 << (j))
static inline int vtkDPDLog2(int j, int& exact)
{
  int counter=0;
  exact = 1;
  while(j)
    {
    if ( ( j & 1 ) && (j >> 1) )
      {
      exact = 0;
      }
    j = j >> 1;
    counter++;
    }
  return counter-1;
}

//----------------------------------------------------------------------------
void vtkDuplicatePolyData::InitializeSchedule(int numProcs)
{
  int i, j, k, exact;
  int *procFlags = NULL;

  if (this->NumberOfProcesses == numProcs)
    {
    return;
    }

  // Free old schedule.
  for (i = 0; i < this->NumberOfProcesses; ++i)
    {
    delete [] this->Schedule[i];
    this->Schedule[i] = NULL;
    }
  delete [] this->Schedule;
  this->Schedule = NULL;

  this->NumberOfProcesses = numProcs;
  if (numProcs == 0)
    {
    return;
    }

  i = vtkDPDLog2(numProcs, exact);
  if (!exact)
    {
    ++i;
    }
  this->ScheduleLength = vtkDPDPow2(i) - 1;
  this->Schedule = new int*[numProcs];
  for (i = 0; i < numProcs; ++i)
    {
    this->Schedule[i] = new int[this->ScheduleLength];
    for (j = 0; j < this->ScheduleLength; ++j)
      {
      this->Schedule[i][j] = -1;
      }
    }

  // Temporary array to record which processes have been used.
  procFlags = new int[numProcs];

  for (j = 0; j < this->ScheduleLength; ++j)
    {
    for (i = 0; i < numProcs; ++i)
      {
      if (this->Schedule[i][j] == -1)
        {
        // Try to find a available process that we have not paired with yet.
        for (k = 0; k < numProcs; ++k)
          {
          procFlags[k] = 0;
          }
        // Eliminate this process as a candidate.
        procFlags[i] = 1;
        // Eliminate procs already communicating during this cycle.
        for (k = 0; k < numProcs; ++k)
          {
          if (this->Schedule[k][j] != -1)
            {
            procFlags[this->Schedule[k][j]] = 1;
            }
          }
        // Eliminate process we have already paired with.
        for (k = 0; k < j; ++k)
          {
          if (this->Schedule[i][k] != -1)
            {
            procFlags[this->Schedule[i][k]] = 1;
            }
          }
        // Look for the first appropriate process.
        for (k = 0; k < numProcs; ++k)
          {
          if (procFlags[k] == 0)
            {
            // Set the pair in the schedule for communication.
            this->Schedule[i][j] = k;
            this->Schedule[k][j] = i;
            // Break the loop.
            k = numProcs;
            }
          }
        }
      }
    }

  delete [] procFlags;
  procFlags = NULL;
}

//--------------------------------------------------------------------------
int vtkDuplicatePolyData::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//----------------------------------------------------------------------------
int vtkDuplicatePolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int myId, partner;
  int idx;

  if (this->SocketController && this->ClientFlag)
    {
    this->ClientExecute(output);
    return 1;
    }

  if (this->Controller == NULL)
    {
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    if (this->SocketController && ! this->ClientFlag)
      {
      this->SocketController->Send(output, 1, 18732);
      }
    return 1;
    }

  myId = this->Controller->GetLocalProcessId();

  // Collect.
  vtkPolyData *pd = NULL;;

  vtkAppendPolyData *append = vtkAppendPolyData::New();
  // First append the input from this process.
  pd = vtkPolyData::New();
  pd->CopyStructure(input);
  pd->GetPointData()->PassData(input->GetPointData());
  pd->GetCellData()->PassData(input->GetCellData());
  append->AddInputData(pd);
  pd->Delete();

  for (idx = 0; idx < this->ScheduleLength; ++idx)
    {
    partner = this->Schedule[myId][idx];
    if (partner >= 0)
      {
      // Matching the order may not be necessary and may slow things down,
      // but it is a reasonable precaution.
      if (partner > myId || ! this->Synchronous)
        {
        this->Controller->Send(input, partner, 131767);

        pd = vtkPolyData::New();
        this->Controller->Receive(pd, partner, 131767);
        append->AddInputData(pd);
        pd->Delete();
        pd = NULL;
        }
      else
        {
        pd = vtkPolyData::New();
        this->Controller->Receive(pd, partner, 131767);
        append->AddInputData(pd);
        pd->Delete();
        pd = NULL;

        this->Controller->Send(input, partner, 131767);
        }
      }
    }
  append->Update();
  input = append->GetOutput();

  // Copy to output.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  append->Delete();
  append = NULL;

  if (this->SocketController && ! this->ClientFlag)
    {
    this->SocketController->Send(output, 1, 18732);
    }

  this->MemorySize = output->GetActualMemorySize();

  return 1;
}

//----------------------------------------------------------------------------
void vtkDuplicatePolyData::ClientExecute(vtkPolyData *output)
{
  vtkPolyData *tmp = vtkPolyData::New();

  // No data is on the client, so we just have to get the data
  // from node 0 of the server.
  this->SocketController->Receive(tmp, 1, 18732);
  output->CopyStructure(tmp);
  output->GetPointData()->PassData(tmp->GetPointData());
  output->GetCellData()->PassData(tmp->GetCellData());
}

//----------------------------------------------------------------------------
void vtkDuplicatePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  int i, j;

  os << indent << "Controller: (" << this->Controller << ")\n";
  if (this->SocketController)
    {
    os << indent << "SocketController: (" << this->SocketController << ")\n";
    os << indent << "ClientFlag: " << this->ClientFlag << endl;
    }
  os << indent << "Synchronous: " << this->Synchronous << endl;

  os << indent << "Schedule:\n";
  indent = indent.GetNextIndent();
  for (i = 0; i < this->NumberOfProcesses; ++i)
    {
    os << indent << i << ": ";
    if (this->Schedule[i][0] >= 0)
      {
      os << this->Schedule[i][0];
      }
    else
      {
      os << "X";
      }
    for (j = 1; j < this->ScheduleLength; ++j)
      {
      os << ", ";
      if (this->Schedule[i][j] >= 0)
        {
        os << this->Schedule[i][j];
        }
      else
        {
        os << "X";
        }
      }
    os << endl;
    }

  os << indent << "MemorySize: " << this->MemorySize << endl;
}
