/*=========================================================================

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
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

// .NAME HaloFinder - drive the halo finding from input to output
//                 - developer version of HaloFinder
//
// .SECTION Description
// Reads the input description file for driving the halo finder
// Uses ParticleDistribute to read data from files
// Uses ParticleExchange to place overloaded particles on neighbor processors
// Uses CosmoHaloFinderP to locate the halos on each processor and merge
// Uses FOFHaloProperties to calculate data on FOF halos
// Uses SODHalo to calculate data on SOD halos
// Uses HaloCenterFinder to find either MCP or MBP particles per FOF halo
// Uses SubHaloFinder to find subhalos of FOF halos
//

#include "HaloFinderInput.h"
#include "Partition.h"
#include "Timings.h"

#include "ParticleDistribute.h"
#include "ParticleExchange.h"

#include "CosmoHaloFinderP.h"

#include "FOFHaloProperties.h"
#include "HaloCenterFinder.h"
#include "SubHaloFinder.h"

#include "SODHalo.h"
#include "ChainingMesh.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <mpi.h>

using namespace std;

namespace cosmologytools {
/////////////////////////////////////////////////////////////////////////////
//
// Class for testing cosmology code
//
/////////////////////////////////////////////////////////////////////////////

class HaloFinder {
public:
  HaloFinder(int argc, char** argv);
  ~HaloFinder();

  // Reads particles from file, distributes to processor
  void DistributeParticles();

  // Finds FOF halos uniquely on each processor
  void FOFHaloFinder();

  // Calculates properties that only require halo lists from FOFHaloFinder
  void BasicFOFHaloProperties();

  // Center finding requires extraction of halo particles from FOF halo list
  // and building of a chaining mesh of those particles
  void FOFCenterFinding();

  // Most bound particle center finding uses either N^2/2 algorithm on small
  // halos or A* refinement algorithm which uses chaining mesh of halo particles
  // and can operate on independent arrays of particle locations
  int MBPCenterFinding(
  POTENTIAL_T* minPotential,
  long particleCount,
  POSVEL_T* xLocHalo,
  POSVEL_T* yLocHalo,
  POSVEL_T* zLocHalo,
  POSVEL_T* massHalo,
  ID_T* id);

  // Most connected particle center finding uses either N^2/2 algorithm on small
  // halos or a chaining mesh algorithm and can operate on independent arrays
  // of particle locations
  int MCPCenterFinding(
  long particleCount,
  POSVEL_T* xLocHalo,
  POSVEL_T* yLocHalo,
  POSVEL_T* zLocHalo,
  POSVEL_T* massHalo,
  ID_T* id);

  // Subhalo finding requires extraction of halo particles from FOF halo list
  // and building of a Barnes Hut tree of those particles
  void FOFSubHaloFinding();

  // Write the halo catalog which is same .cosmo format as particles
  // Each halo has one entry with mass being the size of the halo and
  // location being the halo center.  Can be visualized as particle data is.
  void FOFHaloCatalog();

  // Using the FOF halo center, calculate a spherically over dense halo
  void SODHaloFinding();

  // Write a .cosmo file of halos of this size or greater
  void WriteCosmoFiles(int size);

  int numProc;			// Number of processors
  int myProc;			// Rank of this processor

private:
  HaloFinderInput    haloIn;	// Read input file to direct computation
  ParticleDistribute distribute;// Distributes particles to processors
  ParticleExchange   exchange;	// Exchanges ghost particles
  CosmoHaloFinderP   haloFinder;// FOF halo finder
  FOFHaloProperties  fof;	// FOF halo properties

  string haloInputFile;		// Name of driver input file
  string inFile;		// Base name of input particle files
  string outFile;		// Base name of output particle files
  string dataType;		// BLOCK or RECORD structure input
  string distributeType;	// ROUND_ROBIN every proc looks at all files
        // ONE_TO_ONE files match to processors

  float massConvertFactor;	// Multiply every mass read by factor
  float distConvertFactor;	// Multiply every pos read by factor
  float rhocConvertFactor;	// RHO_C based on Mpc/h, convert to match units
  float sodMassConvertFactor;	// SOD_MASS based on Msun/h, convert units
        // If units are Msun and Mpc and user wants to
                                // keep them, rhoc * hubble * hubble
                                // If units are Msun and Mpc and user wants to
                                // convert massFactor = hubble,
                                // distFactor = hubble, rhocFactor = 1.0

  POSVEL_T rL;			// Physical coordinate box size
  POSVEL_T deadSize;		// Physical coordinate dead or ghost zone
  POSVEL_T bb;			// Distance between particles in a halo
  POSVEL_T omegadm;		// Matter density contribution dark matter
  POSVEL_T deut;		// Matter density contribution baryonic matter
  POSVEL_T omegatot;		// Total matter density
  POSVEL_T hubble;		// Hubble constant

  POSVEL_T RHOC;		// Critical density of universe, Mpc/h, Msun/h
  POSVEL_T SODMASS;		// Factor used to find initial radius for SOD
  POSVEL_T particleMass;	// Physical mass of one particle
  int pmin;			// Minimum number of particles to make a halo
  int np;			// Number of particles in problem cubed

  POSVEL_T alphaFactor;		// Subhalo finding cut/grow factor
  POSVEL_T betaFactor;		// Subhalo finding Poisson noise significance
  int minCandidateSize;		// Smallest particle count for subhalo
  int numSPHNeighbors;		// For calculating smoothing length for subhalo
  int numNeighbors;		// Number of neighbors for subhalo build

  int numberOfFOFHalos;		// Total FOF halos on this processor

  vector<POSVEL_T>* xx;		// Locations of particles on this processor
  vector<POSVEL_T>* yy;
  vector<POSVEL_T>* zz;
  vector<POSVEL_T>* vx;		// Velocities of particles on this processor
  vector<POSVEL_T>* vy;
  vector<POSVEL_T>* vz;
  vector<POSVEL_T>* mass;	// Mass of particles on this processor
  vector<ID_T>* tag;		// Id within entire problem of particle
  vector<STATUS_T>* status;	// ALIVE, DEAD and other information
  vector<POTENTIAL_T>* potential;
  vector<MASK_T>* mask;

  vector<int>* fofCenter;	// Index of center particle of halo
  vector<POSVEL_T>* fofMass;	// Mass of halo
  vector<POSVEL_T>* fofXPos;	// Average position of particles
  vector<POSVEL_T>* fofYPos;
  vector<POSVEL_T>* fofZPos;
  vector<POSVEL_T>* fofXCofMass;// Center of mass of particles
  vector<POSVEL_T>* fofYCofMass;
  vector<POSVEL_T>* fofZCofMass;
  vector<POSVEL_T>* fofXVel;	// Average velocity of halo
  vector<POSVEL_T>* fofYVel;
  vector<POSVEL_T>* fofZVel;
  vector<POSVEL_T>* fofVelDisp;	// Velocity dispersion of halo
};


/////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
/////////////////////////////////////////////////////////////////////////////

HaloFinder::HaloFinder(int argc, char* argv[])
{
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();

  if (argc != 2) {
    cout << "Usage: mpirun -np # HaloFinder halo_finder_input_file" << endl;
  }

  this->haloInputFile = argv[1];
  this->haloIn.initialize(this->haloInputFile);

  // Base file name (Actual file names are basename.proc#
  this->inFile = this->haloIn.getInputBaseName();
  this->outFile = this->haloIn.getOutputBaseName();

  // Mass and distance units
  this->massConvertFactor = this->haloIn.getMassConvertFactor();
  this->distConvertFactor = this->haloIn.getDistConvertFactor();
  this->rhocConvertFactor = this->haloIn.getRHOCConvertFactor();
  this->sodMassConvertFactor = this->haloIn.getSODMassConvertFactor();

  // Physical coordinate box size
  this->rL = (POSVEL_T) this->haloIn.getBoxSize();
  this->rL *= this->distConvertFactor;

  // Physical coordinate dead zone area
  this->deadSize = (POSVEL_T) this->haloIn.getOverloadSize();
  this->deadSize *= this->distConvertFactor;

  // Superimposed grid on physical box used to determine wraparound
  this->np = this->haloIn.getNumberOfParticles();

  // BB parameter for distance between particles comprising a halo
  // This is in grid units for positions normalized on np x np x np grid
  this->bb = (POSVEL_T) this->haloIn.getMinParticleDistance();

  // Minimum number of particles to make a halo
  this->pmin = this->haloIn.getMinParticlesPerHalo();

  // Omegadm
  this->omegadm = (POSVEL_T) this->haloIn.getOmegadm();

  // Hubble constant
  this->hubble = (POSVEL_T) this->haloIn.getHubbleConstant();

  // Deut
  this->deut = (POSVEL_T) this->haloIn.getDeut();

  // Critical density of the universe
  this->RHOC = RHO_C * this->rhocConvertFactor;

  // Factor used to determine initial radius guess for SOD halos from FOF halos
  this->SODMASS = SOD_MASS * this->sodMassConvertFactor;

  // Input is BLOCK or RECORD structured data
  this->dataType = this->haloIn.getInputType();

  // Distribution mechanism is ROUND_ROBIN which selects alive and dead
  // as the data files are read and passed around
  // or EXCHANGE where the alive data is immediately read on a processor
  // and the dead particles must be bundled and shared
  this->distributeType = this->haloIn.getDistributeType();

  // Mass of one particle based on size of problem
  // test4:
  //   rL = 90.1408, hubble = 0.71, omegadm = .27, deut = 0.02218, np = 256
  //        particleMass = 1.917756e09
  // sb256:
  //   rL = 64.0, hubble = 0.5, omegadm = 1.0, deut = 0.0, np = 256
  //        particleMass = 1.08413e09
  // Both of these masses appear in the files and so are correct
  //
  this->omegatot = omegadm + deut/hubble/hubble;
  this->particleMass = this->RHOC * rL * rL * rL * omegatot / np / np / np;
  this->particleMass /= this->massConvertFactor;
  if (this->myProc == 0)
    cout << "Particle mass calculated: " << this->particleMass << endl;

  // Alpha factor controls the CUT/GROW of candidates
  // 1.0 / alphaFactor is the number of times larger a candidate must be
  // in order for the smaller to be CUT rather than allowed to GROW
  // Set to 1.0 and it always cuts, set to 0.01 to aggressively grow
  // small subhalos
  this->alphaFactor = (POSVEL_T) this->haloIn.getAlphaSubhalo();

  // Beta factor controls the Poisson noise significance of candidates
  // Original SUBFIND algorithm would have beta = 0.0 meaning that all
  // candidates are allowed to remain as separate and not be COMBINED
  // immediately into the saddle point partner.  If beta is larger it will
  // allow the identification of very small substructures such as tails
  this->betaFactor = (POSVEL_T) this->haloIn.getBetaSubhalo();

  // Minimum size of a subhalo candidate
  this->minCandidateSize = this->haloIn.getMinSubhaloSize();

  // BHTree, SPH and density parameters
  this->numSPHNeighbors = this->haloIn.getNumSPHDensity();
  this->numNeighbors = this->haloIn.getNumSubhaloNeighbors();

  if (this->myProc == 0 && this->haloIn.getOutputSubhaloProperties() == 1) {
    cout << "Particle mass: " << this->particleMass << endl;
    cout << "Gravitational constant: " << GRAVITY_C << endl;
    cout << "Potential energy factor: "
         << (this->particleMass * GRAVITY_C) << endl;
    cout << "Cut/Grow factor: " << this->alphaFactor << endl;
    cout << "Poisson noise factor: " << this->betaFactor << endl;
    cout << "Minimum candidate size: " << this->minCandidateSize << endl;
    cout << "Number of neighbors for SPH: " << this->numSPHNeighbors << endl;
    cout << "Number of neighbors for subgroups: " << this->numNeighbors << endl;
  }

  this->mass = 0;
  this->tag = 0;
  this->status = 0;
  this->potential = 0;
  this->mask = 0;
  this->fofCenter = 0;
  this->fofMass = 0;
  this->fofXPos = 0;
  this->fofYPos = 0;
  this->fofZPos = 0;
  this->fofXVel = 0;
  this->fofYVel = 0;
  this->fofZVel = 0;
  this->fofVelDisp = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
/////////////////////////////////////////////////////////////////////////////

HaloFinder::~HaloFinder()
{
/*
  if (this->xx != 0) delete this->xx;
  if (this->yy != 0) delete this->yy;
  if (this->zz != 0) delete this->zz;
  if (this->vx != 0) delete this->vx;
  if (this->vy != 0) delete this->vy;
  if (this->vz != 0) delete this->vz;

  if (this->mass != 0) delete this->mass;
  if (this->tag != 0) delete this->tag;
  if (this->status != 0) delete this->status;
  if (this->potential != 0) delete this->potential;
  if (this->mask != 0) delete this->mask;

  if (this->fofCenter != 0) delete this->fofCenter;
  if (this->fofMass != 0) delete this->fofMass;
  if (this->fofXPos != 0) delete this->fofXPos;
  if (this->fofYPos != 0) delete this->fofYPos;
  if (this->fofZPos != 0) delete this->fofZPos;
  if (this->fofXVel != 0) delete this->fofXVel;
  if (this->fofYVel != 0) delete this->fofYVel;
  if (this->fofZVel != 0) delete this->fofZVel;
  if (this->fofVelDisp != 0) delete this->fofVelDisp;
*/
}

