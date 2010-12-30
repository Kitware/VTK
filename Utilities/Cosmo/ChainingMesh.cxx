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

#include "Partition.h"
#include "ChainingMesh.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <vector>
#include <algorithm>
#include <math.h>

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// ChainingMesh assigns all particles on this processor to a 3D mesh 
// of buckets for more efficient iteration on particles in an area
//
/////////////////////////////////////////////////////////////////////////

ChainingMesh::ChainingMesh(
                        POSVEL_T rL,
                        POSVEL_T deadSz,
                        POSVEL_T chainSz,
                        vector<POSVEL_T>* xLoc,
                        vector<POSVEL_T>* yLoc,
                        vector<POSVEL_T>* zLoc)
{
  // Sizes for the entire problem
  this->boxSize = rL;
  this->deadSize = deadSz;

  // Imposed bucket size on this processor
  this->chainSize = chainSz;

  // Extract the contiguous data block from a vector pointer
  this->particleCount = (long)xLoc->size();
  this->xx = &(*xLoc)[0];
  this->yy = &(*yLoc)[0];
  this->zz = &(*zLoc)[0];

  // Get the number of processors and rank of this processor
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();

  // Get the number of processors in each dimension
  Partition::getDecompSize(this->layoutSize);

  // Get my position within the Cartesian topology
  Partition::getMyPosition(this->layoutPos);

  // Calculate the physical boundary on this processor for alive particles
  POSVEL_T boxStep[DIMENSION];
  POSVEL_T minAlive[DIMENSION];
  POSVEL_T maxAlive[DIMENSION];
  this->meshSize = new int[DIMENSION];
  this->minRange = new POSVEL_T[DIMENSION];
  this->maxRange = new POSVEL_T[DIMENSION];

  for (int dim = 0; dim < DIMENSION; dim++) {
    boxStep[dim] = this->boxSize / this->layoutSize[dim];
   
    // Region of particles that are alive on this processor
    minAlive[dim] = this->layoutPos[dim] * boxStep[dim];
    maxAlive[dim] = minAlive[dim] + boxStep[dim];
    if (maxAlive[dim] > this->boxSize)
      maxAlive[dim] = this->boxSize;
      
    // Allow for the boundary of dead particles, normalized to 0
    // Overall boundary will be [0:(rL+2*deadSize)]
    this->minRange[dim] = minAlive[dim] - this->deadSize;
    this->maxRange[dim] = maxAlive[dim] + this->deadSize;

    // How many chain mesh grids will fit
    this->meshSize[dim] = (int)((this->maxRange[dim] - this->minRange[dim]) / 
				this->chainSize) + 1;
  }

  // Create the chaining mesh
  createChainingMesh();
}

/////////////////////////////////////////////////////////////////////////
//
// ChainingMesh assigns all particles in a halo to a 3D mesh
// of buckets for more efficient iteration on particles in an area
//
/////////////////////////////////////////////////////////////////////////

ChainingMesh::ChainingMesh(
                        POSVEL_T* minLoc,
                        POSVEL_T* maxLoc,
                        POSVEL_T chainSz,
                        int haloCount,
                        POSVEL_T* xLoc,
                        POSVEL_T* yLoc,
                        POSVEL_T* zLoc)
{
  this->meshSize = new int[DIMENSION];
  this->minRange = new POSVEL_T[DIMENSION];
  this->maxRange = new POSVEL_T[DIMENSION];

  // Bucket size
  this->chainSize = chainSz;

  // Extract the contiguous data block from a vector pointer
  this->particleCount = haloCount;
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;

  // Find the grid size of this chaining mesh
  for (int dim = 0; dim < DIMENSION; dim++) {
    this->minRange[dim] = minLoc[dim];
    this->maxRange[dim] = maxLoc[dim];
    this->meshSize[dim] = (int)((this->maxRange[dim] - this->minRange[dim]) / 
				this->chainSize) + 1;
  }

  // Create the chaining mesh
  createChainingMesh();
}

/////////////////////////////////////////////////////////////////////////
//
// Destructor of buckets
//
/////////////////////////////////////////////////////////////////////////

