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
#include <cassert>

#include "Partition.h"
#include "CosmoHaloFinderP.h"

using namespace std;
using cosmologytools::Partition;

#ifdef COSMO_USE_GENERIC_IO
# include "GenericIO.h"
using gio::GenericIO;
#endif


namespace cosmologytools {

/////////////////////////////////////////////////////////////////////////
//
// Parallel manager for serial CosmoHaloFinder
// Particle data space is partitioned for the number of processors
// which currently is a factor of two but is easily extended.  Particles
// are read in from files where each processor reads one file into a buffer,
// extracts the particles which really belong on the processor (ALIVE) and
// those in a buffer region around the edge (DEAD).  The buffer is then
// passed round robin to every other processor so that all particles are
// examined by all processors.  All dead particles are tagged with the
// neighbor zone (26 neighbors in 3D) so that later halos can be associated
// with zones.
//
// The serial halo finder is called on each processor and returns enough
// information so that it can be determined if a halo is completely ALIVE,
// completely DEAD, or mixed.  A mixed halo that is shared between on two
// processors is kept by the processor that contains it in one of its
// high plane neighbors, and is given up if contained in a low plane neighbor.
//
// Mixed halos that cross more than two processors are bundled up and sent
// to the MASTER processor which decides the processor that should own it.
//
/////////////////////////////////////////////////////////////////////////

CosmoHaloFinderP::CosmoHaloFinderP()
{
  // Get the number of processors and rank of this processor
  this->numProc = cosmologytools::Partition::getNumProc();
  this->myProc = cosmologytools::Partition::getMyProc();

  // Get the number of processors in each dimension
  cosmologytools::Partition::getDecompSize(this->layoutSize);

  // Get my position within the Cartesian topology
  cosmologytools::Partition::getMyPosition(this->layoutPos);

  // Get the neighbors of this processor
 cosmologytools::Partition::getNeighbors(this->neighbor);

  // For each neighbor zone, how many dead particles does it contain to start
  // and how many dead halos does it contain after the serial halo finder
  // For analysis but not necessary to run the code
  //
  for (int n = 0; n < NUM_OF_NEIGHBORS; n++) {
    this->deadParticle[n] = 0;
    this->deadHalo[n] = 0;
  }
  this->haloTag   = NULL;
  this->haloList  = NULL;
  this->haloStart = NULL;
  this->haloSize  = NULL;
  //this->haloData  = NULL;
  this->myMixedHalos.resize(0);
  this->allMixedHalos.resize(0);
}

CosmoHaloFinderP::~CosmoHaloFinderP()
{
  clearHaloTag();
  clearHaloStart();
  clearHaloList();
  clearHaloSize();
}

void CosmoHaloFinderP::initializeHaloFinder()
{
  // Get the number of processors and rank of this processor
  this->numProc = cosmologytools::Partition::getNumProc();
  this->myProc = cosmologytools::Partition::getMyProc();

  // Get the number of processors in each dimension
  cosmologytools::Partition::getDecompSize(this->layoutSize);

  // Get my position within the Cartesian topology
  cosmologytools::Partition::getMyPosition(this->layoutPos);

  // Get the neighbors of this processor
  cosmologytools::Partition::getNeighbors(this->neighbor);
}

//
// Halo structure information is allocated here and passed to serial halo
// finder for filling and then some is passed to the calling simulator
// for other analysis.  So memory is not allocated and freed nicely.
//
void CosmoHaloFinderP::clearHaloTag()
{
  // haloTag holds the index of the particle which is the first in the halo
  // so if haloTag[p] != p then this particle is in a halo
  // may be released after tagged particles are written or after
  // all halos are collected for merging
  if (this->haloTag != 0) {
    delete [] this->haloTag;
    this->haloTag = 0;
  }
}

void CosmoHaloFinderP::clearHaloStart()
{
  // haloStart holds the index of the first particle in a halo
  // used with haloList to locate all particles in a halo
  // may be released after merged halos because info is put in halos vector
  if (this->haloStart != 0) {
    delete [] this->haloStart;
    this->haloStart = 0;
  }
}

void CosmoHaloFinderP::clearHaloList()
{
  // haloList is used with haloStart or with halos vector for locating all
  // particles in a halo.  It must stay around through all analysis.
  // may be released only on next call to executeHaloFinder
  if (this->haloList != 0) {
    delete [] this->haloList;
    this->haloList = 0;
  }
}

void CosmoHaloFinderP::clearHaloSize()
{
  // haloSize holds the size of the halo associated with any particle
  // may be released after tagged particles are written or after
  // all halos are collected for merging
  if (this->haloSize != 0) {
    delete [] this->haloSize;
    this->haloSize = 0;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for the serial halo finder
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::setParameters(
                        const string& outName,
                        POSVEL_T _rL,
                        POSVEL_T _deadSz,
                        long _np,
                        int _pmin,
                        POSVEL_T _bb,
                        int _nmin)
{
  // Particles for this processor output to file
  this->outFile = outName;

  // Halo finder parameters
  this->np = _np;
  this->pmin = _pmin;
  this->bb = _bb;
  this->nmin = _nmin;
  this->boxSize = _rL;
  this->deadSize = _deadSz;

  // Unnormalize bb so that it will work with box size distances
  this->haloFinder.bb = _bb * (POSVEL_T) ((1.0 * _rL) / _np);;

  this->haloFinder.np = _np;
  this->haloFinder.pmin = _pmin;
  this->haloFinder.nmin = _nmin;
  this->haloFinder.rL = _rL;
  this->haloFinder.periodic = false;
  this->haloFinder.textmode = "ascii";

  if (this->myProc == MASTER) {
    cout << endl << "------------------------------------" << endl;
    cout << "np:       " << this->np << endl;
    cout << "bb:       " << this->bb << endl;
    cout << "nmin:     " << this->nmin << endl;
    cout << "pmin:     " << this->pmin << endl << endl;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::setParticles(
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

void CosmoHaloFinderP::setParticles(
        POSVEL_T *xLoc,
        POSVEL_T *yLoc,
        POSVEL_T *zLoc,
        POSVEL_T *xVel,
        POSVEL_T *yVel,
        POSVEL_T *zVel,
        POTENTIAL_T *potential,
        ID_T *id,
        MASK_T *mask,
        STATUS_T *state,
        long NumParticles)
{
  assert("pre: xLoc is NULL!" && (xLoc != NULL));
  assert("pre: yLoc is NULL!" && (yLoc != NULL));
  assert("pre: zLoc is NULL!" && (zLoc != NULL));
  assert("pre: xVel is NULL!" && (xVel != NULL));
  assert("pre: yVel is NULL!" && (yVel != NULL));
  assert("pre: zVel is NULL!" && (zVel != NULL));
  assert("pre: potential is NULL!" && (potential != NULL));
  assert("pre: id is NULL!" && (id != NULL));
  assert("pre: mask is NULL!" && (mask != NULL));
  assert("pre: state is NULL!" && (state != NULL));
  assert("pre: NumParticles >= 0" && (NumParticles >= 0) );

  this->xx     = xLoc;
  this->yy     = yLoc;
  this->zz     = zLoc;
  this->vx     = xVel;
  this->vy     = yVel;
  this->vz     = zVel;
  this->pot    = potential;
  this->tag    = id;
  this->mask   = mask;
  this->status = state;

  this->particleCount = NumParticles;
}

/////////////////////////////////////////////////////////////////////////
//
// Execute the serial halo finder on all particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::executeHaloFinder()
{
  // Clear old halo structure and allocate new
  clearHaloTag();
  clearHaloStart();
  clearHaloList();
  clearHaloSize();

  this->haloTag = new int[this->particleCount];
  this->haloStart = new int[this->particleCount];
  this->haloList = new int[this->particleCount];
  this->haloSize = new int[this->particleCount];

  // Set the input locations for the serial halo finder
  this->haloFinder.setParticleLocations(this->xx, this->yy, this->zz);

  // Set the output locations for the serial halo finder
  this->haloFinder.setHaloLocations(
                            this->haloTag,
                            this->haloStart,
                            this->haloList);

  this->haloFinder.setNumberOfParticles(this->particleCount);
  this->haloFinder.setMyProc(this->myProc);
  this->haloFinder.setOutFile(this->outFile);

#ifdef HALO_FINDER_VERBOSE
  cout << "Rank " << setw(3) << this->myProc
       << " RUNNING SERIAL HALO FINDER on "
       << particleCount << " particles" << endl;
#endif // HALO_FINDER_VERBOSE

#ifndef USE_SERIAL_COSMO
  MPI_Barrier(Partition::getComm());
#endif

  if (this->particleCount > 0)
    this->haloFinder.Finding();

#ifndef USE_SERIAL_COSMO
  MPI_Barrier(Partition::getComm());
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// At this point each serial halo finder ran and the particles handed to it
// included alive and dead.  Structure to locate all particles in a halo
// were returned in haloTag, haloStart and haloList
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::collectHalos(bool clearTag)
{
  // Record the halo size of each particle on this processor
  this->haloAliveSize = new int[this->particleCount];
  for (int p = 0; p < this->particleCount; p++) {
    this->haloSize[p] = 0;
    this->haloAliveSize[p] = 0;
  }

  // Build the chaining mesh of particles in all the halos and count particles
  buildHaloStructure();

  // Mixed halos are saved separately so that they can be merged
  processMixedHalos();

  // Clear the data associated with tag and size which won't be needed
  if (clearTag) {
    clearHaloTag();
    clearHaloSize();
  }
  delete [] this->haloAliveSize;
}

/////////////////////////////////////////////////////////////////////////
//
// Examine every particle on this processor, both ALIVE and DEAD
// For that particle increment the count for the corresponding halo
// which is indicated by the lowest particle index in that halo
// Also build the haloList so that we can find all particles in any halo
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::buildHaloStructure()
{
    // Count particles in the halos
  for (int p = 0; p < this->particleCount; p++) {
    if (this->status[p] == ALIVE)
      this->haloAliveSize[this->haloTag[p]]++;
    this->haloSize[this->haloTag[p]]++;
  }

  // Iterate over particles and create a CosmoHalo for halos with size > pmin
  // only for the mixed halos, not for those completely alive or dead
  this->numberOfAliveHalos = 0;
  this->numberOfDeadHalos = 0;
  this->numberOfMixedHalos = 0;

  // Only the first particle id for a halo records the size
  // Succeeding particles which are members of a halo have a size of 0
  // Record the start index of any legal halo which will allow the
  // following of the chaining mesh to identify all particles in a halo
  this->numberOfHaloParticles = 0;
  for (ID_T p = 0; p < this->particleCount; p++) {

    if (this->haloSize[p] >= this->pmin) {

      if (this->haloAliveSize[p] == this->haloSize[p]) {
        this->numberOfAliveHalos++;
        this->numberOfHaloParticles += this->haloAliveSize[p];

        // Save start of legal alive halo for halo properties
        this->halos.push_back(this->haloStart[p]);
        this->haloCount.push_back(this->haloAliveSize[p]);
      }
      else if (this->haloAliveSize[p] == 0) {
        this->numberOfDeadHalos++;
      }
      else {
        this->numberOfMixedHalos++;
        CosmoHalo* halo = new CosmoHalo(p,
                                this->haloAliveSize[p],
                                this->haloSize[p] - this->haloAliveSize[p]);
        this->myMixedHalos.push_back(halo);
      }
    }
  }


#ifdef DEBUG
  cout << "Rank " << this->myProc
       << " #alive halos = " << this->numberOfAliveHalos
       << " #dead halos = " << this->numberOfDeadHalos
       << " #mixed halos = " << this->numberOfMixedHalos << endl;
#endif

}

/////////////////////////////////////////////////////////////////////////
//
// Mixed halos (which cross several processors) have been collected
// By applying a high/low rule most mixed halos are assigned immediately
// to one processor or another.  This requires extra processing so that
// it is known which neighbor processors share the halo.
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::processMixedHalos()
{
  // Iterate over all particles and add tags to large mixed halos
  for (ID_T p = 0; p < this->particleCount; p++) {

    // All particles in the same halo have the same haloTag
    if (this->haloSize[this->haloTag[p]] >= pmin &&
        this->haloAliveSize[this->haloTag[p]] > 0 &&
        this->haloAliveSize[this->haloTag[p]] <
              this->haloSize[this->haloTag[p]]) {

          // Check all each mixed halo to see which this particle belongs to
          for (unsigned int h = 0; h < this->myMixedHalos.size(); h++) {

            // If the tag of the particle matches the halo ID it belongs
            if (this->haloTag[p] == this->myMixedHalos[h]->getHaloID()) {

              // Add the index to that mixed halo.  Also record which neighbor
              // the dead particle is associated with for merging
              this->myMixedHalos[h]->addParticle(
                                        p, this->tag[p], this->status[p]);

              // For debugging only
              if (this->status[p] > 0)
                this->deadHalo[this->status[p]]++;

              // Do some bookkeeping for the final output
              // This processor should output all ALIVE particles, unless they
              // are in a mixed halo that ends up being INVALID
              // This processor should output none of the DEAD particles,
              // unless they are in a mixed halo that ends up being VALID

              // So since this particle is in a mixed halo set it to MIXED
              // which is going to be one less than ALIVE.  Later when we
              // determine we have a VALID mixed, we'll add one to the status
              // for every particle turning all into ALIVE

              // Now when we output we only do the ALIVE particles
              this->status[p] = MIXED;
            }
          }
    }
  }

  // Iterate over the mixed halos that were just created checking to see if
  // the halo is on the "high" side of the 3D data space or not
  // If it is on the high side and is shared with one other processor, keep it
  // If it is on the low side and is shared with one other processor, delete it
  // Any remaining halos are shared with more than two processors and must
  // be merged by having the MASTER node decide
  //
  for (unsigned int h = 0; h < this->myMixedHalos.size(); h++) {
    int lowCount = 0;
    int highCount = 0;
    set<int>* neighbors = this->myMixedHalos[h]->getNeighbors();
    set<int>::iterator iter;
    set<int> haloNeighbor;

    for (iter = neighbors->begin(); iter != neighbors->end(); ++iter) {
      if ((*iter) == X1 || (*iter) == Y1 || (*iter) == Z1 ||
          (*iter) == X1_Y1 || (*iter) == Y1_Z1 || (*iter) == Z1_X1 ||
          (*iter) == X1_Y1_Z1) {
            highCount++;
      } else {
            lowCount++;
      }
      // Neighbor zones are on what actual processors
      haloNeighbor.insert(this->neighbor[(*iter)]);
    }

    // Halo is kept by this processor and is marked as VALID
    // May be in multiple neighbor zones, but all the same processor neighbor
    if (highCount > 0 && lowCount == 0 && haloNeighbor.size() == 1) {
      this->numberOfAliveHalos++;
      this->numberOfMixedHalos--;
      this->myMixedHalos[h]->setValid(VALID);
      int id = this->myMixedHalos[h]->getHaloID();
      int newAliveParticles = this->myMixedHalos[h]->getAliveCount() +
                              this->myMixedHalos[h]->getDeadCount();
      this->numberOfHaloParticles += newAliveParticles;

      // Add this halo to valid halos on this processor for
      // subsequent halo properties analysis
      this->halos.push_back(this->haloStart[id]);
      this->haloCount.push_back(newAliveParticles);

      // Output trick - since the status of this particle was marked MIXED
      // when it was added to the mixed CosmoHalo vector, and now it has
      // been declared VALID, change it to ALIVE even if it was dead before
      vector<ID_T>* particles = this->myMixedHalos[h]->getParticles();
      vector<ID_T>::iterator iter2;
      for (iter2 = particles->begin(); iter2 != particles->end(); ++iter2)
        this->status[(*iter2)] = ALIVE;
    }

    // Halo will be kept by some other processor and is marked INVALID
    // May be in multiple neighbor zones, but all the same processor neighbor
    else if (highCount == 0 && lowCount > 0 && haloNeighbor.size() == 1) {
      this->numberOfDeadHalos++;
      this->numberOfMixedHalos--;
      this->myMixedHalos[h]->setValid(INVALID);
    }

    // Remaining mixed halos must be examined by MASTER and stay UNMARKED
    // Sort them on the tag field for easy comparison
    else {
      this->myMixedHalos[h]->setValid(UNMARKED);
      this->myMixedHalos[h]->sortParticleTags();
    }
  }

  // If only one processor is running there are no halos to merge
  if (this->numProc == 1)
    for (unsigned int h = 0; h < this->myMixedHalos.size(); h++)
       this->myMixedHalos[h]->setValid(INVALID);
}

/////////////////////////////////////////////////////////////////////////
//
// Using the MASTER node merge all mixed halos so that only one processor
// takes credit for them.
//
// Each processor containing mixed halos that are UNMARKED sends:
//    Rank
//    Number of mixed halos to merge
//    for each halo
//      id
//      number of alive (for debugging)
//      number of dead  (for debugging)
//      first pmin particle ids (for merging)
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::mergeHalos()
{
  // What size integer buffer is needed to hold the largest halo data
  int maxNumberOfMixed;
  int numberOfMixed = (int)this->myMixedHalos.size();

#ifdef USE_SERIAL_COSMO
  maxNumberOfMixed = numberOfMixed;
#else
  MPI_Allreduce((void*) &numberOfMixed, (void*) &maxNumberOfMixed,
                1, MPI_INT, MPI_MAX, cosmologytools::Partition::getComm());
#endif

  // Everyone creates the buffer for maximum halos
  // MASTER will receive into it, others will send from it
  int haloBufSize = maxNumberOfMixed * this->pmin * 2;
  ID_T* haloBuffer = NULL;

  if (maxNumberOfMixed != 0)
    {
    haloBuffer = new ID_T[haloBufSize];

    // MASTER moves its own mixed halos to mixed halo vector (change index to tag)
    // then gets messages from others and creates those mixed halos
    collectMixedHalos(haloBuffer, haloBufSize);
    #ifndef USE_SERIAL_COSMO
      MPI_Barrier(Partition::getComm());
    #endif

    // MASTER has all data and runs algorithm to make decisions
    assignMixedHalos();
    #ifndef USE_SERIAL_COSMO
      MPI_Barrier(Partition::getComm());
    #endif

    // MASTER sends merge results to all processors
    sendMixedHaloResults(haloBuffer, haloBufSize);
    #ifndef USE_SERIAL_COSMO
      MPI_Barrier(Partition::getComm());
    #endif
    }

  // Collect totals for result checking
  int totalAliveHalos;
#ifdef USE_SERIAL_COSMO
  totalAliveHalos = this->numberOfAliveHalos;
#else
  MPI_Allreduce((void*) &this->numberOfAliveHalos, (void*) &totalAliveHalos,
                1, MPI_INT, MPI_SUM, cosmologytools::Partition::getComm());
#endif

  int totalAliveHaloParticles;
#ifdef USE_SERIAL_COSMO
  totalAliveHaloParticles = this->numberOfHaloParticles;
#else
  MPI_Allreduce((void*) &this->numberOfHaloParticles,
                (void*) &totalAliveHaloParticles,
                1, MPI_INT, MPI_SUM, Partition::getComm());
#endif

  if (this->myProc == MASTER) {
    cout << endl;
    cout << "Number of mixed halos: " << maxNumberOfMixed        << endl;
    cout << "Total halos found:    "  << totalAliveHalos         << endl;
    cout << "Total halo particles: "  << totalAliveHaloParticles << endl;
    cout.flush();
  }

  for (unsigned int i = 0; i < this->myMixedHalos.size(); i++)
    delete this->myMixedHalos[i];
  this->myMixedHalos.clear();

  for (unsigned int i = 0; i < this->allMixedHalos.size(); i++)
    delete this->allMixedHalos[i];
  this->allMixedHalos.clear();

  // haloStart information has been moved to vector<int> halos
  clearHaloStart();

  if( haloBuffer != NULL )
    delete [] haloBuffer;
}

/////////////////////////////////////////////////////////////////////////
//
// MASTER collects all mixed halos which are UNMARKED from all processors
// including its own mixed halos
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::collectMixedHalos
#ifdef USE_SERIAL_COSMO
  (ID_T* , int )
#else
  (ID_T* haloBuffer, int haloBufSize)
#endif
{
  // How many processors have mixed halos
  int haveMixedHalo = (this->numberOfMixedHalos > 0 ? 1 : 0);
  int processorsWithMixedHalos;
#ifdef USE_SERIAL_COSMO
  processorsWithMixedHalos = haveMixedHalo;
#else
  MPI_Allreduce((void*) &haveMixedHalo, (void*) &processorsWithMixedHalos,
                1, MPI_INT, MPI_SUM, cosmologytools::Partition::getComm());
#endif

  // MASTER moves its own mixed halos to mixed halo vector (change index to tag)
  // then gets messages from others and creates those mixed halos
#ifndef USE_SERIAL_COSMO
  if (this->myProc == MASTER) {
#endif

    // If MASTER has any mixed halos add them to the mixed halo vector
    if (this->numberOfMixedHalos > 0) {
      processorsWithMixedHalos--;

      for (unsigned int h = 0; h < this->myMixedHalos.size(); h++) {
        if (this->myMixedHalos[h]->getValid() == UNMARKED) {
          CosmoHalo* halo = new CosmoHalo(
                                     this->myMixedHalos[h]->getHaloID(),
                                     this->myMixedHalos[h]->getAliveCount(),
                                     this->myMixedHalos[h]->getDeadCount());
          halo->setRankID(this->myProc);
          this->allMixedHalos.push_back(halo);

          // Translate index of particle to tag of particle
          vector<ID_T>* tags = this->myMixedHalos[h]->getTags();
          for (int i = 0; i < this->pmin; i++)
            halo->addParticle((*tags)[i]);

        }
      }
    }

#ifndef USE_SERIAL_COSMO
    // Wait on messages from other processors and process
    int notReceived = processorsWithMixedHalos;
    MPI_Status mpistatus;
    while (notReceived > 0) {

      // Get message containing mixed halo information
#ifdef ID_64
      MPI_Recv(haloBuffer, haloBufSize, MPI_LONG, MPI_ANY_SOURCE,
               0, cosmologytools::Partition::getComm(), &mpistatus);
#else
      MPI_Recv(haloBuffer, haloBufSize, MPI_INT, MPI_ANY_SOURCE,
               0, cosmologytools::Partition::getComm(), &mpistatus);
#endif

      // Gather halo information from the message
      int index = 0;
      int rank = haloBuffer[index++];
      int numMixed = haloBuffer[index++];

      for (int m = 0; m < numMixed; m++) {
        ID_T id = haloBuffer[index++];
        int aliveCount = haloBuffer[index++];
        int deadCount = haloBuffer[index++];

        // Create the CosmoHalo to hold the data and add to vector
        CosmoHalo* halo = new CosmoHalo(id, aliveCount, deadCount);

        halo->setRankID(rank);
        this->allMixedHalos.push_back(halo);

        for (int t = 0; t < this->pmin; t++)
          halo->addParticle(haloBuffer[index++]);
      }
      notReceived--;
    }

    cout << "Number of halos to merge: " << this->allMixedHalos.size() << endl;
  }

  // Other processors bundle up mixed and send to MASTER
  else {
    int index = 0;
    if (this->numberOfMixedHalos > 0) {
      haloBuffer[index++] = this->myProc;
      haloBuffer[index++] = this->numberOfMixedHalos;

      for (unsigned int h = 0; h < this->myMixedHalos.size(); h++) {
        if (this->myMixedHalos[h]->getValid() == UNMARKED) {

          haloBuffer[index++] = this->myMixedHalos[h]->getHaloID();
          haloBuffer[index++] = this->myMixedHalos[h]->getAliveCount();
          haloBuffer[index++] = this->myMixedHalos[h]->getDeadCount();

          vector<ID_T>* tags = this->myMixedHalos[h]->getTags();
          for (int i = 0; i < this->pmin; i++) {
            haloBuffer[index++] = (*tags)[i];
          }
        }
      }
      MPI_Request request;
#ifdef ID_64
      MPI_Isend(haloBuffer, haloBufSize, MPI_LONG, MASTER,
                0, cosmologytools::Partition::getComm(), &request);
#else
      MPI_Isend(haloBuffer, haloBufSize, MPI_INT, MASTER,
                0, cosmologytools::Partition::getComm(), &request);
#endif
    }
  }

#endif // USE_SERIAL_COSMO
}

/////////////////////////////////////////////////////////////////////////
//
// MASTER has collected all the mixed halos and decides which processors
// will get which by matching them up
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::assignMixedHalos()
{
  // MASTER has all data and runs algorithm to make decisions
  if (this->myProc == MASTER) {


#ifdef DEBUG
    for (int m = 0; m < (int) this->allMixedHalos.size(); m++) {
      vector<ID_T>* tags = this->allMixedHalos[m]->getTags();
      cout << "Mixed Halo " << m << ": "
           << " rank=" << this->allMixedHalos[m]->getRankID()
           << " index=" << this->allMixedHalos[m]->getHaloID()
           << " tag=" << (*tags)[0]
           << " alive=" << this->allMixedHalos[m]->getAliveCount()
           << " dead=" << this->allMixedHalos[m]->getDeadCount() << endl;
    }
#endif

    // Iterate over mixed halo vector and match and mark
    // Remember that I can have 3 or 4 that match
    for (unsigned int m = 0; m < this->allMixedHalos.size(); m++) {

      // If this halo has not already been paired with another
      if (this->allMixedHalos[m]->getPartners()->empty() == true) {

        // Current mixed halo has the most alive particles so far
        int numberAlive = this->allMixedHalos[m]->getAliveCount();
        int haloWithLeastAlive = m;

        // Iterate on the rest of the mixed halos
        unsigned int n = m + 1;
        while (n < this->allMixedHalos.size()) {

          // Compare to see if there are a number of tags in common
          int match = compareHalos(this->allMixedHalos[m],
                                   this->allMixedHalos[n]);

          // Keep track of the mixed halo with the most alive particles
          if (match > 0) {
            if (numberAlive > this->allMixedHalos[n]->getAliveCount()) {
              numberAlive = this->allMixedHalos[n]->getAliveCount();
              haloWithLeastAlive = n;
            }
            this->allMixedHalos[m]->addPartner(n);
            this->allMixedHalos[n]->addPartner(m);
            this->allMixedHalos[m]->setValid(INVALID);
            this->allMixedHalos[n]->setValid(INVALID);
          }
          n++;
        }
        // Mixed halo with the least alive particles gets it as VALID
        this->allMixedHalos[haloWithLeastAlive]->setValid(VALID);
      }
    }

#ifdef DEBUG
    for (unsigned int m = 0; m < this->allMixedHalos.size(); m++) {

      cout << "Mixed Halo " << m;
      if (this->allMixedHalos[m]->getValid() == VALID)
        cout << " is VALID on "
             << " Rank " << this->allMixedHalos[m]->getRankID();

      cout << " partners with ";
      set<int>::iterator iter;
      set<int>* partner = this->allMixedHalos[m]->getPartners();
      for (iter = partner->begin(); iter != partner->end(); ++iter)
        cout << (*iter) << " ";
      cout << endl;
    }
#endif
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Compare the tags of two halos to see if they are somewhat the same
// This needs to be made better
//
/////////////////////////////////////////////////////////////////////////

int CosmoHaloFinderP::compareHalos(CosmoHalo* halo1, CosmoHalo* halo2)
{
  vector<ID_T>* member1 = halo1->getTags();
  vector<ID_T>* member2 = halo2->getTags();

  int numFound = 0;
  for (unsigned int i = 0; i < member1->size(); i++) {
    bool done = false;
    unsigned int j = 0;
    while (!done &&
           (*member1)[i] >= (*member2)[j] &&
           j < member2->size()) {
      if ((*member1)[i] == (*member2)[j]) {
        done = true;
        numFound++;
      }
      j++;
    }
  }
  if (numFound == 0)
    return numFound;
  else
    return numFound;
}

/////////////////////////////////////////////////////////////////////////
//
// MASTER sends the result of the merge back to the processors which
// label their previously UNMARKED mixed halos as VALID or INVALID
// VALID halos have all their particles made ALIVE for output
// INVALID halos have all their particles made DEAD because other
// processors will report them
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::sendMixedHaloResults
#ifdef USE_SERIAL_COSMO
(ID_T* haloBuffer, int)
#else
(ID_T* haloBuffer, int haloBufSize)
#endif
{
#ifndef USE_SERIAL_COSMO
  // MASTER sends merge results to all processors
  if (this->myProc == MASTER) {
#endif

    // Share the information
    // Send to each processor the rank, id, and valid status
    // Use the same haloBuffer
    int index = 0;
    haloBuffer[index++] = (ID_T)this->allMixedHalos.size();
    for (unsigned int m = 0; m < this->allMixedHalos.size(); m++) {
      haloBuffer[index++] = this->allMixedHalos[m]->getRankID();
      haloBuffer[index++] = this->allMixedHalos[m]->getHaloID();
      haloBuffer[index++] = this->allMixedHalos[m]->getValid();
    }

#ifndef USE_SERIAL_COSMO
    MPI_Request request;
    for (int proc = 1; proc < this->numProc; proc++) {
#ifdef ID_64
      MPI_Isend(haloBuffer, haloBufSize, MPI_LONG, proc,
                0, cosmologytools::Partition::getComm(), &request);
#else
      MPI_Isend(haloBuffer, haloBufSize, MPI_INT, proc,
                0, cosmologytools::Partition::getComm(), &request);
#endif
    }
#endif

    // MASTER must claim the mixed halos assigned to him
    for (unsigned int m = 0; m < this->allMixedHalos.size(); m++) {
      if (this->allMixedHalos[m]->getRankID() == MASTER &&
          this->allMixedHalos[m]->getValid() == VALID) {

        // Locate the mixed halo in question
        for (unsigned int h = 0; h < this->myMixedHalos.size(); h++) {
          int id = this->myMixedHalos[h]->getHaloID();
          if (id == this->allMixedHalos[m]->getHaloID()) {
            this->myMixedHalos[h]->setValid(VALID);
            int newAliveParticles = this->myMixedHalos[h]->getAliveCount() +
                                    this->myMixedHalos[h]->getDeadCount();
            this->numberOfHaloParticles += newAliveParticles;
            this->numberOfAliveHalos++;

            // Add this halo to valid halos on this processor for
            // subsequent halo properties analysis
            this->halos.push_back(this->haloStart[id]);
            this->haloCount.push_back(newAliveParticles);

            // Output trick - since the status of this particle was marked MIXED
            // when it was added to the mixed CosmoHalo vector, and now it has
            // been declared VALID, change it to ALIVE even if it was dead
            vector<ID_T>* particles = this->myMixedHalos[h]->getParticles();
            vector<ID_T>::iterator iter;
            for (iter = particles->begin(); iter != particles->end(); ++iter)
              this->status[(*iter)] = ALIVE;
          }
        }
      }
    }

#ifndef USE_SERIAL_COSMO
  }

  // Other processors wait for result and adjust their halo vector
  else {
    MPI_Status mpistatus;
#ifdef ID_64
    MPI_Recv(haloBuffer, haloBufSize, MPI_LONG, MASTER,
             0, cosmologytools::Partition::getComm(), &mpistatus);
#else
    MPI_Recv(haloBuffer, haloBufSize, MPI_INT, MASTER,
             0, cosmologytools::Partition::getComm(), &mpistatus);
#endif

    // Unpack information to see which of mixed halos are still valid
    int index = 0;
    int numMixed = haloBuffer[index++];
    for (int m = 0; m < numMixed; m++) {
      int rank = haloBuffer[index++];
      int id = haloBuffer[index++];
      int valid = haloBuffer[index++];

      // If this mixed halo is on my processor
      if (rank == this->myProc && valid == VALID) {

        // Locate the mixed halo in question
        for (unsigned int h = 0; h < this->myMixedHalos.size(); h++) {
          if (this->myMixedHalos[h]->getHaloID() == id) {
            this->myMixedHalos[h]->setValid(VALID);
            int newAliveParticles = this->myMixedHalos[h]->getAliveCount() +
                                    this->myMixedHalos[h]->getDeadCount();
            this->numberOfHaloParticles += newAliveParticles;
            this->numberOfAliveHalos++;

            // Add this halo to valid halos on this processor for
            // subsequent halo properties analysis
            this->halos.push_back(this->haloStart[id]);
            this->haloCount.push_back(newAliveParticles);

            // Output trick - since the status of this particle was marked MIXED
            // when it was added to the mixed CosmoHalo vector, and now it has
            // been declared VALID, change it to ALIVE even if it was dead
            vector<ID_T>* particles = this->myMixedHalos[h]->getParticles();
            vector<ID_T>::iterator iter;
            for (iter = particles->begin(); iter != particles->end(); ++iter)
              this->status[(*iter)] = ALIVE;
          }
        }
      }
    }
  }
#endif // USE_SERIAL_COSMO
}

/////////////////////////////////////////////////////////////////////////
//
// Write the output of the halo finder in the form of the input .cosmo file
//
// Encoded mixed halo VALID or INVALID into the status array such that
// ALIVE particles that are part of an INVALID mixed array will not write
// but DEAD particles that are part of a VALID mixed array will be written
//
// In order to make the output consistent with the serial output where the
// lowest tagged particle in a halo owns the halo, work must be done to
// identify the lowest tag.  This is because as particles are read onto
// this processor using the round robin read of every particle, those
// particles are no longer in tag order.  When the serial halo finder is
// called it has to use the index of the particle on this processor which
// is no longer the tag.
//
//      p    haloTag     tag    haloSize
//      0          0     523           3
//      1          0     522           0
//      2          0     266           0
//
// In the above example the halo will be credited to 523 instead of 266
// because the index of 523 is 0 and the index of 266 is 2.  So we must
// make a pass to map the indexes.
//
/////////////////////////////////////////////////////////////////////////

#ifndef COSMO_USE_GENERIC_IO
void CosmoHaloFinderP::writeTaggedParticles(int, float, bool, bool)
{
}
#else
void CosmoHaloFinderP::writeTaggedParticles(int hmin, float ss, bool writePV,
                                            bool clearTag)
{
  // Map the index of the particle on this process to the index of the
  // particle with the lowest tag value so that the written output refers
  // to the lowest tag as being the owner of the halo
  int* mapIndex = new int[this->particleCount];
  for (int p = 0; p < this->particleCount; p++)
    mapIndex[p] = p;

  // If the tag for the first particle of this halo is bigger than the tag
  // for this particle, change the map to identify this particle as the lowest
  for (int p = 0; p < this->particleCount; p++) {
    if (this->tag[mapIndex[this->haloTag[p]]] > this->tag[p])
      mapIndex[this->haloTag[p]] = p;
  }

  if (hmin == 0 && ss == 1.0) {
    ID_T *particleHaloTag = new ID_T[this->particleCount];
    for (int p = 0; p < this->particleCount; p++) {
      particleHaloTag[p] = (this->haloSize[this->haloTag[p]] < this->pmin)
        ? -1: this->tag[this->haloTag[p]];
    }

    // Write the tagged particle file
    GenericIO GIO(Partition::getComm(), outFile + ".haloparticles");
    GIO.setNumElems(this->particleCount);
    if (writePV) {
      GIO.addVariable("x", this->xx);
      GIO.addVariable("y", this->yy);
      GIO.addVariable("z", this->zz);
      GIO.addVariable("vx", this->vx);
      GIO.addVariable("vy", this->vy);
      GIO.addVariable("vz", this->vz);
    }
    GIO.addVariable("id", this->tag);
    GIO.addVariable("fof_halo_tag", particleHaloTag);
    GIO.write();

    delete [] particleHaloTag;
  } else {
    vector<ID_T> ssTag, ssParticleHaloTag;
    vector<POSVEL_T> ssX, ssY, ssZ, ssVX, ssVY, ssVZ;

    size_t reserveSize = (size_t) (ss*this->particleCount);
    ssTag.reserve(reserveSize);
    ssParticleHaloTag.reserve(reserveSize);
    if (writePV) {
      ssX.reserve(reserveSize);
      ssY.reserve(reserveSize);
      ssZ.reserve(reserveSize);
      ssVX.reserve(reserveSize);
      ssVY.reserve(reserveSize);
      ssVZ.reserve(reserveSize);
    }

    for (int p = 0; p < this->particleCount; p++) {
      if (this->haloSize[this->haloTag[p]] < hmin)
        continue;

      if (drand48() > ss)
        continue;

      ssTag.push_back(this->tag[p]);
      ssParticleHaloTag.push_back(
        (this->haloSize[this->haloTag[p]] < this->pmin)
        ? -1: this->tag[this->haloTag[p]]);
      if (writePV) {
        ssX.push_back(this->xx[p]);
        ssY.push_back(this->yy[p]);
        ssZ.push_back(this->zz[p]);
        ssVX.push_back(this->vx[p]);
        ssVY.push_back(this->vy[p]);
        ssVZ.push_back(this->vz[p]);
      }
    }

    // Write the tagged particle file
    GenericIO GIO(Partition::getComm(), outFile + ".haloparticles");
    GIO.setNumElems(ssTag.size());
    if (writePV) {
      GIO.addVariable("x", ssX);
      GIO.addVariable("y", ssY);
      GIO.addVariable("z", ssZ);
      GIO.addVariable("vx", ssVX);
      GIO.addVariable("vy", ssVY);
      GIO.addVariable("vz", ssVZ);
    }
    GIO.addVariable("id", ssTag);
    GIO.addVariable("fof_halo_tag", ssParticleHaloTag);
    GIO.write();
  }

  delete [] mapIndex;

  // Clear the data stored in serial halo finder
  if (clearTag) {
    clearHaloTag();
    clearHaloSize();
  }
}
#endif

} /* end cosmologytools namespace */
