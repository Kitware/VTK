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

#ifndef USE_VTK_COSMO
#define USE_VTK_COSMO
#endif

#include "vtkPCosmoReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkSmartPointer.h"
#include "vtkDummyController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkFloatArray.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkDataObject.h"
#include "vtkStdString.h"
#include "vtkCellArray.h"

#include <vector>

using namespace std;

// RRU stuff
#include "CosmoDefinition.h"
#include "Partition.h"
#include "ParticleExchange.h"
#include "ParticleDistribute.h"

vtkStandardNewMacro(vtkPCosmoReader);

//----------------------------------------------------------------------------
vtkPCosmoReader::vtkPCosmoReader()
{
  this->SetNumberOfInputPorts(0);

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if(!this->Controller)
    {
      this->SetController(vtkSmartPointer<vtkDummyController>::New());
    }

  this->FileName = NULL;
  this->RL = 100;
  this->Overlap = 5;
  this->ReadMode = 1;
  this->CosmoFormat = 1;
}

//----------------------------------------------------------------------------
vtkPCosmoReader::~vtkPCosmoReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }

  this->SetController(0);
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

  os << indent << "FileName: " << (this->FileName != NULL ? this->FileName : "") << endl;
  os << indent << "rL: " << this->RL << endl;
  os << indent << "Overlap: " << this->Overlap << endl;
  os << indent << "ReadMode: " << this->ReadMode << endl;
  os << indent << "CosmoFormat: " << this->CosmoFormat << endl;
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

vtkMultiProcessController* vtkPCosmoReader::GetController()
{
  return (vtkMultiProcessController*)this->Controller;
}

//----------------------------------------------------------------------------
int vtkPCosmoReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // set the pieces as the number of processes
  outputVector->GetInformationObject(0)->Set
    (vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
     this->Controller->GetNumberOfProcesses());

  outputVector->GetInformationObject(0)->Set
    (vtkDataObject::DATA_NUMBER_OF_PIECES(),
     this->Controller->GetNumberOfProcesses());

  // set the ghost levels
  outputVector->GetInformationObject(0)->Set
    (vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPCosmoReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

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

   if (this->FileName == NULL || this->FileName == '\0')
    {
    vtkErrorMacro(<< "No FileName specified!");
    return 0;
    }

  // RRU code
  // Initialize the partitioner which uses MPI Cartesian Topology
  Partition::initialize();

  // Construct the particle distributor, exchanger and halo finder
  ParticleDistribute distribute;
  ParticleExchange exchange;

  // Initialize classes for reading, exchanging and calculating
  if(this->CosmoFormat)
    {
    distribute.setParameters(this->FileName, this->RL, "RECORD");
    }
  else
    {
    distribute.setParameters(this->FileName, this->RL, "BLOCK");
    }
  exchange.setParameters(this->RL, this->Overlap);

  distribute.initialize();
  exchange.initialize();

  // Read alive particles only from files
  // In ROUND_ROBIN all files are read and particles are passed round robin
  // to every other processor so that every processor chooses its own
  // In ONE_TO_ONE every processor reads its own processor in the topology
  // which has already been populated with the correct alive particles
  vector<POSVEL_T>* xx = new vector<POSVEL_T>;
  vector<POSVEL_T>* yy = new vector<POSVEL_T>;
  vector<POSVEL_T>* zz = new vector<POSVEL_T>;
  vector<POSVEL_T>* vx = new vector<POSVEL_T>;
  vector<POSVEL_T>* vy = new vector<POSVEL_T>;
  vector<POSVEL_T>* vz = new vector<POSVEL_T>;
  vector<POSVEL_T>* mass = new vector<POSVEL_T>;
  vector<ID_T>* tag = new vector<ID_T>;
  vector<STATUS_T>* status = new vector<STATUS_T>;

  distribute.setParticles(xx, yy, zz, vx, vy, vz, mass, tag);
  if(this->ReadMode)
    {
    distribute.readParticlesRoundRobin();
    }
  else
    {
    distribute.readParticlesOneToOne();
    }

  // Create the mask and potential vectors which will be filled in elsewhere
  int numberOfParticles = (int)xx->size();
  vector<POTENTIAL_T>* potential = new vector<POTENTIAL_T>(numberOfParticles);
  vector<MASK_T>* mask = new vector<MASK_T>(numberOfParticles);

  // Exchange particles adds dead particles to all the vectors
  exchange.setParticles(xx, yy, zz, vx, vy, vz, mass, potential, tag,
                        mask, status);
  exchange.exchangeParticles();

  // create VTK structures
  numberOfParticles = (int)xx->size();
  potential->clear();
  mask->clear();

  vtkPoints* points = vtkPoints::New();
  points->SetDataTypeToFloat();
  points->Allocate(numberOfParticles);
  vtkCellArray* cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(numberOfParticles, 1));

  vtkFloatArray* vel = vtkFloatArray::New();
  vel->SetName("velocity");
  vel->SetNumberOfComponents(DIMENSION);
  vel->Allocate(numberOfParticles);
  vtkFloatArray* m = vtkFloatArray::New();
  m->SetName("mass");
  m->Allocate(numberOfParticles);
  vtkIntArray* uid = vtkIntArray::New();
  uid->SetName("tag");
  uid->Allocate(numberOfParticles);
  vtkIntArray* owner = vtkIntArray::New();
  owner->SetName("ghost");
  owner->Allocate(numberOfParticles);
  vtkUnsignedCharArray* ghost = vtkUnsignedCharArray::New();
  ghost->SetName("vtkGhostLevels");
  ghost->Allocate(numberOfParticles);

  // put it into the correct VTK structure
  for(vtkIdType i = 0; i < numberOfParticles; i = i + 1)
    {
    float pt[DIMENSION];

    // insert point and cell
    pt[0] = xx->back();
    xx->pop_back();
    pt[1] = yy->back();
    yy->pop_back();
    pt[2] = zz->back();
    zz->pop_back();

    vtkIdType pid = points->InsertNextPoint(pt);
    cells->InsertNextCell(1, &pid);

    // insert velocity
    pt[0] = vx->back();
    vx->pop_back();
    pt[1] = vy->back();
    vy->pop_back();
    pt[2] = vz->back();
    vz->pop_back();

    vel->InsertNextTuple(pt);

    // insert mass
    pt[0] = mass->back();
    mass->pop_back();

    m->InsertNextValue(pt[0]);

    // insert tag
    int particle = tag->back();
    tag->pop_back();

    uid->InsertNextValue(particle);

    // insert ghost status
    int neighbor = status->back();
    unsigned char level = neighbor < 0 ? 0 : 1;
    status->pop_back();

    owner->InsertNextValue(neighbor);
    ghost->InsertNextValue(level);
    }

  // cleanup
  output->SetPoints(points);
  output->SetCells(1, cells);
  output->GetPointData()->AddArray(vel);
  output->GetPointData()->AddArray(m);
  output->GetPointData()->AddArray(uid);
  output->GetPointData()->AddArray(owner);
  output->GetPointData()->AddArray(ghost);

  output->Squeeze();

  points->Delete();
  cells->Delete();
  vel->Delete();
  m->Delete();
  uid->Delete();
  owner->Delete();
  ghost->Delete();

  delete xx;
  delete yy;
  delete zz;
  delete vx;
  delete vy;
  delete vz;
  delete mass;
  delete tag;
  delete status;
  delete potential;
  delete mask;

  return 1;
}
