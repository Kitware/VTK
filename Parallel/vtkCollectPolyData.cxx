/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectPolyData.cxx
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
#include "vtkCollectPolyData.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkSocketController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkCollectPolyData, "1.12");
vtkStandardNewMacro(vtkCollectPolyData);

vtkCxxSetObjectMacro(vtkCollectPolyData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkCollectPolyData,SocketController, vtkSocketController);

//----------------------------------------------------------------------------
vtkCollectPolyData::vtkCollectPolyData()
{
  this->MemorySize = 0;
  this->Threshold = 1000;
  this->SocketController = NULL;

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
  this->Collected = 0;
}

//----------------------------------------------------------------------------
vtkCollectPolyData::~vtkCollectPolyData()
{
  this->SetController(0);
  this->SetSocketController(0);
}

//----------------------------------------------------------------------------
void vtkCollectPolyData::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//--------------------------------------------------------------------------
void vtkCollectPolyData::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkPolyData *input = this->GetInput();
  int piece = output->GetUpdatePiece();
  int numPieces = output->GetUpdateNumberOfPieces();
  int ghostLevel = output->GetUpdateGhostLevel();

  if (input == NULL)
    {
    return;
    }
  input->SetUpdatePiece(piece);
  input->SetUpdateNumberOfPieces(numPieces);
  input->SetUpdateGhostLevel(ghostLevel);
}

  
//----------------------------------------------------------------------------
void vtkCollectPolyData::ExecuteData(vtkDataObject*)
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  unsigned long size;
  unsigned long tmp=0;
  int numProcs, myId;
  int idx;

  if (input == NULL)
    {
    vtkErrorMacro("Input has not been set.");
    return;
    }

  if (this->Controller == NULL && this->SocketController == NULL)
    { // Running as a single process.
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    this->Collected = 1;
    return;
    }

  if (this->Controller == NULL && this->SocketController != NULL)
    { // This is a client.  We assume no data on client for input.
    // How large will the data be if it is collected.
    this->SocketController->Receive(&size, 1, 1, 839823);
    // Save for external use.
    this->MemorySize = size;
    if (size > this->Threshold)
      {
      this->Collected = 0;
      }
    else
      {
      this->Collected = 1;
      }
    // Communicate descision to all processes.
    this->SocketController->Send(&this->Collected, 1, 1, 839824);
    if (this->Collected)
      {
      vtkPolyData *pd = NULL;;
      pd = vtkPolyData::New();
      this->SocketController->Receive(pd, 1, 121767);
      output->CopyStructure(pd);
      output->GetPointData()->PassData(pd->GetPointData());
      output->GetCellData()->PassData(pd->GetCellData());
      pd->Delete();
      pd = NULL;
      }
    // If not collected, output will be empty from initialization.
    return;
    }
  
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();
  // How large will the data be if it is collected.
  size = input->GetActualMemorySize();
  if (myId == 0)
    {
    for (idx = 1; idx < numProcs; ++idx)
      {
      //cerr << "Receive size.\n";
      this->Controller->Receive(&tmp, 1, idx, 839823);
      size += tmp;
      }
    // Save for external use.
    this->MemorySize = size;
    // If there is a client, then send the size to it.
    if (this->SocketController)
      {
      this->SocketController->Send(&size, 1, 1, 839823);
      this->SocketController->Receive(&this->Collected, 1, 1, 839824);
      }
    else
      {
      // We make the collection decision here.      
      if (size > this->Threshold)
        {
        this->Collected = 0;
        }
      else
        {
        this->Collected = 1;
        }
      }
    // Communicate descision to all processes.
    for (idx = 1; idx < numProcs; ++idx)
      {
      //cerr << "Sending collection descision" << this->Collected << endl;
      this->Controller->Send(&this->Collected, 1, idx, 839824);
      }
    }
  else
    { // Satellite on server.
    //cerr << "Sending size" << size << endl;
    this->Controller->Send(&size, 1, 0, 839823);
    //cerr << "Receive collection decision.\n";
    this->Controller->Receive(&this->Collected, 1, 0, 839824);
    }

  if ( ! this->Collected)
    {
    // Just copy and return (no collection).
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return;
    }

  // Collect.
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  vtkPolyData *pd = NULL;;

  if (myId == 0)
    {
    pd = vtkPolyData::New();
    pd->CopyStructure(input);
    pd->GetPointData()->PassData(input->GetPointData());
    pd->GetCellData()->PassData(input->GetCellData());
    append->AddInput(pd);
    pd->Delete();
    for (idx = 1; idx < numProcs; ++idx)
      {
      pd = vtkPolyData::New();
      this->Controller->Receive(pd, idx, 121767);
      append->AddInput(pd);
      pd->Delete();
      pd = NULL;
      }
    append->Update();
    input = append->GetOutput();
    if (this->SocketController)
      { // Send collected data onto client.
      this->SocketController->Send(input, 1, 121767);
      // output will be empty.
      }
    else
      { // No client. Keep the output here.
      output->CopyStructure(input);
      output->GetPointData()->PassData(input->GetPointData());
      output->GetCellData()->PassData(input->GetCellData());
      }
    append->Delete();
    append = NULL;
    }
  else
    {
    this->Controller->Send(input, 0, 121767);
    append->Delete();
    append = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkCollectPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "MemorySize: " << this->MemorySize << endl;
  os << indent << "Threshold: " << this->Threshold << "\n";
  os << indent << "Collected: " << this->Collected << "\n";
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketController: (" << this->SocketController << ")\n";
}

