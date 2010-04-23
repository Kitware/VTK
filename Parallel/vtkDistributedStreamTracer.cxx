/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedStreamTracer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistributedStreamTracer.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkAbstractInterpolatedVelocityField.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRungeKutta2.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkDistributedStreamTracer);

vtkDistributedStreamTracer::vtkDistributedStreamTracer()
{
}

vtkDistributedStreamTracer::~vtkDistributedStreamTracer()
{
}

void vtkDistributedStreamTracer::ForwardTask(double seed[3], 
                                             int direction, 
                                             int isNewSeed,
                                             int lastId,
                                             int lastCellId,
                                             int currentLine,
                                             double* firstNormal,
                                             double propagation,
                                             vtkIdType numSteps)
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
  this->Controller->Send(&lastId, 1, nextid, 322);
  if (isNewSeed != 2)
    {
    this->Controller->Send(&lastCellId, 1, nextid, 322);
    this->Controller->Send(seed, 3, nextid, 333);
    this->Controller->Send(&direction, 1, nextid, 344);
    this->Controller->Send(&currentLine, 1, nextid, 355);
    double tmpNormal[4];
    if (firstNormal)
      {
      tmpNormal[0] = 1;
      memcpy(tmpNormal+1, firstNormal, 3*sizeof(double));
      }
    else
      {
      tmpNormal[0] = 0;
      }
    this->Controller->Send(tmpNormal, 4, nextid, 366);
    this->Controller->Send(&propagation, 1, nextid, 367);
    this->Controller->Send(&numSteps, 1, nextid, 368);
    }
}

int vtkDistributedStreamTracer::ReceiveAndProcessTask()
{
  int isNewSeed = 0;
  int lastId = 0;
  int lastCellId = 0;
  int currentLine = 0;
  int direction=FORWARD;
  double seed[3] = {0.0, 0.0, 0.0};
  int myid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();

  this->Controller->Receive(&isNewSeed, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            311);
  this->Controller->Receive(&lastId, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            322);

  // isNewSeed == 2 means that we were to stop.
  if ( isNewSeed == 2 )
    {
    if ( (( myid == numProcs-1 && lastId == 0 ) ||
          ( myid != numProcs-1 && lastId == myid + 1) ))
      {
      // All processes have been already told to stop. No need to tell
      // the next one.
      return 0;
      }
    double propagation = 0.0;
    vtkIdType numSteps = 0;
    this->ForwardTask(
      seed, direction, 2, lastId, lastCellId, 0, 0, propagation, numSteps);
    return 0;
    }
  this->Controller->Receive(&lastCellId, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            322);
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
  double tmpNormal[4];
  this->Controller->Receive(tmpNormal, 
                            4, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            366);
  double propagation;
  this->Controller->Receive(&propagation, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            367);
  vtkIdType numSteps;
  this->Controller->Receive(&numSteps, 
                            1, 
                            vtkMultiProcessController::ANY_SOURCE, 
                            368);

  double* firstNormal=0;
  if (tmpNormal[0] != 0)
    {
    firstNormal = &(tmpNormal[1]);
    }
  return this->ProcessTask(seed, 
                           direction, 
                           isNewSeed, 
                           lastId, 
                           lastCellId, 
                           currentLine, 
                           firstNormal,
                           propagation,
                           numSteps);
}

int vtkDistributedStreamTracer::ProcessNextLine(int currentLine)
{
  int myid = this->Controller->GetLocalProcessId();

  vtkIdType numLines = this->SeedIds->GetNumberOfIds();
  currentLine++;
  if ( currentLine < numLines )
    {
    double propagation = 0.0;
    vtkIdType numSteps = 0;

    return this->ProcessTask(
      this->Seeds->GetTuple(this->SeedIds->GetId(currentLine)), 
      this->IntegrationDirections->GetValue(currentLine),
      1, myid, -1, currentLine, 0, propagation, numSteps);
    }

  // All done. Tell everybody to stop.
  double seed[3] = {0.0, 0.0, 0.0};
  double propagation = 0.0;
  vtkIdType numSteps = 0;
  this->ForwardTask(seed, 0, 2, myid, 0, 0, 0, propagation, numSteps);
  return 0;

}

