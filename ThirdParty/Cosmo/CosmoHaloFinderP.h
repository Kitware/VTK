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

// .NAME CosmoHaloFinderP - find halos within a cosmology data file in parallel
//
// .SECTION Description
// CosmoHaloFinderP takes a series of data files containing .cosmo data
// along with parameters defining the box size for the data and for
// determining halos within the particle data.  It distributes the data
// across processors including a healthy dead zone of particles belonging
// to neighbor processors.  By definition all halos can be determined
// completely for any processor because of this dead zone.  The serial
// halo finder is called on each processor.
//
// Halos returned from the serial halo finder either contain all particles
// interior to this processor (ALIVE), all particles completely in the dead
// zone (DEAD) or a combination (MIXED).
//
// If mixed halos are shared with only one neighbor the rule followed is if
// the halo is in the upper planes of the processor (high values of x,y,z)
// then this processor will keep that halo as alive.  If the halo is in the
// low planes it is given up as dead, with the understanding that the
// adjacent processors will claim it as alive.  When more than two processors
// claim a halo the information is sent to the MASTER processor which
// determines which processor can claim that halo and the other two give
// it up.
//

#ifndef CosmoHaloFinderP_h
#define CosmoHaloFinderP_h

#ifdef USE_VTK_COSMO
#include "CosmoDefinition.h"
#else
#include "Definition.h"
#endif

#include "CosmoHaloFinder.h"
#include "CosmoHalo.h"

#include <string>
#include <vector>

using namespace std;

#ifdef USE_VTK_COSMO
class COSMO_EXPORT CosmoHaloFinderP {
#else
class CosmoHaloFinderP {
#endif
public:
  CosmoHaloFinderP();
  ~CosmoHaloFinderP();

  // Set parameters for serial halo finder which does the work
  void setParameters(
        const string& outName,  // Base name of output halo files
        POSVEL_T rL,            // Box size of the physical problem
        POSVEL_T deadSize,      // Dead size used to normalize for non periodic
        long np,                // Number of particles in the problem
        int pmin,               // Minimum number of particles in a halo
        POSVEL_T bb);           // Normalized distance between particles
                                // which define a single halo

  // Execute the serial halo finder for this processor
  void executeHaloFinder();

  // Collect the halo information from the serial halo finder
  // Save the mixed halos so as to determine which processor owns them
  void collectHalos();
  void buildHaloStructure();
  void processMixedHalos();

  // MASTER node merges the mixed halos which cross more than two processors
  void mergeHalos();
  void collectMixedHalos(ID_T* buffer, int bufSize);
  void assignMixedHalos();
  void sendMixedHaloResults(ID_T* buffer, int bufSize);
  int compareHalos(CosmoHalo* halo1, CosmoHalo* halo2);

#ifndef USE_VTK_COSMO
  // Write the particles with mass field containing halo tags
  void writeTaggedParticles();
#endif

  // Set alive particle vectors which were created elsewhere
  void setParticles(
        vector<POSVEL_T>* xLoc,
        vector<POSVEL_T>* yLoc,
        vector<POSVEL_T>* zLod,
        vector<POSVEL_T>* xVel,
        vector<POSVEL_T>* yVel,
        vector<POSVEL_T>* zVel,
        vector<POTENTIAL_T>* potential,
        vector<ID_T>* id,
        vector<MASK_T>* mask,
        vector<STATUS_T>* state);

  // Return information needed by halo center finder
  int getNumberOfHalos()        { return (int)this->halos.size(); }
  int* getHalos()               { return &this->halos[0]; }
  int* getHaloCount()           { return &this->haloCount[0]; }
  int* getHaloList()            { return this->haloList; }
  int* getHaloTag()             { return this->haloTag; }
  int* getHaloSize()            { return this->haloSize; }

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  int    layoutSize[DIMENSION]; // Decomposition of processors
  int    layoutPos[DIMENSION];  // Position of this processor in decomposition

  string outFile;               // File of particles written by this processor
  string outHaloFile;           // File of halo tag and size of halo
                                // used for looping on round robin share of data

  CosmoHaloFinder haloFinder;   // Serial halo finder for this processor

  POSVEL_T boxSize;             // Physical box size of the data set
  POSVEL_T deadSize;            // Border size for dead particles
  long    np;                   // Number of particles in the problem
  int    pmin;                  // Minimum number of particles in a halo
  POSVEL_T bb;                  // Minimum normalized distance between
                                // particles in a halo
  POSVEL_T normalizeFactor;     // Convert physical location to grid location

  long   particleCount;         // Running index used to store data
                                // Ends up as the number of alive plus dead

  int    neighbor[NUM_OF_NEIGHBORS];    // Neighbor processor ids
  int    deadParticle[NUM_OF_NEIGHBORS];// Number of neighbor dead particles
  int    deadHalo[NUM_OF_NEIGHBORS];    // Number of neighbor mixed halos

  POSVEL_T* xx;                 // X location for particles on this processor
  POSVEL_T* yy;                 // Y location for particles on this processor
  POSVEL_T* zz;                 // Z location for particles on this processor
  POSVEL_T* vx;                 // X velocity for particles on this processor
  POSVEL_T* vy;                 // Y velocity for particles on this processor
  POSVEL_T* vz;                 // Z velocity for particles on this processor
  POTENTIAL_T* pot;             // Particle potential
  ID_T* tag;                    // Id tag for particles on this processor
  MASK_T* mask;                 // Particle information

  POSVEL_T** haloData;          // Normalized data for serial halo finder

  STATUS_T* status;             // Particle is ALIVE or labeled with neighbor
                                // processor index where it is ALIVE

  int* haloTag;                 // From serial halo finder, the index of the
                                // first particle in a halo

  int* haloSize;                // From serial halo finder, the size of a halo
                                // where the first particle has the actual size
                                // and other member particles have size=0
  int* haloAliveSize;
  int* haloDeadSize;

  int numberOfAliveHalos;       // Number of alive or valid halos
  int numberOfDeadHalos;        // Number of dead halos
  int numberOfMixedHalos;       // Number of halos with both alive and dead
  int numberOfHaloParticles;    // Number of particles in all VALID halos

  vector<CosmoHalo*> myMixedHalos;      // Mixed halos on this processor
  vector<CosmoHalo*> allMixedHalos;     // Combined mixed halos on MASTER

  vector<int> halos;            // First particle index into haloList
  vector<int> haloCount;        // Size of each halo 

  int* haloList;                // Indices of next particle in halo
  int* haloStart;               // Index of first particle in halo
                                // Chain is built backwards but using these two
                                // arrays, all particle indices for a halo
                                // can be found
};

#endif