/////////////////////////////////////////////////////////////////////////////
//
// ParticleDistribute reads particles from files and distributes to the
// appropriate processor based on decomposition.
//
// ParticleExchange causes particles on the edges to be shared with
// neighboring processsors making some particles ALIVE and some DEAD
//
/////////////////////////////////////////////////////////////////////////////

void HaloFinder::DistributeParticles()
{
  static Timings::TimerRef dtimer = Timings::getTimer("Distribute Particles");
  Timings::startTimer(dtimer);

  // Initialize classes for reading, exchanging and calculating
  this->distribute.setParameters(this->inFile, this->rL, this->dataType);
  this->distribute.setConvertParameters(this->massConvertFactor,
                                        this->distConvertFactor);
  this->exchange.setParameters(this->rL, this->deadSize);

  this->distribute.initialize();
  this->exchange.initialize();

  // Read alive particles only from files
  // In ROUND_ROBIN all files are read and particles are passed round robin
  // to every other processor so that every processor chooses its own
  // In ONE_TO_ONE every processor reads its own processor in the topology
  // which has already been populated with the correct alive particles

  this->xx = new vector<POSVEL_T>;
  this->yy = new vector<POSVEL_T>;
  this->zz = new vector<POSVEL_T>;
  this->vx = new vector<POSVEL_T>;
  this->vy = new vector<POSVEL_T>;
  this->vz = new vector<POSVEL_T>;
  this->mass = new vector<POSVEL_T>;
  this->tag = new vector<ID_T>;
  this->status = new vector<STATUS_T>;

  this->distribute.setParticles(this->xx, this->yy, this->zz,
                                this->vx, this->vy, this->vz,
                                this->mass, this->tag);
  if (this->distributeType == "ROUND_ROBIN")
    this->distribute.readParticlesRoundRobin();
  else if (this->distributeType == "ONE_TO_ONE")
    this->distribute.readParticlesOneToOne();

  int numberOfParticles;	// Total particles on this processor

  // Create the mask and potential vectors which will be filled in elsewhere
  numberOfParticles = this->xx->size();
  this->potential = new vector<POTENTIAL_T>(numberOfParticles);
  this->mask = new vector<MASK_T>(numberOfParticles);

  // If mass is a constant 1.0 must reset to particleMass
  for (int i = 0; i < numberOfParticles; i++)
    if ((*this->mass)[i] == 1.0)
      (*this->mass)[i] = this->particleMass;

  // Exchange particles adds dead particles to all the vectors
  this->exchange.setParticles(this->xx, this->yy, this->zz,
                              this->vx, this->vy, this->vz, this->mass,
                              this->potential, this->tag,
                              this->mask, this->status);
  this->exchange.exchangeParticles();

  numberOfParticles = this->xx->size();

  // If mass is a constant 1.0 must reset to particleMass
  for (int i = 0; i < numberOfParticles; i++)
    if ((*this->mass)[i] == 1.0)
      (*this->mass)[i] = this->particleMass;

  Timings::stopTimer(dtimer);
}

