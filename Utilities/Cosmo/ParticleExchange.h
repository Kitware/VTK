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

// .NAME ParticleExchange - read or get pointer to alive particles on this
//                          process and exchange dead particles with neighbors
//
// .SECTION Description
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
// Information exchanged are x,y,z locations and vectors and integer unique
// tags per particle.  Also when the data is shared, the particle status is
// filled in with the number of the neighbor that shared the particle.  This
// is to make the halo finder faster because instead of listing a particle
// as just alive or dead, we know where the dead particle is located.
//

#ifndef ParticleExchange_h
#define ParticleExchange_h

#include "Message.h"

#ifdef USE_VTK_COSMO 
#include "CosmoDefinition.h"
#include "vtkstd/string"
#include "vtkstd/vector"

using namespace vtkstd;
#else
#include "Definition.h"
#include <string>
#include <vector>

using namespace std;
#endif 

#ifdef USE_VTK_COSMO
class COSMO_EXPORT ParticleExchange {
#else
class ParticleExchange {
#endif
public:
  ParticleExchange();
  ~ParticleExchange();

  // Set parameters particle distribution
  void setParameters(
        POSVEL_T rL,            // Box size of the physical problem
        POSVEL_T deadSize);     // Dead delta border for each processor

  // Calculate the factor to add to locations when doing wraparound shares
  void calculateOffsetFactor();

  // Set neighbor processor numbers and calculate dead regions
  void initialize();

  // Calculate physical range of alive particles which must be shared
  void calculateExchangeRegions();

  // Set alive particle vectors which were created elsewhere
  void setParticles(
        vector<POSVEL_T>* xx,
        vector<POSVEL_T>* yy,
        vector<POSVEL_T>* zz,
        vector<POSVEL_T>* vx,
        vector<POSVEL_T>* vy,
        vector<POSVEL_T>* vz,
        vector<POSVEL_T>* mass,
        vector<POTENTIAL_T>* potential,
        vector<ID_T>* tag,
        vector<MASK_T>* mask,
        vector<STATUS_T>* status);

  // Identify and exchange alive particles which must be shared with neighbors
  void exchangeParticles();
  void identifyExchangeParticles();
  void exchangeNeighborParticles();
  void exchange(
        int sendTo,             // Neighbor to send particles to
        int recvFrom,           // Neighbor to receive particles from
        Message* sendMessage,
        Message* recvMessage);

  // Return data needed by other software
  int getParticleCount()                { return this->particleCount; }

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  long   totalParticles;        // Number of particles on all files
  int    headerSize;            // For BLOCK files

  int    layoutSize[DIMENSION]; // Decomposition of processors
  int    layoutPos[DIMENSION];  // Position of this processor in decomposition

  POSVEL_T boxSize;             // Physical box size (rL)
  POSVEL_T deadSize;            // Border size for dead particles

  long   numberOfAliveParticles;
  long   numberOfDeadParticles;
  long   particleCount;         // Running index used to store data
                                // Ends up as the number of alive plus dead

  POSVEL_T minMine[DIMENSION];  // Minimum alive particle not exchanged
  POSVEL_T maxMine[DIMENSION];  // Maximum alive particle not exchanged
  POSVEL_T minShare[DIMENSION]; // Minimum alive particle shared
  POSVEL_T maxShare[DIMENSION]; // Maximum alive particle shared

  int      neighbor[NUM_OF_NEIGHBORS];            // Neighbor processor indices
  POSVEL_T minRange[NUM_OF_NEIGHBORS][DIMENSION]; // Range of dead particles
  POSVEL_T maxRange[NUM_OF_NEIGHBORS][DIMENSION]; // Range of dead particles

  int    overLoadFactor[NUM_OF_NEIGHBORS][DIMENSION];
                                // When sending location factor to multiply
                                // boxSize by for wraparound alteration

  vector<ID_T> neighborParticles[NUM_OF_NEIGHBORS];
                                // Particle ids sent to each neighbor as DEAD

  vector<POSVEL_T>* xx;         // X location for particles on this processor
  vector<POSVEL_T>* yy;         // Y location for particles on this processor
  vector<POSVEL_T>* zz;         // Z location for particles on this processor
  vector<POSVEL_T>* vx;         // X velocity for particles on this processor
  vector<POSVEL_T>* vy;         // Y velocity for particles on this processor
  vector<POSVEL_T>* vz;         // Z velocity for particles on this processor
  vector<POSVEL_T>* ms;         // Mass for particles on this processor
  vector<ID_T>* tag;            // Id tag for particles on this processor
  vector<STATUS_T>* status;     // Particle is ALIVE or labeled with neighbor
                                // processor index where it is ALIVE
  vector<POTENTIAL_T>* pot;     // Id tag for particles on this processor
  vector<MASK_T>* mask;         // Id tag for particles on this processor
};

#endif
