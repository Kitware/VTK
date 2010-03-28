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

// .NAME FOFHaloProperties - calculate properties of all FOF halos
//
// FOFHaloProperties takes data from CosmoHaloFinderP about individual halos
// and data from all particles and calculates properties.
//

#ifndef FOFHaloProperties_h
#define FOFHaloProperties_h

#include "CosmoDefinition.h"
#include "ChainingMesh.h"
#include <string>
#include <vector>

using namespace std;

class COSMO_EXPORT FOFHaloProperties {
public:
  FOFHaloProperties();
  ~FOFHaloProperties();

  // Set parameters for sizes of the dead/alive space
  void setParameters(
        const string& outName,  // Base name of output halo files
        POSVEL_T rL,            // Box size of the physical problem
        POSVEL_T deadSize,      // Dead size used to normalize for non periodic
        POSVEL_T particleMass,  // Mass of a single particle in system
        POSVEL_T bb);           // Inter particle distance for halos

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

  // Set the halo information from the FOF halo finder
  void setHalos(
        int  numberOfHalos,     // Number of halos found
        int* halos,             // Index into haloList of first particle
        int* haloCount,         // Number of particles in the matching halo
        int* haloList);         // Chain of indices of all particles in halo

  /////////////////////////////////////////////////////////////////////
  //
  // FOF (Friend of friends) center finding
  //
  /////////////////////////////////////////////////////////////////////

  // Find the halo centers (minimum potential) finding minimum of array
  void FOFHaloCenterMinimumPotential(vector<int>* haloCenter);

  // Find the halo centers using most bound particle (N^2/2)
  void FOFHaloCenterMBP(vector<int>* haloCenter);
  int  mostBoundParticleN2(int h, POTENTIAL_T* minPotential);

  // Initial guess of A* contains an actual part and an estimated part
  int  mostBoundParticleAStar(int h);

  // Calculate actual values between particles within a bucket
  void aStarThisBucketPart(
        ChainingMesh* haloChain,        // Buckets of particles
        POSVEL_T* xLocHalo,             // Locations within buckets
        POSVEL_T* yLocHalo,
        POSVEL_T* zLocHalo,
        int* bucketID,                  // Map from particle to bucket
        POSVEL_T* estimate);            // Running minimum potential

  // Calculate actual values for 26 neighbors in the center of halo
  // Level 1 refinement done for initial guess
  void aStarActualNeighborPart(
        ChainingMesh* haloChain,        // Buckets of particles
        int* minActual,                 // Range for doing actual vs estimated
        int* maxActual,
        POSVEL_T* xLocHalo,             // Locations within buckets
        POSVEL_T* yLocHalo,
        POSVEL_T* zLocHalo,
        int* refineLevel,               // Refinement level of each particle
        POSVEL_T* estimate);            // Running minimum potential

  // Calculate estimated values for 26 neighbors around the edges of halo
  // Level 0 refinement done for initial guess
  void aStarEstimatedNeighborPart(
        ChainingMesh* haloChain,        // Buckets of particles
        int* minActual,                 // Range for doing actual vs estimated
        int* maxActual,
        POSVEL_T* xLocHalo,             // Locations within buckets
        POSVEL_T* yLocHalo,
        POSVEL_T* zLocHalo,
        int* refineLevel,               // Refinement level of each particle
        POSVEL_T* estimate,             // Running minimum potential
        POSVEL_T boundarySize);         // Boundary around bucket for estimation

  // Calculate estimates for all buckets beyond the 27 closest
  void aStarEstimatedPart(
        ChainingMesh* haloChain,        // Buckets of particles
        POSVEL_T* xLocHalo,             // Locations within buckets
        POSVEL_T* yLocHalo,
        POSVEL_T* zLocHalo,
        POSVEL_T* estimate);            // Running minimum potential

  // Refinement of 0 to 1
  void refineAStarLevel_1(
        ChainingMesh* haloChain,        // Buckets of particles
        int bi,                         // Bucket containing particle to refine
        int bj,
        int bk,
        int* minActual,                 // Range for doing actual vs estimated
        int* maxActual,
        POSVEL_T* xLocHalo,             // Locations within buckets
        POSVEL_T* yLocHalo,
        POSVEL_T* zLocHalo,
        int minParticle,                // Particle to refine
        POSVEL_T* estimate,             // Running minimum potential
        POSVEL_T boundarySize);         // Boundary around bucket for estimation