/////////////////////////////////////////////////////////////////////////////
//
// Find the FOF Halos
//
/////////////////////////////////////////////////////////////////////////////

void HaloFinder::FOFHaloFinder()
{
  static Timings::TimerRef h1timer = Timings::getTimer("FOF Halo Finder");
  Timings::startTimer(h1timer);

  this->haloFinder.setParameters(this->outFile, this->rL, this->deadSize,
                                 this->np, this->pmin, this->bb);
  this->haloFinder.setParticles(this->xx, this->yy, this->zz,
                                this->vx, this->vy, this->vz,
                                this->potential, this->tag,
                                this->mask, this->status);

  // Run serial halo finder
  this->haloFinder.executeHaloFinder();

  // Merge the resulting halos between processors
  this->haloFinder.collectHalos();
  this->haloFinder.mergeHalos();

  // Write file of particles with mass field replaced by halo tag
  if (this->haloIn.getOutputParticles() == 1)
    this->haloFinder.writeTaggedParticles(0, 1.0, true);

  Timings::stopTimer(h1timer);
}

/////////////////////////////////////////////////////////////////////////////
//
// Find the simple FOF properties for all halos
//
/////////////////////////////////////////////////////////////////////////////

void HaloFinder::BasicFOFHaloProperties()
{
  static Timings::TimerRef ftimer = Timings::getTimer("FOF Properties");
  Timings::startTimer(ftimer);

  if (this->myProc == 0)
    cout << "Run Basic FOF halo properties" << endl;

  this->numberOfFOFHalos = this->haloFinder.getNumberOfHalos();
  int* fofHalos = this->haloFinder.getHalos();
  int* fofHaloCount = this->haloFinder.getHaloCount();
  int* fofHaloList = this->haloFinder.getHaloList();

  // Construct the FOF properties class
  this->fof.setHalos(this->numberOfFOFHalos,
                     fofHalos, fofHaloCount, fofHaloList);
  this->fof.setParameters(this->outFile, this->rL, this->deadSize, this->bb);
  this->fof.setParticles(this->xx, this->yy, this->zz,
                         this->vx, this->vy, this->vz, this->mass,
                         this->potential, this->tag, this->mask, this->status);

  // Find the mass of every FOF halo
  this->fofMass = new vector<POSVEL_T>;
  this->fof.FOFHaloMass(this->fofMass);

  // Find the average position of every FOF halo
  this->fofXPos = new vector<POSVEL_T>;
  this->fofYPos = new vector<POSVEL_T>;
  this->fofZPos = new vector<POSVEL_T>;
  this->fof.FOFPosition(this->fofXPos, this->fofYPos, this->fofZPos);

  // Find the center of mass of every FOF halo
  this->fofXCofMass = new vector<POSVEL_T>;
  this->fofYCofMass = new vector<POSVEL_T>;
  this->fofZCofMass = new vector<POSVEL_T>;
  this->fof.FOFCenterOfMass(
                this->fofXCofMass, this->fofYCofMass, this->fofZCofMass);

  // Find the average velocity of every FOF halo
  this->fofXVel = new vector<POSVEL_T>;
  this->fofYVel = new vector<POSVEL_T>;
  this->fofZVel = new vector<POSVEL_T>;
  this->fof.FOFVelocity(this->fofXVel, this->fofYVel, this->fofZVel);

  // Find the velocity dispersion of every FOF halo
  this->fofVelDisp = new vector<POSVEL_T>;
  this->fof.FOFVelocityDispersion(this->fofXVel,
                                this->fofYVel, this->fofZVel, this->fofVelDisp);

  Timings::stopTimer(ftimer);
}

