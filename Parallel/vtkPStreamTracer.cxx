/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPStreamTracer.cxx
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
#include "vtkPStreamTracer.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkInterpolatedVelocityField.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRungeKutta2.h"

vtkCxxRevisionMacro(vtkPStreamTracer, "1.6");
vtkStandardNewMacro(vtkPStreamTracer);

vtkCxxSetObjectMacro(vtkPStreamTracer, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkPStreamTracer, 
                     Interpolator,
                     vtkInterpolatedVelocityField);

vtkPStreamTracer::vtkPStreamTracer()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  if (this->Controller)
    {
    this->Controller->Register(this);
    }
  this->Interpolator = 0;
  this->Seeds = 0;
  this->SeedIds = 0;
  this->IntegrationDirections = 0;
}

vtkPStreamTracer::~vtkPStreamTracer()
{
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = 0;
    }
  this->SetInterpolator(0);
  if (this->Seeds)
    {
    this->Seeds->Delete();
    }
  if (this->SeedIds)
    {
    this->SeedIds->Delete();
    }
  if (this->IntegrationDirections)
    {
    this->IntegrationDirections->Delete();
    }
}

void vtkPStreamTracer::ForwardTask(float seed[3], 
                                   int direction, 
                                   int isNewSeed,
                                   int lastid,
                                   int currentLine)
{
  int myid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  int nextid;
  if (myid == numProcs-1)
    {
    nextid = 0;
    }
  else
    {
    nextid = myid+1;
    }

  this->Controller->Send(&isNewSeed, 1, nextid, 311);
  this->Controller->Send(&lastid, 1, nextid, 322);
  if (isNewSeed != 2)
    {
    this->Controller->Send(seed, 3, nextid, 333);
    this->Controller->Send(&direction, 1, nextid, 344);
    this->Controller->Send(&currentLine, 1, nextid, 355);
    }
}

int vtkPStreamTracer::ReceiveAndProcessTask()
{
  int isNewSeed = 0;
  int lastid = 0;
  int currentLine = 0;
  int direction=FORWARD;
  float seed[3] = {0.0, 0.0, 0.0};
  int myid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();

  this->Controller->Receive(&isNewSeed, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            311);
  this->Controller->Receive(&lastid, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            322);
  if ( isNewSeed == 2 )
    {
    if ( (( myid == numProcs-1 && lastid == 0 ) ||
          ( myid != numProcs-1 && lastid == myid + 1) ))
      {
      // All processes have been already told to stop. No need to tell
      // the next one.
      return 0;
      }
    this->ForwardTask(seed, direction, 2, lastid, 0);
    return 0;
    }
  this->Controller->Receive(seed, 
                            3, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            333);
  this->Controller->Receive(&direction, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            344);
  this->Controller->Receive(&currentLine, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            355);
  return this->ProcessTask(seed, direction, isNewSeed, lastid, currentLine);
                   
}

