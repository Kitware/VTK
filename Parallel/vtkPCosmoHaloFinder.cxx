/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCosmoHaloFinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPCosmoHaloFinder.cxx

Copyright (c) 2007, 2009, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC. 
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

#include "vtkPCosmoHaloFinder.h"

#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkSmartPointer.h"
#include "vtkDummyController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDemandDrivenPipeline.h"

#include "CosmoHaloFinderP.h"
#include "CosmoDefinition.h"
#include "FOFHaloProperties.h"
#include "Partition.h"

vtkStandardNewMacro(vtkPCosmoHaloFinder);

/****************************************************************************/
vtkPCosmoHaloFinder::vtkPCosmoHaloFinder()
{
  this->SetNumberOfOutputPorts(2);

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if(!this->Controller)
    {
      this->SetController(vtkSmartPointer<vtkDummyController>::New());
    }

  this->NP = 256;
  this->RL = 90.140846;
  this->Overlap = 5;
  this->BB = .2;
  this->PMin = 10;
  this->ParticleMass = 1;
  this->CopyHaloDataToParticles = 1;
}

/****************************************************************************/
vtkPCosmoHaloFinder::~vtkPCosmoHaloFinder()
{
  this->SetController(0);
}

/****************************************************************************/

void vtkPCosmoHaloFinder::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "NP: " << this->NP << endl;
  os << indent << "rL: " << this->RL << endl;
  os << indent << "Overlap: " << this->Overlap << endl;
  os << indent << "bb: " << this->BB << endl;
  os << indent << "pmin: " << this->PMin << endl;
  os << indent << "ParticleMass: " << this->ParticleMass << endl;
  os << indent << "CopyHaloDataToParticles: " << this->CopyHaloDataToParticles << endl;
}

//----------------------------------------------------------------------------
void vtkPCosmoHaloFinder::SetController(vtkMultiProcessController *c)
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

vtkMultiProcessController* vtkPCosmoHaloFinder::GetController()
{
  return (vtkMultiProcessController*)this->Controller;
}

