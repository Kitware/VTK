/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitPolyDataPiece.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkTransmitPolyDataPiece.h"
#include "vtkExtractPolyDataPiece.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkTransmitPolyDataPiece* vtkTransmitPolyDataPiece::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTransmitPolyDataPiece");
  if(ret)
    {
    return (vtkTransmitPolyDataPiece*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTransmitPolyDataPiece;
}

//----------------------------------------------------------------------------
vtkTransmitPolyDataPiece::vtkTransmitPolyDataPiece()
{
  this->CreateGhostCells = 1;

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  

  this->Buffer = vtkPolyData::New();
}

//----------------------------------------------------------------------------
vtkTransmitPolyDataPiece::~vtkTransmitPolyDataPiece()
{
  this->Buffer->Delete();
  this->Buffer = NULL;
}

//----------------------------------------------------------------------------
void vtkTransmitPolyDataPiece::ComputeInputUpdateExtents(vtkDataObject *out)
{
  vtkPolyData *input = this->GetInput();
  
  out = out;
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }

  if (this->Controller == NULL)
    {
    input->SetUpdateNumberOfPieces(1);
    input->SetUpdatePiece(0);
    input->SetUpdateGhostLevel(0);
    return;
    }
  
  if (this->Controller->GetLocalProcessId() == 0)
    { // Request everything.
    input->SetUpdateNumberOfPieces(1);
    input->SetUpdatePiece(0);
    input->SetUpdateGhostLevel(0);
    }
  else
    { // Request nothing.
    input->SetUpdateNumberOfPieces(0);
    input->SetUpdatePiece(0);
    input->SetUpdateGhostLevel(0);
    }
}

//----------------------------------------------------------------------------
void vtkTransmitPolyDataPiece::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}
  
//----------------------------------------------------------------------------
void vtkTransmitPolyDataPiece::Execute()
{
  int procId;
  vtkPolyData *output = this->GetOutput();
  int updateGhostLevel = output->GetUpdateGhostLevel();

  // Just use the buffer if possible.
  if (output->GetPipelineMTime() < this->Buffer->GetMTime()
      && output->GetUpdatePiece() == this->Buffer->GetPiece()
      && output->GetUpdateNumberOfPieces() == this->Buffer->GetNumberOfPieces()
      && updateGhostLevel <= this->Buffer->GetUpdateGhostLevel())
    {
    // We deep copy, because we do not want to modify the buffer 
    // when we remove ghost cells from the output.
    output->DeepCopy(this->Buffer);
    if (updateGhostLevel < this->Buffer->GetUpdateGhostLevel())
      {
      output->RemoveGhostCells(updateGhostLevel+1);
      }
    return;
    }

  if (this->Controller == NULL)
    {
    vtkErrorMacro("Could not find Controller.");
    return;
    }

  procId = this->Controller->GetLocalProcessId();
  if (procId == 0)
    {
    // It is important to synchronize these calls (all processes execute)
    cerr << "Root Execute\n";
    this->RootExecute();
    }
  else
    {
    cerr << "Satellite Execute " << procId << endl;
    this->SatelliteExecute(procId);
    }

  // Save the output in the buffer.
  this->Buffer->ShallowCopy(output);
}

//----------------------------------------------------------------------------
void vtkTransmitPolyDataPiece::RootExecute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *tmp = vtkPolyData::New();
  vtkPolyData *output = this->GetOutput();
  vtkExtractPolyDataPiece *extract = vtkExtractPolyDataPiece::New();
  int ext[3];
  int numProcs, i;


  if (output->GetUpdatePiece() != 0)
    {
    vtkWarningMacro(<< "Piece " << output->GetUpdatePiece() 
                    << " does not match process 0.  " 
                    << "Altering request to try to avoid a deadlock.");
    }

  // First, set up the pipeline and handle local request.
  tmp->ShallowCopy(input);
  tmp->SetReleaseDataFlag(0);
  extract->SetCreateGhostCells(this->CreateGhostCells);
  extract->SetInput(tmp);
  extract->GetOutput()->SetUpdateNumberOfPieces(
				output->GetUpdateNumberOfPieces());
  extract->GetOutput()->SetUpdatePiece(0);
  extract->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());

  extract->Update();
  // Copy geometry without copying information.
  output->CopyStructure(extract->GetOutput());
  output->GetPointData()->PassData(extract->GetOutput()->GetPointData());
  output->GetCellData()->PassData(extract->GetOutput()->GetCellData());

  // Now do each of the satellite requests.
  numProcs = this->Controller->GetNumberOfProcesses();
  // If less pieces are requested, exclude some processes.
  if (output->GetUpdateNumberOfPieces() < numProcs)
    {
    numProcs = output->GetUpdateNumberOfPieces();
    }
  for (i = 1; i < numProcs; ++i)
    {
    this->Controller->Receive(ext, 3, i, 22341);
    if (ext[0] != i)
      {
      vtkWarningMacro(<< "Piece " << ext[0] 
                      << " does not match process " << i << ".  " 
                      << "Altering request to try to avoid a deadlock.");
      ext[0] = i;
      }
    if (ext[1] != output->GetUpdateNumberOfPieces())
      {
      vtkWarningMacro("Number of pieces mismatch between processes.");
      }
    extract->GetOutput()->SetUpdateNumberOfPieces(ext[1]);
    extract->GetOutput()->SetUpdatePiece(ext[0]);
    extract->GetOutput()->SetUpdateGhostLevel(ext[2]);
    extract->Update();
    this->Controller->Send(extract->GetOutput(), i, 22342);
    }
  tmp->Delete();
  extract->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitPolyDataPiece::SatelliteExecute(int procId)
{
  vtkPolyData *tmp = vtkPolyData::New();
  vtkPolyData *output = this->GetOutput();
  int ext[3];

  ext[0] = output->GetUpdatePiece();
  ext[1] = output->GetUpdateNumberOfPieces();
  ext[2] = output->GetUpdateGhostLevel();

  if (procId > ext[1])
    {
    vtkWarningMacro("Ignoring request " << ext[0] << " of " << ext[1]
                    << " in process " << procId 
                    << ". Trying to avoid deadlock.");
    return;
    }

  this->Controller->Send(ext, 3, 0, 22341);
  this->Controller->Receive(tmp, 0, 22342);

  // Copy geometry without copying information.
  output->CopyStructure(tmp);
  output->GetPointData()->PassData(tmp->GetPointData());
  output->GetCellData()->PassData(tmp->GetCellData());

  tmp->Delete();
}

//----------------------------------------------------------------------------
void vtkTransmitPolyDataPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
  
  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");
  
  os << indent << "Controller: (" << this->Controller << ")\n";

}

