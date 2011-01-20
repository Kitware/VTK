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
#include "SODHalo.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <vector>
#include <algorithm>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// SODHalo uses the results of the CosmoHaloFinder to locate the
// particle within every halo in order to calculate properties on halos
//
/////////////////////////////////////////////////////////////////////////

SODHalo::SODHalo()
{
  // Get the number of processors and rank of this processor
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();
  this->numberOfParticles = 0;

  this->binCount = 0;
  this->binRadius = 0;
  this->binMass = 0;
  this->binRho = 0;
  this->binRhoRatio = 0;
  this->binInfo = 0;

  this->avgRadius = 0;
  this->avgRadVelocity = 0;
  this->particleIndex = 0;
  this->particleRadius = 0;
}

SODHalo::~SODHalo()
{
  if (this->binCount) delete [] this->binCount;
  if (this->binRadius) delete [] this->binRadius;
  if (this->binMass) delete [] this->binMass;
  if (this->binRho) delete [] this->binRho;
  if (this->binRhoRatio) delete [] this->binRhoRatio;
  if (this->binInfo) delete [] this->binInfo;

  if (this->avgRadius) delete [] this->avgRadius;
  if (this->avgRadVelocity) delete [] this->avgRadVelocity;
  if (this->particleIndex) delete [] this->particleIndex;
  if (this->particleRadius) delete [] this->particleRadius;
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for the halo center finder
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::setParameters(
                        ChainingMesh* chainMesh,
                        int numBins,
                        POSVEL_T rL,
                        POSVEL_T np,
                        POSVEL_T rhoc,
                        POSVEL_T sodmass,
                        POSVEL_T densRatio,
                        POSVEL_T minFactor,
                        POSVEL_T maxFactor)
{
  // Get information from the chaining mesh
  this->chain = chainMesh;
  this->buckets = chain->getBuckets();
  this->bucketList = chain->getBucketList();

  // Halo finder parameters
  this->rSmooth = rL / np;
  this->rhoRatio = densRatio;
  this->cMinFactor = minFactor;
  this->cMaxFactor = maxFactor;
  this->RHOC = rhoc;
  this->SODMASS = sodmass;

  // Make the number of bins one larger so that all particles less than the
  // minimum radius can be collected into bin 0
  this->numberOfBins = numBins + 1;

  // Allocate memory based on bins
  this->binRadius = new POSVEL_T[this->numberOfBins];
  this->binRho = new double[this->numberOfBins];
  this->binRhoRatio = new double[this->numberOfBins];
  this->binCount = new int[this->numberOfBins];
  this->binMass = new double[this->numberOfBins];
  this->binInfo = new vector<RadiusID>[this->numberOfBins];

  this->avgRadius = new double[this->numberOfBins];
  this->avgRadVelocity = new double[this->numberOfBins];
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::setParticles(
                        vector<POSVEL_T>* xLoc,
                        vector<POSVEL_T>* yLoc,
                        vector<POSVEL_T>* zLoc,
                        vector<POSVEL_T>* xVel,
                        vector<POSVEL_T>* yVel,
                        vector<POSVEL_T>* zVel,
                        vector<POSVEL_T>* pmass,
                        vector<ID_T>* id)
{
  this->particleCount = (long)xLoc->size();

  // Extract the contiguous data block from a vector pointer
  this->xx = &(*xLoc)[0];
  this->yy = &(*yLoc)[0];
  this->zz = &(*zLoc)[0];
  this->vx = &(*xVel)[0];
  this->vy = &(*yVel)[0];
  this->vz = &(*zVel)[0];
  this->mass = &(*pmass)[0];
  this->tag = &(*id)[0];
}

/////////////////////////////////////////////////////////////////////////
//
// SOD (Spherically Over Dense) halos centered at FOF center of minimum size
//
// Initial estimate of characteristic radius
//    Choose a Delta which is the average density of a sphere around xcenter
//                   divided by the critical density of the universe
//                   Typically choose Delta = 200 for initial estimate
//    Delta = Mass(r_delta) / Volume(r_delta)
//          = m_delta / (4/3)(PI)(r_delta)^3
//    r_initial = cube_root(mass_FOF / 10^14)
//
// Mass profile
//    Choose r_max = c_max * r_initial
//       c_max is approximately 2
//    Collect all particles from the chaining mesh buckets such that they
//       fall within the r_max sphere
//    Choose r_min = c_min * r_smooth
//       c_min is approximately 1
//       r_smooth is boxSize / gridSize
//    Arrange bins logarithmically between r_min and r_max storing (r, mass)
//       radius of sphere and mass of particles within that sphere
//
// Improved estimate of characteristic radius
//    Using the (r_j, m_j) spheres find the two that surround r_200
//    Interpolate between the values of density to get a better r_200
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::createSODHalo(
                        int FOFhaloCount,
                        POSVEL_T centerXLocation,
                        POSVEL_T centerYLocation,
                        POSVEL_T centerZLocation,
                        POSVEL_T avgXVelocity,
                        POSVEL_T avgYVelocity,
                        POSVEL_T avgZVelocity,
                        POSVEL_T FOFhaloMass)
{
  this->fofCenterLocation[0] = centerXLocation;
  this->fofCenterLocation[1] = centerYLocation;
  this->fofCenterLocation[2] = centerZLocation;

  this->fofHaloVelocity[0] = avgXVelocity;
  this->fofHaloVelocity[1] = avgYVelocity;
  this->fofHaloVelocity[2] = avgZVelocity;

  this->fofHaloCount = FOFhaloCount;
  this->initRadius = (POSVEL_T)pow
  ((POSVEL_T)(FOFhaloMass / this->SODMASS), (POSVEL_T)(1.0 / 3.0));

  // Binning for concentric spheres over radius range
  this->minRadius = this->cMinFactor * this->rSmooth;
  this->maxRadius = this->cMaxFactor * this->initRadius;

  // Calculate logarithmic radial bins containing count, mass and RadiusID pairs
  calculateMassProfile();

#ifdef DEBUG
  for (int bin = 0; bin < this->numberOfBins; bin++) {
    double bmass = 0.0;
    if (binCount[bin] > 0)
      bmass = binMass[bin] / binCount[bin];

    cout << "Bin radius " << binRadius[bin]
         << " Avg Radius " << avgRadius[bin]
         << " Radial Velocity " << avgRadVelocity[bin]
         << " Avg Mass " << bmass
         << " Count " << binCount[bin] << endl;
  }
  cout << endl;
#endif

  // Calculate the characteristic radius for requested density ratio
  calculateCharacteristicRadius();

  if (this->charRadius > 0.0) {

    // Gather all particles less than the characteristic radius
    // Collect average velocity at the same time
    gatherSODParticles();

    // Calculate velocity dispersion
    calculateVelocityDispersion();
  }

#ifdef DEBUG
  cout << "Initial radius = " << this->initRadius << endl;
  cout << "Characteristic radius " << this->charRadius << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Divide the radius between the minimum and maximum radius into bins
// Iterate over all particles in the buckets, incrementing the count for
// a bin if the radius falls within the boundary.
// Return the bin pairs (radius, mass of particles within radius)
//
// Collect both the mass profile pairs for the number of bins and also
// store the distances for each particle and sort that array of distances
// Then we should be able to calculate r_200 and r_approximate_200
//
// Return radius[numBins], count[numBins], vector<POSVEL_T> distance sorted
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::calculateMassProfile()
{
  // If the max radius runs into the corner of data for this processor
  // adjust down so as to get a complete sphere
  POSVEL_T limit;
  for (int dim = 0; dim < DIMENSION; dim++) {
    limit = this->chain->getMaxMine(dim) - this->fofCenterLocation[dim];
    if (this->maxRadius > limit)
      this->maxRadius = limit;
    limit = this->fofCenterLocation[dim] - chain->getMinMine(dim);
    if (this->maxRadius > limit)
      this->maxRadius = limit;
  }

#ifndef USE_VTK_COSMO
  if (this->maxRadius < requiredMaxRadius) {
    cout << "Reset max radius from " << requiredMaxRadius
         << " to " << maxRadius << endl;
    cout << "Might need to make the dead size (overload) larger" << endl;
  }
#endif

  // Calculate the delta radius in log scale
  // Number of bins was increased by one for particles less than the min
  this->deltaRadius = (POSVEL_T) log10(this->maxRadius / this->minRadius) /
                                 (this->numberOfBins - 1);

  // Bin 0 is for all particles less than the minimum
  this->binRadius[0] = this->minRadius;
  for (int bin = 1; bin < this->numberOfBins; bin++) {
    this->binRadius[bin] = (POSVEL_T)pow
      ((POSVEL_T)10.0, (POSVEL_T)((this->deltaRadius * bin) *
      this->minRadius));
  }

  for (int bin = 0; bin < this->numberOfBins; bin++) {
    this->binCount[bin] = 0;
    this->binMass[bin] = 0.0;
    this->avgRadius[bin] = 0.0;
    this->avgRadVelocity[bin] = 0.0;
  }

  // Grid in the bucket grid containing the FOF center
  int centerIndex[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    centerIndex[dim] =
      (int) ((this->fofCenterLocation[dim] - chain->getMinMine(dim)) /
             chain->getChainSize());
  }

  // Number of grids to look at in each direction
  int gridOffset = (int) (this->maxRadius / chain->getChainSize()) + 1;

  // Range of grid positions to examine for this particle center
  int first[DIMENSION], last[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    first[dim] = centerIndex[dim] - gridOffset;
    last[dim] = centerIndex[dim] + gridOffset;
    if (first[dim] < 0)
      first[dim] = 0;
    if (last[dim] > chain->getMeshSize(dim))
      last[dim] = chain->getMeshSize(dim);
  }

  // Iterate over every possible grid and examine particles in the bucket
  // Count the number of particles in each of the logarithmic bins
  POSVEL_T location[DIMENSION];
  for (int i = first[0]; i <= last[0]; i++) {
    for (int j = first[1]; j <= last[1]; j++) {
      for (int k = first[2]; k <= last[2]; k++) {


        // Iterate on all particles in this bucket
        // Index of first particle in bucket
        int p = this->buckets[i][j][k];
        while (p != -1) {
          location[0] = this->xx[p];
          location[1] = this->yy[p];
          location[2] = this->zz[p];

          // Calculate distance between this particle and the center
          POSVEL_T diff[DIMENSION];
          for (int dim = 0; dim < DIMENSION; dim++)
            diff[dim] = location[dim] - this->fofCenterLocation[dim];

          POSVEL_T dist = sqrt((diff[0] * diff[0]) +
                               (diff[1] * diff[1]) +
                               (diff[2] * diff[2]));

          // If this particle is within the max radius
          if (dist < this->maxRadius) {

            // Calculate the unit vector for this particle
            POSVEL_T unit[DIMENSION];
            for (int dim = 0; dim < DIMENSION; dim++)
              {
              if (dist > 0.0)
                {
                unit[dim] = diff[dim] / dist;
                }
              else
                {
                unit[dim] = 0.0;
                }
              }

            // Calculate the relative velocity vector of particle wrt center
            POSVEL_T relVel[DIMENSION];
            relVel[0] = this->vx[p] - this->fofHaloVelocity[0];
            relVel[1] = this->vy[p] - this->fofHaloVelocity[1];
            relVel[2] = this->vz[p] - this->fofHaloVelocity[2];

            // Calculate the radial velocity
            POSVEL_T radVel = 0.0;
            for (int dim = 0; dim < DIMENSION; dim++)
              radVel += unit[dim] * relVel[dim];

            // Calculate the bin this particle goes in
            // Bin 0 contains all particles less than the min radius
            int bin = 0;
            if (dist > this->minRadius) {
              bin = (int) (floor(log10(dist/this->minRadius) /
                                    this->deltaRadius)) + 1;
            }
            this->binCount[bin]++;
            this->binMass[bin] += this->mass[p];
            this->avgRadius[bin] += dist;
            this->avgRadVelocity[bin] += radVel;

            // Store the actual radius and index of particle on this processor
            RadiusID pair;
            pair.radius = dist;
            pair.index = p;
            this->binInfo[bin].push_back(pair);
          }

          // Next particle in bucket
          p = this->bucketList[p];
        }
      }
    }
  }

  // Calculate the average radius per bin
  for (int bin = 0; bin < this->numberOfBins; bin++) {
    if (binCount[bin] > 0) {
      avgRadius[bin] /= binCount[bin];
      avgRadVelocity[bin] /= binCount[bin];
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Given the mass profile for an SOD halo calculate the
// characteristic radius matching the requested density
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::calculateCharacteristicRadius()
{
  // Calculate the mass, volume and density of every sphere bin
  // Bin 0 contains information on particles less than the minimum radius
  int totBinCount = this->binCount[0];
  double totBinMass = this->binMass[0];

  for (int bin = 1; bin < this->numberOfBins; bin++) {
    totBinCount += this->binCount[bin];
    totBinMass += this->binMass[bin];

    double r = (double) this->avgRadius[bin];
    double volume = ((4.0 * M_PI) / 3.0) * r * r * r;

    this->binRho[bin] = totBinMass / volume;
    this->binRhoRatio[bin] = this->binRho[bin] / this->RHOC;
#ifdef DEBUG
    cout << "Radius " << this->binRadius[bin]
         << " Avg Radius " << this->avgRadius[bin]
         << " Mass " << this->binMass[bin]
         << " Rho " << this->binRho[bin]
         << " Rho Ratio " << this->binRhoRatio[bin] << endl;
#endif
  }

  // Find the two bins that the density should be between
  // Interpolate the radius matching the requested density
  vector<int> possibleBins;
  for (int bin = 1; bin < (this->numberOfBins - 1); bin++) {
    if (this->binRhoRatio[bin] > RHO_RATIO &&
        this->binRhoRatio[bin+1] < RHO_RATIO)
      possibleBins.push_back(bin);
  }

  // Zero bins means a badly behaved SOD region
  // More than one bin means use the first
  if (possibleBins.size() < 1) {
    this->criticalBin = 0;
    this->charRadius = 0.0;
    return;
  }
  this->criticalBin = possibleBins[0] + 1;

  // Sort each bin's radius/id pair vector up to the critical bin
  for (int bin = 0; bin <= criticalBin; bin++)
    sort(this->binInfo[bin].begin(), this->binInfo[bin].end(), RadiusIDLT());

  // Accumulate mass for all bins lower than the critical bin
  double totParticleMass = 0.0;
  for (int bin = 0; bin < criticalBin; bin++)
    totParticleMass += this->binMass[bin];

  // Iterate over particles in the critical bin until we exceed critical density
  int i = 0;
  bool found = false;
  this->charRadius = 0.0;

  while (i < (int) this->binInfo[this->criticalBin].size() && found == false) {
    double r = (double) this->binInfo[this->criticalBin][i].radius;
    int index = this->binInfo[this->criticalBin][i].index;
    totParticleMass += (double) this->mass[index];
    double volume = ((4.0 * M_PI) / 3.0) * r * r * r;
    double ratio = (totParticleMass / volume) / this->RHOC;

    if (ratio < this->rhoRatio) {
      this->criticalIndex = i;
      this->charRadius = r;
      found = true;
    }
    i++;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Gather the accepted SOD particles from the bin RadiusID vector
// into array of particle index on this processor and matching radius
// Also collect some statistics
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::gatherSODParticles()
{
  // Allocate memory to hold the indices using bins to approximate
  int totalCount = 0;
  for (int bin = 0; bin <= this->criticalBin; bin++) {
    totalCount += this->binCount[bin];
  }
  this->particleIndex = new int[totalCount];
  this->particleRadius = new POSVEL_T[totalCount];

  // Collect average velocity and average location of SOD particles
  for (int dim = 0; dim < DIMENSION; dim++) {
    this->avgVelocity[dim] = 0.0;
    this->avgLocation[dim] = 0.0;
    this->centerOfMass[dim] = 0.0;
  }

  // Iterate over all bins less than the critical bin collecting particles
  this->numberOfParticles = 0;
  this->totalMass = 0.0;

  for (int bin = 0; bin < this->criticalBin; bin++) {
    for (int i = 0; i < (int) this->binInfo[bin].size(); i++) {
      int p = this->binInfo[bin][i].index;

      this->particleIndex[this->numberOfParticles] = p;
      this->particleRadius[this->numberOfParticles] =
        this->binInfo[bin][i].radius;
      numberOfParticles++;
      this->totalMass += (double) this->mass[p];

      // Collect average location of SOD particles
      this->avgLocation[0] += (double) this->xx[p];
      this->avgLocation[1] += (double) this->yy[p];
      this->avgLocation[2] += (double) this->zz[p];

      // Collect center of mass of SOD particles
      this->centerOfMass[0] += (double) this->xx[p] * (double) this->mass[p];
      this->centerOfMass[1] += (double) this->yy[p] * (double) this->mass[p];
      this->centerOfMass[2] += (double) this->zz[p] * (double) this->mass[p];

      // Collect average velocity of SOD particles
      this->avgVelocity[0] += (double) this->vx[p];
      this->avgVelocity[1] += (double) this->vy[p];
      this->avgVelocity[2] += (double) this->vz[p];
    }
  }

  // Iterate over the critical bin to the critical index
  for (int i = 0; i < this->criticalIndex; i++) {
    int p = this->binInfo[criticalBin][i].index;

    this->particleIndex[this->numberOfParticles] = p;
    this->particleRadius[this->numberOfParticles] =
      this->binInfo[criticalBin][i].radius;
    this->numberOfParticles++;
    this->totalMass += (double) this->mass[p];

    // Collect average location of SOD particles
    this->avgLocation[0] += (double) this->xx[p];
    this->avgLocation[1] += (double) this->yy[p];
    this->avgLocation[2] += (double) this->zz[p];

    // Collect center of mass of SOD particles
    this->centerOfMass[0] += (double) this->xx[p] * (double) this->mass[p];
    this->centerOfMass[1] += (double) this->yy[p] * (double) this->mass[p];
    this->centerOfMass[2] += (double) this->zz[p] * (double) this->mass[p];

    // Collect average velocity of SOD particles
    this->avgVelocity[0] += (double) this->vx[p];
    this->avgVelocity[1] += (double) this->vy[p];
    this->avgVelocity[2] += (double) this->vz[p];
  }

  for (int dim = 0; dim < DIMENSION; dim++) {
    this->avgLocation[dim] /= this->numberOfParticles;
    this->centerOfMass[dim] /= this->totalMass;
    this->avgVelocity[dim] /= this->numberOfParticles;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Copy locations, velocities and tags of halo particles to the allocated arrays
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::extractInformation(
                        int* actualIndx,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
                        POSVEL_T* xVelHalo,
                        POSVEL_T* yVelHalo,
                        POSVEL_T* zVelHalo,
                        POSVEL_T* massHalo,
                        POSVEL_T* radius,
                        ID_T* id)
{
  for (int i = 0; i < this->numberOfParticles; i++) {
    int p = this->particleIndex[i];
    radius[i] = this->particleRadius[i];

    xLocHalo[i] = this->xx[p];
    yLocHalo[i] = this->yy[p];
    zLocHalo[i] = this->zz[p];
    xVelHalo[i] = this->vx[p];
    yVelHalo[i] = this->vy[p];
    zVelHalo[i] = this->vz[p];
    massHalo[i] = this->mass[p];
    id[i] = this->tag[p];
    actualIndx[i] = p;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the velocity dispersion of the SOD halo
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::calculateVelocityDispersion()
{
  POSVEL_T particleDot = 0.0;
  for (int i = 0; i < this->numberOfParticles; i++) {
    int p = this->particleIndex[i];
    particleDot += dotProduct(this->vx[p], this->vy[p], this->vz[p]);
  }

  // Average of all the dot products
  particleDot /= this->numberOfParticles;

  // Dot product of the average velocity for the entire halo
  POSVEL_T haloDot = dotProduct(avgVelocity[0], avgVelocity[1], avgVelocity[2]);

  // Velocity dispersion
  this-> velocityDispersion = sqrt((particleDot - haloDot) / 3.0);
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the mass of the SOD halo
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::calculateMass()
{
  this->totalMass = 0.0;
  for (int i = 0; i < this->numberOfParticles; i++) {
    int p = this->particleIndex[i];
    this->totalMass += this->mass[p];
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Return information for mass profile and mass density profile
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::SODProfile(
                        int* bCount,
                        POSVEL_T* bMass,
                        POSVEL_T* bRadius,
                        POSVEL_T* bRho,
                        POSVEL_T* bRhoRatio,
                        POSVEL_T* bRadVelocity)
{
  for (int bin = 1; bin < this->numberOfBins; bin++) {
    bCount[bin-1] = this->binCount[bin];
    bMass[bin-1] = (POSVEL_T) this->binMass[bin];
    bRadius[bin-1] = (POSVEL_T) this->binRadius[bin];
    bRho[bin-1] = (POSVEL_T) this->binRho[bin];
    bRhoRatio[bin-1] = (POSVEL_T) this->binRhoRatio[bin];
    bRadVelocity[bin-1] = (POSVEL_T) this->avgRadVelocity[bin];
  }
}

void SODHalo::SODAverageLocation(POSVEL_T* pos)
{
  for (int dim = 0; dim < DIMENSION; dim++)
    pos[dim] = (POSVEL_T) this->avgLocation[dim];
}

void SODHalo::SODCenterOfMass(POSVEL_T* com)
{
  for (int dim = 0; dim < DIMENSION; dim++)
    com[dim] = (POSVEL_T) this->centerOfMass[dim];
}

void SODHalo::SODAverageVelocity(POSVEL_T* vel)
{
  for (int dim = 0; dim < DIMENSION; dim++)
    vel[dim] = (POSVEL_T) this->avgVelocity[dim];
}

void SODHalo::SODVelocityDispersion(POSVEL_T* velDisp)
{
  (*velDisp) = (POSVEL_T) this->velocityDispersion;
}

void SODHalo::SODMass(POSVEL_T* sodmass)
{
  (*sodmass) = (POSVEL_T) this->totalMass;
}

/////////////////////////////////////////////////////////////////////////
//
// Dot product of a vector
//
/////////////////////////////////////////////////////////////////////////

POSVEL_T SODHalo::dotProduct(POSVEL_T x, POSVEL_T y, POSVEL_T z)
{
  POSVEL_T dotProd = x * x + y * y + z * z;
  return dotProd;
}

/////////////////////////////////////////////////////////////////////////
//
// Cubic spline from Numerical Recipes (altered for zero based arrays)
// Called only once to process entire tabulated function
//
// Given arrays x[0..n-1] and y[0..n-1] containing a tabulated function
// with x0 < x1 < .. < xn-1, and given values yp1 and ypn for the
// first derivative of the interpolating function at points 0 and n-1,
// this routine returns an array y2[0..n-1] that contains the second
// derivatives of the interpolating function.  If yp1 or ypn > e30
// the rougine is signaled to set the corresponding boundary condition
// for a natural spline, with zero second derivative on that boundary.
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::spline(
                POSVEL_T* x, POSVEL_T* y,       // arrays
                int n,                          // size of arrays
                POSVEL_T* y2)                   // return array
{
  // Set boundary conditions
  POSVEL_T yp1 = 1.0e31;
  POSVEL_T ypn = 1.0e31;
  POSVEL_T qn, un;
  POSVEL_T* u = new POSVEL_T[n];

  // Lower boundary condition set to natural spline
  if (yp1 > 0.99e30)
    y2[0] = u[0] = 0.0;

  // Lower boundary condition set to specified first derivative
  else {
    y2[0] = -0.5;
    u[0]=(3.0/(x[1]-x[0]))*((y[1]-y[0])/(x[1]-x[0])-yp1);
  }

  // Decomposition loop of tridiagonal algorithm
  for (int i = 1; i < n-1; i++) {
    POSVEL_T sig = (x[i] - x[i-1]) / (x[i+1] - x[i-1]);
    POSVEL_T p = sig * y2[i-1] + 2.0;
    y2[i] = (sig - 1.0) / p;
    u[i] = (y[i+1] - y[i]) / (x[i+1] - x[i]) -
           (y[i] - y[i-1]) / (x[i] - x[i-1]);
    u[i] = (6.0 * u[i] / (x[i+1] - x[i-1]) - sig * u[i-1]) / p;
  }

  // Upper boundary condition set to natural spline
  if (ypn > 0.99e30)
    qn = un = 0.0;

  // Upper boundary condition set to specified first derivative
  else {
    qn = 0.5;
    un = (3.0 / (x[n-1] - x[n-2])) *
         (ypn - (y[n-1] - y[n-2]) / (x[n-1] -x [n-2]));
  }

  // Back substitution loop of tridiagonal algorithm
  y2[n-1] = (un - qn * u[n-2]) / (qn * y2[n-2] + 1.0);
  for (int k = n - 2; k >= 0; k--)
    y2[k] = y2[k] * y2[k+1] + u[k];

#ifndef USE_VTK_COSMO
  for (int i = 0; i < n; i++)
    cout << "x " << x[i] << "   y " << y[i] << "    result " << y2[i] << endl;
#endif

  delete [] u;
}

/////////////////////////////////////////////////////////////////////////
//
// Cubic spline interpolation from Numerical Recipes
// Called succeeding times after spline is called once
// Given x, y and y2 arrays from spline return cubic spline interpolated
//
/////////////////////////////////////////////////////////////////////////

void SODHalo::splint(
                POSVEL_T* xa, POSVEL_T* ya,     // arrays sent to spline
                POSVEL_T* y2a,                  // result from spline
                int n,                          // size of arrays
                POSVEL_T x,                     //
                POSVEL_T* y)                    // interpolated value
{
  // Find the right place in the table by means of bisection
  // Optimal is sequential calls are at random values of x
  int klo = 0;
  int khi = n - 1;
  while (khi - klo > 1) {
    int k = (khi + klo + 1) >> 1;
    if (xa[k] > x)
      khi = k;
    else
      klo = k;
  }

  POSVEL_T h = xa[khi] - xa[klo];
  POSVEL_T a = (xa[khi] - x) / h;
  POSVEL_T b = (x - xa[klo]) / h;
  *y = a * ya[klo] + b * ya[khi] +
       ((a * a * a - a) * y2a[klo] +
       (b * b * b - b) * y2a[khi]) * (h * h) / 6.0;
}