int vtkPStreamTracer::ProcessTask(float seed[3], 
                                  int direction, 
                                  int isNewSeed,
                                  int lastid,
                                  int currentLine)
{
  int myid = this->Controller->GetLocalProcessId();

  // This seed was visited by everybody and nobody had it.
  // Must be out of domain.
  // TEMP: TELL ALL PROCESSES TO STOP
  if (isNewSeed == 0 && lastid == myid)
    {
    vtkIdType numLines = this->SeedIds->GetNumberOfIds();
    currentLine++;
    if ( currentLine < numLines )
      {
      this->ProcessTask(
        this->Seeds->GetTuple(this->SeedIds->GetId(currentLine)), 
        this->IntegrationDirections->GetValue(currentLine),
        1,
        myid,
        currentLine);
      return 1;
      }
    else
      {
      this->ForwardTask(seed, direction, 2, myid, currentLine);
      return 0;
      }
    }

  float velocity[3];
  // We don't have it, let's forward it to the next guy
  if (!this->Interpolator->FunctionValues(seed, velocity))
    {
    this->ForwardTask(seed, direction, 0, lastid, currentLine);
    return 1;
    }

  float lastPoint[3];

  vtkFloatArray* seeds = vtkFloatArray::New();
  seeds->SetNumberOfComponents(3);
  seeds->InsertNextTuple(seed);

  vtkIdList* seedIds = vtkIdList::New();
  seedIds->InsertNextId(0);

  vtkIntArray* integrationDirections = vtkIntArray::New();
  integrationDirections->InsertNextValue(direction);

  vtkPolyData* tmpOutput = 0;
  vtkPolyData* output = this->GetOutput();
  if ( output->GetNumberOfCells() > 0 )
    {
    tmpOutput = vtkPolyData::New();
    this->Integrate(tmpOutput, 
                    seeds, 
                    seedIds, 
                    integrationDirections, 
                    lastPoint);
    if ( tmpOutput->GetNumberOfCells() > 0 )
      {
      vtkPolyData* outputcp = vtkPolyData::New();
      outputcp->ShallowCopy(output);
      vtkAppendPolyData* append = vtkAppendPolyData::New();
      append->AddInput(outputcp);
      append->AddInput(tmpOutput);
      append->Update();
      vtkPolyData* appoutput = append->GetOutput();
      output->CopyStructure(appoutput);
      output->GetPointData()->PassData(appoutput->GetPointData());
      output->GetCellData()->PassData(appoutput->GetCellData());
      append->Delete();
      outputcp->Delete();
      }
    tmpOutput->Register(this);
    tmpOutput->Delete();
    }
  else
    {
    this->Integrate(output, 
                    seeds, 
                    seedIds, 
                    integrationDirections, 
                    lastPoint);
    tmpOutput = output;
    tmpOutput->Register(this);
    }

  int numPoints = tmpOutput->GetNumberOfPoints();
  if (numPoints == 0)
    {
    this->ForwardTask(lastPoint, direction, 2, myid, currentLine);
    seeds->Delete(); 
    seedIds->Delete();
    integrationDirections->Delete();
    tmpOutput->UnRegister(this);
    return 0;
    }

  tmpOutput->GetPoint(numPoints-1, lastPoint);
  tmpOutput->UnRegister(this);

  vtkInitialValueProblemSolver* ivp = this->Integrator;
  ivp->Register(this);
  
  vtkRungeKutta2* tmpSolver = vtkRungeKutta2::New();
  this->SetIntegrator(tmpSolver);
  tmpSolver->Delete();
  seeds->SetTuple(0, lastPoint);
  seedIds->SetId(0,0);
  vtkPolyData* dummyOutput = vtkPolyData::New();
  this->Integrate(dummyOutput, 
                  seeds, 
                  seedIds, 
                  integrationDirections, 
                  lastPoint);
  dummyOutput->Delete();
  this->SetIntegrator(ivp);
  ivp->UnRegister(this);
  
  // New seed
  this->ForwardTask(lastPoint, direction, 1, myid, currentLine);
  
  seeds->Delete(); 
  seedIds->Delete();
  integrationDirections->Delete();

  return 1;
}

void vtkPStreamTracer::ComputeInputUpdateExtents( vtkDataObject *output )
{
  for (int idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      if (idx != 1 )
        {
        this->Inputs[idx]->SetUpdateExtent(output->GetUpdatePiece(),
                                           output->GetUpdateNumberOfPieces(),
                                           output->GetUpdateGhostLevel());
        }
      else
        {
        this->Inputs[idx]->SetUpdateExtent(0, 1, 0);
        }
      }
    }
}

void vtkPStreamTracer::ExecuteInformation()
{
  vtkDataSet *output = this->GetOutput();
  output->SetMaximumNumberOfPieces(-1);
}

void vtkPStreamTracer::Execute()
{
  if (!this->Controller)
    {
    vtkErrorMacro("No controller assigned. Can not execute.");
    return;
    }

  if (this->Controller->GetNumberOfProcesses() == 1)
    {
    this->Superclass::Execute();
    return;
    }

  vtkInterpolatedVelocityField* func;
  int maxCellSize = 0;
  if (this->CheckInputs(func, &maxCellSize) != VTK_OK)
    {
    vtkErrorMacro("No appropriate inputs have been found. Can not execute.");
    func->Delete();
    // >>>>>>>>>> TODO: All should pass this test.
    return;
    }
  func->SetCaching(0);
  this->SetInterpolator(func);
  func->Delete();

  this->InitializeSeeds(this->Seeds, 
                        this->SeedIds, 
                        this->IntegrationDirections);
  
  int myid = this->Controller->GetLocalProcessId();
  if (this->Seeds)
    {
    if (myid == 0)
      {
      int currentLine = 0;
      this->ProcessTask(
        this->Seeds->GetTuple(this->SeedIds->GetId(currentLine)), 
        this->IntegrationDirections->GetValue(currentLine),
        1,
        0,
        currentLine);
      }
    while(1) 
      {
      if (!this->ReceiveAndProcessTask()) { break; }
      }
    }

  if (this->Seeds) 
    {
    this->Seeds->Delete(); 
    this->Seeds = 0;
    }
  this->IntegrationDirections->Delete();
  this->IntegrationDirections = 0;
  this->SeedIds->Delete();
  this->SeedIds = 0;
}

void vtkPStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "InputVectorsSelection: " 
     << (this->InputVectorsSelection ? this->InputVectorsSelection : "(none)")
     << endl;
}
