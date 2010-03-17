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
#include <set>
#include <math.h>

#include "Partition.h"
#include "FOFHaloProperties.h"
#ifndef USE_VTK_COSMO
#include "Timings.h"
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// FOFHaloProperties uses the results of the CosmoHaloFinder to locate the
// particle within every halo in order to calculate properties on halos
//
/////////////////////////////////////////////////////////////////////////

FOFHaloProperties::FOFHaloProperties()
{
  // Get the number of processors and rank of this processor
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();
}

FOFHaloProperties::~FOFHaloProperties()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Set linked list structure which will locate all particles in a halo
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::setHalos(
                        int numberHalos,
                        int* haloStartIndex,
                        int* haloParticleCount,
                        int* nextParticleIndex)
{
  this->numberOfHalos = numberHalos;
  this->halos = haloStartIndex;
  this->haloCount = haloParticleCount;
  this->haloList = nextParticleIndex;
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for the halo center finder
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::setParameters(
                        const string& outName,
                        POSVEL_T rL,
                        POSVEL_T deadSz,
                        POSVEL_T pMass,
                        POSVEL_T pDist)
{
  this->outFile = outName;

  // Halo finder parameters
  this->boxSize = rL;
  this->deadSize = deadSz;
  this->particleMass = pMass;
  this->bb = pDist;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::setParticles(
                        vector<POSVEL_T>* xLoc,
                        vector<POSVEL_T>* yLoc,
                        vector<POSVEL_T>* zLoc,
                        vector<POSVEL_T>* xVel,
                        vector<POSVEL_T>* yVel,
                        vector<POSVEL_T>* zVel,
                        vector<POTENTIAL_T>* potential,
                        vector<ID_T>* id,
                        vector<MASK_T>* maskData,
                        vector<STATUS_T>* state)
{
  this->particleCount = (long)xLoc->size();

  // Extract the contiguous data block from a vector pointer
  this->xx = &(*xLoc)[0];
  this->yy = &(*yLoc)[0];
  this->zz = &(*zLoc)[0];
  this->vx = &(*xVel)[0];
  this->vy = &(*yVel)[0];
  this->vz = &(*zVel)[0];
  this->pot = &(*potential)[0];
  this->tag = &(*id)[0];
  this->mask = &(*maskData)[0];
  this->status = &(*state)[0];
}

/////////////////////////////////////////////////////////////////////////
//
// Find the index of the particle at the center of every FOF halo which is the
// particle with the minimum value in the potential array.
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCenterMinimumPotential(vector<int>* haloCenter)
{
  for (int halo = 0; halo < this->numberOfHalos; halo++) {
                        
    // First particle in halo
    int p = this->halos[halo];
    POTENTIAL_T minPotential = this->pot[p];
    int centerIndex = p;
  
    // Next particle
    p = this->haloList[p];
  
    // Search for minimum
    while (p != -1) {
      if (minPotential > this->pot[p]) {
        minPotential = this->pot[p];
        centerIndex = p;
      }
      p = this->haloList[p];
    }

    // Save the minimum potential index for this halo
    (*haloCenter).push_back(centerIndex);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Find the index of the most bound particle which is the particle
// closest to every other particle in the halo.
// Use the N^2/2 algorithm for small halos
// Use the A* refinement algorithm for large halos
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCenterMBP(vector<int>* haloCenter)
{
  int smallHalo = 0;
  int largeHalo = 0;
  int centerIndex;
  POTENTIAL_T minPotential;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    if (this->haloCount[halo] < 5000) {
#ifndef USE_VTK_COSMO
      static Timings::TimerRef stimer = Timings::getTimer("N2 MBP");
      Timings::startTimer(stimer);
#endif
      smallHalo++;
      centerIndex = mostBoundParticleN2(halo, &minPotential);
      (*haloCenter).push_back(centerIndex);
#ifndef USE_VTK_COSMO
      Timings::stopTimer(stimer);
#endif
    }

     else {
#ifndef USE_VTK_COSMO
      static Timings::TimerRef ltimer = Timings::getTimer("ASTAR MBP");
      Timings::startTimer(ltimer);
#endif
      largeHalo++;
      centerIndex = mostBoundParticleAStar(halo);
      (*haloCenter).push_back(centerIndex);
#ifndef USE_VTK_COSMO
      Timings::stopTimer(ltimer);
#endif
    }
  }

#ifndef USE_VTK_COSMO
  cout << "MBP Rank " << this->myProc 
       << " #small = " << smallHalo
       << " #large = " << largeHalo << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Find the index of the most connected particle which is the particle
// with the most friends (most particles within halo interparticle distance)
// Use the mostConnectedParticle() algorithm
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCenterMCP(vector<int>* haloCenter)
{
  int smallHalo = 0;
  int largeHalo = 0;
  int centerIndex;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {

    if (this->haloCount[halo] < 10000) {
#ifndef USE_VTK_COSMO
      static Timings::TimerRef smtimer = Timings::getTimer("N2 MCP");
      Timings::startTimer(smtimer);
#endif
      smallHalo++;
      centerIndex = mostConnectedParticleN2(halo);
#ifndef USE_VTK_COSMO
      Timings::stopTimer(smtimer);
#endif
    }

    else {
#ifndef USE_VTK_COSMO
      static Timings::TimerRef latimer = Timings::getTimer("ChainMesh MCP");
      Timings::startTimer(latimer);
#endif
      largeHalo++;
      centerIndex = mostConnectedParticleChainMesh(halo);
#ifndef USE_VTK_COSMO
      Timings::stopTimer(latimer);
#endif
    }
    (*haloCenter).push_back(centerIndex);
  }

#ifndef USE_VTK_COSMO
  cout << "MCP Rank " << this->myProc 
       << " #small = " << smallHalo
       << " #large = " << largeHalo << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the mass of every FOF halo
//
// m_FOF = m_P * n_FOF
//    m_FOF is the mass of an FOF halo
//    n_FOF is the number of particles in the halo
//    m_P is (Omega_m * rho_c * L^3) / N_p
//       Omega_m is ratio of mass density to critical density
//       rho_c is 2.7755E11
//       L is length of one side of the simulation box
//       N_p is total number of particles in the simulation (n_p^3)
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloMass(
                        vector<POSVEL_T>* haloMass)
{
  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    POSVEL_T mass = this->particleMass * this->haloCount[halo];
    (*haloMass).push_back(mass);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the average position of particles of every FOF halo
//
// x_FOF = ((Sum i=1 to n_FOF) x_i) / n_FOF
//    x_FOF is the average position vector
//    n_FOF is the number of particles in the halo
//    x_i is the position vector of particle i
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFPosition(
                        vector<POSVEL_T>* xMeanPos,
                        vector<POSVEL_T>* yMeanPos,
                        vector<POSVEL_T>* zMeanPos)
{
  POSVEL_T xMean, yMean, zMean;
  double xKahan, yKahan, zKahan;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    xKahan = KahanSummation(halo, this->xx);
    yKahan = KahanSummation(halo, this->yy);
    zKahan = KahanSummation(halo, this->zz);

    xMean = (POSVEL_T) (xKahan / this->haloCount[halo]);
    yMean = (POSVEL_T) (yKahan / this->haloCount[halo]);
    zMean = (POSVEL_T) (zKahan / this->haloCount[halo]);

    (*xMeanPos).push_back(xMean);
    (*yMeanPos).push_back(yMean);
    (*zMeanPos).push_back(zMean);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the average velocity of particles of every FOF halo
//
// v_FOF = ((Sum i=1 to n_FOF) v_i) / n_FOF
//    v_FOF is the average velocity vector
//    n_FOF is the number of particles in the halo
//    v_i is the velocity vector of particle i
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFVelocity(
                        vector<POSVEL_T>* xMeanVel,
                        vector<POSVEL_T>* yMeanVel,
                        vector<POSVEL_T>* zMeanVel)
{
  POSVEL_T xMean, yMean, zMean;
  double xKahan, yKahan, zKahan;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    xKahan = KahanSummation(halo, this->vx);
    yKahan = KahanSummation(halo, this->vy);
    zKahan = KahanSummation(halo, this->vz);

    xMean = (POSVEL_T) (xKahan / this->haloCount[halo]);
    yMean = (POSVEL_T) (yKahan / this->haloCount[halo]);
    zMean = (POSVEL_T) (zKahan / this->haloCount[halo]);

    (*xMeanVel).push_back(xMean);
    (*yMeanVel).push_back(yMean);
    (*zMeanVel).push_back(zMean);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the velocity dispersion of every FOF halo
//
// o_FOF = sqrt((avg_part_vel_dot_prod - dot_prod_halo_vel) / 3)
//    avg_part_vel_dot_prod = ((Sum i=1 to n_FOF) v_i dot v_i) / n_FOF
//       n_FOF is the number of particles in the halo
//       v_i is the velocity vector of particle i
//    dot_prod_halo_vel = v_FOF dot v_FOF
//       v_FOF is the average velocity vector of all particles in the halo
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFVelocityDispersion(
                        vector<POSVEL_T>* xAvgVel,
                        vector<POSVEL_T>* yAvgVel,
                        vector<POSVEL_T>* zAvgVel,
                        vector<POSVEL_T>* velDisp)
{
  for (int halo = 0; halo < this->numberOfHalos; halo++) {

    // First particle in the halo
    int p = this->halos[halo];
    POSVEL_T particleDot = 0.0;

    // Iterate over all particles in the halo collecting dot products
    while (p != -1) {
      particleDot += dotProduct(this->vx[p], this->vy[p], this->vz[p]);
      p = this->haloList[p];
    }

    // Average of all the dot products
    particleDot /= this->haloCount[halo];

    // Dot product of the average velocity for the entire halo
    POSVEL_T haloDot = dotProduct((*xAvgVel)[halo], 
                                  (*yAvgVel)[halo], (*zAvgVel)[halo]);

    // Velocity dispersion
    POSVEL_T vDispersion = (POSVEL_T)sqrt((particleDot - haloDot) / 3.0);

    // Save onto supplied vector
    velDisp->push_back(vDispersion);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Dot product of a vector
//
/////////////////////////////////////////////////////////////////////////

POSVEL_T FOFHaloProperties::dotProduct(POSVEL_T x, POSVEL_T y, POSVEL_T z)
{
  POSVEL_T dotProd = x * x + y * y + z * z;
  return dotProd;
}

/////////////////////////////////////////////////////////////////////////
//                      
// Calculate the Kahan summation
// Reduces roundoff error in floating point arithmetic
//                      
/////////////////////////////////////////////////////////////////////////
                        
POSVEL_T FOFHaloProperties::KahanSummation(int halo, POSVEL_T* data)
{                       
  POSVEL_T dataSum, dataRem, v, w;
                        
  // First particle in halo and first step in Kahan summation
  int p = this->halos[halo];
  dataSum = data[p];
  dataRem = 0.0;
  
  // Next particle
  p = this->haloList[p];
  
  // Remaining steps in Kahan summation
  while (p != -1) {
    v = data[p] - dataRem;
    w = dataSum + v;
    dataRem = (w - dataSum) - v;
    dataSum = w;

    p = this->haloList[p];
  }
  return dataSum;
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the incremental mean using Kahan summation
//
/////////////////////////////////////////////////////////////////////////

POSVEL_T FOFHaloProperties::incrementalMean(int halo, POSVEL_T* data)
{
  double dataMean, dataRem, diff, value, v, w;

  // First particle in halo and first step in incremental mean
  int p = this->halos[halo];
  dataMean = data[p];
  dataRem = 0.0;
  int count = 1;

  // Next particle
  p = this->haloList[p];
  count++;

  // Remaining steps in incremental mean
  while (p != -1) {
    diff = data[p] - dataMean;
    value = diff / count;
    v = value - dataRem;
    w = dataMean + v;
    dataRem = (w - dataMean) - v;
    dataMean = w;

    p = this->haloList[p];
    count++;
  }
  return (POSVEL_T) dataMean;
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

int FOFHaloProperties::mostConnectedParticleN2(int halo)
{       
  // Arrange in an upper triangular grid of friend counts
  // friendCount will hold number of friends that a particle has
  // actualIndx will hold the particle index matching that halo index
  //
  int* friendCount = new int[this->haloCount[halo]];
  int* actualIndx = new int[this->haloCount[halo]];

  // Initialize friend count and set the actual index of the particle
  // so that at the end we can return a particle index not halo index
  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    friendCount[i] = 0;
    actualIndx[i] = p;
    p = this->haloList[p];
  }

  // Iterate on all particles in halo adding to count if friends of each other
  // Iterate in upper triangular fashion
  p = this->halos[halo];
  int indx1 = 0;
  int indx2 = 1;

  while (p != -1) {

    // Get halo particle after the current one
    int q = this->haloList[p];
    indx2 = indx1 + 1;
    while (q != -1) {

      // Calculate distance betweent the two
      POSVEL_T xdist = fabs(this->xx[p] - this->xx[q]);
      POSVEL_T ydist = fabs(this->yy[p] - this->yy[q]);
      POSVEL_T zdist = fabs(this->zz[p] - this->zz[q]);
      
      if ((xdist < this->bb) && (ydist < this->bb) && (zdist < this->bb)) {
        POSVEL_T dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
        if (dist < this->bb) {
          friendCount[indx1]++;
          friendCount[indx2]++;
        }
      }
      // Next inner particle
      q = this->haloList[q];
      indx2++;
    }
    // Next outer particle
    p = this->haloList[p];
    indx1++;
  }

  // Particle with the most friends
  int maxFriends = 0;
  int result = this->halos[halo];

  for (int i = 0; i < this->haloCount[halo]; i++) {
    if (friendCount[i] > maxFriends) {
      maxFriends = friendCount[i];
      result = actualIndx[i];
    }
  }

  delete [] friendCount;
  delete [] actualIndx;
  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Most connected particle using a chaining mesh of particles in one FOF halo
// Build chaining mesh with a grid size such that all friends will be in
// adjacent mesh grids.
//
/////////////////////////////////////////////////////////////////////////

int FOFHaloProperties::mostConnectedParticleChainMesh(int halo)
{
  // Save the actual particle tag corresponding the particle index within halo
  int* actualIndx = new int[this->haloCount[halo]];
  POSVEL_T* xLocHalo = new POSVEL_T[this->haloCount[halo]];
  POSVEL_T* yLocHalo = new POSVEL_T[this->haloCount[halo]];
  POSVEL_T* zLocHalo = new POSVEL_T[this->haloCount[halo]];

  // Build the chaining mesh
  int chainFactor = 5;
  POSVEL_T chainSize = this->bb / chainFactor;
  ChainingMesh* haloChain = buildChainingMesh(halo, chainSize,
                              xLocHalo, yLocHalo, zLocHalo, actualIndx);

  // Save the number of friends for each particle in the halo
  int* friendCount = new int[this->haloCount[halo]];
  for (int i = 0; i < this->haloCount[halo]; i++)
    friendCount[i] = 0;

  // Get chaining mesh information
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();

  // Walk every bucket in the chaining mesh, processing all particles in bucket
  // Examine particles in a walking window around the current bucket
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;

  for (int bi = 0; bi < meshSize[0]; bi++) {
    for (int bj = 0; bj < meshSize[1]; bj++) {
      for (int bk = 0; bk < meshSize[2]; bk++) {
   
        // Set the walking window around this bucket
        for (int dim = 0; dim < DIMENSION; dim++) {
          first[dim] = bi - chainFactor;
          last[dim] = bi + chainFactor;
          if (first[dim] < 0)
            first[dim] = 0;
          if (last[dim] >= meshSize[dim])
            last[dim] = meshSize[dim] - 1;
        }

        // First particle in the bucket being processed
        int bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // For the current particle in the current bucket
          // compare it against all particles in the walking window buckets
          for (int wi = first[0]; wi <= last[0]; wi++) {
            for (int wj = first[1]; wj <= last[1]; wj++) {
              for (int wk = first[2]; wk <= last[2]; wk++) {
    
                // Iterate on all particles in this bucket
                int wp = buckets[wi][wj][wk];
                while (wp != -1) {
    
                  // Calculate distance between the two
                  xdist = fabs(xLocHalo[bp] - xLocHalo[wp]);
                  ydist = fabs(yLocHalo[bp] - yLocHalo[wp]);
                  zdist = fabs(zLocHalo[bp] - zLocHalo[wp]);
    
                  if ((xdist < this->bb) && 
                      (ydist < this->bb) && 
                      (zdist < this->bb)) {
                        dist = 
                          sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                        if (dist < this->bb)
                          friendCount[bp]++;
                  }
                  wp = bucketList[wp];
                }
              }
            }
          }
          bp = bucketList[bp];
        }
      }
    }
  }
  // Particle with the most friends
  int maxFriends = 0;
  int result = this->halos[halo];

  for (int i = 0; i < this->haloCount[halo]; i++) {
    if (friendCount[i] > maxFriends) {
      maxFriends = friendCount[i];
      result = actualIndx[i];
    }
  }

  delete [] friendCount;
  delete [] actualIndx;
  delete [] xLocHalo;
  delete [] yLocHalo;
  delete [] zLocHalo;
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

int FOFHaloProperties::mostBoundParticleN2(int halo, POTENTIAL_T* minPotential)
{
  // Arrange in an upper triangular grid to save computation
  POTENTIAL_T* lpot = new POTENTIAL_T[this->haloCount[halo]];
  int* actualIndx = new int[this->haloCount[halo]];

  // Initialize potential and set the actual index of the particle
  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    lpot[i] = 0.0;
    actualIndx[i] = p;
    p = this->haloList[p];
  }

  // First particle in halo to calculate minimum potential on
  p = this->halos[halo];
  int indx1 = 0;
  int indx2 = 1;

  while (p != -1 && indx1 < this->haloCount[halo]) {

    // Next particle in halo in minimum potential loop
    int q = this->haloList[p];
    indx2 = indx1 + 1;

    while (q != -1) {

      POSVEL_T xdist = (POSVEL_T)fabs(this->xx[p] - this->xx[q]);
      POSVEL_T ydist = (POSVEL_T)fabs(this->yy[p] - this->yy[q]);
      POSVEL_T zdist = (POSVEL_T)fabs(this->zz[p] - this->zz[q]);

      POSVEL_T r = sqrt((xdist * xdist) + (ydist * ydist) + (zdist * zdist));
      POSVEL_T value = 1.0 / r;
      if (r != 0.0) {
        lpot[indx1] = (POTENTIAL_T)(lpot[indx1] - value);
        lpot[indx2] = (POTENTIAL_T)(lpot[indx2] - value);
      }
      // Next particle
      q = this->haloList[q];
      indx2++;
    }
    // Next particle
    p = this->haloList[p];
    indx1++;
  }

  *minPotential = MAX_FLOAT;
  int minIndex = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    if (lpot[i] < *minPotential) {
      *minPotential = lpot[i];
      minIndex = i;
    }
  } 
  int result = actualIndx[minIndex];
  delete [] lpot;
  delete [] actualIndx;

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

int FOFHaloProperties::mostBoundParticleAStar(int halo)
{
  // Build the chaining mesh, saving actual particle tag for result
  // This is needed because locations of particles in this halo are copied
  // into separate arrays for easy use in the rest of the algorithm
  int* actualIndx = new int[this->haloCount[halo]];
  POSVEL_T* xLocHalo = new POSVEL_T[this->haloCount[halo]];
  POSVEL_T* yLocHalo = new POSVEL_T[this->haloCount[halo]];
  POSVEL_T* zLocHalo = new POSVEL_T[this->haloCount[halo]];

  // Chaining mesh size is a factor of the interparticle halo distance
  POSVEL_T chainFactor = 1.0;
  POSVEL_T chainSize = this->bb * chainFactor;

  // Boundary around edges of a bucket for calculating estimate
  POSVEL_T boundaryFactor = 10.0f * chainFactor;
  POSVEL_T boundarySize = chainSize / boundaryFactor;

  // Actual values calculated for 26 neighbors in the center of a halo
  // Factor to decide what distance this is out from the center
  int eachSideFactor = 7;

  // Create the chaining mesh for this halo
  ChainingMesh* haloChain = buildChainingMesh(halo, chainSize,
                              xLocHalo, yLocHalo, zLocHalo, actualIndx);

  // Get chaining mesh information
  //int*** bucketCount = haloChain->getBucketCount();
  //int*** buckets = haloChain->getBuckets();
  //int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  //POSVEL_T* minRange = haloChain->getMinRange();

  // Bucket ID allows finding the bucket every particle is in
  int* bucketID = new int[this->haloCount[halo]];

  // Refinement level for a particle indicate how many buckets out have actual
  // values calculate rather than estimates
  int* refineLevel = new int[this->haloCount[halo]];

  // Minimum potential made up of actual part and estimated part
  POSVEL_T* estimate = new POSVEL_T[this->haloCount[halo]];
  for (int i = 0; i < this->haloCount[halo]; i++)
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

#ifndef USE_VTK_COSMO
  static Timings::TimerRef atimer = Timings::getTimer("A* PHASE 1 ACT");
  static Timings::TimerRef etimer = Timings::getTimer("A* PHASE 1 EST");
#endif

  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate actual for particles within individual bucket
  //
#ifndef USE_VTK_COSMO
  Timings::startTimer(atimer);
#endif
  aStarThisBucketPart(haloChain, 
                      xLocHalo, yLocHalo, zLocHalo, 
                      bucketID, estimate);

  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate actual values for immediate 26 neighbors for buckets in 
  // the center of the halo (refinement level = 1)
  //
  aStarActualNeighborPart(haloChain, minActual, maxActual,
                          xLocHalo, yLocHalo, zLocHalo,
                          refineLevel, estimate);
#ifndef USE_VTK_COSMO
  Timings::stopTimer(atimer);
#endif

  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate estimated values for immediate 26 neighbors for buckets on 
  // the edges of the halo (refinement level = 0)
  //
#ifndef USE_VTK_COSMO
  Timings::startTimer(etimer);
#endif
  aStarEstimatedNeighborPart(haloChain, minActual, maxActual,
                             xLocHalo, yLocHalo, zLocHalo,
                             refineLevel, estimate, boundarySize);

  //////////////////////////////////////////////////////////////////////////
  //
  // All buckets beyond the 27 nearest get an estimate based on count in
  // the bucket and the distance to the nearest point
  //
  aStarEstimatedPart(haloChain,
                     xLocHalo, yLocHalo, zLocHalo, estimate);
#ifndef USE_VTK_COSMO
  Timings::stopTimer(etimer);
#endif

  //////////////////////////////////////////////////////////////////////////
  //
  // Iterative phase to refine individual particles
  //
  POSVEL_T minDistance = estimate[0];
  int minParticleCur = 0;
  int winDelta = 1;

  // Find the current minimum particle after initial actual and estimates
  for (int i = 0; i < this->haloCount[halo]; i++) {
    if (estimate[i] < minDistance) {
      minDistance = estimate[i];
      minParticleCur = i;
    }
  }
  POSVEL_T minDistanceLast = minDistance;
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
#ifndef USE_VTK_COSMO
  static Timings::TimerRef rtimer = Timings::getTimer("A* REFINE");
  Timings::startTimer(rtimer);
#endif

  int pass = 1;
  while (winDelta <= maxDelta) {
    while (minParticleLast != minParticleCur) {

      // Refine the value for all particles in the same bucket as the minimum
      // Alter the minimum in the reference
      // Return the particle index that is the new minimum of that bucket
      while (winDelta > refineLevel[minParticleCur] &&
             estimate[minParticleCur] <= minDistanceLast) {
        pass++;
        refineLevel[minParticleCur]++;

        // Going from level 0 to level 1 is special because the 27 neighbors
        // are part actual and part estimated.  After that all refinements are
        // replacing an estimate with an actual
        if (refineLevel[minParticleCur] == 1) {
          refineAStarLevel_1(haloChain, bi, bj, bk, minActual, maxActual,
                             xLocHalo, yLocHalo, zLocHalo,
                             minParticleCur, estimate, 
                             boundarySize);
        } else {
          refineAStarLevel_N(haloChain, bi, bj, bk,
                             xLocHalo, yLocHalo, zLocHalo,
                             minParticleCur, estimate,
                             refineLevel[minParticleCur]);
        }
      }
      if (winDelta <= refineLevel[minParticleCur]) {
        minDistanceLast = estimate[minParticleCur];
        minParticleLast = minParticleCur;
      }

      // Find the current minimum particle
      minDistance = minDistanceLast;
      for (int i = 0; i < this->haloCount[halo]; i++) {
        if (estimate[i] <= minDistance) {
          minDistance = estimate[i];
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
#ifndef USE_VTK_COSMO
  Timings::stopTimer(rtimer);
#endif
  int result = actualIndx[minParticleCur];
//cout << endl << endl << " **** RESULT **** " << result <<  "  (" << xx[result] << "," << yy[result] << ","  << zz[result] << ")  count " << haloCount[halo] << "  number of passes " << pass << "   bucket " << bi << "," << bj << "," << bk << " meshSize " << meshSize[0] << ":" << meshSize[1] << ":" << meshSize[2] << " center " << minActual[0] << ":" << maxActual[0] << "  " << minActual[1] << ":" << maxActual[1] << "  " << minActual[2] << ":" << maxActual[2] << endl;

  delete [] estimate;
  delete [] bucketID;
  delete [] refineLevel;
  delete [] actualIndx;
  delete [] xLocHalo;
  delete [] yLocHalo;
  delete [] zLocHalo;
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

void FOFHaloProperties::aStarThisBucketPart(
                        ChainingMesh* haloChain,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
                        int* bucketID,
                        POSVEL_T* estimate)
{
  POSVEL_T xdist, ydist, zdist, dist;
  int /*bp, bp2,*/ bi, bj, bk;

  // Get chaining mesh information
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();

  // Calculate actual values for all particles in the same bucket
  // All pairs are calculated one time and stored twice
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        int bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // Remember the bucket that every particle is in
          bucketID[bp] = (bi * meshSize[1] * meshSize[2]) + 
                         (bj * meshSize[2]) + bk;

          int bp2 = bucketList[bp];
          while (bp2 != -1) {
            xdist = (POSVEL_T)fabs(xLocHalo[bp] - xLocHalo[bp2]);
            ydist = (POSVEL_T)fabs(yLocHalo[bp] - yLocHalo[bp2]);
            zdist = (POSVEL_T)fabs(zLocHalo[bp] - zLocHalo[bp2]);
            dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
            if (dist != 0.0) {
              POSVEL_T value = 1.0 / dist;
              estimate[bp] -= value;
              estimate[bp2] -= value;
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

void FOFHaloProperties::aStarActualNeighborPart(
                        ChainingMesh* haloChain,
                        int* minActual,
                        int* maxActual,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
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
            (bi < minActual[0] || bi > maxActual[0]) ||
            (bj < minActual[1] || bj > maxActual[1]) ||
            (bk < minActual[2] || bk > maxActual[2])) {

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
                      xdist = (POSVEL_T)fabs(xLocHalo[bp] - xLocHalo[wp]);
                      ydist = (POSVEL_T)fabs(yLocHalo[bp] - yLocHalo[wp]);
                      zdist = (POSVEL_T)fabs(zLocHalo[bp] - zLocHalo[wp]);
                      dist = sqrt((xdist*xdist)+(ydist*ydist)+(zdist*zdist));
                      if (dist != 0.0) {
                        POSVEL_T value = 1.0 / dist;
                        estimate[bp] -= value;
                        estimate[wp] -= value;
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
                  xdist = fabs(xLocHalo[bp] - xLocHalo[wp]);
                  ydist = fabs(yLocHalo[bp] - yLocHalo[wp]);
                  zdist = fabs(zLocHalo[bp] - zLocHalo[wp]);
                  dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                  if (dist != 0.0) {
                    POSVEL_T value = 1.0 / dist;
                    estimate[bp] -= value;
                    estimate[wp] -= value;
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
                xdist = (POSVEL_T)fabs(xLocHalo[bp] - xLocHalo[wp]);
                ydist = (POSVEL_T)fabs(yLocHalo[bp] - yLocHalo[wp]);
                zdist = (POSVEL_T)fabs(zLocHalo[bp] - zLocHalo[wp]);
                dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                if (dist != 0) {
                  POSVEL_T value = 1.0 / dist;
                  estimate[bp] -= value;
                  estimate[wp] -= value;
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
              xdist = (POSVEL_T)fabs(xLocHalo[bp] - xLocHalo[wp]);
              ydist = (POSVEL_T)fabs(yLocHalo[bp] - yLocHalo[wp]);
              zdist = (POSVEL_T)fabs(zLocHalo[bp] - zLocHalo[wp]);
              dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
              if (dist != 0.0) {
                POSVEL_T value = 1.0 / dist;
                estimate[bp] -= value;
                estimate[wp] -= value;
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

void FOFHaloProperties::aStarEstimatedNeighborPart(
                        ChainingMesh* haloChain, 
                        int* minActual,
                        int* maxActual,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
                        int* refineLevel,
                        POSVEL_T* estimate,
                        POSVEL_T boundarySize)
{   
  // Walking window extents and size
  int bp, bi, bj, bk;
  int /*wp,*/ wi, wj, wk;
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
            (bi < minActual[0] || bi > maxActual[0]) ||
            (bj < minActual[1] || bj > maxActual[1]) ||
            (bk < minActual[2] || bk > maxActual[2])) {

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

                    int wp = buckets[wi][wj][wk];
                    int estimatedParticleCount = 0;
                    while (wp != -1) {
                      if (xLocHalo[wp] > minBound[0] && 
                          xLocHalo[wp] < maxBound[0] &&
                          yLocHalo[wp] > minBound[1] && 
                          yLocHalo[wp] < maxBound[1] &&
                          zLocHalo[wp] > minBound[2] && 
                          zLocHalo[wp] < maxBound[2]) {

                        // Is the window particle within the boundary condition
                        // Calculate actual potential
                        xdist = (POSVEL_T)fabs(xLocHalo[bp] - xLocHalo[wp]);
                        ydist = (POSVEL_T)fabs(yLocHalo[bp] - yLocHalo[wp]);
                        zdist = (POSVEL_T)fabs(zLocHalo[bp] - zLocHalo[wp]);
                        dist = sqrt(xdist*xdist + ydist*ydist + zdist*zdist);
                        if (dist != 0.0) {
                          POSVEL_T value = 1.0 / dist;
                          estimate[bp] -= value;
                        }
                      } else {
                        // Count to create estimated potential
                        estimatedParticleCount++;
                      }
                      wp = bucketList[wp];
                    }

                    // Find nearest corner or location to this bucket
                    // Calculate estimated value for the part of the bucket
                    xdist = (POSVEL_T)fabs(xLocHalo[bp] - xNear);
                    ydist = (POSVEL_T)fabs(yLocHalo[bp] - yNear);
                    zdist = (POSVEL_T)fabs(zLocHalo[bp] - zNear);
                    dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                    if (dist != 0) {
                      POSVEL_T value = (1.0 / dist) * estimatedParticleCount;
                      estimate[bp] -= value;
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

void FOFHaloProperties::aStarEstimatedPart(
                        ChainingMesh* haloChain,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
                        POSVEL_T* estimate)
{
  // Walking window extents and size
  int bp, bi, bj, bk;
  /*int wi, wj, wk;*/
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

        for (int wi = 0; wi < meshSize[0]; wi++) {
          for (int wj = 0; wj < meshSize[1]; wj++) {
            for (int wk = 0; wk < meshSize[2]; wk++) {
                
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
                if (xLocHalo[bp] > xNear)
                  xNear += chainSize;
                if (yLocHalo[bp] > yNear)
                  yNear += chainSize;
                if (zLocHalo[bp] > zNear)
                  zNear += chainSize;
                  
                // Iterate on all particles in the bucket doing the estimate
                // to the near corner of the other buckets
                while (bp != -1) {
                  xdist = fabs(xLocHalo[bp] - xNear);
                  ydist = fabs(yLocHalo[bp] - yNear);
                  zdist = fabs(zLocHalo[bp] - zNear);
                  dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                  if (dist != 0) {
                    POSVEL_T value = (1.0 / dist) * bucketCount[wi][wj][wk];
                    estimate[bp] -= value;
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
                
void FOFHaloProperties::refineAStarLevel_1(
                        ChainingMesh* haloChain, 
                        int bi,
                        int bj,
                        int bk,
                        int* minActual,
                        int* maxActual,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
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
              (xLocHalo[wp] <= minBound[0] || xLocHalo[wp] >= maxBound[0]) ||
              (yLocHalo[wp] <= minBound[1] || yLocHalo[wp] >= maxBound[1]) ||
              (zLocHalo[wp] <= minBound[2] || zLocHalo[wp] >= maxBound[2])) {

              // Count to create estimated potential which is added
              estimatedParticleCount++;

              // Calculate actual potential
              xdist = (POSVEL_T)fabs(xLocHalo[bp] - xLocHalo[wp]);
              ydist = (POSVEL_T)fabs(yLocHalo[bp] - yLocHalo[wp]);
              zdist = (POSVEL_T)fabs(zLocHalo[bp] - zLocHalo[wp]);
              dist = sqrt(xdist*xdist + ydist*ydist + zdist*zdist);
              if (dist != 0.0) {
                POSVEL_T value = 1.0 / dist;
                estimate[bp] -= value;
              }
            }
            wp = bucketList[wp];
          }

          // Find nearest corner or location to this bucket
          // Calculate estimated value for the part of the bucket
          xdist = (POSVEL_T)fabs(xLocHalo[bp] - xNear);
          ydist = (POSVEL_T)fabs(yLocHalo[bp] - yNear);
          zdist = (POSVEL_T)fabs(zLocHalo[bp] - zNear);
          dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
          if (dist != 0) {
            POSVEL_T value = (1.0 / dist) * estimatedParticleCount;
            estimate[bp] += value;
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

void FOFHaloProperties::refineAStarLevel_N(
                        ChainingMesh* haloChain,
                        int bi,
                        int bj,
                        int bk,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
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
            if (xLocHalo[bp] > xNear) xNear += chainSize;
            if (yLocHalo[bp] > yNear) yNear += chainSize;
            if (zLocHalo[bp] > zNear) zNear += chainSize;
                  
            // Distance of this particle to the corner gives estimate
            // which was subtracted in initialPhase and now is added back
            xdist = (POSVEL_T)fabs(xLocHalo[bp] - xNear);
            ydist = (POSVEL_T)fabs(yLocHalo[bp] - yNear);
            zdist = (POSVEL_T)fabs(zLocHalo[bp] - zNear);
            dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
            if (dist != 0) {
              POSVEL_T value = (1.0 / dist) * bucketCount[wi][wj][wk];
              estimate[bp] += value;
            }

            // Subtract actual values from the new bucket to this particle
            wp = buckets[wi][wj][wk];
            while (wp != -1) {
              xdist = fabs(xLocHalo[bp] - xLocHalo[wp]);
              ydist = fabs(yLocHalo[bp] - yLocHalo[wp]);
              zdist = fabs(zLocHalo[bp] - zLocHalo[wp]);
              dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
              if (dist != 0) {
                POSVEL_T value = 1.0 / dist;
                estimate[bp] -= value;
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

ChainingMesh* FOFHaloProperties::buildChainingMesh(
                        int halo, 
                        POSVEL_T chainSize,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
                        int* actualIndx)
{
  // Find the bounding box of this halo
  POSVEL_T* minLoc = new POSVEL_T[DIMENSION];
  POSVEL_T* maxLoc = new POSVEL_T[DIMENSION];

  // Initialize by finding bounding box, moving locations, setting friends
  // to zero and setting the actual particle tag corresponding to halo index
  int p = this->halos[halo];
  minLoc[0] = maxLoc[0] = this->xx[p];
  minLoc[1] = maxLoc[1] = this->yy[p];
  minLoc[2] = maxLoc[2] = this->zz[p];

  // Transfer the locations for this halo into separate vectors
  for (int i = 0; i < this->haloCount[halo]; i++) {

    xLocHalo[i] = this->xx[p];
    yLocHalo[i] = this->yy[p];
    zLocHalo[i] = this->zz[p];

    if (minLoc[0] > this->xx[p]) minLoc[0] = this->xx[p];
    if (maxLoc[0] < this->xx[p]) maxLoc[0] = this->xx[p];
    if (minLoc[1] > this->yy[p]) minLoc[1] = this->yy[p];
    if (maxLoc[1] < this->yy[p]) maxLoc[1] = this->yy[p];
    if (minLoc[2] > this->zz[p]) minLoc[2] = this->zz[p];
    if (maxLoc[2] < this->zz[p]) maxLoc[2] = this->zz[p];

    actualIndx[i] = p;
    p = this->haloList[p];
  }

  // Build the chaining mesh
  ChainingMesh* haloChain = new ChainingMesh(minLoc, maxLoc, chainSize,
                        this->haloCount[halo], 
                        xLocHalo, yLocHalo, zLocHalo);

  delete [] minLoc;
  delete [] maxLoc;

  return haloChain;
}

#ifndef USE_VTK_COSMO
/////////////////////////////////////////////////////////////////////////
//
// Write the halo catalog file
//
// Output one entry per halo 
// Location (xx,yy,zz) is the location of particle closest to centroid
// Eventually this needs to be the particle with the minimum potential
// Velocity (vx,vy,vz) is the average velocity of all halo particles
// Mass is the #particles in the halo * mass of one particle
// Tag is the unique id of the halo
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCatalog(
                        vector<int>* haloCenter,
                        vector<POSVEL_T>* xMeanVel,
                        vector<POSVEL_T>* yMeanVel,
                        vector<POSVEL_T>* zMeanVel)
{
  // Compose ascii and .cosmo binary file names
  ostringstream aname, cname;
  if (this->numProc == 1) {
    aname << this->outFile << ".halocatalog.ascii";
    cname << this->outFile << ".halocatalog.cosmo";
  } else {
    aname << this->outFile << ".halocatalog.ascii." << myProc;
    cname << this->outFile << ".halocatalog.cosmo." << myProc;
  }
  ofstream aStream(aname.str().c_str(), ios::out);
  ofstream cStream(cname.str().c_str(), ios::out|ios::binary);

  char str[1024];
  float fBlock[COSMO_FLOAT];
  int iBlock[COSMO_INT];

  for (int halo = 0; halo < this->numberOfHalos; halo++) {

    int centerIndex = (*haloCenter)[halo];
    int haloTag = this->tag[this->halos[halo]];
    POSVEL_T haloMass = this->haloCount[halo] * particleMass;

    // Write ascii
    sprintf(str, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12d\n", 
      this->xx[centerIndex],
      (*xMeanVel)[halo],
      this->yy[centerIndex],
      (*yMeanVel)[halo],
      this->zz[centerIndex],
      (*zMeanVel)[halo],
      haloMass, haloTag);
      aStream << str;

    fBlock[0] = this->xx[centerIndex];
    fBlock[1] = (*xMeanVel)[halo];
    fBlock[2] = this->yy[centerIndex];
    fBlock[3] = (*yMeanVel)[halo];
    fBlock[4] = this->zz[centerIndex];
    fBlock[5] = (*zMeanVel)[halo];
    fBlock[6] = haloMass;
    cStream.write(reinterpret_cast<char*>(fBlock), COSMO_FLOAT * sizeof(float));

    iBlock[0] = haloTag;
    cStream.write(reinterpret_cast<char*>(iBlock), COSMO_INT * sizeof(int));
  }
}

/////////////////////////////////////////////////////////////////////////
//
// For each processor print the halo index and size for debugging
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::printHaloSizes(int minSize)
{
  for (int i = 0; i < this->numberOfHalos; i++)
    if (this->haloCount[i] > minSize)
      cout << "Rank " << Partition::getMyProc() 
           << " Halo " << i 
           << " size = " << this->haloCount[i] << endl;
}
 
/////////////////////////////////////////////////////////////////////////
//
// For the requested processor and halo index output locations for
// a scatter plot for debugging
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::printLocations(int halo)
{
  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    cout << "FOF INFO " << this->myProc << " " << halo
         << " INDEX " << p << " TAG " << this->tag[p] << " LOCATION " 
         << this->xx[p] << " " << this->yy[p] << " " << this->zz[p] << endl;
    p = this->haloList[p];
  }
}

/////////////////////////////////////////////////////////////////////////
//
// For the requested processor and halo index output bounding box
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::printBoundingBox(int halo)
{
  POSVEL_T minBox[DIMENSION], maxBox[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    minBox[dim] = this->boxSize;
    maxBox[dim] = 0.0;
  }

  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {

    if (minBox[0] > this->xx[p])
      minBox[0] = this->xx[p];
    if (maxBox[0] < this->xx[p])
      maxBox[0] = this->xx[p];

    if (minBox[1] > this->yy[p])
      minBox[1] = this->yy[p];
    if (maxBox[1] < this->yy[p])
      maxBox[1] = this->yy[p];

    if (minBox[2] > this->zz[p])
      minBox[2] = this->zz[p];
    if (maxBox[2] < this->zz[p])
      maxBox[2] = this->zz[p];

    p = this->haloList[p];
  }
  cout << "FOF BOUNDING BOX " << this->myProc << " " << halo << ": " 
         << minBox[0] << ":" << maxBox[0] << "  "
         << minBox[1] << ":" << maxBox[1] << "  "
         << minBox[2] << ":" << maxBox[2] << "  " << endl;
}
#endif
