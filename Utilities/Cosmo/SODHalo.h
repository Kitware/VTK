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

// .NAME SODHalo - calculate properties of all SOD halos
//
// .SECTION Description
// SODHalo takes data from CosmoHaloFinderP about individual halos
// and data from all particles and calculates properties.
//

#ifndef SODHalo_h
#define SODHalo_h

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

#include "ChainingMesh.h"

///////////////////////////////////////////////////////////////////////////
//
// To calculate the exact r_200 store the distance and mass of each particle
// within the sphere.  When the particles are sorted by distance from the
// center, the exact density at that particle can be calculated
//
///////////////////////////////////////////////////////////////////////////

struct RadiusID {
  POSVEL_T radius;
  int index;
};

class RadiusIDLT {
public:
  bool operator() (const RadiusID& p, const RadiusID& q) const
  {
  return p.radius < q.radius;
  }
};

///////////////////////////////////////////////////////////////////////////
//
// SOD Halo creation using either exact density or approximate with bins
//
///////////////////////////////////////////////////////////////////////////

#ifdef USE_VTK_COSMO
class COSMO_EXPORT SODHalo
#else
class SODHalo
#endif
{
public:
  SODHalo();
  ~SODHalo();

  // Set parameters for SOD calculation
  void setParameters(
        ChainingMesh* chain,    // Particles arranged in buckets
        int numBins,            // Estimation density bins
        POSVEL_T rL,            // Box size of the physical problem
        POSVEL_T np,            // Grid size of problem
        POSVEL_T rho_c,         // Critical density of universe
        POSVEL_T sodMassFactor, // Factor used in initial radius
        POSVEL_T rhoRatio,      // rho / rho_c for virial radius
        POSVEL_T minFactor,     // Min factor for initial radius range
        POSVEL_T maxFactor);    // Max factor for initial radius range

  // Set alive particle vectors which were created elsewhere
  void setParticles(
        vector<POSVEL_T>* xLoc,
        vector<POSVEL_T>* yLoc,
        vector<POSVEL_T>* zLoc,
        vector<POSVEL_T>* xVel,
        vector<POSVEL_T>* yVel,
        vector<POSVEL_T>* zVel,
        vector<POSVEL_T>* pmass,
        vector<ID_T>* id);

  /////////////////////////////////////////////////////////////////////
  //
  // SOD (Spherical over density) halo analysis
  //
  /////////////////////////////////////////////////////////////////////

  // Spherical over-density (SOD) mass profile, velocity dispersion
  void createSODHalo(
        int FOFhaloCount,       // FOF particle count
        POSVEL_T centerXLoc,    // FOF center location for SOD
        POSVEL_T centerYLoc,    // FOF center location for SOD
        POSVEL_T centerZLoc,    // FOF center location for SOD
        POSVEL_T fofHaloXVel,   // FOF halo velocity for SOD radial velocity
        POSVEL_T fofHaloYVel,   // FOF halo velocity for SOD radial velocity
        POSVEL_T fofHaloZVel,   // FOF halo velocity for SOD radial velocity
        POSVEL_T fofHaloMass);  // FOF halo mass for SOD

  // Create the SOD mass profile used to calculate characteristic radius
  void calculateMassProfile();

  // Calculate the characteristic radius of an SOD halo
  void calculateCharacteristicRadius();

  // Gather all particles belonging to the SOD halo
  // Collect average velocity at the same time
  void gatherSODParticles();

  // Calculate velocity dispersion
  void calculateVelocityDispersion();

  // Calculate mass
  void calculateMass();

  // Utilities
  POSVEL_T dotProduct(POSVEL_T x, POSVEL_T y, POSVEL_T z);
  void spline(
        POSVEL_T* x, POSVEL_T* y, int n,
        POSVEL_T* y2);
  void splint(
        POSVEL_T* xa, POSVEL_T* ya, POSVEL_T* y2a, int n,
        POSVEL_T x, POSVEL_T* y);

  int SODHaloSize()             { return this->numberOfParticles; }
  POSVEL_T SODRadius()          { return this->charRadius; }
  int* SODParticles()           { return this->particleIndex; }

  void SODAverageLocation(POSVEL_T* pos);
  void SODCenterOfMass(POSVEL_T* com);
  void SODAverageVelocity(POSVEL_T* vel);
  void SODVelocityDispersion(POSVEL_T* velDisp);
  void SODMass(POSVEL_T* mass);
  void SODProfile(
        int* bCount,
        POSVEL_T* bMass,
        POSVEL_T* bRadius,
        POSVEL_T* bRho,
        POSVEL_T* bRhoRatio,
        POSVEL_T* bRadVelocity);

  // Extract information for all particles in SOD halo
  void extractInformation(
        int* actualIndx,
        POSVEL_T* xLocHalo,
        POSVEL_T* yLocHalo,
        POSVEL_T* zLocHalo,
        POSVEL_T* xVelHalo,
        POSVEL_T* yVelHalo,
        POSVEL_T* zVelHalo,
        POSVEL_T* pmass,
        POSVEL_T* radius,
        ID_T* tag);

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  ChainingMesh* chain;          // Buckets of particles on processor
  int*** buckets;               // First particle index into bucketList
  int* bucketList;              // Indices of next particle in halo

  int minFOFHaloSize;           // Minimum FOF size for building SOD
  int numberOfBins;             // Estimation density concentric spheres
  POSVEL_T rhoRatio;            // rho / rho_c for virial radius
  POSVEL_T cMinFactor;          // Min factor for initial radius range
  POSVEL_T cMaxFactor;          // Max factor for initial radius range
  POSVEL_T rSmooth;             // boxSize / gridSize for getting minimum radius
  POSVEL_T RHOC;                // RHO_C * factor to get units right
  POSVEL_T SODMASS;             // SOD_MASS * factor to get units right

  long   particleCount;         // Total particles on this processor

  POSVEL_T* xx;                 // X location for particles on this processor
  POSVEL_T* yy;                 // Y location for particles on this processor
  POSVEL_T* zz;                 // Z location for particles on this processor
  POSVEL_T* vx;                 // X velocity for particles on this processor
  POSVEL_T* vy;                 // Y velocity for particles on this processor
  POSVEL_T* vz;                 // Z velocity for particles on this processor
  POSVEL_T* mass;               // Mass of particles on this processor
  ID_T* tag;                    // Tag of particles on this processor

  // Information about this SOD halo
  POSVEL_T initRadius;          // First guess at radius based on FOF size
  POSVEL_T minRadius;           // Smallest radius to bin spheres on
  POSVEL_T maxRadius;           // Largest radius to bin spheres on
  POSVEL_T deltaRadius;         // Step on log bins from min to max radius
  POSVEL_T charRadius;          // Characteristic radius (r_200)

  int*    binCount;             // Number of particles assigned to bin
  double* binMass;              // Mass of SOD at this bin
  double* binRho;               // Density of SOD at this bin
  double* binRhoRatio;          // Density ratio of SOD at this bin
  double* avgRadius;            // Average radius of particles assigned to bin
  double* avgRadVelocity;       // Average radial velocity of particles in bin
  POSVEL_T* binRadius;          // Max radius of a log bin
  vector<RadiusID>* binInfo;    // Particles in bin with radius

  int criticalBin;              // Bin holding the critical density ratio
  int criticalIndex;            // Index in critical bin of critical radius

  int numberOfParticles;        // Number in this SOD halo
  int* particleIndex;           // Indices of particles in this halo
  POSVEL_T* particleRadius;     // Matching radius of particles in this halo

  int      fofHaloCount;                // FOF particle count
  POSVEL_T fofCenterLocation[DIMENSION];// FOF center particle location
  POSVEL_T fofHaloVelocity[DIMENSION];  // FOF average velocity of all particles

  double avgVelocity[DIMENSION];        // SOD average veloctiy of all particles
  double avgLocation[DIMENSION];        // SOD average location of particles
  double centerOfMass[DIMENSION];       // SOD center of mass
  double velocityDispersion;            // SOD velocity dispersion
  double totalMass;                     // SOD total mass
};

#endif
