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

// .NAME InitialExchange - Get pointer to alive plus dead particles for this
//                         processor.  Store the alive in supplied vectors
//                         and send the dead to the correct neighbor processor
//                         where they become alive.
//
// .SECTION Description
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

#ifndef InitialExchange_h
#define InitialExchange_h

#include "Definition.h"
#include "Message.h"
#include <string>
#include <vector>

#define INITIAL_EXCHANGE_FUDGE 4.0

using std::vector;

namespace cosmologytools {

class COSMO_EXPORT InitialExchange {
public:
  InitialExchange();
  ~InitialExchange();

  // Set parameters particle distribution
  void setParameters(
	POSVEL_T rL,		// Box size of the physical problem
	POSVEL_T deadSize);	// Dead delta border for each processor

  // Set neighbor processor numbers and calculate dead regions
  void initialize();

  // Calculate physical range of alive particles which must be shared
  void calculateExchangeRegions();

  // Set alive particle arrays from the initializer
  void setParticleArrays(
	long count,
	POSVEL_T* xx,
	POSVEL_T* yy,
	POSVEL_T* zz,
	POSVEL_T* vx,
	POSVEL_T* vy,
	POSVEL_T* vz,
	POTENTIAL_T* potential,
	ID_T* tag,
	MASK_T* mask);

  // Set alive particle vectors which will be filled in
  void setParticleVectors(
	vector<POSVEL_T>* xx,
	vector<POSVEL_T>* yy,
	vector<POSVEL_T>* zz,
	vector<POSVEL_T>* vx,
	vector<POSVEL_T>* vy,
	vector<POSVEL_T>* vz,
	vector<POTENTIAL_T>* potential,
	vector<ID_T>* tag,
	vector<MASK_T>* mask,
	vector<STATUS_T>* status);

  // Identify and exchange alive particles which must be shared with neighbors
  void exchangeParticles();
  void identifyExchangeParticles();
  void exchangeNeighborParticles();
  void exchange(
	int sendTo,		// Neighbor to send particles to
	int recvFrom,		// Neighbor to receive particles from
	Message* sendMessage,
	Message* recvMessage);

  long getNumberOfAliveParticles() const { return numberOfAliveParticles; }

private:
  int    myProc;		// My processor number
  int    numProc;		// Total number of processors

  int    layoutSize[DIMENSION];	// Decomposition of processors
  int    layoutPos[DIMENSION];	// Position of this processor in decomposition

  POSVEL_T boxSize;		// Physical box size (rL)
  POSVEL_T deadSize;		// Border size for dead particles

  long   numberOfAliveParticles;
  long   particleCount;		// Number of particles received from arrays

  POSVEL_T minAlive[DIMENSION];	// Minimum alive particle not exchanged
  POSVEL_T maxAlive[DIMENSION];	// Maximum alive particle not exchanged
  POSVEL_T minDead[DIMENSION];	// Minimum particle sent
  POSVEL_T maxDead[DIMENSION];	// Maximum particle sent

  int    neighbor[NUM_OF_NEIGHBORS];		 // Neighbor processor indices
  POSVEL_T minRange[NUM_OF_NEIGHBORS][DIMENSION]; // Range of dead particles
  POSVEL_T maxRange[NUM_OF_NEIGHBORS][DIMENSION]; // Range of dead particles

  vector<ID_T> neighborParticles[NUM_OF_NEIGHBORS];
				// Particle ids sent to each neighbor as ALIVE

  POSVEL_T* xxInit;		// X location from initializer
  POSVEL_T* yyInit;		// Y location from initializer
  POSVEL_T* zzInit;		// Z location from initializer
  POSVEL_T* vxInit;		// X velocity from initializer
  POSVEL_T* vyInit;		// Y velocity from initializer
  POSVEL_T* vzInit;		// Z velocity from initializer
  POTENTIAL_T* potInit;		// Particle potential
  ID_T* tagInit;		// Tag from initializer
  MASK_T* maskInit;		// Particle information

  vector<POSVEL_T>* xx;		// X location for particles on this processor
  vector<POSVEL_T>* yy;		// Y location for particles on this processor
  vector<POSVEL_T>* zz;		// Z location for particles on this processor
  vector<POSVEL_T>* vx;		// X velocity for particles on this processor
  vector<POSVEL_T>* vy;		// Y velocity for particles on this processor
  vector<POSVEL_T>* vz;		// Z velocity for particles on this processor
  vector<POTENTIAL_T>* pot;	// Particle potential
  vector<ID_T>* tag;		// Id tag for particles on this processor
  vector<MASK_T>* mask;		// Particle information

  vector<STATUS_T>* status;	// Particle is ALIVE when it leaves
};

}
#endif
