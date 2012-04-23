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
#include "FOFHaloProperties.h"
#include "HaloCenterFinder.h"
#ifndef USE_VTK_COSMO
#include "Timings.h"
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <math.h>

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
                        POSVEL_T pDist)
{
  this->outFile = outName;

  // Halo finder parameters
  this->boxSize = rL;
  this->deadSize = deadSz;
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
                        vector<POSVEL_T>* pmass,
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
  this->mass = &(*pmass)[0];
  this->pot = &(*potential)[0];
  this->tag = &(*id)[0];
  this->mask = &(*maskData)[0];
  this->status = &(*state)[0];
}

void FOFHaloProperties::setParticles(
			long count,
                        POSVEL_T* xLoc,
                        POSVEL_T* yLoc,
                        POSVEL_T* zLoc,
                        POSVEL_T* xVel,
                        POSVEL_T* yVel,
                        POSVEL_T* zVel,
                        POSVEL_T* pmass,
                        ID_T* id)
{
  this->particleCount = count;

  // Extract the contiguous data block from a vector pointer
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->vx = xVel;
  this->vy = yVel;
  this->vz = zVel;
  this->mass = pmass;
  this->tag = id;
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
// Calculate the mass of every FOF halo accumulating individual mass
//
// m_FOF = ((Sum i=1 to n_FOF) m_i) / n_FOF
//    m_FOF is the mass of an FOF halo
//    n_FOF is the number of particles in the halo
//    m_i is the mass of an individual particle
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloMass(
                        vector<POSVEL_T>* haloMass)
{
  POSVEL_T lmass;
  double mKahan;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    mKahan = KahanSummation(halo, this->mass);
    lmass = (POSVEL_T) mKahan;
    (*haloMass).push_back(lmass);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the center of mass of every FOF halo
//
// x_FOF = ((Sum i=1 to n_FOF) x_i * x_mass) / n_mass
//    x_FOF is the center of mass vector
//    n_mass is the total mass of particles in the halo
//    x_i is the position vector of particle i
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFCenterOfMass(
                        vector<POSVEL_T>* xCenterOfMass,
                        vector<POSVEL_T>* yCenterOfMass,
                        vector<POSVEL_T>* zCenterOfMass)
{
  POSVEL_T xCofMass, yCofMass, zCofMass;
  double xKahan, yKahan, zKahan;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {

    double totalMass = KahanSummation(halo, this->mass);

    xKahan = KahanSummation2(halo, this->xx, this->mass);
    yKahan = KahanSummation2(halo, this->yy, this->mass);
    zKahan = KahanSummation2(halo, this->zz, this->mass);

    xCofMass = (POSVEL_T) (xKahan / totalMass);
    yCofMass = (POSVEL_T) (yKahan / totalMass);
    zCofMass = (POSVEL_T) (zKahan / totalMass);

    (*xCenterOfMass).push_back(xCofMass);
    (*yCenterOfMass).push_back(yCofMass);
    (*zCenterOfMass).push_back(zCofMass);
  }
}

/////////////////////////////////////////////////////////////////////////
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
// Calculate the Kahan summation on two variables multiplied
// Reduces roundoff error in floating point arithmetic
//
/////////////////////////////////////////////////////////////////////////

POSVEL_T FOFHaloProperties::KahanSummation2(int halo,
                                            POSVEL_T* data1, POSVEL_T* data2)
{
  POSVEL_T dataSum, dataRem, v, w;

  // First particle in halo and first step in Kahan summation
  int p = this->halos[halo];
  dataSum = data1[p] * data2[p];
  dataRem = 0.0;

  // Next particle
  p = this->haloList[p];

  // Remaining steps in Kahan summation
  while (p != -1) {
    v = (data1[p] * data2[p]) - dataRem;
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
                        vector<POSVEL_T>* haloMass,
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

    // Write ascii
    sprintf(str, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12d\n",
      this->xx[centerIndex],
      (*xMeanVel)[halo],
      this->yy[centerIndex],
      (*yMeanVel)[halo],
      this->zz[centerIndex],
      (*zMeanVel)[halo],
      (*haloMass)[halo],
      haloTag);
      aStream << str;

    fBlock[0] = this->xx[centerIndex];
    fBlock[1] = (*xMeanVel)[halo];
    fBlock[2] = this->yy[centerIndex];
    fBlock[3] = (*yMeanVel)[halo];
    fBlock[4] = this->zz[centerIndex];
    fBlock[5] = (*zMeanVel)[halo];
    fBlock[6] = (*haloMass)[halo];
    cStream.write(reinterpret_cast<char*>(fBlock),
                  COSMO_FLOAT * sizeof(POSVEL_T));

    iBlock[0] = haloTag;
    cStream.write(reinterpret_cast<char*>(iBlock),
                  COSMO_INT * sizeof(ID_T));
  }
  aStream.close();
  cStream.close();
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
// Copy locations and tags of halo particles to the allocated arrays
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::extractLocation(
				int halo,
				int* actualIndx,
				POSVEL_T* xLocHalo,
				POSVEL_T* yLocHalo,
				POSVEL_T* zLocHalo,
				ID_T* id)
{
  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    xLocHalo[i] = this->xx[p];
    yLocHalo[i] = this->yy[p];
    zLocHalo[i] = this->zz[p];
    id[i] = this->tag[p];
    actualIndx[i] = p;
    p = this->haloList[p];
  }
}

#endif

/////////////////////////////////////////////////////////////////////////
//
// Copy locations, velocities and tags of halo particles to the allocated arrays
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::extractInformation(
				int halo,
				int* actualIndx,
				POSVEL_T* xLocHalo,
				POSVEL_T* yLocHalo,
				POSVEL_T* zLocHalo,
				POSVEL_T* xVelHalo,
				POSVEL_T* yVelHalo,
				POSVEL_T* zVelHalo,
				POSVEL_T* massHalo,
				ID_T* id)
{
  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    xLocHalo[i] = this->xx[p];
    yLocHalo[i] = this->yy[p];
    zLocHalo[i] = this->zz[p];
    xVelHalo[i] = this->vx[p];
    yVelHalo[i] = this->vy[p];
    zVelHalo[i] = this->vz[p];
    massHalo[i] = this->mass[p];
    id[i] = this->tag[p];
    actualIndx[i] = p;
    p = this->haloList[p];
  }
}

#ifndef USE_VTK_COSMO

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
