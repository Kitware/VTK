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
#include "HaloCenterFinder.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <math.h>

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// HaloCenterFinder takes all particles in a halo and calculations the
// most bound particle (MBP) or most connected particle (MCP) using
// an N2/2 algorithm on small halos and ChainingMesh algorithms on
// large halos.
//
/////////////////////////////////////////////////////////////////////////

HaloCenterFinder::HaloCenterFinder()
{
  // Get the number of processors and rank of this processor
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();
}

HaloCenterFinder::~HaloCenterFinder()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for the halo center finder
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::setParameters(
                        POSVEL_T pDist,
                        POSVEL_T distConvertFactor)
{
  // Halo finder parameters
  this->bb = pDist;
  this->distFactor = distConvertFactor;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::setParticles(
                        long haloCount,
                        POSVEL_T* xLoc,
                        POSVEL_T* yLoc,
                        POSVEL_T* zLoc,
                        POSVEL_T* massHalo,
                        ID_T* id)
{
  this->particleCount = haloCount;
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->mass = massHalo;
  this->tag = id;
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the most connected particle using (N*(N-1)) / 2 algorithm
// This is the particle with the most friends (most particles within bb)
// Locations of the particles have taken wraparound into account so that
// processors on the low edge of a dimension have particles with negative
// positions and processors on the high edge of a dimension have particles
// with locations greater than the box size
//
/////////////////////////////////////////////////////////////////////////

int HaloCenterFinder::mostConnectedParticleN2()
{
  // Arrange in an upper triangular grid of friend counts
  // friendCount will hold number of friends that a particle has
  //
  int* friendCount = new int[this->particleCount];
  for (int i = 0; i < this->particleCount; i++)
    friendCount[i] = 0;

  // Iterate on all particles in halo adding to count if friends of each other
  // Iterate in upper triangular fashion
  for (int p = 0; p < this->particleCount; p++) {

    // Get halo particle after the current one
    for (int q = p+1; q < this->particleCount; q++) {

      // Calculate distance betweent the two
      POSVEL_T xdist = fabs(this->xx[p] - this->xx[q]);
      POSVEL_T ydist = fabs(this->yy[p] - this->yy[q]);
      POSVEL_T zdist = fabs(this->zz[p] - this->zz[q]);

      if ((xdist < this->bb) && (ydist < this->bb) && (zdist < this->bb)) {
        POSVEL_T dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
        if (dist < this->bb) {
          friendCount[p]++;
          friendCount[q]++;
        }
      }
    }
  }

  // Particle with the most friends
  int maxFriends = 0;
  int result = 0;

  for (int i = 0; i < this->particleCount; i++) {
    if (friendCount[i] > maxFriends) {
      maxFriends = friendCount[i];
      result = i;
    }
  }

  delete [] friendCount;
  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Most connected particle using a chaining mesh of particles in one FOF halo
// Build chaining mesh with a grid size such that all friends will be in
// adjacent mesh grids.
//
/////////////////////////////////////////////////////////////////////////

int HaloCenterFinder::mostConnectedParticleChainMesh()
{
  int bp, bi, bj, bk;
  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;

  // Build the chaining mesh
  int chainFactor = MCP_CHAIN_FACTOR;
  POSVEL_T chainSize = this->bb / chainFactor;
  ChainingMesh* haloChain = buildChainingMesh(chainSize);

  // Save the number of friends for each particle in the halo
  int* friendCount = new int[this->particleCount];
  for (int i = 0; i < this->particleCount; i++)
    friendCount[i] = 0;

  // Get chaining mesh information
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();

  // Calculate the friend count within each bucket using upper triangular loop
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        bp = buckets[bi][bj][bk];
        while (bp != -1) {

          wp = bucketList[bp];
          while (wp != -1) {
            xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
            ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
            zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
            dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
            if (dist != 0.0 && dist < this->bb) {
              friendCount[bp]++;
              friendCount[wp]++;
            }
            wp = bucketList[wp];
          }
          bp = bucketList[bp];
        }
      }
    }
  }

  // Walk every bucket in the chaining mesh, processing all particles in bucket
  // against all neighbor bucket particles one time, storing the friend
  // count in two places, using the sliding window trick
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        // Set the walking window around this bucket
        first[0] = bi - chainFactor; last[0] = bi + chainFactor;
        first[1] = bj - chainFactor; last[1] = bj + chainFactor;
        first[2] = bk - chainFactor; last[2] = bk + chainFactor;

        for (int dim = 0; dim < DIMENSION; dim++) {
          if (first[dim] < 0)
            first[dim] = 0;
          if (last[dim] >= meshSize[dim])
            last[dim] = meshSize[dim] - 1;
        }

        // First particle in the bucket being processed
        bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // For the current particle in the current bucket count friends
          // going to all neighbor buckets in the chaining mesh.
          // With the sliding window we calculate the distance between
          // two particles and can fill in both, but when the second particle's
          // bucket is reached we can't calculate and add in again
          // So we must be aware of which buckets have not already been
          // compared to this bucket and calculate only for planes and rows
          // that have not already been processed

          // Do entire trailing plane of buckets that has not been processed
          for (wi = bi + 1; wi <= last[0]; wi++) {
            for (wj = first[1]; wj <= last[1]; wj++) {
              for (wk = first[2]; wk <= last[2]; wk++) {
                wp = buckets[wi][wj][wk];
                while (wp != -1) {
                  xdist = (POSVEL_T) fabs(this->xx[bp] - this->xx[wp]);
                  ydist = (POSVEL_T) fabs(this->yy[bp] - this->yy[wp]);
                  zdist = (POSVEL_T) fabs(this->zz[bp] - this->zz[wp]);
                  dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                  if (dist != 0.0 && dist < this->bb) {
                    friendCount[bp]++;
                    friendCount[wp]++;
                  }
                  wp = bucketList[wp];
                }
              }
            }
          }

          // Do entire trailing row that has not been processed in this plane
          wi = bi;
          for (wj = bj + 1; wj <= last[1]; wj++) {
            for (wk = first[2]; wk <= last[2]; wk++) {
              wp = buckets[wi][wj][wk];
              while (wp != -1) {
                xdist = (POSVEL_T) fabs(this->xx[bp] - this->xx[wp]);
                ydist = (POSVEL_T) fabs(this->yy[bp] - this->yy[wp]);
                zdist = (POSVEL_T) fabs(this->zz[bp] - this->zz[wp]);
                dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                if (dist != 0.0 && dist < this->bb) {
                  friendCount[bp]++;
                  friendCount[wp]++;
                }
                wp = bucketList[wp];
              }
            }
          }

          // Do bucket trailing buckets in this row
          wi = bi;
          wj = bj;
          for (wk = bk+1; wk <= last[2]; wk++) {
            wp = buckets[wi][wj][wk];
            while (wp != -1) {
              xdist = (POSVEL_T) fabs(this->xx[bp] - this->xx[wp]);
              ydist = (POSVEL_T) fabs(this->yy[bp] - this->yy[wp]);
              zdist = (POSVEL_T) fabs(this->zz[bp] - this->zz[wp]);
              dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
              if (dist != 0.0 && dist < this->bb) {
                friendCount[bp]++;
                friendCount[wp]++;
              }
              wp = bucketList[wp];
            }
          }
          bp = bucketList[bp];
        }
      }
    }
  }
  // Particle with the most friends
  int maxFriends = 0;
  int result = 0;

  for (int i = 0; i < this->particleCount; i++) {
    if (friendCount[i] > maxFriends) {
      maxFriends = friendCount[i];
      result = i;
    }
  }

  delete [] friendCount;
  delete haloChain;

  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the most bound particle using (N*(N-1)) / 2 algorithm
