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

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <sys/types.h>

#include "Partition.h"
#include "ParticleExchange.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// ParticleExchange is initialized with physical size of particle space and
// the margin of dead zone desired for each processor.  It is given the
// physical x,y,z locations for particles on this processor and can get
// the number of each neighbor processor.  Since the desired goal is to
// populate every processor with the alive particles (which it enters this
// class with) and dead particles belonging on the edges of all neighbors,
// each processor categorizes its own particles and arranges to send them
// to the appropriate neighbor, and to receive particles from each neighbor
// which it adds the the location vectors.
//
/////////////////////////////////////////////////////////////////////////

ParticleExchange::ParticleExchange()
{
  // Get the number of processors running this problem and rank
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();

  // Get the number of processors in each dimension
  Partition::getDecompSize(this->layoutSize);

  // Get my position within the Cartesian topology
  Partition::getMyPosition(this->layoutPos);

  // Get neighbors of this processor including the wraparound
  Partition::getNeighbors(this->neighbor);

  // For this processor calculate alterations needed for wraparound locations
  calculateOffsetFactor();

  this->numberOfAliveParticles = 0;
  this->numberOfDeadParticles = 0;
}

ParticleExchange::~ParticleExchange()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for particle distribution
//
/////////////////////////////////////////////////////////////////////////