  // Refinement of 1 to N
  void refineAStarLevel_N(
        ChainingMesh* haloChain,        // Buckets of particles
        int bi,                         // Bucket containing particle to refine
        int bj,
        int bk,
        POSVEL_T* xLocHalo,             // Locations within buckets
        POSVEL_T* yLocHalo,
        POSVEL_T* zLocHalo,
        int minParticle,                // Particle to refine
        POSVEL_T* estimate,             // Running minimum potential
        int winDelta);                  // Number of buckets to refine out to

  // Find the halo centers using most connected particle (N^2/2)
  void FOFHaloCenterMCP(vector<int>* haloCenter);
  int  mostConnectedParticleN2(int h);
  int  mostConnectedParticleChainMesh(int h);

  // Build a chaining mesh of halo particles
  ChainingMesh* buildChainingMesh(
        int halo,
        POSVEL_T chainSize,
        POSVEL_T* xLocHalo,
        POSVEL_T* yLocHalo,
        POSVEL_T* zLocHalo,
        int* actualIndx);

  /////////////////////////////////////////////////////////////////////
  //
  // FOF (Friend of friends) halo analysis
  //
  /////////////////////////////////////////////////////////////////////

  // Find the mass of each halo
  void FOFHaloMass(
        vector<POSVEL_T>* haloMass);

  // Find the average position of FOF halo particles
  void FOFPosition(
        vector<POSVEL_T>* xPos,
        vector<POSVEL_T>* yPos,
        vector<POSVEL_T>* zPos);

  // Find the average velocity of FOF halo particles
  void FOFVelocity(
        vector<POSVEL_T>* xVel,
        vector<POSVEL_T>* yVel,
        vector<POSVEL_T>* zVel);

  // Find the velocity dispersion of FOF halos
  void FOFVelocityDispersion(
        vector<POSVEL_T>* xVel,
        vector<POSVEL_T>* yVel,
        vector<POSVEL_T>* zVel,
        vector<POSVEL_T>* velDisp);

  // Kahan summation of floating point numbers to reduce roundoff error
  POSVEL_T KahanSummation(int halo, POSVEL_T* data);

  // Dot product
  POSVEL_T dotProduct(POSVEL_T x, POSVEL_T y, POSVEL_T z);

  // Incremental mean, possibly needed for very large halos
  POSVEL_T incrementalMean(int halo, POSVEL_T* data);

#ifndef USE_VTK_COSMO
  // Print information about halos for debugging and selection
  void FOFHaloCatalog(
        vector<int>* haloCenter,
        vector<POSVEL_T>* xVel,
        vector<POSVEL_T>* yVel,
        vector<POSVEL_T>* zVel);

  void printHaloSizes(int minSize);
  void printLocations(int haloIndex);
  void printBoundingBox(int haloIndex);
#endif

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  string outFile;               // File of particles written by this processor

  POSVEL_T boxSize;             // Physical box size of the data set
  POSVEL_T deadSize;            // Border size for dead particles
  POSVEL_T particleMass;        // Mass of a single particle
  POSVEL_T bb;                  // Interparticle distance for halos

  long   particleCount;         // Total particles on this processor

  POSVEL_T* xx;                 // X location for particles on this processor
  POSVEL_T* yy;                 // Y location for particles on this processor
  POSVEL_T* zz;                 // Z location for particles on this processor
  POSVEL_T* vx;                 // X velocity for particles on this processor
  POSVEL_T* vy;                 // Y velocity for particles on this processor
  POSVEL_T* vz;                 // Z velocity for particles on this processor
  POTENTIAL_T* pot;             // Particle potential
  ID_T* tag;                    // Id tag for particles on this processor
  MASK_T* mask;                 // Particle information
  STATUS_T* status;             // Particle is ALIVE or labeled with neighbor
                                // processor index where it is ALIVE

  // Information about halos from FOF halo finder
  int  numberOfHalos;           // Number of halos found
  int* halos;                   // First particle index into haloList
  int* haloCount;               // Size of each halo 
  int* haloList;                // Indices of next particle in halo
};

#endif
