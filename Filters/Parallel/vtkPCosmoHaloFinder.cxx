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

#ifndef USE_VTK_COSMO
#define USE_VTK_COSMO
#endif

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
#include "HaloCenterFinder.h"
#include "Partition.h"
#include "ChainingMesh.h"
#include "SODHalo.h"

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
  this->RL = 100;
  this->Overlap = 5;
  this->BB = .2;
  this->PMin = 100;
  this->CopyHaloDataToParticles = 0;

  this->ComputeMostBoundParticle = 0;
  this->ComputeMostConnectedParticle = 0;

  this->ComputeSOD = 0;
  this->SODCenterType = 0;

  this->RhoC = RHO_C;
  this->SODMass = SOD_MASS;
  this->MinRadiusFactor = MIN_RADIUS_FACTOR;
  this->MaxRadiusFactor = MAX_RADIUS_FACTOR;
  this->SODBins = NUM_SOD_BINS;
  this->MinFOFSize = MIN_SOD_SIZE;
  this->MinFOFMass = MIN_SOD_MASS;
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
  os << indent << "CopyHaloDataToParticles: " << this->CopyHaloDataToParticles
     << endl;
  os << indent << "ComputeMostBoundParticle: " << this->ComputeMostBoundParticle << endl;
  os << indent << "ComputeMostConnectedParticle: " << this->ComputeMostConnectedParticle
     << endl;
  os << indent << "ComputeSOD: " << this->ComputeSOD << endl;
  os << indent << "SODCenterType: " << this->SODCenterType << endl;

  os << indent << "RhoC: " << this->RhoC << endl;
  os << indent << "SODMass: " << this->SODMass << endl;
  os << indent << "MinRadiusFactor: " << this->MinRadiusFactor << endl;
  os << indent << "MaxRadiusFactor: " << this->MaxRadiusFactor << endl;
  os << indent << "SODBins: " << this->SODBins << endl;
  os << indent << "MinFOFSize: " << this->MinFOFSize << endl;
  os << indent << "MinFOFMass: " << this->MinFOFMass << endl;
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

  if(!input || !output || !catalog)
    {
    return 0;
    }

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

  if(updatePiece != this->Controller->GetLocalProcessId() ||
     updateTotal != this->Controller->GetNumberOfProcesses())
    {
    vtkErrorMacro(<< "Piece number does not match process number.");
    return 0;
    }

  // shallow total point input to output
  output->ShallowCopy(input);

  // code to short circuit if there are no points
  if(output->GetNumberOfPoints() < 1)
    {
    catalog->Initialize();
    return 1;
    }

  // RRU code
  // Initialize the partitioner which uses MPI Cartesian Topology
  Partition::initialize();

  // halo finder needs vectors so take the time to turn them into vectors
  // FIXME: ought to go into the halo finder and put some #ifdefs
  // so that it can use vtkDataArray as is - ought to do that with the
  // reader as well, because it doubles the memory requirement currently
  if(!output->GetPointData()->HasArray("velocity") ||
     !output->GetPointData()->HasArray("mass") ||
     !output->GetPointData()->HasArray("tag") ||
     !output->GetPointData()->HasArray("ghost"))
    {
    vtkErrorMacro(<< "The input data does not have one or more of " <<
                  "the following point arrays: velocity, mass, tag, or ghost.");
    return 0;
    }

  vtkPoints* points = output->GetPoints();
  vtkFloatArray* velocity = vtkFloatArray::SafeDownCast
    (output->GetPointData()->GetArray("velocity"));
  vtkFloatArray* pmass = vtkFloatArray::SafeDownCast
    (output->GetPointData()->GetArray("mass"));
  vtkIntArray* uid = vtkIntArray::SafeDownCast
    (output->GetPointData()->GetArray("tag"));
  vtkIntArray* owner = vtkIntArray::SafeDownCast
    (output->GetPointData()->GetArray("ghost"));

  if(velocity == 0 || pmass == 0 || uid == 0 || owner == 0 ||
     velocity->GetNumberOfComponents() != DIMENSION)
    {
    vtkErrorMacro(<< "One or more of the input point data arrays is" <<
                  "malformed: velocity, mass, tag, or ghost.");
    return 0;
    }

  // create the empty ones
  vtkIdType numberOfLocalPoints = output->GetNumberOfPoints();
  vector<POTENTIAL_T>* potential = new vector<POTENTIAL_T>(numberOfLocalPoints);
  vector<MASK_T>* mask = new vector<MASK_T>(numberOfLocalPoints);

  // fill in the non empty ones
  vector<POSVEL_T>* xx = new vector<POSVEL_T>;
  vector<POSVEL_T>* yy = new vector<POSVEL_T>;
  vector<POSVEL_T>* zz = new vector<POSVEL_T>;
  vector<POSVEL_T>* vx = new vector<POSVEL_T>;
  vector<POSVEL_T>* vy = new vector<POSVEL_T>;
  vector<POSVEL_T>* vz = new vector<POSVEL_T>;
  vector<POSVEL_T>* mass = new vector<POSVEL_T>;
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

    // get and set the mass
    vel[0] = pmass->GetValue(i);
    mass->push_back(vel[0]);

    // get and set the tag
    int particle = uid->GetValue(i);
    tag->push_back(particle);

    // get and set the status
    int neighbor = owner->GetValue(i);
    status->push_back(neighbor);
    }

  // delete owner/status because it was only needed for halo finding
  output->GetPointData()->RemoveArray("ghost");

  // Run halo finder
  // Collect the serial halo finder results
  // Merge the halos so that only one copy of each is written
  // Parallel halo finder must consult with each of the 26 possible neighbor
  // halo finders to see who will report a particular halo
  CosmoHaloFinderP* haloFinder = new CosmoHaloFinderP();;

  haloFinder->setParameters
    ("", this->RL, this->Overlap, this->NP, this->PMin, this->BB);
  haloFinder->setParticles(xx, yy, zz, vx, vy, vz,
                           potential, tag, mask, status);
  haloFinder->executeHaloFinder();
  haloFinder->collectHalos();
  haloFinder->mergeHalos();

  // adjust ghost cells, because halo finder updates it
  vtkUnsignedCharArray* newghost = vtkUnsignedCharArray::New();
  newghost->SetNumberOfValues(numberOfLocalPoints);
  newghost->SetName("vtkGhostLevels");

  for(int i = 0; i < numberOfLocalPoints; i = i + 1)
    {
    unsigned char level = (*status)[i] < 0 ? 0 : 1;
    newghost->SetValue(i, level);
    }

  // Collect information from the halo finder needed for halo properties
  // Vector halos is the index of the first particle for halo in the haloList
  // Following the chain of indices in the haloList retrieves all particles
  int numberOfFOFHalos = haloFinder->getNumberOfHalos();
  int* fofHalos = haloFinder->getHalos();
  int* fofHaloCount = haloFinder->getHaloCount();
  int* fofHaloList = haloFinder->getHaloList();
  int* fofHaloTags = new int[numberOfFOFHalos];

  FOFHaloProperties* fof = new FOFHaloProperties();
  fof->setHalos(numberOfFOFHalos, fofHalos, fofHaloCount, fofHaloList);
  fof->setParameters("", this->RL, this->Overlap, this->BB);
  fof->setParticles
    (xx, yy, zz, vx, vy, vz, mass, potential, tag, mask, status);

  // Find the mass of every FOF halo
  vector<POSVEL_T>* fofMass = new vector<POSVEL_T>;
  fof->FOFHaloMass(fofMass);

  // Find the average position of every FOF halo
  vector<POSVEL_T>* fofXPos = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofYPos = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofZPos = new vector<POSVEL_T>;
  fof->FOFPosition(fofXPos, fofYPos, fofZPos);

  // Find the center of mass of every FOF halo
  vector<POSVEL_T>* fofXCofMass = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofYCofMass = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofZCofMass = new vector<POSVEL_T>;
  fof->FOFCenterOfMass(fofXCofMass, fofYCofMass, fofZCofMass);

  // Find the average velocity of every FOF halo
  vector<POSVEL_T>* fofXVel = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofYVel = new vector<POSVEL_T>;
  vector<POSVEL_T>* fofZVel = new vector<POSVEL_T>;
  fof->FOFVelocity(fofXVel, fofYVel, fofZVel);

  // Find the velocity dispersion of every FOF halo
  vector<POSVEL_T>* fofVelDisp = new vector<POSVEL_T>;
  fof->FOFVelocityDispersion(fofXVel, fofYVel, fofZVel, fofVelDisp);

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

  // calculate MCP or MBP
  int* mbpCenter = 0;
  int* mcpCenter = 0;
  int mbpOn = this->ComputeMostBoundParticle ||
    (this->ComputeSOD && this->SODCenterType == 2);
  int mcpOn = this->ComputeMostConnectedParticle ||
    (this->ComputeSOD && this->SODCenterType == 3);
  if(mbpOn)
    {
    mbpCenter = new int[numberOfFOFHalos];
    }
  if(mcpOn)
    {
    mcpCenter = new int[numberOfFOFHalos];
    }

  if(mcpOn || mbpOn)
    {
    for(int i = 0; i < numberOfFOFHalos; i = i + 1)
      {
      // skip if it's not large enough
      long size = fofHaloCount[i];
      if(size < this->PMin)
        {
        continue;
        }

      // Allocate arrays which will hold halo particle information
      POSVEL_T* xLocHalo = new POSVEL_T[size];
      POSVEL_T* yLocHalo = new POSVEL_T[size];
      POSVEL_T* zLocHalo = new POSVEL_T[size];
      POSVEL_T* xVelHalo = new POSVEL_T[size];
      POSVEL_T* yVelHalo = new POSVEL_T[size];
      POSVEL_T* zVelHalo = new POSVEL_T[size];
      POSVEL_T* massHalo = new POSVEL_T[size];
      ID_T* id = new ID_T[size];

      int* actualIndex = new int[size];
      fof->extractInformation(i, actualIndex,
                              xLocHalo, yLocHalo, zLocHalo,
                              xVelHalo, yVelHalo, zVelHalo,
                              massHalo, id);

      // Most bound particle method of center finding
      int centerIndex;
      POTENTIAL_T minPotential;
      if(mbpOn)
        {
        HaloCenterFinder centerFinder;
        centerFinder.setParticles(size,
                                  xLocHalo, yLocHalo, zLocHalo,
                                  massHalo, id);
        centerFinder.setParameters(this->BB, this->Overlap);

        // Calculate the halo center using MBP (most bound particle)
        // Combination of n^2/2 algorithm and A* algorithm
        if(size < MBP_THRESHOLD)
          {
          centerIndex = centerFinder.mostBoundParticleN2(&minPotential);
          }
        else
          {
          centerIndex = centerFinder.mostBoundParticleAStar(&minPotential);
          }

        mbpCenter[i] = actualIndex[centerIndex];
        }

      // Most connected particle method of center finding
      if(mcpOn)
        {
        HaloCenterFinder centerFinder;
        centerFinder.setParticles(size,
                                  xLocHalo, yLocHalo, zLocHalo,
                                  massHalo, id);
        centerFinder.setParameters(this->BB, this->Overlap);

        // Calculate the halo center using MCP (most connected particle)
        // Combination of n^2/2 algorithm and chaining mesh algorithm
        if(size < MCP_THRESHOLD)
          {
          centerIndex = centerFinder.mostConnectedParticleN2();
          }
        else
          {
          centerIndex = centerFinder.mostConnectedParticleChainMesh();
          }

        mcpCenter[i] = actualIndex[centerIndex];
        }

      delete [] xLocHalo;
      delete [] yLocHalo;
      delete [] zLocHalo;
      delete [] xVelHalo;
      delete [] yVelHalo;
      delete [] zVelHalo;
      delete [] massHalo;
      delete [] id;
      delete [] actualIndex;
      }
    }

  // calculate SOD halos
  vtkFloatArray* sodPos = 0;
  vtkFloatArray* sodCofMass = 0;
  vtkFloatArray* sodMass = 0;
  vtkFloatArray* sodVelocity = 0;
  vtkFloatArray* sodDispersion = 0;
  vtkFloatArray* sodRadius = 0;
  if(this->ComputeSOD)
    {
    // set up the arrays
    sodPos = vtkFloatArray::New();
    sodPos->SetName("sod_average_position");
    sodPos->SetNumberOfComponents(3);
    sodPos->SetNumberOfTuples(pminHalos);

    sodCofMass = vtkFloatArray::New();
    sodCofMass->SetName("sod_center_of_mass");
    sodCofMass->SetNumberOfComponents(3);
    sodCofMass->SetNumberOfTuples(pminHalos);

    sodMass = vtkFloatArray::New();
    sodMass->SetName("sod_mass");
    sodMass->SetNumberOfTuples(pminHalos);

    sodVelocity = vtkFloatArray::New();
    sodVelocity->SetName("sod_average_velocity");
    sodVelocity->SetNumberOfComponents(3);
    sodVelocity->SetNumberOfTuples(pminHalos);

    sodDispersion = vtkFloatArray::New();
    sodDispersion->SetName("sod_velocity_dispersion");
    sodDispersion->SetNumberOfTuples(pminHalos);

    sodRadius = vtkFloatArray::New();
    sodRadius->SetName("sod_radius");
    sodRadius->SetNumberOfTuples(pminHalos);

    ChainingMesh* chain =
      new ChainingMesh(this->RL, this->Overlap, CHAIN_SIZE, xx, yy, zz);

    int index = 0;
    for(int i = 0; i < numberOfFOFHalos; i = i + 1)
      {
      // skip if it's not large enough
      if(fofHaloCount[i] < this->PMin)
        {
        continue;
        }

      // only calculate the SOD if it is big enough
      if((*fofMass)[i] >= this->MinFOFMass ||
         fofHaloCount[i] >= this->MinFOFSize)
        {
        SODHalo* sod = new SODHalo();
        sod->setParameters(chain, this->SODBins, this->RL, this->NP,
                           this->RhoC, this->SODMass, this->RhoC,
                           this->MinRadiusFactor, this->MaxRadiusFactor);
        sod->setParticles(xx, yy, zz, vx, vy, vz, mass, tag);

        // no minimum potential array like in the sim
        // FIXME: possibly have a minimum potential calculation?

        if(this->SODCenterType == 0)
          {
          sod->createSODHalo(fofHaloCount[i],
                             (*fofXCofMass)[i],
                             (*fofYCofMass)[i],
                             (*fofZCofMass)[i],
                             (*fofXVel)[i],
                             (*fofYVel)[i],
                             (*fofZVel)[i],
                             (*fofMass)[i]);
          }
        else if(this->SODCenterType == 1)
          {
          sod->createSODHalo(fofHaloCount[i],
                             (*fofXPos)[i],
                             (*fofYPos)[i],
                             (*fofZPos)[i],
                             (*fofXVel)[i],
                             (*fofYVel)[i],
                             (*fofZVel)[i],
                             (*fofMass)[i]);
          }
        else
          {
          int center =
            this->SODCenterType == 2 ? mbpCenter[i] :
            mcpCenter[i];

          sod->createSODHalo(fofHaloCount[i],
                             (*xx)[center],
                             (*yy)[center],
                             (*zz)[center],
                             (*fofXVel)[i],
                             (*fofYVel)[i],
                             (*fofZVel)[i],
                             (*fofMass)[i]);
          }

        // get the halo and fill in the array
        if(sod->SODHaloSize() > 0)
          {
          POSVEL_T tempPos[DIMENSION];
          POSVEL_T tempCofMass[DIMENSION];
          POSVEL_T tempMass;
          POSVEL_T tempVelocity[DIMENSION];
          POSVEL_T tempDispersion;
          POSVEL_T tempRadius;

          sod->SODAverageLocation(tempPos);
          sod->SODCenterOfMass(tempCofMass);
          sod->SODMass(&tempMass);
          sod->SODAverageVelocity(tempVelocity);
          sod->SODVelocityDispersion(&tempDispersion);
          tempRadius = sod->SODRadius();

          sodPos->SetComponent(index, 0, tempPos[0]);
          sodPos->SetComponent(index, 1, tempPos[1]);
          sodPos->SetComponent(index, 2, tempPos[2]);

          sodCofMass->SetComponent(index, 0, tempCofMass[0]);
          sodCofMass->SetComponent(index, 1, tempCofMass[1]);
          sodCofMass->SetComponent(index, 2, tempCofMass[2]);

          sodMass->SetComponent(index, 0, tempMass);

          sodVelocity->SetComponent(index, 0, tempVelocity[0]);
          sodVelocity->SetComponent(index, 1, tempVelocity[1]);
          sodVelocity->SetComponent(index, 2, tempVelocity[2]);

          sodDispersion->SetComponent(index, 0, tempDispersion);

          sodRadius->SetComponent(index, 0, tempRadius);
          }
        // fill in a blank entry for the array
        else
          {
          sodPos->SetComponent(index, 0, 0);
          sodPos->SetComponent(index, 1, 0);
          sodPos->SetComponent(index, 2, 0);

          sodCofMass->SetComponent(index, 0, 0);
          sodCofMass->SetComponent(index, 1, 0);
          sodCofMass->SetComponent(index, 2, 0);

          sodMass->SetComponent(index, 0, 0);

          sodVelocity->SetComponent(index, 0, 0);
          sodVelocity->SetComponent(index, 1, 0);
          sodVelocity->SetComponent(index, 2, 0);

          sodDispersion->SetComponent(index, 0, 0);

          sodRadius->SetComponent(index, 0, -1);
          }

        delete sod;
        }
      // fill in a blank entry for the array
      else
        {
        sodPos->SetComponent(index, 0, 0);
        sodPos->SetComponent(index, 1, 0);
        sodPos->SetComponent(index, 2, 0);

        sodCofMass->SetComponent(index, 0, 0);
        sodCofMass->SetComponent(index, 1, 0);
        sodCofMass->SetComponent(index, 2, 0);

        sodMass->SetComponent(index, 0, 0);

        sodVelocity->SetComponent(index, 0, 0);
        sodVelocity->SetComponent(index, 1, 0);
        sodVelocity->SetComponent(index, 2, 0);

        sodDispersion->SetComponent(index, 0, 0);

        sodRadius->SetComponent(index, 0, -1);
        }

      index = index + 1;
      }

    delete chain;
    }

  // walk the list again to set the values for the points and catalog
  vtkIntArray* partTag = 0;
  vtkFloatArray* partPos = 0;
  vtkFloatArray* partCofMass = 0;
  vtkFloatArray* partMass = 0;
  vtkFloatArray* partVelocity = 0;
  vtkFloatArray* partDispersion = 0;

  vtkFloatArray* partMBP = 0;
  vtkFloatArray* partMCP = 0;

  // if we are copying to particles get the arrays ready
  if(this->CopyHaloDataToParticles)
    {
    partTag = vtkIntArray::New();
    partTag->SetName("halo_tag");
    partTag->SetNumberOfValues(numberOfLocalPoints);
    partTag->FillComponent(0, -1);

    partPos = vtkFloatArray::New();
    partPos->SetName("halo_average_position");
    partPos->SetNumberOfComponents(3);
    partPos->SetNumberOfTuples(numberOfLocalPoints);

    partCofMass = vtkFloatArray::New();
    partCofMass->SetName("halo_center_of_mass");
    partCofMass->SetNumberOfComponents(3);
    partCofMass->SetNumberOfTuples(numberOfLocalPoints);

    partMass = vtkFloatArray::New();
    partMass->SetName("halo_mass");
    partMass->SetNumberOfValues(numberOfLocalPoints);

    partVelocity = vtkFloatArray::New();
    partVelocity->SetName("halo_average_velocity");
    partVelocity->SetNumberOfComponents(3);
    partVelocity->SetNumberOfTuples(numberOfLocalPoints);

    partDispersion = vtkFloatArray::New();
    partDispersion->SetName("halo_velocity_dispersion");
    partDispersion->SetNumberOfValues(numberOfLocalPoints);

    if(mbpOn)
      {
      partMBP = vtkFloatArray::New();
      partMBP->SetName("halo_most_bound_particle");
      partMBP->SetNumberOfComponents(3);
      partMBP->SetNumberOfTuples(numberOfLocalPoints);
      }

    if(mcpOn)
      {
      partMCP = vtkFloatArray::New();
      partMCP->SetName("halo_most_connected_particle");
      partMCP->SetNumberOfComponents(3);
      partMCP->SetNumberOfTuples(numberOfLocalPoints);
      }
    }

  // get the catalog arrays ready
  vtkPoints* catpoints = vtkPoints::New();
  catpoints->SetDataTypeToFloat();
  catalog->Allocate(pminHalos);
  catalog->SetPoints(catpoints);

  vtkIntArray* haloTag = vtkIntArray::New();
  haloTag->SetName("halo_tag");
  haloTag->SetNumberOfValues(pminHalos);

  vtkFloatArray* haloPos = vtkFloatArray::New();
  haloPos->SetName("halo_average_position");
  haloPos->SetNumberOfComponents(3);
  haloPos->SetNumberOfTuples(pminHalos);

  vtkFloatArray* haloCofMass = vtkFloatArray::New();
  haloCofMass->SetName("halo_center_of_mass");
  haloCofMass->SetNumberOfComponents(3);
  haloCofMass->SetNumberOfTuples(pminHalos);

  vtkFloatArray* haloMass = vtkFloatArray::New();
  haloMass->SetName("halo_mass");
  haloMass->SetNumberOfValues(pminHalos);

  vtkFloatArray* haloVelocity = vtkFloatArray::New();
  haloVelocity->SetName("halo_average_velocity");
  haloVelocity->SetNumberOfComponents(3);
  haloVelocity->SetNumberOfTuples(pminHalos);

  vtkFloatArray* haloDispersion = vtkFloatArray::New();
  haloDispersion->SetName("halo_velocity_dispersion");
  haloDispersion->SetNumberOfValues(pminHalos);

  vtkFloatArray* haloMBP = 0;
  vtkFloatArray* haloMCP = 0;

  if(mbpOn)
    {
    haloMBP = vtkFloatArray::New();
    haloMBP->SetName("halo_most_bound_particle");
    haloMBP->SetNumberOfComponents(3);
    haloMBP->SetNumberOfTuples(pminHalos);
    }

  if(mcpOn)
    {
    haloMCP = vtkFloatArray::New();
    haloMCP->SetName("halo_most_connected_particle");
    haloMCP->SetNumberOfComponents(3);
    haloMCP->SetNumberOfTuples(pminHalos);
    }

  // walk the halos and copy the data
  int halocount = 0;
  for(int i = 0; i < numberOfFOFHalos; i = i + 1)
    {
    // skip if not large enough
    if(fofHaloCount[i] < this->PMin)
      {
      continue;
      }

    // set the catalog position
    vtkIdType pid;
    pid = catpoints->InsertNextPoint
      ((*fofXPos)[i], (*fofYPos)[i], (*fofZPos)[i]);
    catalog->InsertNextCell(1, 1, &pid);

    // set the halo data
    haloTag->SetValue(halocount, fofHaloTags[i]);
    haloPos->SetComponent(halocount, 0, (*fofXPos)[i]);
    haloPos->SetComponent(halocount, 1, (*fofYPos)[i]);
    haloPos->SetComponent(halocount, 2, (*fofZPos)[i]);
    haloCofMass->SetComponent(halocount, 0, (*fofXCofMass)[i]);
    haloCofMass->SetComponent(halocount, 1, (*fofYCofMass)[i]);
    haloCofMass->SetComponent(halocount, 2, (*fofZCofMass)[i]);
    haloMass->SetValue(halocount, (*fofMass)[i]);
    haloVelocity->SetComponent(halocount, 0, (*fofXVel)[i]);
    haloVelocity->SetComponent(halocount, 1, (*fofYVel)[i]);
    haloVelocity->SetComponent(halocount, 2, (*fofZVel)[i]);
    haloDispersion->SetValue(halocount, (*fofVelDisp)[i]);

    if(haloMBP)
      {
      haloMBP->SetComponent(halocount, 0, (*xx)[mbpCenter[i]]);
      haloMBP->SetComponent(halocount, 1, (*yy)[mbpCenter[i]]);
      haloMBP->SetComponent(halocount, 2, (*zz)[mbpCenter[i]]);
      }

    if(haloMCP)
      {
      haloMCP->SetComponent(halocount, 0, (*xx)[mcpCenter[i]]);
      haloMCP->SetComponent(halocount, 1, (*yy)[mcpCenter[i]]);
      haloMCP->SetComponent(halocount, 2, (*zz)[mcpCenter[i]]);
      }

    // increment to the next halo
    halocount = halocount + 1;

    // set the halo data for the original points
    if(this->CopyHaloDataToParticles)
      {
      int index = fofHalos[i];
      for(int j = 0; j < fofHaloCount[i]; j = j + 1)
        {
        partTag->SetValue(index, fofHaloTags[i]);
        partPos->SetComponent(index, 0, (*fofXPos)[i]);
        partPos->SetComponent(index, 1, (*fofYPos)[i]);
        partPos->SetComponent(index, 2, (*fofZPos)[i]);
        partCofMass->SetComponent(index, 0, (*fofXCofMass)[i]);
        partCofMass->SetComponent(index, 1, (*fofYCofMass)[i]);
        partCofMass->SetComponent(index, 2, (*fofZCofMass)[i]);
        partMass->SetValue(index, (*fofMass)[i]);
        partVelocity->SetComponent(index, 0, (*fofXVel)[i]);
        partVelocity->SetComponent(index, 1, (*fofYVel)[i]);
        partVelocity->SetComponent(index, 2, (*fofZVel)[i]);
        partDispersion->SetValue(index, (*fofVelDisp)[i]);

        if(partMBP)
          {
          partMBP->SetComponent(index, 0, (*xx)[mbpCenter[i]]);
          partMBP->SetComponent(index, 1, (*yy)[mbpCenter[i]]);
          partMBP->SetComponent(index, 2, (*zz)[mbpCenter[i]]);
          }

        if(partMCP)
          {
          partMCP->SetComponent(index, 0, (*xx)[mcpCenter[i]]);
          partMCP->SetComponent(index, 1, (*yy)[mcpCenter[i]]);
          partMCP->SetComponent(index, 2, (*zz)[mcpCenter[i]]);
          }

        index = fofHaloList[index];
        }
      }
    }

  // set the array for particles
  if(this->CopyHaloDataToParticles)
    {
    output->GetPointData()->AddArray(partTag);
    output->GetPointData()->AddArray(partPos);
    output->GetPointData()->AddArray(partCofMass);
    output->GetPointData()->AddArray(partMass);
    output->GetPointData()->AddArray(partVelocity);
    output->GetPointData()->AddArray(partDispersion);
    if(partMBP)
      {
      output->GetPointData()->AddArray(partMBP);
      }
    if(partMCP)
      {
      output->GetPointData()->AddArray(partMCP);
      }
    }
  output->GetPointData()->AddArray(newghost);

  // set the arrays for the catalog
  catalog->GetPointData()->AddArray(haloTag);
  catalog->GetPointData()->AddArray(haloPos);
  catalog->GetPointData()->AddArray(haloCofMass);
  catalog->GetPointData()->AddArray(haloMass);
  catalog->GetPointData()->AddArray(haloVelocity);
  catalog->GetPointData()->AddArray(haloDispersion);
  if(haloMBP)
    {
    catalog->GetPointData()->AddArray(haloMBP);
    }
  if(haloMCP)
    {
    catalog->GetPointData()->AddArray(haloMCP);
    }

  if(sodPos)
    {
    catalog->GetPointData()->AddArray(sodPos);
    catalog->GetPointData()->AddArray(sodCofMass);
    catalog->GetPointData()->AddArray(sodMass);
    catalog->GetPointData()->AddArray(sodVelocity);
    catalog->GetPointData()->AddArray(sodDispersion);
    catalog->GetPointData()->AddArray(sodRadius);
    }

  // cleanup
  if(this->CopyHaloDataToParticles)
    {
    partTag->Delete();
    partPos->Delete();
    partCofMass->Delete();
    partMass->Delete();
    partVelocity->Delete();
    partDispersion->Delete();
    }
  newghost->Delete();

  catpoints->Delete();
  haloTag->Delete();
  haloPos->Delete();
  haloCofMass->Delete();
  haloMass->Delete();
  haloVelocity->Delete();
  haloDispersion->Delete();

  if(partMBP)
    {
    partMBP->Delete();
    }

  if(partMCP)
    {
    partMCP->Delete();
    }

  if(haloMBP)
    {
    haloMBP->Delete();
    }

  if(haloMCP)
    {
    haloMCP->Delete();
    }

  if(mbpCenter)
    {
    delete [] mbpCenter;
    }

  if(mcpCenter)
    {
    delete [] mcpCenter;
    }

  if(sodPos)
    {
    sodPos->Delete();
    sodCofMass->Delete();
    sodMass->Delete();
    sodVelocity->Delete();
    sodDispersion->Delete();
    sodRadius->Delete();
    }

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

  delete [] fofHaloTags;
  delete fofMass;
  delete fofXPos;
  delete fofYPos;
  delete fofZPos;
  delete fofXCofMass;
  delete fofYCofMass;
  delete fofZCofMass;
  delete fofXVel;
  delete fofYVel;
  delete fofZVel;
  delete fofVelDisp;

  delete fof;
  delete haloFinder;

  return 1;
}
