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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCollectPolyData, "1.3");
vtkStandardNewMacro(vtkCollectPolyData);

//----------------------------------------------------------------------------
vtkCollectPolyData::vtkCollectPolyData()
{
  this->Threshold = 1000;

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
  this->Collected = 0;
}

//----------------------------------------------------------------------------
vtkCollectPolyData::~vtkCollectPolyData()
{
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
void vtkCollectPolyData::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int updateGhostLevel = output->GetUpdateGhostLevel();
  unsigned long size, tmp;
  int numProcs, myId;
  int idx;

  if (input == NULL)
    {
    vtkErrorMacro("Input has not been set.");
    return;
    }

  if (this->Controller == NULL)
    {
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    this->Collected = 0;
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
    if (size > this->Threshold)
      {
      this->Collected = 0;
      }
    else
      {
      this->Collected = 1;
      }
    // Communicate descision to all processes.
    for (idx = 1; idx < numProcs; ++idx)
      {
      //cerr << "Sending collection descision" << this->Collected << endl;
      this->Controller->Send(&this->Collected, 1, idx, 839824);
      }
    }
  else
    {
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
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    append->Delete();
    append = NULL;
    return;
    }
  else
    {
    this->Controller->Send(input, 0, 121767);
    }
}


//----------------------------------------------------------------------------
void vtkCollectPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Threshold: " << this->Threshold << "\n";
  os << indent << "Collected: " << this->Collected << "\n";
  os << indent << "Controller: (" << this->Controller << ")\n";
}