// Also minimum potential particle for the halo.
// Locations of the particles have taken wraparound into account so that
// processors on the low edge of a dimension have particles with negative
// positions and processors on the high edge of a dimension have particles
// with locations greater than the box size
//
/////////////////////////////////////////////////////////////////////////

int HaloCenterFinder::mostBoundParticleN2(POTENTIAL_T* minPotential)
{
  // Arrange in an upper triangular grid to save computation
  POTENTIAL_T* lpot = new POTENTIAL_T[this->particleCount];
  for (int i = 0; i < this->particleCount; i++)
    lpot[i] = 0.0;

  // First particle in halo to calculate minimum potential on
  for (int p = 0; p < this->particleCount; p++) {

    // Next particle in halo in minimum potential loop
    for (int q = p+1; q < this->particleCount; q++) {

      POSVEL_T xdist = (POSVEL_T)fabs(this->xx[p] - this->xx[q]);
      POSVEL_T ydist = (POSVEL_T)fabs(this->yy[p] - this->yy[q]);
      POSVEL_T zdist = (POSVEL_T)fabs(this->zz[p] - this->zz[q]);

      POSVEL_T r = sqrt((xdist * xdist) + (ydist * ydist) + (zdist * zdist));

      if (r != 0.0) {
        lpot[p] = (POTENTIAL_T)(lpot[p] - (this->mass[q] / r));
        lpot[q] = (POTENTIAL_T)(lpot[q] - (this->mass[p] / r));
      }
    }
  }

  *minPotential = MAX_FLOAT;
  int result = 0;
  for (int i = 0; i < this->particleCount; i++) {
    if (lpot[i] < *minPotential) {
      *minPotential = lpot[i];
      result = i;
    }
  }
  delete [] lpot;

  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Most bound particle using a chaining mesh of particles in one FOF halo.
// and a combination of actual particle-to-particle values and estimation
// values based on number of particles in a bucket and the distance to the
// nearest corner.
//
// For the center area of a halo calculate the actual values for 26 neigbors.
// For the perimeter area of a halo use a bounding box of those neighbors
// to make up the actual portion and a estimate to other particles in the
// neighbors.  This is to keep a particle from being too close to the
// closest corner and giving a skewed answer.
//
// The refinement in the center buckets will be called level 1 because all
// buckets to a distance of 1 are calculated fully.  The refinement of the
// perimeter buckets will be called level 0 because only the center bucket
// is calculated fully.
//
// Note that in refining, level 0 must be brought up to level 1, and then
// refinement to more buckets becomes the same.
//
/////////////////////////////////////////////////////////////////////////

int HaloCenterFinder::mostBoundParticleAStar(POTENTIAL_T* minimumPotential)
{
  // Chaining mesh size is a factor of the interparticle halo distance
  POSVEL_T chainSize = this->bb * this->distFactor;

  // Boundary around edges of a bucket for calculating estimate
  POSVEL_T boundaryFactor = 10.0f * this->distFactor;
  POSVEL_T boundarySize = chainSize / boundaryFactor;

  // Actual values calculated for 26 neighbors in the center of a halo
  // Factor to decide what distance this is out from the center
  int eachSideFactor = 7;

  // Create the chaining mesh for this halo
  ChainingMesh* haloChain = buildChainingMesh(chainSize);

  // Get chaining mesh information
  int* meshSize = haloChain->getMeshSize();

  // Bucket ID allows finding the bucket every particle is in
  int* bucketID = new int[this->particleCount];

  // Refinement level for a particle indicate how many buckets out have actual
  // values calculate rather than estimates
  int* refineLevel = new int[this->particleCount];

  // Minimum potential made up of actual part and estimated part
  POSVEL_T* estimate = new POSVEL_T[this->particleCount];
  for (int i = 0; i < this->particleCount; i++)
    estimate[i] = 0.0;

  // Calculate better guesses (refinement level 1) around the center of halo
  // Use estimates with boundary around neighbors of perimeter
  int* minActual = new int[DIMENSION];
  int* maxActual = new int[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    int eachSide = meshSize[dim] / eachSideFactor;
    int middle = meshSize[dim] / 2;
    minActual[dim] = middle - eachSide;
    maxActual[dim] = middle + eachSide;
  }


  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate actual for particles within individual bucket
  //
  aStarThisBucketPart(haloChain, bucketID, estimate);

  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate actual values for immediate 26 neighbors for buckets in
  // the center of the halo (refinement level = 1)
  //
  aStarActualNeighborPart(haloChain, minActual, maxActual,
                          refineLevel, estimate);

  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate estimated values for immediate 26 neighbors for buckets on
  // the edges of the halo (refinement level = 0)
  //
  aStarEstimatedNeighborPart(haloChain, minActual, maxActual,
                             refineLevel, estimate, boundarySize);

  //////////////////////////////////////////////////////////////////////////
  //
  // All buckets beyond the 27 nearest get an estimate based on count in
  // the bucket and the distance to the nearest point
  //
  aStarEstimatedPart(haloChain, estimate);

  //////////////////////////////////////////////////////////////////////////
  //
  // Iterative phase to refine individual particles
  //
  POSVEL_T minPotential = estimate[0];
  int minParticleCur = 0;
  int winDelta = 1;

  // Find the current minimum potential particle after actual and estimates
  for (int i = 0; i < this->particleCount; i++) {
    if (estimate[i] < minPotential) {
      minPotential = estimate[i];
      minParticleCur = i;
    }
  }
  POSVEL_T minPotentialLast = minPotential;
  int minParticleLast = -1;

  // Decode the bucket from the ID
  int id = bucketID[minParticleCur];
  int bk = id % meshSize[2];
  id = id - bk;
  int bj = (id % (meshSize[2] * meshSize[1])) / meshSize[2];
  id = id - (bj * meshSize[2]);
  int bi = id / (meshSize[2] * meshSize[1]);

  // Calculate the maximum winDelta for this bucket
  int maxDelta = max(max(
                     max(meshSize[0] - bi, bi), max(meshSize[1] - bj, bj)),
                     max(meshSize[2] - bk, bk));

  // Terminate when a particle is the minimum twice in a row AND
  // it has been calculated precisely without estimates over the entire halo
  int pass = 1;
  while (winDelta <= maxDelta) {
    while (minParticleLast != minParticleCur) {

      // Refine the value for all particles in the same bucket as the minimum
      // Alter the minimum in the reference
      // Return the particle index that is the new minimum of that bucket
      while (winDelta > refineLevel[minParticleCur] &&
             estimate[minParticleCur] <= minPotentialLast) {
        pass++;
        refineLevel[minParticleCur]++;

        // Going from level 0 to level 1 is special because the 27 neighbors
        // are part actual and part estimated.  After that all refinements are
        // replacing an estimate with an actual
        if (refineLevel[minParticleCur] == 1) {
          refineAStarLevel_1(haloChain, bi, bj, bk, minActual, maxActual,
                             minParticleCur, estimate,
                             boundarySize);
        } else {
          refineAStarLevel_N(haloChain, bi, bj, bk,
                             minParticleCur, estimate,
                             refineLevel[minParticleCur]);
        }
      }
      if (winDelta <= refineLevel[minParticleCur]) {
        minPotentialLast = estimate[minParticleCur];
        minParticleLast = minParticleCur;
      }

      // Find the current minimum particle
      minPotential = minPotentialLast;
      for (int i = 0; i < this->particleCount; i++) {
        if (estimate[i] <= minPotential) {
          minPotential = estimate[i];
          minParticleCur = i;
        }
      }

      // Decode the bucket from the ID
      id = bucketID[minParticleCur];
      bk = id % meshSize[2];
      id = id - bk;
      bj = (id % (meshSize[2] * meshSize[1])) / meshSize[2];
      id = id - (bj * meshSize[2]);
      bi = id / (meshSize[2] * meshSize[1]);

      // Calculate the maximum winDelta for this bucket
      maxDelta = max(max(
                     max(meshSize[0] - bi, bi), max(meshSize[1] - bj, bj)),
                     max(meshSize[2] - bk, bk));
    }
    pass++;
    winDelta++;
    minParticleLast = 0;
  }
  int result = minParticleCur;
  *minimumPotential = estimate[minParticleCur];

  delete [] estimate;
  delete [] bucketID;
  delete [] refineLevel;
  delete [] minActual;
  delete [] maxActual;
  delete haloChain;

  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Within a bucket calculate the actual values between all particles
// Set the bucket ID so that the associated bucket can be located quickly
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::aStarThisBucketPart(
                        ChainingMesh* haloChain,
                        int* bucketID,
                        POSVEL_T* estimate)
{
  POSVEL_T xdist, ydist, zdist, dist;
  int bp, bp2, bi, bj, bk;

  // Get chaining mesh information
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();

  // Calculate actual values for all particles in the same bucket
  // All pairs are calculated one time and stored twice
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // Remember the bucket that every particle is in
          bucketID[bp] = (bi * meshSize[1] * meshSize[2]) +
                         (bj * meshSize[2]) + bk;

          bp2 = bucketList[bp];
          while (bp2 != -1) {
            xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[bp2]);
            ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[bp2]);
            zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[bp2]);
            dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
            if (dist != 0.0) {
              estimate[bp] -= (this->mass[bp2] / dist);
              estimate[bp2] -= (this->mass[bp] / dist);
            }
            bp2 = bucketList[bp2];
          }
          bp = bucketList[bp];
        }
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////
//
// Calculate the actual values to particles in 26 immediate neighbors
// only for buckets in the center of the halo, indicated by min/maxActual.
// Do this with a sliding window so that an N^2/2 algorithm is done where
// calculations are stored in both particles at same time.  Set refineLevel
// to 1 indicating buckets to a distance of one from the particle were
// calculated completely.
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::aStarActualNeighborPart(
                        ChainingMesh* haloChain,
                        int* minActual,
                        int* maxActual,
                        int* refineLevel,
                        POSVEL_T* estimate)
{
  // Walking window extents and size
  int bp, bi, bj, bk;
  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;

  // Get chaining mesh information
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();

  // Process the perimeter buckets which contribute to the actual values
  // but which will get estimate values for their own particles
  for (bi = minActual[0] - 1; bi <= maxActual[0] + 1; bi++) {
    for (bj = minActual[1] - 1; bj <= maxActual[1] + 1; bj++) {
      for (bk = minActual[2] - 1; bk <= maxActual[2] + 1; bk++) {

        // Only do the perimeter buckets
        if ((bucketCount[bi][bj][bk] > 0) &&
            ((bi < minActual[0] || bi > maxActual[0]) ||
             (bj < minActual[1] || bj > maxActual[1]) ||
             (bk < minActual[2] || bk > maxActual[2]))) {

          // Set a window around this bucket for calculating actual potentials
          first[0] = bi - 1;    last[0] = bi + 1;
          first[1] = bj - 1;    last[1] = bj + 1;
          first[2] = bk - 1;    last[2] = bk + 1;
          for (int dim = 0; dim < DIMENSION; dim++) {
            if (first[dim] < minActual[dim])
              first[dim] = minActual[dim];
            if (last[dim] > maxActual[dim])
              last[dim] = maxActual[dim];
          }

          bp = buckets[bi][bj][bk];
          while (bp != -1) {

            // Check each bucket in the window
            for (wi = first[0]; wi <= last[0]; wi++) {
              for (wj = first[1]; wj <= last[1]; wj++) {
                for (wk = first[2]; wk <= last[2]; wk++) {

                  // Only do the window bucket if it is in the actual region
                  if (bucketCount[wi][wj][wk] != 0 &&
                      wi >= minActual[0] && wi <= maxActual[0] &&
                      wj >= minActual[1] && wj <= maxActual[1] &&
                      wk >= minActual[2] && wk <= maxActual[2]) {

                    wp = buckets[wi][wj][wk];
                    while (wp != -1) {
                      xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
                      ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
                      zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
                      dist = sqrt((xdist*xdist)+(ydist*ydist)+(zdist*zdist));
                      if (dist != 0.0) {
                        estimate[bp] -= (this->mass[wp] / dist);
                        estimate[wp] -= (this->mass[bp] / dist);
                      }
                      wp = bucketList[wp];
                    }
                  }
                }
              }
            }
            bp = bucketList[bp];
          }
        }
      }
    }
  }

  // Process the buckets in the center
  for (bi = minActual[0]; bi <= maxActual[0]; bi++) {
    for (bj = minActual[1]; bj <= maxActual[1]; bj++) {
      for (bk = minActual[2]; bk <= maxActual[2]; bk++) {

        // Set a window around this bucket for calculating actual potentials
        first[0] = bi - 1;    last[0] = bi + 1;
        first[1] = bj - 1;    last[1] = bj + 1;
        first[2] = bk - 1;    last[2] = bk + 1;
        for (int dim = 0; dim < DIMENSION; dim++) {
          if (first[dim] < minActual[dim])
            first[dim] = minActual[dim];
          if (last[dim] > maxActual[dim])
            last[dim] = maxActual[dim];
        }

        bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // For the current particle in the current bucket calculate
          // the actual part from the 27 surrounding buckets
          // With the sliding window we calculate the distance between
          // two particles and can fill in both, but when the second particle's
          // bucket is reached we can't calculate and add in again
          // So we must be aware of which buckets have not already been
          // compared to this bucket and calculate only for planes and rows
          // that have not already been processed
          refineLevel[bp] = 1;

          // Do entire trailing plane of buckets that has not been processed
          for (wi = bi + 1; wi <= last[0]; wi++) {
            for (wj = first[1]; wj <= last[1]; wj++) {
              for (wk = first[2]; wk <= last[2]; wk++) {

                wp = buckets[wi][wj][wk];
                while (wp != -1) {
                  xdist = fabs(this->xx[bp] - this->xx[wp]);
                  ydist = fabs(this->yy[bp] - this->yy[wp]);
                  zdist = fabs(this->zz[bp] - this->zz[wp]);
                  dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                  if (dist != 0.0) {
                    estimate[bp] -= (this->mass[wp] / dist);
                    estimate[wp] -= (this->mass[bp] / dist);
                  }
                  wp = bucketList[wp];
                }
              }
            }
          }

          // Do entire trailing row that has not been processed in this plane
          wi = bi;
          for (wj = bj + 1; wj <= last[1]; wj++) {
            for (wk = first[2]; wk <= last[2]; wk++) {
              wp = buckets[wi][wj][wk];
              while (wp != -1) {
                xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
                ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
                zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
                dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                if (dist != 0) {
                  estimate[bp] -= (this->mass[wp] / dist);
                  estimate[wp] -= (this->mass[bp] / dist);
                }
                wp = bucketList[wp];
              }
            }
          }

          // Do bucket for right hand neighbor
          wi = bi;
          wj = bj;
          for (wk = bk + 1; wk <= last[2]; wk++) {
            wp = buckets[wi][wj][wk];
            while (wp != -1) {
              xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
              ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
              zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
              dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
              if (dist != 0.0) {
                estimate[bp] -= (this->mass[wp] / dist);
                estimate[wp] -= (this->mass[bp] / dist);
              }
              wp = bucketList[wp];
            }
          }
          bp = bucketList[bp];
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the estimated values to particles in 26 immediate neighbors
// Actual values are calculated within the boundary for safety and
// an estimation to the remaining points using the nearest point in the
// neighbor outside of the boundary
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::aStarEstimatedNeighborPart(
                        ChainingMesh* haloChain,
                        int* minActual,
                        int* maxActual,
                        int* refineLevel,
                        POSVEL_T* estimate,
                        POSVEL_T boundarySize)
{
  // Walking window extents and size
  int bp, bi, bj, bk;
  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T minBound[DIMENSION], maxBound[DIMENSION];
  POSVEL_T xNear = 0.0;
  POSVEL_T yNear = 0.0;
  POSVEL_T zNear = 0.0;
  POSVEL_T xdist, ydist, zdist, dist;

  // Get chaining mesh information
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  POSVEL_T* minRange = haloChain->getMinRange();
  POSVEL_T chainSize = haloChain->getChainSize();

  // Calculate estimates for all buckets not in the center
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        if ((bucketCount[bi][bj][bk] > 0) &&
            ((bi < minActual[0] || bi > maxActual[0]) ||
             (bj < minActual[1] || bj > maxActual[1]) ||
             (bk < minActual[2] || bk > maxActual[2]))) {

          // Set a window around this bucket for calculating estimates
          first[0] = bi - 1;    last[0] = bi + 1;
          first[1] = bj - 1;    last[1] = bj + 1;
          first[2] = bk - 1;    last[2] = bk + 1;

          // Calculate the bounding box around the current bucket
          minBound[0] = minRange[0] + (bi * chainSize) - boundarySize;
          maxBound[0] = minRange[0] + ((bi + 1) * chainSize) + boundarySize;
          minBound[1] = minRange[1] + (bj * chainSize) - boundarySize;
          maxBound[1] = minRange[1] + ((bj + 1) * chainSize) + boundarySize;
          minBound[2] = minRange[2] + (bk * chainSize) - boundarySize;
          maxBound[2] = minRange[2] + ((bk + 1) * chainSize) + boundarySize;

          for (int dim = 0; dim < DIMENSION; dim++) {
            if (first[dim] < 0) {
              first[dim] = 0;
              minBound[dim] = 0.0;
            }
            if (last[dim] >= meshSize[dim]) {
              last[dim] = meshSize[dim] - 1;
              maxBound[dim] = (meshSize[dim] - 1) * chainSize;
            }
          }

          // Calculate actual and estimated for every particle in this bucket
          bp = buckets[bi][bj][bk];
          while (bp != -1) {

            // Since it is not fully calculated refinement level is 0
            refineLevel[bp] = 0;

            // Process all neighbor buckets of this one
            for (wi = first[0]; wi <= last[0]; wi++) {
              for (wj = first[1]; wj <= last[1]; wj++) {
                for (wk = first[2]; wk <= last[2]; wk++) {

                  // If bucket has particles, and is not within the region which
                  // calculates actual neighbor values
                  if ((bucketCount[wi][wj][wk] > 0) &&
                      ((wi > maxActual[0] || wi < minActual[0]) ||
                       (wj > maxActual[1] || wj < minActual[1]) ||
                       (wk > maxActual[2] || wk < minActual[2])) &&
                      (wi != bi || wj != bj || wk != bk)) {

                    // What is the nearest point between buckets
                    if (wi < bi)  xNear = minBound[0];
                    if (wi == bi) xNear = (minBound[0] + maxBound[0]) / 2.0f;
                    if (wi > bi)  xNear = maxBound[0];
                    if (wj < bj)  yNear = minBound[1];
                    if (wj == bj) yNear = (minBound[1] + maxBound[1]) / 2.0f;
                    if (wj > bj)  yNear = maxBound[1];
                    if (wk < bk)  zNear = minBound[2];
                    if (wk == bk) zNear = (minBound[2] + maxBound[2]) / 2.0f;
                    if (wk > bk)  zNear = maxBound[2];

                    wp = buckets[wi][wj][wk];
                    int estimatedParticleCount = 0;
                    while (wp != -1) {
                      if (this->xx[wp] > minBound[0] &&
                          this->xx[wp] < maxBound[0] &&
                          this->yy[wp] > minBound[1] &&
                          this->yy[wp] < maxBound[1] &&
                          this->zz[wp] > minBound[2] &&
                          this->zz[wp] < maxBound[2]) {

                        // Is the window particle within the boundary condition
                        // Calculate actual potential
                        xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
                        ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
                        zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
                        dist = sqrt(xdist*xdist + ydist*ydist + zdist*zdist);
                        if (dist != 0.0) {
                          estimate[bp] -= (this->mass[wp] / dist);
                        }
                      } else {
                        // Count to create estimated potential
                        estimatedParticleCount++;
                      }
                      wp = bucketList[wp];
                    }

                    // Find nearest corner or location to this bucket
                    // Calculate estimated value for the part of the bucket
                    xdist = (POSVEL_T)fabs(this->xx[bp] - xNear);
                    ydist = (POSVEL_T)fabs(this->yy[bp] - yNear);
                    zdist = (POSVEL_T)fabs(this->zz[bp] - zNear);
                    dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                    if (dist != 0) {
                      estimate[bp] -=
                        ((this->mass[bp] / dist) * estimatedParticleCount);
                    }
                  }
                }
              }
            }
            bp = bucketList[bp];
          }
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Add in an estimation for all buckets outside of the immediate 27 neighbors
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::aStarEstimatedPart(
                        ChainingMesh* haloChain,
                        POSVEL_T* estimate)
{
  // Walking window extents and size
  int bp, bi, bj, bk;
  int wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;
  POSVEL_T xNear, yNear, zNear;

  // Get chaining mesh information
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  POSVEL_T chainSize = haloChain->getChainSize();
  POSVEL_T* minRange = haloChain->getMinRange();

  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        // Set a window around this bucket for calculating actual potentials
        first[0] = bi - 1;    last[0] = bi + 1;
        first[1] = bj - 1;    last[1] = bj + 1;
        first[2] = bk - 1;    last[2] = bk + 1;
        for (int dim = 0; dim < DIMENSION; dim++) {
          if (first[dim] < 0)
            first[dim] = 0;
          if (last[dim] >= meshSize[dim])
            last[dim] = meshSize[dim] - 1;
        }

        for (wi = 0; wi < meshSize[0]; wi++) {
          for (wj = 0; wj < meshSize[1]; wj++) {
            for (wk = 0; wk < meshSize[2]; wk++) {

              // Exclude the buckets for which actual values were calculated
              if ((wi < first[0] || wi > last[0] ||
                   wj < first[1] || wj > last[1] ||
                   wk < first[2] || wk > last[2]) &&
                  (bucketCount[wi][wj][wk] > 0)) {

                // Nearest corner of the compared bucket to this particle
                bp = buckets[bi][bj][bk];
                xNear = minRange[0] + (wi * chainSize);
                yNear = minRange[1] + (wj * chainSize);
                zNear = minRange[2] + (wk * chainSize);
                if (this->xx[bp] > xNear)
                  xNear += chainSize;
                if (this->yy[bp] > yNear)
                  yNear += chainSize;
                if (this->zz[bp] > zNear)
                  zNear += chainSize;

                // Iterate on all particles in the bucket doing the estimate
                // to the near corner of the other buckets
                while (bp != -1) {
                  xdist = fabs(this->xx[bp] - xNear);
                  ydist = fabs(this->yy[bp] - yNear);
                  zdist = fabs(this->zz[bp] - zNear);
                  dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                  if (dist != 0) {
                    estimate[bp] -=
                      ((this->mass[bp] / dist) * bucketCount[wi][wj][wk]);
                  }
                  bp = bucketList[bp];
                }
              }
            }
          }
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Refine the estimate for the particle in the halo with window delta
// given the buckets in the chaining mesh, relative locations of particles
// in this halo, the index of this halo, and the bucket it is in
// The newly refined estimate is updated.
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::refineAStarLevel_1(
                        ChainingMesh* haloChain,
                        int bi,
                        int bj,
                        int bk,
                        int* minActual,
                        int* maxActual,
                        int bp,
                        POSVEL_T* estimate,
                        POSVEL_T boundarySize)
{
  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;
  POSVEL_T xNear = 0.0;
  POSVEL_T yNear = 0.0;
  POSVEL_T zNear = 0.0;
  POSVEL_T minBound[DIMENSION], maxBound[DIMENSION];

  // Get chaining mesh information
  POSVEL_T chainSize = haloChain->getChainSize();
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  POSVEL_T* minRange = haloChain->getMinRange();

  // Going out window delta in all directions
  // Subtract the estimate from the current value
  // Add the new values
  first[0] = bi - 1;   last[0] = bi + 1;
  first[1] = bj - 1;   last[1] = bj + 1;
  first[2] = bk - 1;   last[2] = bk + 1;

  // Calculate the bounding box around the current bucket
  minBound[0] = minRange[0] + (bi * chainSize) - boundarySize;
  maxBound[0] = minRange[0] + ((bi + 1) * chainSize) + boundarySize;
  minBound[1] = minRange[1] + (bj * chainSize) - boundarySize;
  maxBound[1] = minRange[1] + ((bj + 1) * chainSize) + boundarySize;
  minBound[2] = minRange[2] + (bk * chainSize) - boundarySize;
  maxBound[2] = minRange[2] + ((bk + 1) * chainSize) + boundarySize;

  for (int dim = 0; dim < DIMENSION; dim++) {
    if (first[dim] < 0) {
      first[dim] = 0;
      minBound[dim] = 0.0;
    }
    if (last[dim] >= meshSize[dim]) {
      last[dim] = meshSize[dim] - 1;
      maxBound[dim] = meshSize[dim] * chainSize;
    }
  }

  for (wi = first[0]; wi <= last[0]; wi++) {
    for (wj = first[1]; wj <= last[1]; wj++) {
      for (wk = first[2]; wk <= last[2]; wk++) {

        // If bucket has particles, and is not within the region which
        // calculates actual neighbor values (because if it is, it would
        // have already calculated actuals for this bucket) and if it is
        // not this bucket which already had the n^2 algorithm run
        if ((bucketCount[wi][wj][wk] > 0) &&
            ((wi > maxActual[0] || wi < minActual[0]) ||
             (wj > maxActual[1] || wj < minActual[1]) ||
             (wk > maxActual[2] || wk < minActual[2])) &&
            (wi != bi || wj != bj || wk != bk)) {


          // What is the nearest point between buckets
          if (wi < bi)  xNear = minBound[0];
          if (wi == bi) xNear = (minBound[0] + maxBound[0]) / 2.0;
          if (wi > bi)  xNear = maxBound[0];
          if (wj < bj)  yNear = minBound[1];
          if (wj == bj) yNear = (minBound[1] + maxBound[1]) / 2.0;
          if (wj > bj)  yNear = maxBound[1];
          if (wk < bk)  zNear = minBound[2];
          if (wk == bk) zNear = (minBound[2] + maxBound[2]) / 2.0;
          if (wk > bk)  zNear = maxBound[2];

          wp = buckets[wi][wj][wk];
          int estimatedParticleCount = 0;
          while (wp != -1) {

            // If inside the boundary around the bucket ignore because
            // actual potential was already calculated in initialPhase
            if (
              (this->xx[wp] <= minBound[0] || this->xx[wp] >= maxBound[0]) ||
              (this->yy[wp] <= minBound[1] || this->yy[wp] >= maxBound[1]) ||
              (this->zz[wp] <= minBound[2] || this->zz[wp] >= maxBound[2])) {

              // Count to create estimated potential which is added
              estimatedParticleCount++;

              // Calculate actual potential
              xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
              ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
              zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
              dist = sqrt(xdist*xdist + ydist*ydist + zdist*zdist);
              if (dist != 0.0) {
                estimate[bp] -= (this->mass[wp] / dist);
              }
            }
            wp = bucketList[wp];
          }

          // Find nearest corner or location to this bucket
          // Calculate estimated value for the part of the bucket
          xdist = (POSVEL_T)fabs(this->xx[bp] - xNear);
          ydist = (POSVEL_T)fabs(this->yy[bp] - yNear);
          zdist = (POSVEL_T)fabs(this->zz[bp] - zNear);
          dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
          if (dist != 0) {
            estimate[bp] += ((this->mass[bp] / dist) * estimatedParticleCount);
          }
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Refine the estimate for the particle in the halo with window delta
// given the buckets in the chaining mesh, relative locations of particles
// in this halo, the index of this halo, and the bucket it is in
// The newly refined estimate is updated.
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::refineAStarLevel_N(
                        ChainingMesh* haloChain,
                        int bi,
                        int bj,
                        int bk,
                        int bp,
                        POSVEL_T* estimate,
                        int winDelta)
{
  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  int oldDelta = winDelta - 1;
  POSVEL_T xdist, ydist, zdist, dist;
  POSVEL_T xNear, yNear, zNear;

  // Get chaining mesh information
  POSVEL_T chainSize = haloChain->getChainSize();
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  POSVEL_T* minRange = haloChain->getMinRange();

  // Going out window delta in all directions
  // Subtract the estimate from the current value
  // Add the new values
  first[0] = bi - winDelta;   last[0] = bi + winDelta;
  first[1] = bj - winDelta;   last[1] = bj + winDelta;
  first[2] = bk - winDelta;   last[2] = bk + winDelta;
  for (int dim = 0; dim < DIMENSION; dim++) {
    if (first[dim] < 0)
      first[dim] = 0;
    if (last[dim] >= meshSize[dim])
      last[dim] = meshSize[dim] - 1;
  }

  // Walk the new delta window
  // Exclude buckets which already contributed actual values
  // For other buckets add the estimate and subtract the actual
  for (wi = first[0]; wi <= last[0]; wi++) {
    for (wj = first[1]; wj <= last[1]; wj++) {
      for (wk = first[2]; wk <= last[2]; wk++) {

        if ((wi < (bi - oldDelta) || wi > (bi + oldDelta) ||
             wj < (bj - oldDelta) || wj > (bj + oldDelta) ||
             wk < (bk - oldDelta) || wk > (bk + oldDelta)) &&
            (bucketCount[wi][wj][wk] > 0)) {

            // Nearest corner of the bucket to contribute new actuals
            xNear = minRange[0] + (wi * chainSize);
            yNear = minRange[1] + (wj * chainSize);
            zNear = minRange[2] + (wk * chainSize);
            if (this->xx[bp] > xNear) xNear += chainSize;
            if (this->yy[bp] > yNear) yNear += chainSize;
            if (this->zz[bp] > zNear) zNear += chainSize;

            // Distance of this particle to the corner gives estimate
            // which was subtracted in initialPhase and now is added back
            xdist = (POSVEL_T)fabs(this->xx[bp] - xNear);
            ydist = (POSVEL_T)fabs(this->yy[bp] - yNear);
            zdist = (POSVEL_T)fabs(this->zz[bp] - zNear);
            dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
            if (dist != 0) {
              estimate[bp] +=
                ((this->mass[bp] / dist) * bucketCount[wi][wj][wk]);
            }

            // Subtract actual values from the new bucket to this particle
            wp = buckets[wi][wj][wk];
            while (wp != -1) {
              xdist = fabs(this->xx[bp] - this->xx[wp]);
              ydist = fabs(this->yy[bp] - this->yy[wp]);
              zdist = fabs(this->zz[bp] - this->zz[wp]);
              dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
              if (dist != 0) {
                estimate[bp] -= (this->mass[wp] / dist);
              }
              wp = bucketList[wp];
            }
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Build a chaining mesh from the particles of a single halo
// Used to find most connected and most bound particles for halo center
// Space is allocated for locations of the halo and for a mapping of
// the index within a halo to the index of the particle within the processor
//
/////////////////////////////////////////////////////////////////////////

ChainingMesh* HaloCenterFinder::buildChainingMesh(POSVEL_T chainSize)
{
  // Find the bounding box of this halo
  POSVEL_T* minLoc = new POSVEL_T[DIMENSION];
  POSVEL_T* maxLoc = new POSVEL_T[DIMENSION];
  minLoc[0] = maxLoc[0] = this->xx[0];
  minLoc[1] = maxLoc[1] = this->yy[0];
  minLoc[2] = maxLoc[2] = this->zz[0];

  // Transfer the locations for this halo into separate vectors
  for (int p = 0; p < this->particleCount; p++) {

    if (minLoc[0] > this->xx[p]) minLoc[0] = this->xx[p];
    if (maxLoc[0] < this->xx[p]) maxLoc[0] = this->xx[p];
    if (minLoc[1] > this->yy[p]) minLoc[1] = this->yy[p];
    if (maxLoc[1] < this->yy[p]) maxLoc[1] = this->yy[p];
    if (minLoc[2] > this->zz[p]) minLoc[2] = this->zz[p];
    if (maxLoc[2] < this->zz[p]) maxLoc[2] = this->zz[p];
  }

  // Want chaining mesh greater than 2 in any dimension
  bool tooSmall = true;
  while (tooSmall == true) {
    tooSmall = false;
    for (int dim = 0; dim < DIMENSION; dim++) {
      if (((maxLoc[dim] - minLoc[dim]) / chainSize) < 3.0)
        tooSmall = true;
    }
    if (tooSmall == true) {
      chainSize /= 2.0;
    }
  }

  // Build the chaining mesh
  ChainingMesh* haloChain = new ChainingMesh(minLoc, maxLoc, chainSize,
                        this->particleCount,
                        this->xx, this->yy, this->zz);
  delete [] minLoc;
  delete [] maxLoc;

  return haloChain;
}
