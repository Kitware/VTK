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

vtkCxxRevisionMacro(vtkCollectPolyData, "1.14");
vtkStandardNewMacro(vtkCollectPolyData);

vtkCxxSetObjectMacro(vtkCollectPolyData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkCollectPolyData,SocketController, vtkSocketController);

//----------------------------------------------------------------------------
vtkCollectPolyData::vtkCollectPolyData()
{
  this->PassThrough = 0;
  this->SocketController = NULL;

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
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
  unsigned long size=0;
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
    return;
    }

  if (this->Controller == NULL && this->SocketController != NULL)
    { // This is a client.  We assume no data on client for input.
    if ( ! this->PassThrough)
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

  if (this->PassThrough)
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
  
  os << indent << "PassThough: " << this->PassThrough << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketController: (" << this->SocketController << ")\n";
}

