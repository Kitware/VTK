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
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkInterpolatedVelocityField.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkPStreamTracer, "1.9");

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

  this->GenerateNormalsInIntegrate = 0;
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


// After the integration is over, we need to add one point
// at the end of streamline pieces which were not the end
// piece. This has to be done in order to close the gap between
// pieces which appear due to jump from one process to another.
// This method waits until a process sends its first points.
void vtkPStreamTracer::ReceiveLastPoints()
{
  int streamId = 0;

  while(1)
    {
    this->Controller->Receive(&streamId, 
                              1, 
                              vtkMultiProcessController::ANY_SOURCE,
                              733);
    if (streamId < 0)
      {
      break;
      }
    this->ReceiveCellPoint(this->GetOutput(), streamId, -1);
    }
  // We were told that it is our turn to send first points.
  if (streamId == -2)
    {
    this->SendFirstPoints();
    }
}

// Once we are done sending, let's tell the next guy (unless
// this is the last process) to send it's first points
void vtkPStreamTracer::MoveToNextSend()
{
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myid = this->Controller->GetLocalProcessId();

  int tag;
  if (myid == numProcs - 1)
    {
    tag = -1;
    for(int i=0; i<numProcs; i++)
      {
      if (i != myid)
        {
        this->Controller->Send(&tag, 1, i, 733);
        }
      }
    }
  else
    {
    tag = -2;
    this->Controller->Send(&tag, 1, myid+1, 733);
    this->ReceiveLastPoints();
    }

}

// After the integration is over, we need to add one point
// at the end of streamline pieces which were not the end
// piece. This has to be done in order to close the gap between
// pieces which appear due to jump from one process to another.
// This method sends the first point of each streamline which
// originated in another process to that process. This information
// is stored in the "Streamline Origin" array.
void vtkPStreamTracer::SendFirstPoints()
{
  vtkPolyData* output = this->GetOutput();
  vtkIntArray* strOrigin = vtkIntArray::SafeDownCast(
    output->GetCellData()->GetArray("Streamline Origin"));
  if (!strOrigin)
    {
    this->MoveToNextSend();
    return;
    }
  int numLines = strOrigin->GetNumberOfTuples();
  int streamId, sendToId;
  int i;
  for(i=0; i<numLines; i++)
    {
    sendToId = strOrigin->GetValue(2*i);
    streamId = strOrigin->GetValue(2*i+1);
    if (streamId != -1)
      {
      this->Controller->Send(&streamId, 1, sendToId, 733);
      this->SendCellPoint(output, i, 0, sendToId);
      }
    }
  this->MoveToNextSend();

}

// Receive one point and add it to the given cell.
void vtkPStreamTracer::ReceiveCellPoint(vtkPolyData* tomod,
                                        int streamId,
                                        vtkIdType idx)
{
  vtkPolyData* input = vtkPolyData::New();

  // Receive a polydata which contains one point.
  this->Controller->Receive(input, vtkMultiProcessController::ANY_SOURCE, 765);

  int numCells = tomod->GetNumberOfCells();
  // Use the "Streamline Ids" array to locate the right cell.
  vtkIntArray* streamIds = vtkIntArray::SafeDownCast(
    tomod->GetCellData()->GetArray("Streamline Ids"));
  if (!streamIds)
    {
    input->Delete();
    return;
    }
  vtkIdType cellId=-1;
  for(vtkIdType cellIdx=0; cellIdx < numCells; cellIdx++)
    {
    if (streamIds->GetValue(cellIdx) == streamId)
      {
      cellId = cellIdx;
      break;
      }
    }

  if ( cellId == -1 )
    {
    return;
    }

  // Find the point to be modified. We don't actually add a point,
  // we just replace replace the attributes of one (usually the last)
  // with the new attributes we received.
  vtkIdType ptId;
  vtkIdType npts;
  vtkIdType* pts;
  tomod->GetCellPoints(cellId, npts, pts);
  if (idx == -1)
    {
    idx = npts-1;
    }
  ptId = pts[idx];

  // Replace attributes
  vtkPointData* pd = input->GetPointData();
  int numArrays = pd->GetNumberOfArrays();
  int i;
  vtkPointData* outputPD = tomod->GetPointData();
  for(i=0; i<numArrays; i++)
    {
    vtkDataArray* da = pd->GetArray(i);
    const char* name = da->GetName();
    if (name)
      {
      vtkDataArray* outputDA = outputPD->GetArray(name);
      outputDA->SetTuple(ptId, da->GetTuple(0));
      }
    }
  
  input->Delete();
}

// Send one point and all of it's attributes to another process
void vtkPStreamTracer::SendCellPoint(vtkPolyData* togo,
                                     vtkIdType cellId, 
                                     vtkIdType idx, 
                                     int sendToId)
{
  // We create a dummy dataset which will contain the point
  // we want to send and it's attributes.
  vtkPolyData* copy = vtkPolyData::New();
  
  vtkIdType ptId;
  vtkIdType npts;
  vtkIdType* pts;
  togo->GetCellPoints(cellId, npts, pts);
  ptId = pts[idx];
    
  vtkPoints* points = vtkPoints::New();
  points->SetNumberOfPoints(1);
  points->SetPoint(0, togo->GetPoint(ptId));
  copy->SetPoints(points);
  points->Delete();

  vtkPointData* togoPD = togo->GetPointData();
  vtkPointData* copyPD = copy->GetPointData();

  copyPD->CopyAllocate(togoPD, 1);
  copyPD->CopyData(togoPD, ptId, 0);

  this->Controller->Send(copy, sendToId, 765);

  copy->Delete();
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
        // This is the seed source, we need it all.
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
    this->GenerateNormalsInIntegrate = 1;
    this->Superclass::Execute();
    this->GenerateNormalsInIntegrate = 0;
    return;
    }

  vtkPolyData* output = this->GetOutput();

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
  

  this->TmpOutputs.erase(this->TmpOutputs.begin(), this->TmpOutputs.end());
  this->ParallelIntegrate();

  // The parallel integration adds all streamlines to TmpOutputs
  // container. We append them all together here.
  vtkAppendPolyData* append = vtkAppendPolyData::New();
  for (TmpOutputsType::iterator it = this->TmpOutputs.begin();
       it != this->TmpOutputs.end(); it++)
    {
    vtkPolyData* inp = it->GetPointer();
    if ( inp->GetNumberOfCells() > 0 )
      {
      append->AddInput(inp);
      }
    }
  append->Update();
  vtkPolyData* appoutput = append->GetOutput();
  output->CopyStructure(appoutput);
  output->GetPointData()->PassData(appoutput->GetPointData());
  output->GetCellData()->PassData(appoutput->GetCellData());
  append->Delete();
  this->TmpOutputs.erase(this->TmpOutputs.begin(), this->TmpOutputs.end());

  // Fill the gaps between streamlines.
  this->GetOutput()->BuildCells();
  int myid = this->Controller->GetLocalProcessId();
  if (myid == 0)
    {
    this->SendFirstPoints();
    }
  else
    {
    this->ReceiveLastPoints();
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