/////////////////////////////////////////////////////////////////////////////
//
// Center finder requires special data structures so extract the locations
// for each individual FOF halo into arrays for passing to HaloCenterFinder
// An advantage to passing arrays to these classes is that they can
// be used with any set of particles a user wants processed
// because they don't iterate over halos and haloList
//
/////////////////////////////////////////////////////////////////////////////

void HaloFinder::FOFCenterFinding()
{
  // Find the index of the particle at the FOF center using potential array
  this->fofCenter = new vector<int>;
  if (this->haloIn.getUseMinimumPotential() == 1) {
    this->fof.FOFHaloCenterMinimumPotential(this->fofCenter);
  }

  // Find the index of particle at FOF center using MBP or MCP
  else if (this->haloIn.getUseMCPCenterFinder() == 1 ||
           this->haloIn.getUseMBPCenterFinder() == 1) {

    if (this->myProc == 0)
      cout << "Run center finder" << endl;

    int* fofHaloCount = this->haloFinder.getHaloCount();

    for (int halo = 0; halo < this->numberOfFOFHalos; halo++) {

      // Allocate arrays which will hold halo particle information
      long particleCount = fofHaloCount[halo];
      POSVEL_T* xLocHalo = new POSVEL_T[particleCount];
      POSVEL_T* yLocHalo = new POSVEL_T[particleCount];
      POSVEL_T* zLocHalo = new POSVEL_T[particleCount];
      POSVEL_T* xVelHalo = new POSVEL_T[particleCount];
      POSVEL_T* yVelHalo = new POSVEL_T[particleCount];
      POSVEL_T* zVelHalo = new POSVEL_T[particleCount];
      POSVEL_T* massHalo = new POSVEL_T[particleCount];
      ID_T* id = new ID_T[particleCount];

      // Need array to map halo index to actual particle index to find locations
      int* actualIndx = new int[particleCount];

      this->fof.extractInformation(halo, actualIndx,
                                   xLocHalo, yLocHalo, zLocHalo,
                                   xVelHalo, yVelHalo, zVelHalo, massHalo, id);

      // Most bound particle method of center finding
      int centerIndex;
      POTENTIAL_T minPotential;
      if (this->haloIn.getUseMBPCenterFinder() == 1) {
        centerIndex = MBPCenterFinding(&minPotential, particleCount,
                                  xLocHalo, yLocHalo, zLocHalo, massHalo, id);
        this->fofCenter->push_back(actualIndx[centerIndex]);
      }

      // Most connected particle method of center finding
      else if (this->haloIn.getUseMCPCenterFinder() == 1) {
        centerIndex = MCPCenterFinding(particleCount,
                                  xLocHalo, yLocHalo, zLocHalo, massHalo, id);
        this->fofCenter->push_back(actualIndx[centerIndex]);
      }

      delete [] xLocHalo;
      delete [] yLocHalo;
      delete [] zLocHalo;
      delete [] xVelHalo;
      delete [] yVelHalo;
      delete [] zVelHalo;
      delete [] massHalo;
      delete [] id;
      delete [] actualIndx;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Most bound particle center finding
//
/////////////////////////////////////////////////////////////////////////////

int HaloFinder::MBPCenterFinding(
      POTENTIAL_T* minPotential,
                        long particleCount,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
                        POSVEL_T* massHalo,
                        ID_T* id)

{
  // Find the index of the particle at the FOF center
  static Timings::TimerRef cftimer = Timings::getTimer("MBP Center Finder");
  Timings::startTimer(cftimer);
  int centerIndex;

  // Create the center finder
  HaloCenterFinder centerFinder;
  centerFinder.setParticles(particleCount,
                            xLocHalo, yLocHalo, zLocHalo, massHalo, id);
  centerFinder.setParameters(this->bb, this->distConvertFactor);

  // Calculate the halo center using MBP (most bound particle)
  // Combination of n^2/2 algorithm and A* algorithm
  if (particleCount < MBP_THRESHOLD) {
    centerIndex = centerFinder.mostBoundParticleN2(minPotential);
  } else {
    centerIndex = centerFinder.mostBoundParticleAStar(minPotential);
  }

  Timings::stopTimer(cftimer);
  return centerIndex;
}


/////////////////////////////////////////////////////////////////////////////
//
// Most connected particle center finding
//
/////////////////////////////////////////////////////////////////////////////

int HaloFinder::MCPCenterFinding(
      long particleCount,
      POSVEL_T* xLocHalo,
      POSVEL_T* yLocHalo,
      POSVEL_T* zLocHalo,
      POSVEL_T* massHalo,
      ID_T* id)

{
  // Find the index of the particle at the FOF center
  static Timings::TimerRef cftimer = Timings::getTimer("MCP Center Finder");
  Timings::startTimer(cftimer);
  int centerIndex = 0;

  // Create the center finder
  HaloCenterFinder centerFinder;
  centerFinder.setParticles(particleCount,
                            xLocHalo, yLocHalo, zLocHalo, massHalo, id);
  centerFinder.setParameters(this->bb, this->distConvertFactor);

  // Calculate the halo center using MCP (most connected particle)
  // Combination of n^2/2 algorithm and chaining mesh algorithm
  if (particleCount < MCP_THRESHOLD) {
    centerIndex = centerFinder.mostConnectedParticleN2();
  }
  else {
    centerIndex = centerFinder.mostConnectedParticleChainMesh();
  }
  Timings::stopTimer(cftimer);
  return centerIndex;
}

/////////////////////////////////////////////////////////////////////////////
//
// Subhalo finder requires special data structures so extract the locations
// for each individual FOF halo into arrays for passing to SubHaloFinder
// An advantage to passing arrays to these classes is that they can
// be used with any set of particles a user wants processed
// because they don't iterate over halos and haloList
//
/////////////////////////////////////////////////////////////////////////////

void HaloFinder::FOFSubHaloFinding()
{
  static Timings::TimerRef shtimer = Timings::getTimer("SubHalo Finder");
  Timings::startTimer(shtimer);

  // Debug output file per processor
  ostringstream sname;
  if (numProc == 1) {
    sname << outFile << ".subhalo";
  } else {
    sname << outFile << ".subhalo." << myProc;
  }
  ofstream sStream(sname.str().c_str(), ios::out);

  // Get FOF halo information
  int* fofHaloCount = this->haloFinder.getHaloCount();
  int* fofHalos = this->haloFinder.getHalos();

  if (this->haloIn.getOutputSubhaloProperties() == 1) {
    if (this->myProc == 0)
      cout << "Run Subhalo finder" << endl;

    for (int halo = 0; halo < this->numberOfFOFHalos; halo++) {

      // Allocate arrays which will hold halo particle locations
      long particleCount = fofHaloCount[halo];
      if (particleCount > this->haloIn.getMinFOFSubhalo()) {

        cout << "Rank: " << this->myProc
             << " Subhalo find on FOF halo " << halo
             << " count " << particleCount << endl;

        sStream << "FOF Halo: " << halo << endl
                << "  FOF count = " << fofHaloCount[halo] << endl
                << "  FOF tag = " << fofHalos[halo] << endl
                << "  FOF mass = " << (*this->fofMass)[halo] << endl
                << "  FOF center of mass = ["
                                   << (*this->fofXCofMass)[halo] << ","
                                   << (*this->fofYCofMass)[halo] << ","
                                   << (*this->fofZCofMass)[halo] << "]" << endl
                << "  FOF avg loc = ["
                                   << (*this->fofXPos)[halo] << ","
                                   << (*this->fofYPos)[halo] << ","
                                   << (*this->fofZPos)[halo] << "]" << endl
                << "  FOF avg vel = ["
                                   << (*this->fofXVel)[halo] << ","
                                   << (*this->fofYVel)[halo] << ","
                                   << (*this->fofZVel)[halo] << "]" << endl
                << "  FOF velocity dispersion = "
                                   << (*this->fofVelDisp)[halo] << endl << endl;

        POSVEL_T* xLocHalo = new POSVEL_T[particleCount];
        POSVEL_T* yLocHalo = new POSVEL_T[particleCount];
        POSVEL_T* zLocHalo = new POSVEL_T[particleCount];
        POSVEL_T* xVelHalo = new POSVEL_T[particleCount];
        POSVEL_T* yVelHalo = new POSVEL_T[particleCount];
        POSVEL_T* zVelHalo = new POSVEL_T[particleCount];
        POSVEL_T* massHalo = new POSVEL_T[particleCount];
        ID_T* id = new ID_T[particleCount];

        // Map halo index to actual particle index to find locations
        int* actualIndx = new int[particleCount];

        this->fof.extractInformation(halo, actualIndx,
                                 xLocHalo, yLocHalo, zLocHalo,
                                 xVelHalo, yVelHalo, zVelHalo, massHalo, id);

        // Look for subhalos within the FOF halo using extract location arrays
        SubHaloFinder* subFinder = new SubHaloFinder();
        subFinder->setParameters(this->particleMass, GRAVITY_C,
                                 this->alphaFactor, this->betaFactor,
                                 this->minCandidateSize,
                                 this->numSPHNeighbors, this->numNeighbors);

        subFinder->setParticles(particleCount, xLocHalo, yLocHalo, zLocHalo,
                                xVelHalo, yVelHalo, zVelHalo, massHalo, id);

        subFinder->findSubHalos();

        // Retrieve subhalo information suitable for FOFHaloProperties
        int numberOfSubhalos = subFinder->getNumberOfSubhalos();
        int* fofSubhalos = subFinder->getSubhalos();
        int* fofSubhaloCount = subFinder->getSubhaloCount();
        int* fofSubhaloList = subFinder->getSubhaloList();

        // Construct the FOF properties class
        FOFHaloProperties subhaloProp;
        subhaloProp.setHalos(numberOfSubhalos,
                     fofSubhalos, fofSubhaloCount, fofSubhaloList);
        subhaloProp.setParameters(this->outFile,
                                  this->rL, this->deadSize, this->bb);
        subhaloProp.setParticles(particleCount, xLocHalo, yLocHalo, zLocHalo,
                         xVelHalo, yVelHalo, zVelHalo, massHalo, id);

        // Run some halo properties on subhalos
        // Find the mass of every subhalo
        vector<POSVEL_T> subhaloMass;
        subhaloProp.FOFHaloMass(&subhaloMass);

        // Find the average position of every subhalo
        vector<POSVEL_T> subhaloXPos;
        vector<POSVEL_T> subhaloYPos;
        vector<POSVEL_T> subhaloZPos;
        subhaloProp.FOFPosition(
               &subhaloXPos, &subhaloYPos, &subhaloZPos);

        // Find the center of mass of every subhalo
        vector<POSVEL_T> subhaloXCofMass;
        vector<POSVEL_T> subhaloYCofMass;
        vector<POSVEL_T> subhaloZCofMass;
        subhaloProp.FOFCenterOfMass(
               &subhaloXCofMass, &subhaloYCofMass, &subhaloZCofMass);

        // Find the average velocity of every subhalo
        vector<POSVEL_T> subhaloXVel;
        vector<POSVEL_T> subhaloYVel;
        vector<POSVEL_T> subhaloZVel;
        subhaloProp.FOFVelocity(
               &subhaloXVel, &subhaloYVel, &subhaloZVel);

        // Find the velocity dispersion of every subhalo
        vector<POSVEL_T> subhaloVelDisp;
        subhaloProp.FOFVelocityDispersion(
               &subhaloXVel, &subhaloYVel, &subhaloZVel, &subhaloVelDisp);

        for (int sindx = 0; sindx < numberOfSubhalos; sindx++) {
          sStream << "  Subhalo: " << sindx << endl
                  << "    count = " << fofSubhaloCount[sindx] << endl
                  << "    mass = "  << subhaloMass[sindx] << endl
                  << "    center of mass = ["
                                  << subhaloXCofMass[sindx] << ","
                                  << subhaloYCofMass[sindx] << ","
                                  << subhaloZCofMass[sindx] << "]" << endl
                  << "    avg loc = ["
                                  << subhaloXPos[sindx] << ","
                                  << subhaloYPos[sindx] << ","
                                  << subhaloZPos[sindx] << "]" << endl
                  << "    avg vel = ["
                                  << subhaloXVel[sindx] << ","
                                  << subhaloYVel[sindx] << ","
                                  << subhaloZVel[sindx] << "]" << endl
                  << "    velocity dispersion = "
                                  << subhaloVelDisp[sindx] << endl;
        }
        sStream << "------------------------------------------" << endl << endl;

        // Write individual subhalos to file
        ostringstream name;
        name << outFile << "_subhalo_" << halo
             << "_" << particleCount << ".cosmo";
        string fileName = name.str();
        subFinder->writeSubhaloCosmoFile(fileName);

        delete [] xLocHalo;
        delete [] yLocHalo;
        delete [] zLocHalo;
        delete [] xVelHalo;
        delete [] yVelHalo;
        delete [] zVelHalo;
        delete [] massHalo;
        delete [] id;
        delete [] actualIndx;

        delete subFinder;
      }
    }
  }
  Timings::stopTimer(shtimer);
}

/////////////////////////////////////////////////////////////////////////////
//
// Write halo catalog
//
/////////////////////////////////////////////////////////////////////////////

void HaloFinder::FOFHaloCatalog()
{
  int* fofHalos = this->haloFinder.getHalos();
  int* fofHaloCount = this->haloFinder.getHaloCount();

  if (this->haloIn.getOutputHaloCatalog() == 1 && this->fofCenter->size() > 0)
    this->fof.FOFHaloCatalog(this->fofCenter, this->fofMass,
                             this->fofXVel, this->fofYVel, this->fofZVel);

  // Write a summary of FOF properties
  if (this->haloIn.getOutputFOFProperties() == 1) {
    ostringstream sname;
    if (numProc == 1) {
      sname << outFile << ".fofproperties";
    } else {
      sname << outFile << ".fofproperties." << myProc;
    }
    ofstream sStream(sname.str().c_str(), ios::out);

    for (int halo = 0; halo < numberOfFOFHalos; halo++) {
      int center = (*fofCenter)[halo];
      sStream << "Halo: " << halo << endl
              << "  FOF count = " << fofHaloCount[halo] << endl
              << "  FOF tag = " << fofHalos[halo] << endl
              << "  FOF mass = " << (*this->fofMass)[halo] << endl
              << "  FOF center = ["
                                 << (*this->xx)[center] << ","
                                 << (*this->yy)[center] << ","
                                 << (*this->zz)[center] << "]" << endl
              << "  FOF center of mass = ["
                                 << (*this->fofXCofMass)[halo] << ","
                                 << (*this->fofYCofMass)[halo] << ","
                                 << (*this->fofZCofMass)[halo] << "]" << endl
              << "  FOF avg loc = ["
                                 << (*this->fofXPos)[halo] << ","
                                 << (*this->fofYPos)[halo] << ","
                                 << (*this->fofZPos)[halo] << "]" << endl
              << "  FOF avg vel = ["
                                 << (*this->fofXVel)[halo] << ","
                                 << (*this->fofYVel)[halo] << ","
                                 << (*this->fofZVel)[halo] << "]" << endl
              << "  FOF velocity dispersion = "
                                 << (*this->fofVelDisp)[halo] << endl;
    }
  }
}

////////////////////////////////////////////////////////////////////////////
//
// SOD (Spherical Over Density) halo profile requires a chaining mesh
// because it must examine all particles, not just those in FOF
// Requires the FOF halo center
//
////////////////////////////////////////////////////////////////////////////

void HaloFinder::SODHaloFinding()
{
  static Timings::TimerRef sodtimer = Timings::getTimer("SOD Halo Finder");
  Timings::startTimer(sodtimer);

  int* fofHalos = this->haloFinder.getHalos();
  int* fofHaloCount = this->haloFinder.getHaloCount();

  if (this->haloIn.getOutputSODProperties() == 1) {
    if (this->myProc == 0)
      cout << "Run SOD halo finder" << endl;

    // Open files to hold the SOD output
    ostringstream sname;
    if (numProc == 1) {
      sname << outFile << ".sodproperties";
    } else {
      sname << outFile << ".sodproperties." << myProc;
    }
    ofstream sStream(sname.str().c_str(), ios::out);

    // Construct the chaining mesh of all particles on this processor which
    // is used to determine particles in the SOD
    POSVEL_T chainSize = CHAIN_SIZE;
    ChainingMesh* chain = new ChainingMesh(rL, deadSize, chainSize, xx, yy, zz);
    MPI_Barrier(MPI_COMM_WORLD);

    // SOD regions are built using the center of an FOF halo of minimum size
    // Initial radius is calculated from mass of halo and factors are
    // applied to get a radius range to examine to find the density to
    // critical density ratio desired
    POSVEL_T minHaloMass = MIN_SOD_MASS;
    int numberOfBins = NUM_SOD_BINS;
    POSVEL_T rhoRatio = RHO_RATIO;
    POSVEL_T cMinFactor = MIN_RADIUS_FACTOR;
    POSVEL_T cMaxFactor = MAX_RADIUS_FACTOR;

    // For every FOF halo construct a SOD halo
    for (int halo = 0; halo < numberOfFOFHalos; halo++) {

      // Only build SOD for large enough FOF halos
      if ((*fofMass)[halo] > minHaloMass) {

        // Construct around the center particle of the FOF halo
        int center = (*fofCenter)[halo];

        SODHalo* sod = new SODHalo();
        sod->setParameters(chain, numberOfBins, rL, np,
                           this->RHOC, this->SODMASS,
                           rhoRatio, cMinFactor, cMaxFactor);
        sod->setParticles(this->xx, this->yy, this->zz,
                          this->vx, this->vy, this->vz, this->mass, this->tag);

        // SOD halos are calculated from the center particle location of FOF
        // Send average velocity of FOF halo for calculating radial velocity
        sod->createSODHalo(
             fofHaloCount[halo],
             (*xx)[center],
             (*yy)[center],
             (*zz)[center],
             (*fofXVel)[halo],
             (*fofYVel)[halo],
             (*fofZVel)[halo],
             (*fofMass)[halo]);

        // SOD halo properties
        long particleCount = sod->SODHaloSize();
        if (particleCount > 0) {

          POSVEL_T sodRadius = sod->SODRadius();
          POSVEL_T sodCenterOfMass[DIMENSION];
          POSVEL_T sodAverageLocation[DIMENSION];
          POSVEL_T sodMinPotLocation[DIMENSION];
          POSVEL_T sodAverageVelocity[DIMENSION];
          POSVEL_T sodVelDisp;
          POSVEL_T sodMass;

          sod->SODCenterOfMass(sodCenterOfMass);
          sod->SODAverageLocation(sodAverageLocation);
          sod->SODAverageVelocity(sodAverageVelocity);
          sod->SODVelocityDispersion(&sodVelDisp);
          sod->SODMass(&sodMass);

          // Get information for profiles which is in bins
          int binCount[numberOfBins];		// Number per bin
          POSVEL_T binMass[numberOfBins];	// Mass per bin
          POSVEL_T binRadius[numberOfBins];	// Radius of bin
          POSVEL_T binRho[numberOfBins];	// total mass / volume at radius
          POSVEL_T binRhoRatio[numberOfBins];	// rho / rho_c
          POSVEL_T binRadVelocity[numberOfBins];// avg radial velocity

          sod->SODProfile(binCount, binMass, binRadius,
                          binRho, binRhoRatio, binRadVelocity);

          // Show how to extract information from SODHalo
          POSVEL_T* xLocHalo = new POSVEL_T[particleCount];
          POSVEL_T* yLocHalo = new POSVEL_T[particleCount];
          POSVEL_T* zLocHalo = new POSVEL_T[particleCount];
          POSVEL_T* xVelHalo = new POSVEL_T[particleCount];
          POSVEL_T* yVelHalo = new POSVEL_T[particleCount];
          POSVEL_T* zVelHalo = new POSVEL_T[particleCount];
          POSVEL_T* massHalo = new POSVEL_T[particleCount];
          POSVEL_T* radius = new POSVEL_T[particleCount];
          ID_T* id = new ID_T[particleCount];

          // Map halo index to actual particle index on this processor
          // Different still from id tag which is unique across all processors
          int* actualIndx = new int[particleCount];

          sod->extractInformation(actualIndx,
                               xLocHalo, yLocHalo, zLocHalo,
                               xVelHalo, yVelHalo, zVelHalo,
                               massHalo, radius, id);

          // Most bound particle method of center finding
          int centerIndex;
          POTENTIAL_T minPotential;
          if (this->haloIn.getUseMBPCenterFinder() == 1) {
            centerIndex = MBPCenterFinding(&minPotential, particleCount,
                                  xLocHalo, yLocHalo, zLocHalo, massHalo, id);
            sodMinPotLocation[0] = (*xx)[actualIndx[centerIndex]];
            sodMinPotLocation[1] = (*yy)[actualIndx[centerIndex]];
            sodMinPotLocation[2] = (*zz)[actualIndx[centerIndex]];
          }

          // Most connected particle method of center finding
          else if (this->haloIn.getUseMCPCenterFinder() == 1) {
            centerIndex = MCPCenterFinding(particleCount,
                                  xLocHalo, yLocHalo, zLocHalo, massHalo, id);
            sodMinPotLocation[0] = (*xx)[actualIndx[centerIndex]];
            sodMinPotLocation[1] = (*yy)[actualIndx[centerIndex]];
            sodMinPotLocation[2] = (*zz)[actualIndx[centerIndex]];
          }

          // Write profile information
          sStream << "Halo " << (*tag)[fofHalos[halo]] << endl
                  << "  FOF count = " << fofHaloCount[halo] << endl
                  << "  FOF center = ["
                                     << (*xx)[center] << " , "
                                     << (*yy)[center] << " , "
                                     << (*zz)[center] << "]" << endl
                  << "  SOD count = " << particleCount << endl
                  << "  SOD radius = " << sodRadius << endl
                  << "  SOD mass = " << sodMass << endl
                  << "  SOD min pot location = ["
                                     << sodMinPotLocation[0] << " , "
                                     << sodMinPotLocation[1] << " , "
                                     << sodMinPotLocation[2] << "]" << endl
                  << "  SOD center of mass = ["
                                     << sodCenterOfMass[0] << " , "
                                     << sodCenterOfMass[1] << " , "
                                     << sodCenterOfMass[2] << "]" << endl
                  << "  SOD avg location = ["
                                     << sodAverageLocation[0] << " , "
                                     << sodAverageLocation[1] << " , "
                                     << sodAverageLocation[2] << "]" << endl
                  << "  SOD velocity = ["
                                     << sodAverageVelocity[0] << " , "
                                     << sodAverageVelocity[1] << " , "
                                     << sodAverageVelocity[2] << "]" << endl
                  << "  SOD velocity dispersion = " << sodVelDisp << endl;

          for (int bin = 0; bin < numberOfBins; bin++)
            sStream << "    Bin " << bin
                    << " count: " << binCount[bin]
                    << " mass: " << binMass[bin]
                    << " radius: " << binRadius[bin]
                    << " rho: " << binRho[bin]
                    << " rho ratio: " << binRhoRatio[bin]
                    << " rad vel: " << binRadVelocity[bin] << endl;

          delete [] xLocHalo;
          delete [] yLocHalo;
          delete [] zLocHalo;
          delete [] xVelHalo;
          delete [] yVelHalo;
          delete [] zVelHalo;
          delete [] massHalo;
          delete [] radius;
          delete [] id;
          delete [] actualIndx;
          delete sod;
        }
      }
    }
    delete chain;
  }
  Timings::stopTimer(sodtimer);
}

/////////////////////////////////////////////////////////////////////////////
//
// Write a .cosmo file of the requested halo
//
/////////////////////////////////////////////////////////////////////////////

void HaloFinder::WriteCosmoFiles(int size)
{
  int* fofHaloCount = this->haloFinder.getHaloCount();
  for (int halo = 0; halo < this->numberOfFOFHalos; halo++) {

    long particleCount = fofHaloCount[halo];
    if (particleCount == size && halo == 0) {

      ostringstream name;
      name << "SubHalo_" << particleCount << ".cosmo";
      ofstream cStream(name.str().c_str(), ios::out|ios::binary);

      float fBlock[COSMO_FLOAT];
      int iBlock[COSMO_INT];

      // Allocate arrays which will hold halo particle information
      POSVEL_T* xLocHalo = new POSVEL_T[particleCount];
      POSVEL_T* yLocHalo = new POSVEL_T[particleCount];
      POSVEL_T* zLocHalo = new POSVEL_T[particleCount];
      POSVEL_T* xVelHalo = new POSVEL_T[particleCount];
      POSVEL_T* yVelHalo = new POSVEL_T[particleCount];
      POSVEL_T* zVelHalo = new POSVEL_T[particleCount];
      POSVEL_T* massHalo = new POSVEL_T[particleCount];
      ID_T* id = new ID_T[particleCount];

      // Need array to map halo index to actual particle index to find locations
      int* actualIndx = new int[particleCount];

      this->fof.extractInformation(halo, actualIndx,
                                   xLocHalo, yLocHalo, zLocHalo,
                                   xVelHalo, yVelHalo, zVelHalo, massHalo, id);

      for (int p = 0; p < particleCount; p++) {
        fBlock[0] = xLocHalo[p];
        fBlock[1] = xVelHalo[p];
        fBlock[2] = yLocHalo[p];
        fBlock[3] = yVelHalo[p];
        fBlock[4] = zLocHalo[p];
        fBlock[5] = zVelHalo[p];
        fBlock[6] = massHalo[p];
        cStream.write(reinterpret_cast<char*>(fBlock),
                      COSMO_FLOAT * sizeof(POSVEL_T));
        iBlock[0] = id[p];
        cStream.write(reinterpret_cast<char*>(iBlock),
                      COSMO_INT * sizeof(ID_T));
      }
      cStream.close();

      delete [] xLocHalo;
      delete [] yLocHalo;
      delete [] zLocHalo;
      delete [] xVelHalo;
      delete [] yVelHalo;
      delete [] zVelHalo;
      delete [] massHalo;
      delete [] id;
      delete [] actualIndx;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Test driver
//
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  // Initialize MPI and set the decomposition in the Partition class
  MPI_Init(&argc, &argv);
  Partition::initialize();

  // Construct the tester class
  HaloFinder haloFinder(argc, argv);

  // Read, distribute and exchange particles
  haloFinder.DistributeParticles();
  MPI_Barrier(MPI_COMM_WORLD);

  // Run the parallel FOF halo finder and collect the resulting structure
  haloFinder.FOFHaloFinder();
  MPI_Barrier(MPI_COMM_WORLD);

  // Run the simple FOF properties on all halos using the haloList structure
  // This must be run for either SOD or subhalo because extract of
  // FOF particles is needed
  haloFinder.BasicFOFHaloProperties();

  // Find particle center of halo using most bound, most connected particle
  // Required for SOD halo finding
  haloFinder.FOFCenterFinding();

  // Write the halo catalog and a summary of FOF properties
  haloFinder.FOFHaloCatalog();

  // Find SOD halos
  haloFinder.SODHaloFinding();
  MPI_Barrier(MPI_COMM_WORLD);

  // Find subhalos within each FOF halo
  haloFinder.FOFSubHaloFinding();
  cout << "Rank " << Partition::getMyProc() << " FINISHED " << endl;

  MPI_Barrier(MPI_COMM_WORLD);
  Timings::print();

  // Shut down MPI
  Partition::finalize();
  MPI_Finalize();

  return 0;
}

}