ChainingMesh::~ChainingMesh()
{
  for (int i = 0; i < this->meshSize[0]; i++) {
    for (int j = 0; j < this->meshSize[1]; j++) {
      delete [] this->buckets[i][j];
      delete [] this->bucketCount[i][j];
    }
    delete [] this->buckets[i];
    delete [] this->bucketCount[i];
  }
  delete [] this->buckets;
  delete [] this->bucketCount;
  delete [] this->bucketList;
  delete [] this->meshSize;
  delete [] this->minRange;
  delete [] this->maxRange;
}

/////////////////////////////////////////////////////////////////////////
//
// Create the chaining mesh which organizes particles into location grids
// by creating buckets of locations and chaining the indices of the
// particles so that all particles in a bucket can be located
//
/////////////////////////////////////////////////////////////////////////

void ChainingMesh::createChainingMesh()
{
  // Create the bucket grid and initialize to -1
  this->buckets = new int**[this->meshSize[0]];
  this->bucketCount = new int**[this->meshSize[0]];

  for (int i = 0; i < this->meshSize[0]; i++) {
    this->buckets[i] = new int*[this->meshSize[1]];
    this->bucketCount[i] = new int*[this->meshSize[1]];

    for (int j = 0; j < this->meshSize[1]; j++) {
      this->buckets[i][j] = new int[this->meshSize[2]];
      this->bucketCount[i][j] = new int[this->meshSize[2]];

      for (int k = 0; k < this->meshSize[2]; k++) {
        this->buckets[i][j][k] = -1;
        this->bucketCount[i][j][k] = 0;
      }
    }
  }

  // Create the chaining list of particles and initialize to -1
  this->bucketList = new int[this->particleCount];
  for (int p = 0; p < this->particleCount; p++)
    this->bucketList[p] = -1;

  // Iterate on all particles on this processor and assign a bucket grid
  // First particle index is assigned to the actual bucket grid
  // Next particle found is assigned to the bucket grid, only after the
  // index which is already there is assigned to the new particles index
  // position in the bucketList.
  // Then to iterate through all particles in a bucket, start with the index
  // in the buckets grid and follow it through the bucketList until -1 reached

  for (int p = 0; p < this->particleCount; p++) {

    POSVEL_T loc[DIMENSION];
    loc[0] = this->xx[p];
    loc[1] = this->yy[p];
    loc[2] = this->zz[p];

    int i = (int)((loc[0] - this->minRange[0]) / this->chainSize);
    int j = (int)((loc[1] - this->minRange[1]) / this->chainSize);
    int k = (int)((loc[2] - this->minRange[2]) / this->chainSize);

    // First particle in bucket
    if (this->buckets[i][j][k] == -1) {
      this->buckets[i][j][k] = p;
      this->bucketCount[i][j][k]++;
    }

    // Other particles in same bucket
    else {
      this->bucketList[p] = this->buckets[i][j][k];
      this->buckets[i][j][k] = p;
      this->bucketCount[i][j][k]++;
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Demonstration method to show how to iterate over the chaining mesh
// Calculate the centroid of each bucket
//
/////////////////////////////////////////////////////////////////////////

void ChainingMesh::printChainingMeshCentroids()
{
  // Test by calculating centroid of each bucket grid
  POSVEL_T centroid[DIMENSION];

  // Iterate on all particles in a bucket
  for (int i = 0; i < this->meshSize[0]; i++) {
    for (int j = 0; j < this->meshSize[1]; j++) {
      for (int k = 0; k < this->meshSize[2]; k++) {

        centroid[0] = 0.0;
        centroid[1] = 0.0;
        centroid[2] = 0.0;
    
        // First particle in the bucket
        int p = this->buckets[i][j][k];

        while (p != -1) {
          centroid[0] += this->xx[p];
          centroid[1] += this->yy[p];
          centroid[2] += this->zz[p];

          // Next particle in the bucket
          p = this->bucketList[p];
        }
        for (int dim = 0; dim < DIMENSION; dim++) {
          if (centroid[dim] != 0.0)
            centroid[dim] /= this->bucketCount[i][j][k];
        }
#ifndef USE_VTK_COSMO
        cout << "Bucket " << i << "," << j << "," << k 
             << " count = " << bucketCount[i][j][k]
             << " centroid = " << centroid[0] << "," << centroid[1] << "," 
             << centroid[2] << endl;
#endif
      }
    }
  }
}