void ParticleExchange::setParameters(POSVEL_T rL, POSVEL_T deadSz)
{
  // Physical total space and amount of physical space to use for dead particles
  this->boxSize = rL;
  this->deadSize = deadSz;

#ifndef USE_VTK_COSMO
  if (this->myProc == MASTER) {
    cout << endl << "------------------------------------" << endl;
    cout << "boxSize:  " << this->boxSize << endl;
    cout << "deltaBox: " << this->deadSize << endl;
  }
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// ParticleExchange will start with only ALIVE particles and will determine
// which of those particles must be sent to neighbors for the overloading
// of DEAD particles.  As a particle is examined it may fall into several
// sharing regions.  For instance a particle in a corner will be sent across
// three faces, three edges and one corner.  As it is sent the x,y,z must
// be altered in different ways.  Face overloading requires changing one
// dimension's location, while corner overloading require three changes.
// And these changes are only needed for processors on an edge of the
// decomposition where layoutPos = 0 or layoutPos = layoutSize - 1.
//
// This method calculates a simple matrix which can be applied at the
// time that the exchange buffer is filled with locations.  The rule for
// sending a location is location = location + (overLoadFactor * boxSize);
//
// The factors are
//      0       location in that dimension is not changed
//     +1       location in that dimension is incremented by box size
//     -1       location in that dimension is decremented by box size
//
/////////////////////////////////////////////////////////////////////////

void ParticleExchange::calculateOffsetFactor()
{
   // Default is that a location is not changed when shared with a neighbor
   // This is the case for all interior processors
   for (int n = 0; n < NUM_OF_NEIGHBORS; n++)
      for (int dim = 0; dim < DIMENSION; dim++)
         this->overLoadFactor[n][dim] = 0;

   // If this processor is on the edge of the decomposition then when it
   // sends overloaded locations they must be altered.  This will depend on
   // the position of this processor in the layout and on the neighbor 
   // which is receiving the data

   // Processor is on front edge in X dimension so add rL to wraparound x
   if (this->layoutPos[0] == 0) {
      this->overLoadFactor[X0][0] = 1;
      this->overLoadFactor[X0_Y0][0] = 1;
      this->overLoadFactor[X0_Y1][0] = 1;
      this->overLoadFactor[Z0_X0][0] = 1;
      this->overLoadFactor[Z1_X0][0] = 1;
      this->overLoadFactor[X0_Y0_Z0][0] = 1;
      this->overLoadFactor[X0_Y0_Z1][0] = 1;
      this->overLoadFactor[X0_Y1_Z0][0] = 1;
      this->overLoadFactor[X0_Y1_Z1][0] = 1;
   }

   // Processor is on back edge in X dimension so subtract rL from wraparound x
   if (this->layoutPos[0] == (this->layoutSize[0] - 1)) {
      this->overLoadFactor[X1][0] = -1;
      this->overLoadFactor[X1_Y1][0] = -1;
      this->overLoadFactor[X1_Y0][0] = -1;
      this->overLoadFactor[Z1_X1][0] = -1;
      this->overLoadFactor[Z0_X1][0] = -1;
      this->overLoadFactor[X1_Y1_Z1][0] = -1;
      this->overLoadFactor[X1_Y1_Z0][0] = -1;
      this->overLoadFactor[X1_Y0_Z1][0] = -1;
      this->overLoadFactor[X1_Y0_Z0][0] = -1;
   }

   // Processor is on front edge in Y dimension so add rL to wraparound y
   if (this->layoutPos[1] == 0) {
      this->overLoadFactor[Y0][1] = 1;
      this->overLoadFactor[X0_Y0][1] = 1;
      this->overLoadFactor[X1_Y0][1] = 1;
      this->overLoadFactor[Y0_Z0][1] = 1;
      this->overLoadFactor[Y0_Z1][1] = 1;
      this->overLoadFactor[X0_Y0_Z0][1] = 1;
      this->overLoadFactor[X0_Y0_Z1][1] = 1;
      this->overLoadFactor[X1_Y0_Z1][1] = 1;
      this->overLoadFactor[X1_Y0_Z0][1] = 1;
   }

   // Processor is on back edge in Y dimension so subtract rL from wraparound y
   if (this->layoutPos[1] == (this->layoutSize[1] - 1)) {
      this->overLoadFactor[Y1][1] = -1;
      this->overLoadFactor[X1_Y1][1] = -1;
      this->overLoadFactor[X0_Y1][1] = -1;
      this->overLoadFactor[Y1_Z1][1] = -1;
      this->overLoadFactor[Y1_Z0][1] = -1;
      this->overLoadFactor[X1_Y1_Z1][1] = -1;
      this->overLoadFactor[X1_Y1_Z0][1] = -1;
      this->overLoadFactor[X0_Y1_Z0][1] = -1;
      this->overLoadFactor[X0_Y1_Z1][1] = -1;
   }

   // Processor is on front edge in Z dimension so add rL to wraparound z
   if (this->layoutPos[2] == 0) {
      this->overLoadFactor[Z0][2] = 1;
      this->overLoadFactor[Y0_Z0][2] = 1;
      this->overLoadFactor[Y1_Z0][2] = 1;
      this->overLoadFactor[Z0_X0][2] = 1;
      this->overLoadFactor[Z0_X1][2] = 1;
      this->overLoadFactor[X0_Y0_Z0][2] = 1;
      this->overLoadFactor[X1_Y1_Z0][2] = 1;
      this->overLoadFactor[X0_Y1_Z0][2] = 1;
      this->overLoadFactor[X1_Y0_Z0][2] = 1;
   }

   // Processor is on back edge in Z dimension so subtract rL from wraparound z
   if (this->layoutPos[2] == (this->layoutSize[2] - 1)) {
      this->overLoadFactor[Z1][2] = -1;
      this->overLoadFactor[Y1_Z1][2] = -1;
      this->overLoadFactor[Y0_Z1][2] = -1;
      this->overLoadFactor[Z1_X1][2] = -1;
      this->overLoadFactor[Z1_X0][2] = -1;
      this->overLoadFactor[X1_Y1_Z1][2] = -1;
      this->overLoadFactor[X0_Y0_Z1][2] = -1;
      this->overLoadFactor[X1_Y0_Z1][2] = -1;
      this->overLoadFactor[X0_Y1_Z1][2] = -1;
   }
}

/////////////////////////////////////////////////////////////////////////
//
// All particles on this processor initially are alive, but some of those
// alive must be exchanged with neighbors.  Determine the physical range
// on this processor where an ALIVE particle will never be exchanged and
// the ranges for each neighbor's future DEAD particles.  Then when
// reading each particle it can quickly be assigned.
//
/////////////////////////////////////////////////////////////////////////

void ParticleExchange::initialize()
{
#ifndef USE_VTK_COSMO
#ifdef DEBUG
  if (this->myProc == MASTER)
    cout << "Decomposition: [" << this->layoutSize[0] << ":"
         << this->layoutSize[1] << ":" << this->layoutSize[2] << "]" << endl;
#endif
#endif

  // Set subextents on particle locations for this processor
  POSVEL_T boxStep[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    boxStep[dim] = this->boxSize / this->layoutSize[dim];

    // All particles are alive and available for sharing
    this->minShare[dim] = this->layoutPos[dim] * boxStep[dim];
    this->maxShare[dim] = this->minShare[dim] + boxStep[dim];
    if (this->maxShare[dim] > this->boxSize)
      this->maxShare[dim] = this->boxSize;

    // Particles in the middle of the shared region will not be shared
    this->minMine[dim] = this->minShare[dim] + this->deadSize;
    this->maxMine[dim] = this->maxShare[dim] - this->deadSize;
  }

  // Set the ranges on the dead particles for each neighbor direction
  calculateExchangeRegions();
}

/////////////////////////////////////////////////////////////////////////
//
// Each of the 26 neighbors will be sent a rectangular region of my particles
// Calculate the range in each dimension of the ghost area
//
/////////////////////////////////////////////////////////////////////////

void ParticleExchange::calculateExchangeRegions()
{
  // Initialize all neighbors to the entire available exchange range
  for (int i = 0; i < NUM_OF_NEIGHBORS; i++) {
    for (int dim = 0; dim < DIMENSION; dim++) {
      this->minRange[i][dim] = this->minShare[dim];
      this->maxRange[i][dim] = this->maxShare[dim];
    }
  }

  // Left face
  this->minRange[X0][0] = this->minShare[0];
  this->maxRange[X0][0] = this->minMine[0];

  // Right face
  this->minRange[X1][0] = this->maxMine[0];
  this->maxRange[X1][0] = this->maxShare[0];

  // Bottom face
  this->minRange[Y0][1] = this->minShare[1];
  this->maxRange[Y0][1] = this->minMine[1];

  // Top face
  this->minRange[Y1][1] = this->maxMine[1];
  this->maxRange[Y1][1] = this->maxShare[1];

  // Front face
  this->minRange[Z0][2] = this->minShare[2];
  this->maxRange[Z0][2] = this->minMine[2];

  // Back face
  this->minRange[Z1][2] = this->maxMine[2];
  this->maxRange[Z1][2] = this->maxShare[2];

  // Left bottom and top bars
  this->minRange[X0_Y0][0] = this->minShare[0];
  this->maxRange[X0_Y0][0] = this->minMine[0];
  this->minRange[X0_Y0][1] = this->minShare[1];
  this->maxRange[X0_Y0][1] = this->minMine[1];

  this->minRange[X0_Y1][0] = this->minShare[0];
  this->maxRange[X0_Y1][0] = this->minMine[0];
  this->minRange[X0_Y1][1] = this->maxMine[1];
  this->maxRange[X0_Y1][1] = this->maxShare[1];

  // Right bottom and top bars
  this->minRange[X1_Y0][0] = this->maxMine[0];
  this->maxRange[X1_Y0][0] = this->maxShare[0];
  this->minRange[X1_Y0][1] = this->minShare[1];
  this->maxRange[X1_Y0][1] = this->minMine[1];

  this->minRange[X1_Y1][0] = this->maxMine[0];
  this->maxRange[X1_Y1][0] = this->maxShare[0];
  this->minRange[X1_Y1][1] = this->maxMine[1];
  this->maxRange[X1_Y1][1] = this->maxShare[1];

  // Bottom front and back bars
  this->minRange[Y0_Z0][1] = this->minShare[1];
  this->maxRange[Y0_Z0][1] = this->minMine[1];
  this->minRange[Y0_Z0][2] = this->minShare[2];
  this->maxRange[Y0_Z0][2] = this->minMine[2];

  this->minRange[Y0_Z1][1] = this->minShare[1];
  this->maxRange[Y0_Z1][1] = this->minMine[1];
  this->minRange[Y0_Z1][2] = this->maxMine[2];
  this->maxRange[Y0_Z1][2] = this->maxShare[2];

  // Top front and back bars 
  this->minRange[Y1_Z0][1] = this->maxMine[1];
  this->maxRange[Y1_Z0][1] = this->maxShare[1];
  this->minRange[Y1_Z0][2] = this->minShare[2];
  this->maxRange[Y1_Z0][2] = this->minMine[2];

  this->minRange[Y1_Z1][1] = this->maxMine[1];
  this->maxRange[Y1_Z1][1] = this->maxShare[1];
  this->minRange[Y1_Z1][2] = this->maxMine[2];
  this->maxRange[Y1_Z1][2] = this->maxShare[2];

  // Left front and back bars (vertical)
  this->minRange[Z0_X0][0] = this->minShare[0];
  this->maxRange[Z0_X0][0] = this->minMine[0];
  this->minRange[Z0_X0][2] = this->minShare[2];
  this->maxRange[Z0_X0][2] = this->minMine[2];

  this->minRange[Z1_X0][0] = this->minShare[0];
  this->maxRange[Z1_X0][0] = this->minMine[0];
  this->minRange[Z1_X0][2] = this->maxMine[2];
  this->maxRange[Z1_X0][2] = this->maxShare[2];

  // Right front and back bars (vertical)
  this->minRange[Z0_X1][0] = this->maxMine[0];
  this->maxRange[Z0_X1][0] = this->maxShare[0];
  this->minRange[Z0_X1][2] = this->minShare[2];
  this->maxRange[Z0_X1][2] = this->minMine[2];

  this->minRange[Z1_X1][0] = this->maxMine[0];
  this->maxRange[Z1_X1][0] = this->maxShare[0];
  this->minRange[Z1_X1][2] = this->maxMine[2];
  this->maxRange[Z1_X1][2] = this->maxShare[2];

  // Left bottom front corner
  this->minRange[X0_Y0_Z0][0] = this->minShare[0];
  this->maxRange[X0_Y0_Z0][0] = this->minMine[0];
  this->minRange[X0_Y0_Z0][1] = this->minShare[1];
  this->maxRange[X0_Y0_Z0][1] = this->minMine[1];
  this->minRange[X0_Y0_Z0][2] = this->minShare[2];
  this->maxRange[X0_Y0_Z0][2] = this->minMine[2];

  // Left bottom back corner
  this->minRange[X0_Y0_Z1][0] = this->minShare[0];
  this->maxRange[X0_Y0_Z1][0] = this->minMine[0];
  this->minRange[X0_Y0_Z1][1] = this->minShare[1];
  this->maxRange[X0_Y0_Z1][1] = this->minMine[1];
  this->minRange[X0_Y0_Z1][2] = this->maxMine[2];
  this->maxRange[X0_Y0_Z1][2] = this->maxShare[2];

  // Left top front corner
  this->minRange[X0_Y1_Z0][0] = this->minShare[0];
  this->maxRange[X0_Y1_Z0][0] = this->minMine[0];
  this->minRange[X0_Y1_Z0][1] = this->maxMine[1];
  this->maxRange[X0_Y1_Z0][1] = this->maxShare[1];
  this->minRange[X0_Y1_Z0][2] = this->minShare[2];
  this->maxRange[X0_Y1_Z0][2] = this->minMine[2];

  // Left top back corner
  this->minRange[X0_Y1_Z1][0] = this->minShare[0];
  this->maxRange[X0_Y1_Z1][0] = this->minMine[0];
  this->minRange[X0_Y1_Z1][1] = this->maxMine[1];
  this->maxRange[X0_Y1_Z1][1] = this->maxShare[1];
  this->minRange[X0_Y1_Z1][2] = this->maxMine[2];
  this->maxRange[X0_Y1_Z1][2] = this->maxShare[2];

  // Right bottom front corner
  this->minRange[X1_Y0_Z0][0] = this->maxMine[0];
  this->maxRange[X1_Y0_Z0][0] = this->maxShare[0];
  this->minRange[X1_Y0_Z0][1] = this->minShare[1];
  this->maxRange[X1_Y0_Z0][1] = this->minMine[1];
  this->minRange[X1_Y0_Z0][2] = this->minShare[2];
  this->maxRange[X1_Y0_Z0][2] = this->minMine[2];

  // Right bottom back corner
  this->minRange[X1_Y0_Z1][0] = this->maxMine[0];
  this->maxRange[X1_Y0_Z1][0] = this->maxShare[0];
  this->minRange[X1_Y0_Z1][1] = this->minShare[1];
  this->maxRange[X1_Y0_Z1][1] = this->minMine[1];
  this->minRange[X1_Y0_Z1][2] = this->maxMine[2];
  this->maxRange[X1_Y0_Z1][2] = this->maxShare[2];

  // Right top front corner
  this->minRange[X1_Y1_Z0][0] = this->maxMine[0];
  this->maxRange[X1_Y1_Z0][0] = this->maxShare[0];
  this->minRange[X1_Y1_Z0][1] = this->maxMine[1];
  this->maxRange[X1_Y1_Z0][1] = this->maxShare[1];
  this->minRange[X1_Y1_Z0][2] = this->minShare[2];
  this->maxRange[X1_Y1_Z0][2] = this->minMine[2];

  // Right top back corner
  this->minRange[X1_Y1_Z1][0] = this->maxMine[0];
  this->maxRange[X1_Y1_Z1][0] = this->maxShare[0];
  this->minRange[X1_Y1_Z1][1] = this->maxMine[1];
  this->maxRange[X1_Y1_Z1][1] = this->maxShare[1];
  this->minRange[X1_Y1_Z1][2] = this->maxMine[2];
  this->maxRange[X1_Y1_Z1][2] = this->maxShare[2];
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void ParticleExchange::setParticles(
                        vector<POSVEL_T>* xLoc,
                        vector<POSVEL_T>* yLoc,
                        vector<POSVEL_T>* zLoc,
                        vector<POSVEL_T>* xVel,
                        vector<POSVEL_T>* yVel,
                        vector<POSVEL_T>* zVel,
                        vector<POSVEL_T>* mass,
                        vector<POTENTIAL_T>* potential,
                        vector<ID_T>* id,
                        vector<MASK_T>* maskData,
                        vector<STATUS_T>* type)
{
  this->particleCount = (long)xLoc->size();
  this->numberOfAliveParticles = this->particleCount;
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->vx = xVel;
  this->vy = yVel;
  this->vz = zVel;
  this->ms = mass;
  this->pot = potential;
  this->tag = id;
  this->mask = maskData;
  this->status = type;
  this->status->clear();
}
        
/////////////////////////////////////////////////////////////////////////////
//
// Alive particles are contained on each processor.  Identify the border
// particles which will be dead on other processors and exchange them
//
/////////////////////////////////////////////////////////////////////////////

void ParticleExchange::exchangeParticles()
{
  // Identify alive particles on this processor which must be shared
  // because they are dead particles on neighbor processors
  // x,y,z are still in physical units (because deadSize is given that way)
  identifyExchangeParticles();

  // Exchange those particles with appropriate neighbors
  // x,y,z are not in normalized units
  exchangeNeighborParticles();

  // Count the particles across processors
  long totalAliveParticles = 0;
  long totalDeadParticles = 0;

#ifdef USE_SERIAL_COSMO
  totalAliveParticles = this->numberOfAliveParticles;
  totalDeadParticles = this->numberOfDeadParticles;
#else
  MPI_Allreduce((void*) &this->numberOfAliveParticles, 
                (void*) &totalAliveParticles, 
                1, MPI_LONG, MPI_SUM, Partition::getComm());
  MPI_Allreduce((void*) &this->numberOfDeadParticles,
                (void*) &totalDeadParticles, 
                1, MPI_LONG, MPI_SUM, Partition::getComm());
#endif

#ifndef USE_VTK_COSMO
#ifdef DEBUG
  cout << "Exchange Particles Rank " << setw(3) << this->myProc 
       << " #alive = " << this->numberOfAliveParticles
       << " #dead = " << this->numberOfDeadParticles << endl;
#endif
 
  if (this->myProc == MASTER) {
    cout << "TotalAliveParticles " << totalAliveParticles << endl;
    cout << "TotalDeadParticles  " << totalDeadParticles << endl << endl;
  }
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
// Iterate over all the alive particles on this processor and determine
// which must be shared and add them to the vector for that neighbor
//
/////////////////////////////////////////////////////////////////////////////

void ParticleExchange::identifyExchangeParticles()
{
  long notSharedCount = 0;
  long sharedCount = 0;

  // All initial particles before the exchange are ALIVE
  for (long i = 0; i < this->particleCount; i++) {
    this->status->push_back(ALIVE);
    if (((*this->xx)[i] > this->minMine[0] && 
         (*this->xx)[i] < this->maxMine[0]) &&
        ((*this->yy)[i] > this->minMine[1] && 
         (*this->yy)[i] < this->maxMine[1]) &&
        ((*this->zz)[i] > this->minMine[2] && 
         (*this->zz)[i] < this->maxMine[2])) {
          notSharedCount++;
    } else {
      // Particle is alive here but which processors need it as dead
      for (int n = 0; n < NUM_OF_NEIGHBORS; n++) {
        if ((*this->xx)[i] >= minRange[n][0] && 
            (*this->xx)[i] <= maxRange[n][0] &&
            (*this->yy)[i] >= minRange[n][1] && 
            (*this->yy)[i] <= maxRange[n][1] &&
            (*this->zz)[i] >= minRange[n][2] && 
            (*this->zz)[i] <= maxRange[n][2]) {
                this->neighborParticles[n].push_back(i);
                sharedCount++;
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Exchange the appropriate particles with neighbors
// Only the index of the particle to be exchanged is stored so fill out
// the message with location, velocity, tag.  Status information doesn't
// have to be sent because when the message is received, the neighbor
// containing the new dead particle will be known
//
// Use the Cartesian communicator for neighbor exchange
//
/////////////////////////////////////////////////////////////////////////////

void ParticleExchange::exchangeNeighborParticles()
{
  // Calculate the maximum number of particles to share for calculating buffer
  int myShareSize = 0;
  for (int n = 0; n < NUM_OF_NEIGHBORS; n++)
    if (myShareSize < (int)this->neighborParticles[n].size())
      myShareSize = (int)this->neighborParticles[n].size();

  int maxShareSize;
#ifdef USE_SERIAL_COSMO
  maxShareSize = myShareSize;
#else
  MPI_Allreduce((void*) &myShareSize,
                (void*) &maxShareSize,
                1, MPI_INT, MPI_MAX, Partition::getComm());
#endif

  // Allocate messages to send and receive MPI buffers
  int bufferSize = (1 * sizeof(int)) +          // number of particles
        (maxShareSize * 
          ((COSMO_FLOAT * sizeof(POSVEL_T)) +   // location, velocity, mass
           (1 * sizeof(POSVEL_T)) +             // potential
           (COSMO_INT * sizeof(ID_T)) +        // id tag
           (1 * sizeof(MASK_T))));             // mask

  Message* sendMessage = new Message(bufferSize);
  Message* recvMessage = new Message(bufferSize);

#ifndef USE_VTK_COSMO
  //debug statement added by Adrian to see how much buffer space we're using
  if(this->myProc == MASTER) {
    printf("PXCH buffer = 2*%d = %f MB\n",bufferSize,
           2.0*bufferSize/1024.0/1024.0);
  }
#endif

#ifndef USE_SERIAL_COSMO
  MPI_Barrier(Partition::getComm());
#endif

  // Exchange with each neighbor, with everyone sending in one direction and
  // receiving from the other.  Data corresponding to the particle index
  // must be packed in the buffer.  When the data is received it is unpacked
  // into the location, velocity and tag vectors and the status is set
  // to the neighbor who sent it

  for (int n = 0; n < NUM_OF_NEIGHBORS; n=n+2) {
    // Neighbor pairs in Definition.h must match so that every processor
    // sends and every processor receives on each exchange
    exchange(n, n+1, sendMessage, recvMessage);
    exchange(n+1, n, sendMessage, recvMessage);
  }

  delete sendMessage;
  delete recvMessage;
}

/////////////////////////////////////////////////////////////////////////////
//
// Pack particle data for the indicated neighbor into MPI message
// Send that message and receive from opposite neighbor
// Unpack the received particle data and add to particle buffers with
// an indication of dead and the neighbor on which particle is alive
//
/////////////////////////////////////////////////////////////////////////////

void ParticleExchange::exchange(
                        int sendTo, 
                        int recvFrom, 
                        Message* sendMessage, 
                        Message* recvMessage)
{
  POSVEL_T posValue;
  POTENTIAL_T potValue;
  ID_T idValue;
  MASK_T maskValue;

  // Fill same message for each of the neighbors
  sendMessage->reset();
  recvMessage->reset();

  // Number of particles to share with neighbor
  int sendParticleCount = (int)this->neighborParticles[sendTo].size();

  // Overload factor alters the x,y,z dimension for wraparound depending on
  // the neighbor receiving the data and the position this processor
  // has in the decomposition
  POSVEL_T offset[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++)
    offset[dim] = this->overLoadFactor[sendTo][dim] * this->boxSize;

  // If this processor would be sending to itself skip the MPI
  if (this->neighbor[sendTo] == this->myProc) {
    for (int i = 0; i < sendParticleCount; i++) {

      int deadIndex = this->neighborParticles[sendTo][i];
      this->xx->push_back((*this->xx)[deadIndex] + offset[0]);
      this->yy->push_back((*this->yy)[deadIndex] + offset[1]);
      this->zz->push_back((*this->zz)[deadIndex] + offset[2]);
      this->vx->push_back((*this->vx)[deadIndex]);
      this->vy->push_back((*this->vy)[deadIndex]);
      this->vz->push_back((*this->vz)[deadIndex]);
      this->ms->push_back((*this->ms)[deadIndex]);
      this->pot->push_back((*this->pot)[deadIndex]);
      this->tag->push_back((*this->tag)[deadIndex]);
      this->mask->push_back((*this->mask)[deadIndex]);
      this->status->push_back(recvFrom);

      this->numberOfDeadParticles++;
      this->particleCount++;
    }
    return;
  }

  // Pack the number of particles being sent
  sendMessage->putValue(&sendParticleCount);

  for (int i = 0; i < sendParticleCount; i++) {
    int deadIndex = this->neighborParticles[sendTo][i];

    // Locations are altered by wraparound if needed
    posValue = (*this->xx)[deadIndex] + offset[0];
    sendMessage->putValue(&posValue);
    posValue = (*this->yy)[deadIndex] + offset[1];
    sendMessage->putValue(&posValue);
    posValue = (*this->zz)[deadIndex] + offset[2];
    sendMessage->putValue(&posValue);

    // Other values are just sent
    sendMessage->putValue(&(*this->vx)[deadIndex]);
    sendMessage->putValue(&(*this->vy)[deadIndex]);
    sendMessage->putValue(&(*this->vz)[deadIndex]);
    sendMessage->putValue(&(*this->ms)[deadIndex]);
    sendMessage->putValue(&(*this->pot)[deadIndex]);
    sendMessage->putValue(&(*this->tag)[deadIndex]);
    sendMessage->putValue(&(*this->mask)[deadIndex]);
  }

  // Send the message buffer
  sendMessage->send(this->neighbor[sendTo]);

  // Receive the buffer from neighbor on other side
  recvMessage->receive(this->neighbor[recvFrom]);

#ifndef USE_SERIAL_COSMO
  MPI_Barrier(Partition::getComm());
#endif

  // Process the received buffer
  int recvParticleCount;
  recvMessage->getValue(&recvParticleCount);

  for (int i = 0; i < recvParticleCount; i++) {
    recvMessage->getValue(&posValue);
    this->xx->push_back(posValue);
    recvMessage->getValue(&posValue);
    this->yy->push_back(posValue);
    recvMessage->getValue(&posValue);
    this->zz->push_back(posValue);
    recvMessage->getValue(&posValue);
    this->vx->push_back(posValue);
    recvMessage->getValue(&posValue);
    this->vy->push_back(posValue);
    recvMessage->getValue(&posValue);
    this->vz->push_back(posValue);
    recvMessage->getValue(&posValue);
    this->ms->push_back(posValue);
    recvMessage->getValue(&potValue);
    this->pot->push_back(potValue);
    recvMessage->getValue(&idValue);
    this->tag->push_back(idValue);
    recvMessage->getValue(&maskValue);
    this->mask->push_back(maskValue);
    this->status->push_back(recvFrom);

    this->numberOfDeadParticles++;
    this->particleCount++;
  }
}