// Integrate a streamline
int vtkDistributedStreamTracer::ProcessTask(double seed[3], 
                                            int direction, 
                                            int isNewSeed,
                                            int lastId,
                                            int lastCellId,
                                            int currentLine,
                                            double* firstNormal,
                                            double propagation,
                                            vtkIdType numSteps)
{
  int myid = this->Controller->GetLocalProcessId();

  // This seed was visited by everybody and nobody had it.
  // Must be out of domain.
  if (isNewSeed == 0 && lastId == myid)
    {
    return this->ProcessNextLine(currentLine);
    }

  this->UpdateProgress(
    (double)currentLine/this->SeedIds->GetNumberOfIds());

  int retVal = 1;
  if (!this->EmptyData)
    {
    double velocity[3];
    this->Interpolator->ClearLastCellId();
    retVal = this->Interpolator->FunctionValues(seed, velocity);
    }
  // We don't have it, let's forward it to the next guy
  if (!retVal || this->EmptyData)
    {
    this->ForwardTask(seed, 
                      direction, 
                      0, 
                      lastId, 
                      lastCellId, 
                      currentLine, 
                      firstNormal,
                      propagation,
                      numSteps);
    return 1;
    }

  // We have it, let's integrate
  double lastPoint[3];

  vtkFloatArray* seeds = vtkFloatArray::New();
  seeds->SetNumberOfComponents(3);
  seeds->InsertNextTuple(seed);

  vtkIdList* seedIds = vtkIdList::New();
  seedIds->InsertNextId(0);

  vtkIntArray* integrationDirections = vtkIntArray::New();
  integrationDirections->InsertNextValue(direction);

  // Keep track of all streamlines by adding them to TmpOutputs.
  // They will be appended together after all the integration is done.
  vtkPolyData* tmpOutput = vtkPolyData::New();
  this->TmpOutputs.push_back(tmpOutput);

  vtkAbstractInterpolatedVelocityField* func;
  int maxCellSize = 0;
  this->CheckInputs(func, &maxCellSize);

  vtkCompositeDataIterator* iter = this->InputData->NewIterator();
  vtkSmartPointer<vtkCompositeDataIterator> iterP(iter);
  iter->Delete();
  
  iterP->GoToFirstItem();
  vtkDataSet* input0 = 0;
  if (!iterP->IsDoneWithTraversal())
    {
    input0 = vtkDataSet::SafeDownCast(iterP->GetCurrentDataObject());
    }
  vtkDataArray *vectors = this->GetInputArrayToProcess(0,input0);
  const char *vecName = vectors->GetName();
  this->Integrate(input0,
                  tmpOutput,
                  seeds,
                  seedIds,
                  integrationDirections,
                  lastPoint,
                  func,
                  maxCellSize, 
                  vecName,
                  propagation,
                  numSteps);
  this->GenerateNormals(tmpOutput, firstNormal, vecName);

  // These are used to keep track of where the seed came from
  // and where it will go. Used later to fill the gaps between
  // streamlines.
  vtkIntArray* strOrigin = vtkIntArray::New();
  strOrigin->SetNumberOfComponents(2);
  strOrigin->SetNumberOfTuples(1);
  strOrigin->SetName("Streamline Origin");
  strOrigin->SetValue(0, lastId);
  strOrigin->SetValue(1, lastCellId);
  tmpOutput->GetCellData()->AddArray(strOrigin);
  strOrigin->Delete();

  vtkIntArray* streamIds = vtkIntArray::New();
  streamIds->SetNumberOfTuples(1);
  streamIds->SetName("Streamline Ids");
  lastCellId = static_cast<int>(this->TmpOutputs.size()) - 1;
  streamIds->SetComponent(0, 0, lastCellId);
  tmpOutput->GetCellData()->AddArray(streamIds);
  streamIds->Delete();

  // We have to know why the integration terminated
  vtkIntArray* resTermArray = vtkIntArray::SafeDownCast(
    tmpOutput->GetCellData()->GetArray("ReasonForTermination"));
  int resTerm=vtkStreamTracer::OUT_OF_DOMAIN;
  if (resTermArray)
    {
    resTerm = resTermArray->GetValue(0);
    }

  int numPoints = tmpOutput->GetNumberOfPoints();
  // If the interation terminated due to something other than
  // moving outside the domain, move to the next seed.
  if (numPoints == 0 || resTerm != vtkStreamTracer::OUT_OF_DOMAIN)
    {
    retVal = this->ProcessNextLine(currentLine);
    seeds->Delete(); 
    seedIds->Delete();
    integrationDirections->Delete();
    tmpOutput->Delete();
    func->Delete();
    return retVal;
    }

  // Continue the integration a bit further to obtain a point
  // outside. The main integration step can not always be used
  // for this, specially if the integration is not 2nd order.
  tmpOutput->GetPoint(numPoints-1, lastPoint);

  vtkInitialValueProblemSolver* ivp = this->Integrator;
  ivp->Register(this);
  
  vtkRungeKutta2* tmpSolver = vtkRungeKutta2::New();
  this->SetIntegrator(tmpSolver);
  tmpSolver->Delete();

  double tmpseed[3];
  memcpy(tmpseed, lastPoint, 3*sizeof(double));
  this->SimpleIntegrate(tmpseed, lastPoint, this->LastUsedStepSize, func);
  func->Delete();

  this->SetIntegrator(ivp);
  ivp->UnRegister(this);
  
  double* lastNormal = 0;
  vtkDataArray* normals = tmpOutput->GetPointData()->GetArray("Normals");
  if (normals)
    {
    lastNormal = new double[3];
    normals->GetTuple(normals->GetNumberOfTuples()-1, lastNormal);
    }
  
  tmpOutput->GetPoints()->SetPoint(numPoints-1, lastPoint);
  tmpOutput->Delete();

  // New seed, send it to the next guy
  this->ForwardTask(lastPoint, 
                    direction, 
                    1, 
                    myid, 
                    lastCellId, 
                    currentLine, 
                    lastNormal,
                    propagation,
                    numSteps);

  delete[] lastNormal;

  seeds->Delete(); 
  seedIds->Delete();
  integrationDirections->Delete();

  return 1;
}

void vtkDistributedStreamTracer::ParallelIntegrate()
{

  int myid = this->Controller->GetLocalProcessId();
  if (this->Seeds)
    {
    int doLoop = 1;
    // First process starts by integrating the first point
    if (myid == 0)
      {
      int currentLine = 0;
      double propagation = 0.0;
      vtkIdType numSteps = 0;
      doLoop = this->ProcessTask(
        this->Seeds->GetTuple(this->SeedIds->GetId(currentLine)), 
        this->IntegrationDirections->GetValue(currentLine),
        1, myid, -1,currentLine, 0, propagation, numSteps);
      }
    // Wait for someone to send us a seed to start from.
    while(doLoop) 
      {
      if (!this->ReceiveAndProcessTask()) { break; }
      }
    }
}

void vtkDistributedStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
