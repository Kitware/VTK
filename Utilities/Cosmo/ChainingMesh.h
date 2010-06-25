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

// .NAME ChainingMesh - Assign all particles on this processor to a 3D bucket
//
// .SECTION Description
// ChainingMesh takes particle locations and assigns particles to a mesh
// location in a 3D grid so that when an area of interest must be searched,
// only the particles in buckets for that area will be examined.  The 
// chaining mesh is designed such that the 3D mesh holds the first particle
// index in the bucket, and that array position points to the next particle
// in the bucket.
//

#ifndef ChainingMesh_h
#define ChainingMesh_h

#ifdef USE_VTK_COSMO
#include "CosmoDefinition.h"
#else
#include "Definition.h"
#endif

#include <string>
#include <vector>

using namespace std;

#ifdef USE_VTK_COSMO
class COSMO_EXPORT ChainingMesh {
#else
class ChainingMesh {
#endif
public:
  // Chaining mesh for all particles on a processor
  ChainingMesh(
        POSVEL_T rL,            // Box size of entire physical problem
        POSVEL_T deadSize,      // Dead size of overflow particles
        POSVEL_T chainSize,     // Mesh size imposed on this physical space
        vector<POSVEL_T>* xLoc, // Locations of every particle on processor
        vector<POSVEL_T>* yLoc,
        vector<POSVEL_T>* zLoc);

  // Chaining mesh for a single halo
  ChainingMesh(
        POSVEL_T* minLoc,       // Bounding box of halo
        POSVEL_T* maxLoc,       // Bounding box of halo
        POSVEL_T chainSize,     // Mesh size imposed on this physical space
        int haloCount,          // Number of particles in halo
        POSVEL_T* xLoc,         // Locations of every particle
        POSVEL_T* yLoc,
        POSVEL_T* zLoc);

  ~ChainingMesh();

  // Construct the chaining mesh
  void createChainingMesh();

  // Demonstration method to show how to iterate over the chaining mesh
  void printChainingMeshCentroids();

  POSVEL_T getChainSize()       { return this->chainSize; }

  POSVEL_T getMinMine(int dim)  { return this->minRange[dim]; }
  POSVEL_T getMaxMine(int dim)  { return this->maxRange[dim]; }
  int getMeshSize(int dim)      { return this->meshSize[dim]; }

  POSVEL_T* getMinRange()       { return this->minRange; }
  POSVEL_T* getMaxRange()       { return this->maxRange; }
  int* getMeshSize()            { return this->meshSize; }

  int*** getBucketCount()       { return this->bucketCount; }
  int*** getBuckets()           { return this->buckets; }
  int* getBucketList()          { return this->bucketList; }

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  int    layoutSize[DIMENSION]; // Decomposition of processors
  int    layoutPos[DIMENSION];  // Position of this processor in decomposition

  POSVEL_T boxSize;             // Physical box size of the data set
  POSVEL_T deadSize;            // Physical size of dead particle region

  long   particleCount;         // Total particles on this processor
  POSVEL_T* xx;                 // X location for particles on this processor
  POSVEL_T* yy;                 // Y location for particles on this processor
  POSVEL_T* zz;                 // Z location for particles on this processor
                                // processor index where it is ALIVE

  POSVEL_T chainSize;           // Grid size in chaining mesh
  POSVEL_T* minRange;           // Physical range on processor, including dead
  POSVEL_T* maxRange;           // Physical range on processor, including dead
  int* meshSize;                // Chaining mesh grid dimension

  int*** buckets;               // First particle index into bucketList
  int*** bucketCount;           // Size of each bucket 
  int* bucketList;              // Indices of next particle in halo
};

#endif
