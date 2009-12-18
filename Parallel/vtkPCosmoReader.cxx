/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCosmoReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPCosmoReader.cxx

Copyright (c) 2009 Los Alamos National Security, LLC

All rights reserved.

Copyright 2009. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkPCosmoReader.h"
#include "vtkDataArraySelection.h"
#include "vtkErrorCode.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkFieldData.h"
#include "vtkPointData.h"
#include "vtkByteSwap.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkDataArray.h"
#include "vtkConfigure.h"
#include "vtkStdString.h"
#include "vtkMultiProcessController.h"
#include "vtkSmartPointer.h"
#include "vtkDummyController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCommunicator.h"

vtkCxxRevisionMacro(vtkPCosmoReader, "1.4");
vtkStandardNewMacro(vtkPCosmoReader);

using namespace cosmo;

//----------------------------------------------------------------------------
vtkPCosmoReader::vtkPCosmoReader()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if(!this->Controller)
    {
      this->SetController(vtkSmartPointer<vtkDummyController>::New());
    }

  this->TakeTurns = 0;
}

//----------------------------------------------------------------------------
vtkPCosmoReader::~vtkPCosmoReader()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkPCosmoReader::SetController(vtkMultiProcessController *c)
{
  if(this->Controller == c)
    {
    return;
    }

  this->Modified();

  if(this->Controller != 0)
    {
    this->Controller->UnRegister(this);
    this->Controller = 0;
    }

  if(c == 0)
    {
    return;
    }

  this->Controller = c;
  c->Register(this);
}

//----------------------------------------------------------------------------
void vtkPCosmoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Controller)
    {
    os << indent << "Controller: " << this->Controller << endl;
    }
  else
    {
    os << indent << "Controller: (null)\n";
    }
}

//----------------------------------------------------------------------------
int vtkPCosmoReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // check for controller
  if(!this->Controller) 
    {
    vtkErrorMacro(<< "Unable to work without a Controller.");
    return 0;
    }

  // All verify that file exists
  if ( !this->FileName )
    {
    vtkErrorMacro(<< "No filename specified");
    return 0;
    }

#if defined(_WIN32) && !defined(__CYGWIN__)
    this->FileStream = new ifstream(this->FileName, ios::in | ios::binary);
#else
    this->FileStream = new ifstream(this->FileName, ios::in);
#endif

  // All verify can the file be opened
  if (this->FileStream->fail())
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    delete this->FileStream;
    this->FileStream = NULL;
    vtkErrorMacro(<< "Specified filename not found");
    return 0;
    }
                                                   
  // Calculates the number of particles based on record size
  this->ComputeDefaultRange();

  // Fields associated with each particle point: velocity, mass, tag
  this->NumberOfVariables = NUMBER_OF_VAR;

  this->VariableName[0] = "velocity";
  this->ComponentNumber[0] = DIMENSION; // x, y, z velocities

  this->VariableName[1] = "mass";
  this->ComponentNumber[1] = 1;         // mass of particle

  this->VariableName[2] = "tag";
  this->ComponentNumber[2] = 1;         // tag id of particle
                                                                                
  // Add scalar arrays for each field to both points and cells
  for (int i = 0; i < this->NumberOfVariables; i++)
    this->PointDataArraySelection->AddArray(this->VariableName[i].c_str());

  // set the pieces as the number of processes
  outputVector->GetInformationObject(0)->Set
    (vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
     this->Controller->GetNumberOfProcesses());

  // debug information
  vtkDebugMacro( << "RequestInformation: NumberOfNodes = "
                 << this->NumberOfNodes  << endl);
  vtkDebugMacro( << "end of RequestInformation\n");

  delete this->FileStream;
  this->FileStream = 0;

  return 1;
}

//----------------------------------------------------------------------------
int vtkPCosmoReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  int rank = this->Controller->GetLocalProcessId();
  int size = this->Controller->GetNumberOfProcesses();

  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
                                                                                
  // get the output
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
                                                                                
  vtkDebugMacro( << "Reading Cosmo file");
                              
  // check that the piece number is correct
  int updatePiece = 0;
  int updateTotal = 1;
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
      updatePiece = outInfo->
        Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
      updateTotal = outInfo->
        Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }

  if(updatePiece != this->Controller->GetLocalProcessId() ||
     updateTotal != this->Controller->GetNumberOfProcesses())
    {
      vtkErrorMacro(<< "Piece number does not match process number.");
      return 0;
    }

  // Read the file into the output unstructured grid
  if(this->TakeTurns)
    {
    for(int i = 0; i < size; i = i + 1) 
      {
      if(i == rank) 
        {
        this->ReadFile(output);
        }

      // wait for everyone to sync
      this->Controller->Barrier();
      }
    }
  else 
    {
    this->ReadFile(output);
    }

  return 1;
}

//----------------------------------------------------------------------------
// Sets the range of particle indices based on length of file
void vtkPCosmoReader::ComputeDefaultRange()
{
  // figure out how to partition it
  int rank = this->Controller->GetLocalProcessId();
  int size = this->Controller->GetNumberOfProcesses();

  int readproc = this->ReadProcessors;
  readproc = readproc < 1 ? size : (readproc > size ? size : readproc);

  // just have rank 0 read the length
  size_t fileLength;
  if(rank == 0) 
    {
      this->FileStream->seekg(0L, ios::end);
      fileLength = this->FileStream->tellg();
    }

  // communicate the length to everyone
  Controller->Broadcast((char*)&fileLength, sizeof(size_t), 0);
  
  size_t tagBytes;
  if(this->TagSize)
    {
    tagBytes = sizeof(vtkTypeInt64);
    }
  else 
    {
    tagBytes = sizeof(vtkTypeInt32);
    }

  // Divide by number of components per record (x,xv,y,yv,z,zv,mass,tag)
  // Divide by 4 for single precision float
  this->NumberOfNodes = fileLength / (BYTES_PER_DATA_MINUS_TAG + tagBytes);

  // figure out the range on this processor
  if(rank < readproc) 
    {
    this->PositionRange[0] = rank * this->NumberOfNodes / readproc;
    this->PositionRange[1] = (rank + 1) * this->NumberOfNodes / readproc - 1;
    }
  else 
    {
    // read nothing
    this->PositionRange[0] = 1;
    this->PositionRange[1] = 0;
    }
}

