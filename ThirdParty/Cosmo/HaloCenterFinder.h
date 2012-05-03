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

// .NAME HaloCenterFinder - find the particle in the center of halo particles
//
// HaloCenterFinder takes location data and a means of recognizing individual
// halos within the location and finds the most bound particle or the most
// connect particle.
//
// The class can be called with an array of locations, or can be called with
// all locations on a processor and two arrays used to thread halos through
// all particles.  The first array gives the index of the first particle
// in the halo and the second array takes that index and gives the index
// of the next particle in the halo.  Follow this until -1 which is the
// end of the halo.
//
// Can operate on FOF halos, subhalos or SOD halos depending on the form
// of the input.

#ifndef HaloCenterFinder_h
#define HaloCenterFinder_h

#ifdef USE_VTK_COSMO
#include "CosmoDefinition.h"
#else
#include "Definition.h"
#endif

#include "ChainingMesh.h"
#include <string>
#include <vector>

using namespace std;

#ifdef USE_VTK_COSMO
class COSMO_EXPORT HaloCenterFinder {
#else
class HaloCenterFinder {
#endif
public:
  HaloCenterFinder();
  ~HaloCenterFinder();

  // Set parameters for sizes of the dead/alive space
  void setParameters(
        POSVEL_T bb,                  // Inter particle distance for halos
        POSVEL_T distConvertFactor);  // Scale positions by

  // Set alive particle vectors which were created elsewhere
  void setParticles(
        long particleCount,
        POSVEL_T* xLoc,
        POSVEL_T* yLoc,
        POSVEL_T* zLoc,
        POSVEL_T* massHalo,
        ID_T* id);

  // Find the halo centers using most bound particle (N^2/2)
  int  mostBoundParticleN2(POTENTIAL_T* minPotential);

  // Initial guess of A* contains an actual part and an estimated part
  int  mostBoundParticleAStar(POTENTIAL_T* minPotential);

  // Calculate actual values between particles within a bucket
  void aStarThisBucketPart(
        ChainingMesh* haloChain,        // Buckets of particles
        int* bucketID,                  // Map from particle to bucket
        POSVEL_T* estimate);            // Running minimum potential

  // Calculate actual values for 26 neighbors in the center of halo
  // Level 1 refinement done for initial guess
  void aStarActualNeighborPart(
        ChainingMesh* haloChain,        // Buckets of particles
        int* minActual,                 // Range for doing actual vs estimated
        int* maxActual,
        int* refineLevel,               // Refinement level of each particle
        POSVEL_T* estimate);            // Running minimum potential

  // Calculate estimated values for 26 neighbors around the edges of halo
  // Level 0 refinement done for initial guess
  void aStarEstimatedNeighborPart(
        ChainingMesh* haloChain,        // Buckets of particles
        int* minActual,                 // Range for doing actual vs estimated
        int* maxActual,
        int* refineLevel,               // Refinement level of each particle
        POSVEL_T* estimate,             // Running minimum potential
        POSVEL_T boundarySize);         // Boundary around bucket for estimation

  // Calculate estimates for all buckets beyond the 27 closest
  void aStarEstimatedPart(
        ChainingMesh* haloChain,        // Buckets of particles
        POSVEL_T* estimate);            // Running minimum potential

  // Refinement of 0 to 1
  void refineAStarLevel_1(
        ChainingMesh* haloChain,        // Buckets of particles
        int bi,                         // Bucket containing particle to refine
        int bj,
        int bk,
        int* minActual,                 // Range for doing actual vs estimated
        int* maxActual,
        int minParticle,                // Particle to refine
        POSVEL_T* estimate,             // Running minimum potential
        POSVEL_T boundarySize);         // Boundary around bucket for estimation

  // Refinement of 1 to N
  void refineAStarLevel_N(
        ChainingMesh* haloChain,        // Buckets of particles
        int bi,                         // Bucket containing particle to refine
        int bj,
        int bk,
        int minParticle,                // Particle to refine
        POSVEL_T* estimate,             // Running minimum potential
        int winDelta);                  // Number of buckets to refine out to

  // Find the halo centers using most connected particle (N^2/2)
  int  mostConnectedParticleN2();
  int  mostConnectedParticleChainMesh();

  // Build a chaining mesh of halo particles
  ChainingMesh* buildChainingMesh(
        POSVEL_T chainSize);

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  string outFile;               // File of particles written by this processor

  POSVEL_T boxSize;             // Physical box size of the data set
  POSVEL_T deadSize;            // Border size for dead particles
  POSVEL_T bb;                  // Interparticle distance for halos
  POSVEL_T distFactor;          // Scale positions by, used in chain size

  long   particleCount;         // Total particles on this processor

  POSVEL_T* xx;                 // X location for particles on this processor
  POSVEL_T* yy;                 // Y location for particles on this processor
  POSVEL_T* zz;                 // Z location for particles on this processor
  POSVEL_T* mass;               // mass for particles on this processor
  ID_T* tag;                    // Id tag for particles on this processor
};

#endif
