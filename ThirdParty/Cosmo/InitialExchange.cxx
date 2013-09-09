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

#ifndef _WIN32
# include <sys/types.h>
# include <dirent.h>
#endif


#include "Partition.h"
#include "InitialExchange.h"

using namespace std;

namespace cosmologytools {

/////////////////////////////////////////////////////////////////////////
//
// InitialExchange takes input from the particle initializer which originally
// contained alive particles on this processor, but after the particles move
// a little they might have moved to a neighbor processor.  Input is in the
// form of arrays, but vectors are supplied which will be filled with the
// alive particles for this processor.  As each particle is examined it is
// placed immediately in the vector if it is alive on this processor and the
// index is placed by neighbor if it is dead on this processor.  After
// categorizing all particles, the dead are exchanged with neighbors where
// they become alive.
//
// After this initial exchange, ParticleExchange is called to place all
// dead particles (which will be the ones sent in this step being returned
// plus the others which were originally placed correctly on the neighbor).
//
/////////////////////////////////////////////////////////////////////////

InitialExchange::InitialExchange()
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

  this->numberOfAliveParticles = 0;
}

InitialExchange::~InitialExchange()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for particle distribution
//
/////////////////////////////////////////////////////////////////////////

void InitialExchange::setParameters(POSVEL_T rL, POSVEL_T deadSz)
{
  // Physical total space and amount of physical space to use for dead particles
  this->boxSize = rL;
  this->deadSize = deadSz;

#ifdef DEBUG
  if (this->myProc == MASTER) {
    cout << endl << "------------------------------------" << endl;
    cout << "boxSize:  " << this->boxSize << endl;
    cout << "deltaBox: " << this->deadSize << endl;
  }
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Initialize the regions outside the alive area for this processor which
// contain the particles which must be sent away to neighbors
//
/////////////////////////////////////////////////////////////////////////

void InitialExchange::initialize()
{
#ifdef DEBUG
  if (this->myProc == MASTER)
    cout << "Decomposition: [" << this->layoutSize[0] << ":"
         << this->layoutSize[1] << ":" << this->layoutSize[2] << "]" << endl;
#endif

  // Set subextents on particle locations for this processor
  POSVEL_T boxStep[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    boxStep[dim] = this->boxSize / this->layoutSize[dim];

    // Particles in this region belong to this processor as alive
    this->minAlive[dim] = this->layoutPos[dim] * boxStep[dim];
    this->maxAlive[dim] = this->minAlive[dim] + boxStep[dim];
    if (this->maxAlive[dim] > this->boxSize)
      this->maxAlive[dim] = this->boxSize;

    // Particles in this region are dead on this processor but alive elsewhere
    this->minDead[dim] = this->minAlive[dim] - this->deadSize;
    this->maxDead[dim] = this->maxAlive[dim] + this->deadSize;
  }

  // Set the ranges on the dead particles for each neighbor direction
  calculateExchangeRegions();
}

/////////////////////////////////////////////////////////////////////////
//
// Each of the 26 neighbors will be sent a rectangular region of particles
// on this processor where they are dead.  The locations coming from the
// initializer's arrays are in the range [0:rL] and so particles to the
// left of the leftmost processor will have values at the far end of
// the box.  When they are sent they won't need to be modified by offsets.
//
/////////////////////////////////////////////////////////////////////////

void InitialExchange::calculateExchangeRegions()
{
  // Initialize all neighbors to the entire available exchange range
  for (int i = 0; i < NUM_OF_NEIGHBORS; i++) {
    for (int dim = 0; dim < DIMENSION; dim++) {
      this->minRange[i][dim] = this->minAlive[dim];
      this->maxRange[i][dim] = this->maxAlive[dim];
    }
  }

  // Left face
  this->minRange[X0][0] = this->minDead[0];
  this->maxRange[X0][0] = this->minAlive[0];

  // Right face
  this->minRange[X1][0] = this->maxAlive[0];
  this->maxRange[X1][0] = this->maxDead[0];

  // Bottom face
  this->minRange[Y0][1] = this->minDead[1];
  this->maxRange[Y0][1] = this->minAlive[1];

  // Top face
  this->minRange[Y1][1] = this->maxAlive[1];
  this->maxRange[Y1][1] = this->maxDead[1];

  // Front face
  this->minRange[Z0][2] = this->minDead[2];
  this->maxRange[Z0][2] = this->minAlive[2];

  // Back face
  this->minRange[Z1][2] = this->maxAlive[2];
  this->maxRange[Z1][2] = this->maxDead[2];

  // Left bottom and top bars
  this->minRange[X0_Y0][0] = this->minDead[0];
  this->maxRange[X0_Y0][0] = this->minAlive[0];
  this->minRange[X0_Y0][1] = this->minDead[1];
  this->maxRange[X0_Y0][1] = this->minAlive[1];

  this->minRange[X0_Y1][0] = this->minDead[0];
  this->maxRange[X0_Y1][0] = this->minAlive[0];
  this->minRange[X0_Y1][1] = this->maxAlive[1];
  this->maxRange[X0_Y1][1] = this->maxDead[1];

  // Right bottom and top bars
  this->minRange[X1_Y0][0] = this->maxAlive[0];
  this->maxRange[X1_Y0][0] = this->maxDead[0];
  this->minRange[X1_Y0][1] = this->minDead[1];
  this->maxRange[X1_Y0][1] = this->minAlive[1];

  this->minRange[X1_Y1][0] = this->maxAlive[0];
  this->maxRange[X1_Y1][0] = this->maxDead[0];
  this->minRange[X1_Y1][1] = this->maxAlive[1];
  this->maxRange[X1_Y1][1] = this->maxDead[1];

  // Bottom front and back bars
  this->minRange[Y0_Z0][1] = this->minDead[1];
  this->maxRange[Y0_Z0][1] = this->minAlive[1];
  this->minRange[Y0_Z0][2] = this->minDead[2];
  this->maxRange[Y0_Z0][2] = this->minAlive[2];

  this->minRange[Y0_Z1][1] = this->minDead[1];
  this->maxRange[Y0_Z1][1] = this->minAlive[1];
  this->minRange[Y0_Z1][2] = this->maxAlive[2];
  this->maxRange[Y0_Z1][2] = this->maxDead[2];

  // Top front and back bars 
  this->minRange[Y1_Z0][1] = this->maxAlive[1];
  this->maxRange[Y1_Z0][1] = this->maxDead[1];
  this->minRange[Y1_Z0][2] = this->minDead[2];
  this->maxRange[Y1_Z0][2] = this->minAlive[2];

  this->minRange[Y1_Z1][1] = this->maxAlive[1];
  this->maxRange[Y1_Z1][1] = this->maxDead[1];
  this->minRange[Y1_Z1][2] = this->maxAlive[2];
  this->maxRange[Y1_Z1][2] = this->maxDead[2];

  // Left front and back bars (vertical)
  this->minRange[Z0_X0][0] = this->minDead[0];
  this->maxRange[Z0_X0][0] = this->minAlive[0];
  this->minRange[Z0_X0][2] = this->minDead[2];
  this->maxRange[Z0_X0][2] = this->minAlive[2];

  this->minRange[Z1_X0][0] = this->minDead[0];
  this->maxRange[Z1_X0][0] = this->minAlive[0];
  this->minRange[Z1_X0][2] = this->maxAlive[2];
  this->maxRange[Z1_X0][2] = this->maxDead[2];

  // Right front and back bars (vertical)
  this->minRange[Z0_X1][0] = this->maxAlive[0];
  this->maxRange[Z0_X1][0] = this->maxDead[0];
  this->minRange[Z0_X1][2] = this->minDead[2];
  this->maxRange[Z0_X1][2] = this->minAlive[2];

  this->minRange[Z1_X1][0] = this->maxAlive[0];
  this->maxRange[Z1_X1][0] = this->maxDead[0];
  this->minRange[Z1_X1][2] = this->maxAlive[2];
  this->maxRange[Z1_X1][2] = this->maxDead[2];

  // Left bottom front corner
  this->minRange[X0_Y0_Z0][0] = this->minDead[0];
  this->maxRange[X0_Y0_Z0][0] = this->minAlive[0];
  this->minRange[X0_Y0_Z0][1] = this->minDead[1];
  this->maxRange[X0_Y0_Z0][1] = this->minAlive[1];
  this->minRange[X0_Y0_Z0][2] = this->minDead[2];
  this->maxRange[X0_Y0_Z0][2] = this->minAlive[2];

  // Left bottom back corner
  this->minRange[X0_Y0_Z1][0] = this->minDead[0];
  this->maxRange[X0_Y0_Z1][0] = this->minAlive[0];
  this->minRange[X0_Y0_Z1][1] = this->minDead[1];
  this->maxRange[X0_Y0_Z1][1] = this->minAlive[1];
  this->minRange[X0_Y0_Z1][2] = this->maxAlive[2];
  this->maxRange[X0_Y0_Z1][2] = this->maxDead[2];

  // Left top front corner
  this->minRange[X0_Y1_Z0][0] = this->minDead[0];
  this->maxRange[X0_Y1_Z0][0] = this->minAlive[0];
  this->minRange[X0_Y1_Z0][1] = this->maxAlive[1];
  this->maxRange[X0_Y1_Z0][1] = this->maxDead[1];
  this->minRange[X0_Y1_Z0][2] = this->minDead[2];
  this->maxRange[X0_Y1_Z0][2] = this->minAlive[2];

  // Left top back corner
  this->minRange[X0_Y1_Z1][0] = this->minDead[0];
  this->maxRange[X0_Y1_Z1][0] = this->minAlive[0];
  this->minRange[X0_Y1_Z1][1] = this->maxAlive[1];
  this->maxRange[X0_Y1_Z1][1] = this->maxDead[1];
  this->minRange[X0_Y1_Z1][2] = this->maxAlive[2];
  this->maxRange[X0_Y1_Z1][2] = this->maxDead[2];

  // Right bottom front corner
  this->minRange[X1_Y0_Z0][0] = this->maxAlive[0];
  this->maxRange[X1_Y0_Z0][0] = this->maxDead[0];
  this->minRange[X1_Y0_Z0][1] = this->minDead[1];
  this->maxRange[X1_Y0_Z0][1] = this->minAlive[1];
  this->minRange[X1_Y0_Z0][2] = this->minDead[2];
  this->maxRange[X1_Y0_Z0][2] = this->minAlive[2];

  // Right bottom back corner
  this->minRange[X1_Y0_Z1][0] = this->maxAlive[0];
  this->maxRange[X1_Y0_Z1][0] = this->maxDead[0];
  this->minRange[X1_Y0_Z1][1] = this->minDead[1];
  this->maxRange[X1_Y0_Z1][1] = this->minAlive[1];
  this->minRange[X1_Y0_Z1][2] = this->maxAlive[2];
  this->maxRange[X1_Y0_Z1][2] = this->maxDead[2];

  // Right top front corner
  this->minRange[X1_Y1_Z0][0] = this->maxAlive[0];
  this->maxRange[X1_Y1_Z0][0] = this->maxDead[0];
  this->minRange[X1_Y1_Z0][1] = this->maxAlive[1];
  this->maxRange[X1_Y1_Z0][1] = this->maxDead[1];
  this->minRange[X1_Y1_Z0][2] = this->minDead[2];
  this->maxRange[X1_Y1_Z0][2] = this->minAlive[2];

  // Right top back corner
  this->minRange[X1_Y1_Z1][0] = this->maxAlive[0];
  this->maxRange[X1_Y1_Z1][0] = this->maxDead[0];
  this->minRange[X1_Y1_Z1][1] = this->maxAlive[1];
  this->maxRange[X1_Y1_Z1][1] = this->maxDead[1];
  this->minRange[X1_Y1_Z1][2] = this->maxAlive[2];
  this->maxRange[X1_Y1_Z1][2] = this->maxDead[2];

  // Fix ranges for processors on a face in the decomposition
  // Processor is on front edge in X dimension
  if (this->layoutPos[0] == 0) {
    this->minRange[X0][0] = this->boxSize - this->deadSize;
    this->minRange[X0_Y0][0] = this->boxSize - this->deadSize;
    this->minRange[X0_Y1][0] = this->boxSize - this->deadSize;
    this->minRange[Z0_X0][0] = this->boxSize - this->deadSize;
    this->minRange[Z1_X0][0] = this->boxSize - this->deadSize;
    this->minRange[X0_Y0_Z0][0] = this->boxSize - this->deadSize;
    this->minRange[X0_Y0_Z1][0] = this->boxSize - this->deadSize;
    this->minRange[X0_Y1_Z0][0] = this->boxSize - this->deadSize;
    this->minRange[X0_Y1_Z1][0] = this->boxSize - this->deadSize;

    this->maxRange[X0][0] = this->boxSize;
    this->maxRange[X0_Y0][0] = this->boxSize;
    this->maxRange[X0_Y1][0] = this->boxSize;
    this->maxRange[Z0_X0][0] = this->boxSize;
    this->maxRange[Z1_X0][0] = this->boxSize;
    this->maxRange[X0_Y0_Z0][0] = this->boxSize;
    this->maxRange[X0_Y0_Z1][0] = this->boxSize;
    this->maxRange[X0_Y1_Z0][0] = this->boxSize;
    this->maxRange[X0_Y1_Z1][0] = this->boxSize;
  }

   // Processor is on back edge in X dimension
   if (this->layoutPos[0] == (this->layoutSize[0] - 1)) {
      this->minRange[X1][0] = 0;
      this->minRange[X1_Y1][0] = 0;
      this->minRange[X1_Y0][0] = 0;
      this->minRange[Z1_X1][0] = 0;
      this->minRange[Z0_X1][0] = 0;
      this->minRange[X1_Y1_Z1][0] = 0;
      this->minRange[X1_Y1_Z0][0] = 0;
      this->minRange[X1_Y0_Z1][0] = 0;
      this->minRange[X1_Y0_Z0][0] = 0;

      this->maxRange[X1][0] = this->deadSize;
      this->maxRange[X1_Y1][0] = this->deadSize;
      this->maxRange[X1_Y0][0] = this->deadSize;
      this->maxRange[Z1_X1][0] = this->deadSize;
      this->maxRange[Z0_X1][0] = this->deadSize;
      this->maxRange[X1_Y1_Z1][0] = this->deadSize;
      this->maxRange[X1_Y1_Z0][0] = this->deadSize;
      this->maxRange[X1_Y0_Z1][0] = this->deadSize;
      this->maxRange[X1_Y0_Z0][0] = this->deadSize;
   }

   // Processor is on front edge in Y dimension
   if (this->layoutPos[1] == 0) {
      this->minRange[Y0][1] = this->boxSize - this->deadSize;
      this->minRange[X0_Y0][1] = this->boxSize - this->deadSize; 
      this->minRange[X1_Y0][1] = this->boxSize - this->deadSize;
      this->minRange[Y0_Z0][1] = this->boxSize - this->deadSize;
      this->minRange[Y0_Z1][1] = this->boxSize - this->deadSize;
      this->minRange[X0_Y0_Z0][1] = this->boxSize - this->deadSize;
      this->minRange[X0_Y0_Z1][1] = this->boxSize - this->deadSize;
      this->minRange[X1_Y0_Z1][1] = this->boxSize - this->deadSize;
      this->minRange[X1_Y0_Z0][1] = this->boxSize - this->deadSize;

      this->maxRange[Y0][1] = this->boxSize;
      this->maxRange[X0_Y0][1] = this->boxSize;
      this->maxRange[X1_Y0][1] = this->boxSize;
      this->maxRange[Y0_Z0][1] = this->boxSize;
      this->maxRange[Y0_Z1][1] = this->boxSize;
      this->maxRange[X0_Y0_Z0][1] = this->boxSize;
      this->maxRange[X0_Y0_Z1][1] = this->boxSize;
      this->maxRange[X1_Y0_Z1][1] = this->boxSize;
      this->maxRange[X1_Y0_Z0][1] = this->boxSize;
   }

   // Processor is on back edge in Y dimension
   if (this->layoutPos[1] == (this->layoutSize[1] - 1)) {
      this->minRange[Y1][1] = 0;
      this->minRange[X1_Y1][1] = 0;
      this->minRange[X0_Y1][1] = 0;
      this->minRange[Y1_Z1][1] = 0;
      this->minRange[Y1_Z0][1] = 0;
      this->minRange[X1_Y1_Z1][1] = 0;
      this->minRange[X1_Y1_Z0][1] = 0;
      this->minRange[X0_Y1_Z0][1] = 0;
      this->minRange[X0_Y1_Z1][1] = 0;

      this->maxRange[Y1][1] = this->deadSize;
      this->maxRange[X1_Y1][1] = this->deadSize;
      this->maxRange[X0_Y1][1] = this->deadSize;
      this->maxRange[Y1_Z1][1] = this->deadSize;
      this->maxRange[Y1_Z0][1] = this->deadSize;
      this->maxRange[X1_Y1_Z1][1] = this->deadSize;
      this->maxRange[X1_Y1_Z0][1] = this->deadSize;
      this->maxRange[X0_Y1_Z0][1] = this->deadSize;
      this->maxRange[X0_Y1_Z1][1] = this->deadSize;
   }
  
   // Processor is on front edge in Z dimension
   if (this->layoutPos[2] == 0) {
      this->minRange[Z0][2] = this->boxSize - this->deadSize;
      this->minRange[Y0_Z0][2] = this->boxSize - this->deadSize;
      this->minRange[Y1_Z0][2] = this->boxSize - this->deadSize;
      this->minRange[Z0_X0][2] = this->boxSize - this->deadSize;
      this->minRange[Z0_X1][2] = this->boxSize - this->deadSize;
      this->minRange[X0_Y0_Z0][2] = this->boxSize - this->deadSize;
      this->minRange[X1_Y1_Z0][2] = this->boxSize - this->deadSize;
      this->minRange[X0_Y1_Z0][2] = this->boxSize - this->deadSize;
      this->minRange[X1_Y0_Z0][2] = this->boxSize - this->deadSize;

      this->maxRange[Z0][2] = this->boxSize;
      this->maxRange[Y0_Z0][2] = this->boxSize;
      this->maxRange[Y1_Z0][2] = this->boxSize;
      this->maxRange[Z0_X0][2] = this->boxSize;
      this->maxRange[Z0_X1][2] = this->boxSize;
      this->maxRange[X0_Y0_Z0][2] = this->boxSize;
      this->maxRange[X1_Y1_Z0][2] = this->boxSize;
      this->maxRange[X0_Y1_Z0][2] = this->boxSize;
      this->maxRange[X1_Y0_Z0][2] = this->boxSize;
   }
    
   // Processor is on back edge in Z dimension
   if (this->layoutPos[2] == (this->layoutSize[2] - 1)) {
      this->minRange[Z1][2] = 0;
      this->minRange[Y1_Z1][2] = 0;
      this->minRange[Y0_Z1][2] = 0;
      this->minRange[Z1_X1][2] = 0;
      this->minRange[Z1_X0][2] = 0;
      this->minRange[X1_Y1_Z1][2] = 0;
      this->minRange[X0_Y0_Z1][2] = 0;
      this->minRange[X1_Y0_Z1][2] = 0;
      this->minRange[X0_Y1_Z1][2] = 0;

      this->maxRange[Z1][2] = this->deadSize;
      this->maxRange[Y1_Z1][2] = this->deadSize;
      this->maxRange[Y0_Z1][2] = this->deadSize;
      this->maxRange[Z1_X1][2] = this->deadSize;
      this->maxRange[Z1_X0][2] = this->deadSize;
      this->maxRange[X1_Y1_Z1][2] = this->deadSize;
      this->maxRange[X0_Y0_Z1][2] = this->deadSize;
      this->maxRange[X1_Y0_Z1][2] = this->deadSize;
      this->maxRange[X0_Y1_Z1][2] = this->deadSize;
   }
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void InitialExchange::setParticleArrays(
			long count,
			POSVEL_T* xLoc,
			POSVEL_T* yLoc,
			POSVEL_T* zLoc,
			POSVEL_T* xVel,
			POSVEL_T* yVel,
			POSVEL_T* zVel,
			POTENTIAL_T* potential,
			ID_T* id,
			MASK_T* maskData)
{
  this->particleCount = count;
  this->xxInit = xLoc;
  this->yyInit = yLoc;
  this->zzInit = zLoc;
  this->vxInit = xVel;
  this->vyInit = yVel;
  this->vzInit = zVel;
  this->potInit = potential;
  this->tagInit = id;
  this->maskInit = maskData;
}

void InitialExchange::setParticleVectors(
			vector<POSVEL_T>* xLoc,
			vector<POSVEL_T>* yLoc,
			vector<POSVEL_T>* zLoc,
			vector<POSVEL_T>* xVel,
			vector<POSVEL_T>* yVel,
			vector<POSVEL_T>* zVel,
			vector<POTENTIAL_T>* potential,
			vector<ID_T>* id,
			vector<MASK_T>* maskData,
			vector<STATUS_T>* type)
{
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->vx = xVel;
  this->vy = yVel;
  this->vz = zVel;
  this->pot = potential;
  this->tag = id;
  this->mask = maskData;
  this->status = type;
}
	
/////////////////////////////////////////////////////////////////////////////
//
// Identify the border particles which will be alive on other processors
// and send them and receive the particles which are dead on other processors
// but alive on this processor.  Store all the newly acquired alive particles
// in the empty supplied vectors for the rest of the simulation.
//
/////////////////////////////////////////////////////////////////////////////

void InitialExchange::exchangeParticles()
{
  // Identify dead particles on this processor which must be sent
  // because they are alive particles on neighbor processors
  // x,y,z are still in physical units with wraparound included
  identifyExchangeParticles();

  // Exchange those particles with appropriate neighbors
  // x,y,z are not in normalized units
  exchangeNeighborParticles();

  // Count the particles across processors
  long totalAliveParticles = 0;
  MPI_Allreduce((void*) &this->numberOfAliveParticles, 
                (void*) &totalAliveParticles, 
                1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);

#ifdef INITIAL_EXCHANGE_VERBOSE
  cout << "InitialExchange Particles Rank " << setw(3) << this->myProc 
       << " #alive = " << this->numberOfAliveParticles << endl;
#endif
 
  if (this->myProc == MASTER) {
    cout << "InitialExchange TotalAliveParticles " 
         << totalAliveParticles << endl;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Iterate over all the particles on this processor and determine which are
// dead and  must be sent away and add them to the vector for that neighbor
//
/////////////////////////////////////////////////////////////////////////////

void InitialExchange::identifyExchangeParticles()
{
  bool hadProblems = false;
  for (int i = 0; i < this->particleCount; i++) {
    bool found = false;

    // Particle is alive on this processor so add to vectors
    if ((this->xxInit[i] >= this->minAlive[0] && 
         this->xxInit[i] < this->maxAlive[0]) &&
        (this->yyInit[i] >= this->minAlive[1] && 
         this->yyInit[i] < this->maxAlive[1]) &&
        (this->zzInit[i] >= this->minAlive[2] && 
         this->zzInit[i] < this->maxAlive[2])) {
            this->xx->push_back(this->xxInit[i]);
            this->yy->push_back(this->yyInit[i]);
            this->zz->push_back(this->zzInit[i]);
            this->vx->push_back(this->vxInit[i]);
            this->vy->push_back(this->vyInit[i]);
            this->vz->push_back(this->vzInit[i]);
            this->tag->push_back(this->tagInit[i]);
	    this->pot->push_back(this->potInit[i]);
	    this->mask->push_back(this->maskInit[i]);
            this->numberOfAliveParticles++;
            found = true;

    } else {
      // Particle is dead here but which processor needs it as alive
      for (int n = 0; n < NUM_OF_NEIGHBORS; n++) {
        if ((this->xxInit[i] >= minRange[n][0]) && 
            (this->xxInit[i] < maxRange[n][0]) &&
            (this->yyInit[i] >= minRange[n][1]) && 
            (this->yyInit[i] < maxRange[n][1]) &&
            (this->zzInit[i] >= minRange[n][2]) && 
            (this->zzInit[i] < maxRange[n][2])) {
                this->neighborParticles[n].push_back(i);
                found = true;
        }
      }
    }
    if (found == false) {
      hadProblems = true;
      cout << "Rank " << myProc 
	   << " Problem particle " << this->tagInit[i] << " ("
	   << xxInit[i] << "," << yyInit[i] << "," << zzInit[i] 
	   << ") not in ["
	   << this->minAlive[0] << ":" << this->maxAlive[0] << ","
	   << this->minAlive[1] << ":" << this->maxAlive[1] << ","
	   << this->minAlive[2] << ":" << this->maxAlive[2]
	   << "] or neighbors" << endl;
    }
  }

  if (hadProblems) {
    cout << "Rank " << myProc << " had problem particles!" << endl;
    for (int n = 0; n < NUM_OF_NEIGHBORS; n++) {
      cout << "Rank " << myProc << " neighbor " << n
           << " has ["
           << minRange[n][0] << ":" << maxRange[n][0] << ","
           << minRange[n][1] << ":" << maxRange[n][1] << ","
           << minRange[n][2] << ":" << maxRange[n][2]
           << "]" << endl;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Exchange the appropriate particles with neighbors
// Only the index of the particle to be exchanged is stored so fill out
// the message with location, velocity, tag.  Status information doesn't
// have to be sent because all particles will be alive.
// Use the Cartesian communicator for neighbor exchange
//
/////////////////////////////////////////////////////////////////////////////

void InitialExchange::exchangeNeighborParticles()
{
  // Calculate the maximum number of particles to share for calculating buffer
  int myShareSize = 0;
  for (int n = 0; n < NUM_OF_NEIGHBORS; n++)
    if (myShareSize < (int) this->neighborParticles[n].size())
      myShareSize = this->neighborParticles[n].size();

  int maxDeadSize;
  MPI_Allreduce((void*) &myShareSize,
                (void*) &maxDeadSize,
                1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

  // Allocate messages to send and receive MPI buffers
  int bufferSize = (1 * sizeof(int)) +          // number of particles
                   (maxDeadSize * 
                     ((7 * sizeof(POSVEL_T)) +  // location, velocity, potential
                      (1 * sizeof(ID_T)) +      // id tag
                      (1 * sizeof(MASK_T))));   // mask
  Message* sendMessage = new Message(bufferSize);
  Message* recvMessage = new Message(bufferSize);
  
  //debug statement added by Adrian to see how much buffer space we're using
  if(this->myProc == MASTER) {
    printf("PXCH buffer = 2*%d = %f MB\n",bufferSize,
           2.0*bufferSize/1024.0/1024.0);
  }
  MPI_Barrier(MPI_COMM_WORLD);

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

void InitialExchange::exchange(
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
  int sendParticleCount = this->neighborParticles[sendTo].size();

  // Pack the number of particles being sent
  sendMessage->putValue(&sendParticleCount);

  for (int i = 0; i < sendParticleCount; i++) {
    int deadIndex = this->neighborParticles[sendTo][i];
    sendMessage->putValue(&this->xxInit[deadIndex]);
    sendMessage->putValue(&this->yyInit[deadIndex]);
    sendMessage->putValue(&this->zzInit[deadIndex]);
    sendMessage->putValue(&this->vxInit[deadIndex]);
    sendMessage->putValue(&this->vyInit[deadIndex]);
    sendMessage->putValue(&this->vzInit[deadIndex]);
    sendMessage->putValue(&this->potInit[deadIndex]);
    sendMessage->putValue(&this->tagInit[deadIndex]);
    sendMessage->putValue(&this->maskInit[deadIndex]);
    this->particleCount--;
  }

  // Send the message buffer
  sendMessage->send(this->neighbor[sendTo]);

  // Receive the buffer from neighbor on other side
  recvMessage->receive(this->neighbor[recvFrom]);
  MPI_Barrier(Partition::getComm());

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
    recvMessage->getValue(&potValue);
    this->pot->push_back(potValue);
    recvMessage->getValue(&idValue);
    this->tag->push_back(idValue);
    recvMessage->getValue(&maskValue);
    this->mask->push_back(maskValue);

    this->numberOfAliveParticles++;
    this->particleCount++;
  }
}

}