//----------------------------------------------------------------------------
int vtkPCosmoHaloFinder::RequestInformation
(vtkInformation* vtkNotUsed(request),
 vtkInformationVector** inputVector,
 vtkInformationVector* outputVector)
{
#ifndef USE_SERIAL_COSMO
  // check for controller
  if(!this->Controller) 
    {
    vtkErrorMacro(<< "Unable to work without a Controller.");
    return 0;
    }
#endif

  // set the other outputs to have the same number of pieces
  if((*inputVector)->GetInformationObject(0)->Has(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()))
    {
    if(outputVector->GetInformationObject(1)->Has(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()))
      {
      if(outputVector->GetInformationObject(0)->Get
         (vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()) !=
         outputVector->GetInformationObject(1)->Get
         (vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()))
        {
        outputVector->GetInformationObject(1)->Set
          (vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
           outputVector->GetInformationObject(0)->Get
           (vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));
        }
      }
    else
      {
      outputVector->GetInformationObject(1)->Set
        (vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
         outputVector->GetInformationObject(0)->Get
         (vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPCosmoHaloFinder::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  int rno = request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT()); 

  // get the info objects
  vtkInformation* inInfo = (*inputVector)->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* catInfo = outputVector->GetInformationObject(1);
                                                                        
  // get the input and output
  vtkUnstructuredGrid* input = vtkUnstructuredGrid::SafeDownCast
    (inInfo->Get(vtkDataObject::DATA_OBJECT()));
                                                                 
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast
    (outInfo->Get(vtkDataObject::DATA_OBJECT()));  

  vtkUnstructuredGrid* catalog = vtkUnstructuredGrid::SafeDownCast
    (catInfo->Get(vtkDataObject::DATA_OBJECT()));  

  // check that the piece number is correct
  int updatePiece = 0;
  int updateTotal = 1;
  if(rno == 0) 
    {
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
    }
  else if(rno == 1)
    {
    if(catInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
      {
      updatePiece = catInfo->
        Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
      }
    if(catInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
      {
      updateTotal = catInfo->
        Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
      }
    }

  // shallow total point input to output
  output->ShallowCopy(input);

  // RRU code
  // Initialize the partitioner which uses MPI Cartesian Topology
  Partition::initialize();

  // create the halo finder
  CosmoHaloFinderP haloFinder;

  haloFinder.setParameters
    ("", this->RL, this->Overlap, this->NP, this->PMin, this->BB);

  // halo finder needs vectors so take the time to turn them into vectors
  // FIXME: ought to go into the halo finder and put some #ifdefs
  // so that it can use vtkDataArray as is - ought to do that with the
  // reader as well
  if(!output->GetPointData()->HasArray("velocity") ||
     !output->GetPointData()->HasArray("tag") ||
     !output->GetPointData()->HasArray("ghost"))
    {
    vtkErrorMacro(<< "The input data does not have one or more of" <<
                  "the following point arrays: velocity, tag, or ghost.");
    return 0;
    }

  vtkPoints* points = output->GetPoints();
  vtkFloatArray* velocity = vtkFloatArray::SafeDownCast
    (output->GetPointData()->GetArray("velocity"));
  vtkIntArray* uid = vtkIntArray::SafeDownCast
    (output->GetPointData()->GetArray("tag"));
  vtkIntArray* owner = vtkIntArray::SafeDownCast
    (output->GetPointData()->GetArray("ghost"));

  if(velocity == 0 || uid == 0 || owner == 0 || 
     velocity->GetNumberOfComponents() != DIMENSION) 
    {
    vtkErrorMacro(<< "One or more of the input point data arrays is" <<
                  "malformed: velocity, tag, or ghost.");
    return 0;
    }

  // create the empty ones
  vtkIdType numberOfLocalPoints = output->GetPoints()->GetNumberOfPoints();
  vector<POTENTIAL_T>* potential = new vector<POTENTIAL_T>(numberOfLocalPoints);
  vector<MASK_T>* mask = new vector<MASK_T>(numberOfLocalPoints);

  // fill in the non empty ones
  vector<POSVEL_T>* xx = new vector<POSVEL_T>;
  vector<POSVEL_T>* yy = new vector<POSVEL_T>;
  vector<POSVEL_T>* zz = new vector<POSVEL_T>;
  vector<POSVEL_T>* vx = new vector<POSVEL_T>;
  vector<POSVEL_T>* vy = new vector<POSVEL_T>;
  vector<POSVEL_T>* vz = new vector<POSVEL_T>;
  vector<ID_T>* tag = new vector<ID_T>;
  vector<STATUS_T>* status = new vector<STATUS_T>;

  for(int i = 0; i < numberOfLocalPoints; i = i + 1) 
    {
    // get and set the point
    double pt[DIMENSION];

    points->GetPoint(i, pt);
    xx->push_back((float)pt[0]);
    yy->push_back((float)pt[1]);
    zz->push_back((float)pt[2]);

    // get and set the velocity
    float vel[DIMENSION];
    
    velocity->GetTupleValue(i, vel);
    vx->push_back(vel[0]);
    vy->push_back(vel[1]);
    vz->push_back(vel[2]);

    // get and set the tag
    int particle = uid->GetValue(i);
    tag->push_back(particle);

    // get and set the status
    int neighbor = owner->GetValue(i);
    status->push_back(neighbor);
    }

  // delete owner/status because it was only needed for halo finding
  output->GetPointData()->RemoveArray("ghost");

  ////////////////////////////////////////////////////////////////////////////
  //
  // Run halo finder
  // Collect the serial halo finder results
  // Merge the halos so that only one copy of each is written
  // Parallel halo finder must consult with each of the 26 possible neighbor
  // halo finders to see who will report a particular halo
  haloFinder.setParticles(xx, yy, zz, vx, vy, vz, potential, tag, mask, status);
  haloFinder.executeHaloFinder();
  haloFinder.collectHalos();
  haloFinder.mergeHalos();

  // adjust ghost cells, because halo finder updates it
  vtkUnsignedCharArray* newghost = vtkUnsignedCharArray::New();
  newghost->SetNumberOfValues(numberOfLocalPoints);
  newghost->SetName("vtkGhostLevels");

  for(int i = 0; i < numberOfLocalPoints; i = i + 1)
    {
    unsigned char level = (*status)[i] < 0 ? 0 : 1;
    newghost->SetValue(i, level);
    }

  ////////////////////////////////////////////////////////////////////////////
  //
  // Collect information from the halo finder needed for halo properties
  // Vector halos is the index of the first particle for halo in the haloList
  // Following the chain of indices in the haloList retrieves all particles
  int numberOfFOFHalos = haloFinder.getNumberOfHalos();
  int* fofHalos = haloFinder.getHalos();
  int* fofHaloCount = haloFinder.getHaloCount();
  int* fofHaloList = haloFinder.getHaloList();
  int* fofHaloTags = new int[numberOfFOFHalos];

  FOFHaloProperties fof;
  fof.setHalos(numberOfFOFHalos, fofHalos, fofHaloCount, fofHaloList);
  fof.setParameters("", this->RL, this->Overlap, this->ParticleMass, this->BB);
  fof.setParticles(xx, yy, zz, vx, vy, vz, potential, tag, mask, status);

  // Find the average position of every FOF halo
  vector<POSVEL_T>* fofXPos = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofYPos = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofZPos = new vector<POSVEL_T>;
  fof.FOFPosition(fofXPos, fofYPos, fofZPos);

  // Find the mass of every FOF halo
  vector<POSVEL_T>* fofMass = new vector<POSVEL_T>;
  fof.FOFHaloMass(fofMass);

  // Find the average velocity of every FOF halo
  vector<POSVEL_T>* fofXVel = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofYVel = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofZVel = new vector<POSVEL_T>;
  fof.FOFVelocity(fofXVel, fofYVel, fofZVel);

  // Find the velocity dispersion of every FOF halo
  vector<POSVEL_T>* fofVelDisp = new vector<POSVEL_T>;
  fof.FOFVelocityDispersion(fofXVel, fofYVel, fofZVel, fofVelDisp);

  // set the tags to -1
  for(int i = 0; i < numberOfFOFHalos; i = i + 1)
    {
    fofHaloTags[i] = -1;
    }
  
  // walk the list to get the lowest tag id
  int pminHalos = 0;
  for(int i = 0; i < numberOfFOFHalos; i = i + 1)
    {
    int size = fofHaloCount[i];
    
    if(size >= this->PMin) 
      {
      pminHalos = pminHalos + 1;
      int index = fofHalos[i];
      for(int j = 0; j < size; j = j + 1) 
        {
        if(fofHaloTags[i] == -1 || fofHaloTags[i] > (*tag)[index])
          {
          fofHaloTags[i] = (*tag)[index];
          }

        index = fofHaloList[index];
        }
      }
    }

  // walk the list again to set the values for the points and catalog
  vtkIntArray* partTag = 0;
  vtkFloatArray* partPotential = 0;
  vtkFloatArray* partVelocity = 0;
  vtkFloatArray* partDispersion = 0;
  if(this->CopyHaloDataToParticles)
    {
    partTag = vtkIntArray::New();
    partTag->SetName("halo_tag");
    partTag->SetNumberOfValues(numberOfLocalPoints);
    partTag->FillComponent(0, -1);
    
    partPotential = vtkFloatArray::New();
    partPotential->SetName("halo_mass");
    partPotential->SetNumberOfValues(numberOfLocalPoints);
    partPotential->FillComponent(0, 0);
    
    partVelocity = vtkFloatArray::New();
    partVelocity->SetName("average_velocity");
    partVelocity->SetNumberOfComponents(3);
    partVelocity->SetNumberOfTuples(numberOfLocalPoints);
    partVelocity->FillComponent(0, 0);
    partVelocity->FillComponent(1, 0);
    partVelocity->FillComponent(2, 0);
    
    partDispersion = vtkFloatArray::New();
    partDispersion->SetName("velocity_dispersion");
    partDispersion->SetNumberOfValues(numberOfLocalPoints);
    partDispersion->FillComponent(0, 0);
    }

  vtkPoints* catpoints = vtkPoints::New();
  catpoints->SetDataTypeToFloat();
  catalog->Allocate(pminHalos);
  catalog->SetPoints(catpoints);
  
  vtkIntArray* haloTag = vtkIntArray::New();
  haloTag->SetName("halo_tag");
  haloTag->SetNumberOfValues(pminHalos);

  vtkFloatArray* haloPotential = vtkFloatArray::New();
  haloPotential->SetName("halo_mass");
  haloPotential->SetNumberOfValues(pminHalos);

  vtkFloatArray* haloVelocity = vtkFloatArray::New();
  haloVelocity->SetName("average_velocity");
  haloVelocity->SetNumberOfComponents(3);
  haloVelocity->SetNumberOfTuples(pminHalos);

  vtkFloatArray* haloDispersion = vtkFloatArray::New();
  haloDispersion->SetName("velocity_dispersion");
  haloDispersion->SetNumberOfValues(pminHalos);

  for(int i = 0; i < numberOfFOFHalos; i = i + 1)
    {
    int size = fofHaloCount[i];

    if(size >= this->PMin) 
      {
      // set the catalog and average point
      vtkIdType pid;
      pid = catpoints->InsertNextPoint
        ((*fofXPos)[i], (*fofYPos)[i], (*fofZPos)[i]);
      catalog->InsertNextCell(1, 1, &pid);

      // set the halo data
      haloTag->SetValue(i, fofHaloTags[i]);
      haloPotential->SetValue(i, (*fofMass)[i]);
      haloVelocity->SetComponent(i, 0, (*fofXVel)[i]);
      haloVelocity->SetComponent(i, 1, (*fofYVel)[i]);
      haloVelocity->SetComponent(i, 2, (*fofZVel)[i]);
      haloDispersion->SetValue(i, (*fofVelDisp)[i]);

      // set the halo data for the original points
      if(this->CopyHaloDataToParticles)
        {
        int index = fofHalos[i];
        for(int j = 0; j < size; j = j + 1) 
          {
          partTag->SetValue(index, fofHaloTags[i]); 
          partPotential->SetValue(index, (*fofMass)[i]);
          partVelocity->SetComponent(index, 0, (*fofXVel)[i]);
          partVelocity->SetComponent(index, 1, (*fofYVel)[i]);
          partVelocity->SetComponent(index, 2, (*fofZVel)[i]);
          partDispersion->SetValue(index, (*fofVelDisp)[i]);
          
          index = fofHaloList[index];
          }
        }
      }
    }

  // set the arrays
  if(this->CopyHaloDataToParticles)
    {
    output->GetPointData()->AddArray(partTag);
    output->GetPointData()->AddArray(partPotential);
    output->GetPointData()->AddArray(partVelocity);
    output->GetPointData()->AddArray(partDispersion);
    }
  output->GetPointData()->AddArray(newghost);

  catalog->GetPointData()->AddArray(haloTag);
  catalog->GetPointData()->AddArray(haloPotential);
  catalog->GetPointData()->AddArray(haloVelocity);
  catalog->GetPointData()->AddArray(haloDispersion);

  // cleanup
  if(this->CopyHaloDataToParticles)
    {
    partTag->Delete();
    partPotential->Delete();
    partVelocity->Delete();
    partDispersion->Delete();
    }
  newghost->Delete();

  catpoints->Delete();
  haloTag->Delete();
  haloPotential->Delete();
  haloVelocity->Delete();
  haloDispersion->Delete();

  delete xx;
  delete yy;
  delete zz;
  delete vx;
  delete vy;
  delete vz;
  delete tag;
  delete status;
  delete potential;
  delete mask;

  delete [] fofHaloTags;
  delete fofXPos;
  delete fofYPos;
  delete fofZPos;
  delete fofMass;
  delete fofXVel;
  delete fofYVel;
  delete fofZVel;
  delete fofVelDisp;

  return 1;
}

