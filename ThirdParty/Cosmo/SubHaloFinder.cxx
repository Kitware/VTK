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
#include "SubHaloFinder.h"
#include "FOFHaloProperties.h"
#include "HaloCenterFinder.h"

using namespace std;

namespace cosmologytools {
/////////////////////////////////////////////////////////////////////////
//
// SubHaloFinder uses the results of the CosmoHaloFinder to locate the
// particles belonging to each FOF halo and then to segment those particles
// into subhalos and fuzz
//
/////////////////////////////////////////////////////////////////////////

SubHaloFinder::SubHaloFinder()
{
  // Get the number of processors and rank of this processor
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();

  this->candidateCount = 0;
  this->bhTree = 0;

  this->particleList = 0;
  this->candidateIndx = 0;
  this->subhaloCount = 0;
  this->subhalos = 0;
}

SubHaloFinder::~SubHaloFinder()
{
  if (this->particleList != 0) delete [] this->particleList;
  if (this->candidateIndx != 0) delete [] this->candidateIndx;
  if (this->bhTree != 0) delete this->bhTree;
  if (this->subhaloCount != 0) delete [] this->subhaloCount;
  if (this->subhalos != 0) delete [] this->subhalos;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the parameters for algorithms
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::setParameters(
			POSVEL_T avgMass,
			POSVEL_T G,
			POSVEL_T alpha,
			POSVEL_T beta,
			int minCandSize,
			int numSPH,
			int numClose)
{
  this->particleMass = avgMass;
  this->gravityConstant = G;
  this->alphaFactor = 1.0 / alpha;
  this->betaFactor = beta;

  this->minCandidateSize = minCandSize;

  this->numberOfSPHNeighbors = numSPH;
  this->numberOfCloseNeighbors = numClose;
  this->potentialFactor = this->particleMass * this->gravityConstant;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::setParticles(
			ID_T count,
                        POSVEL_T* xLocHalo,
                        POSVEL_T* yLocHalo,
                        POSVEL_T* zLocHalo,
                        POSVEL_T* xVelHalo,
                        POSVEL_T* yVelHalo,
                        POSVEL_T* zVelHalo,
                        POSVEL_T* pmass,
                        ID_T* id)
{
  this->particleCount = count;
  this->xx = xLocHalo;
  this->yy = yLocHalo;
  this->zz = zLocHalo;
  this->vx = xVelHalo;
  this->vy = yVelHalo;
  this->vz = zVelHalo;
  this->mass = pmass;
  this->tag = id;
}

/////////////////////////////////////////////////////////////////////////
//
// Find the subhalos contained in a body of particles
// This is the basic algorithm described in the HSF paper
//
//   i) Calculate local density for each particle
//      This is done by building a BHTree, calculating the smoothing length,
//      and finally calculating a local density using SPH
//
//  ii) Sort by density
//      Iterate over every particle high density to low
//      Find two closest already placed neighbors (higher density neighbors)
//
//      a) 0 neighbors - make a new structure
//
//      b) 1 neighbor  - join to the neighbor structure
//         2 neighbors in same structure - join to that structure
//         2 neighbors in different structures with one being the massive
//           partner of the other, join to massive partner (boundary particle)
//
//      c) 2 neighbors in different structures, neither massive partner of other
//           this is a saddle point (connects 2 branches together in tree)
//           mark one structure as massive compared to other, or mark as same
//         Calculate significance of both structures
//
//         1) If one structure is not significant COMBINE into the other
//            If neither is significant COMBINE smaller into larger
//
//         2) If both are significant
//              If one is alphaFactor times larger than the other
//                 CUT the smaller (meaning keep it but don't add new particles)
//                 All particles below the saddle point which have these
//                 2 structures will be added to the massive partner
//              If not larger consider structures the same size
//                 but still mark the larger as the massive partner and
//                 add the saddle point to the massive partner
//                 but both remain open so both GROW
//
// iii) Iterate over all structures created in reverse order
//         structure with less than N_cut particles is considered insignificant
//         and it is combined with its massive partner.
//         If it does not have a massive partner (never put in tree?)
//         then mark as fuzz
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::findSubHalos()
{
  // Find the range of particles
  POSVEL_T minLoc[DIMENSION];
  POSVEL_T maxLoc[DIMENSION];
  minLoc[0] = maxLoc[0] = this->xx[0];
  minLoc[1] = maxLoc[1] = this->yy[0];
  minLoc[2] = maxLoc[2] = this->zz[0];

  for (int i = 1; i < this->particleCount; i++) {
    if (minLoc[0] > this->xx[i]) minLoc[0] = this->xx[i];
    if (maxLoc[0] < this->xx[i]) maxLoc[0] = this->xx[i];
    if (minLoc[1] > this->yy[i]) minLoc[1] = this->yy[i];
    if (maxLoc[1] < this->yy[i]) maxLoc[1] = this->yy[i];
    if (minLoc[2] > this->zz[i]) minLoc[2] = this->zz[i];
    if (maxLoc[2] < this->zz[i]) maxLoc[2] = this->zz[i];
  }

  // BHTree is constructed from halo particles
  this->bhTree = new BHTree(minLoc, maxLoc, 
                            this->particleCount,
                            this->xx, this->yy, this->zz, this->mass,
                            this->particleMass);

  // Calculate smoothing length for density calculation
  this->bhTree->calculateInitialSmoothingLength(this->numberOfSPHNeighbors);

  // Refine smoothing length and calculate local density of particles
  this->bhTree->calculateDensity(this->numberOfSPHNeighbors);

  vector<SPHParticle*>& sphParticle = this->bhTree->getSPHParticle();

  for (int p = 0; p < this->particleCount; p++) {
    ValueInfo info;
    info.value = sphParticle[p]->density;
    info.particleId = p;
    this->data.push_back(info);
  }
  sort(this->data.begin(), this->data.end(), ValueGT());

  calculateSubGroups();

  // Collect the totals across all nodes
  int numberOfCandidates = this->candidates.size();
  int cIndx = numberOfCandidates - 1;
  while (cIndx >= 0 && this->candidates[cIndx]->parent == -1) {
    collectAllTotals(cIndx);
    cIndx--;
  }

#ifdef DEBUG
  // Print the initial candidate tree before unbinding
  cout << "CANDIDATE TREE" << endl;
  cIndx = numberOfCandidates - 1;
  while (cIndx >= 0 && this->candidates[cIndx]->parent == -1) {
    printCandidate(cIndx, 0);
    cIndx--;
  }
#endif

  // Move candidates that are too small to massive partner or to fuzz
  // Move unattached candidates to fuzz if too small
  removeSmallCandidates();

  // Unbind remaining candidates
  unbind();

#ifdef DEBUG
  // Print the candidate tree after unbinding
  cout << "POST UNBIND CANDIDATE TREE" << endl;
  cIndx = numberOfCandidates - 1;
  while (cIndx >= 0 && this->candidates[cIndx]->parent == -1) {
    printCandidate(cIndx, 0);
    cIndx--;
  }
#endif

#ifdef DEBUG
  // Print the candidate tree after unbinding
  cout << "SUBHALO SUMMARY " << endl;
  cIndx = numberOfCandidates - 1;
  while (cIndx >= 0 && this->candidates[cIndx]->parent == -1) {
    printSubHalo(cIndx, 0);
    cIndx--;
  }
#endif

  // Summarize FOF halo's subhalos in the same type of structure
  // suitable for running with FOFHaloProperties.  Particle indices are
  // relative to this FOF halo and will need actualIndx to allow all
  // subhalos to be grouped together
  createSubhaloStructure();
}

/////////////////////////////////////////////////////////////////////////
//
// i) Calculate the subgroups of particles which have been identified as a halo
//    Particles have a local density already calculated through SPH algorithm
//
// ii) Sort all particles in decreasing order of density so that high density
//     particles will be placed in candidates first.
//
// Create the following data structures:
//   "candidateIndx" array which indicates which candidate a particle belongs to
//   "particleList" array which through indirect addressing can locate all
//     particles of a single candidate.  Using this structure combined with
//     the member "first" in a candidate allows quick combining of candidates
//   "candidates" array of standalone candidates created
//     candidates are either leaf which will contain particles or branch
//     which maintain structure of a corresponding tree
//   "candidates" tree will be built bottom up by joining leaf candidates into
//     small branches and then into larger branches until the complete
//     tree is formed.  Some leaves may never be attached to the tree.
//
/////////////////////////////////////////////////////////////////////////
//
// For the first particle:
//   Place it in a new candidate in the candidates list
//
// For each successive particle:
//   Collect the closest "numberOfCloseNeighbors" particles (default 20)
//   Of these particles see if any have a higher density than this particle
//     meaning they have already been placed in candidates.
//   Of these neighbors that have been placed consider the closest two only
//     Record the candidate containing each neighbor
//     Record the topmost branch candidate (parent) containing each neighbor
//   Depending on the location of the neighbors within candidates and tree
//     and if the candidate is cut or growing several actions are possible
//
/////////////////////////////////////////////////////////////////////////
//
//   a) Make a new candidate:
//      If none of the close particles have been placed, make a new candidate
//       and add this particle to that candidate
//       Method: makeNewCandidate()
//
/////////////////////////////////////////////////////////////////////////
//
//   b) Join an existing candidate:
//      If there is only one neighbor, or if there are two neighbors 
//        but they are members of the same candidate
//          add this particle to that candidate if it has not been cut
//          otherwise search the massive partners looking for one that 
//            is not cut and add the particle there
//          Method: joinCandidate()
//
//      If there are two neighbors in two different candidates which are 
//        already placed in the same branch in the tree (top is equal)
//        If neither candidate is cut, but one is much smaller than the other
//          cut the smaller candidate and set massive parter to the larger
//          add this particle to the larger candidate
//        If the larger candidate is not cut
//          add this particle to the larger candidate
//        If the larger candidate is cut and the smaller is not
//          add this particle to the smaller candidate
//        If both candidates are cut search the massive partners to find one
//          that is not cut and add the particle there
//        Method: joinCandidate()
//
/////////////////////////////////////////////////////////////////////////
//
//   c) Merge candidates in tree before joining candidate:
//      If there are two neighbors placed in two candidates which are
//        not yet in the tree (neither has a child or a parent) and
//        If the smaller candidate is not significant (beta factor)
//          or if it is much smaller than the larger (alpha factor)
//            Do not alter the tree (larger remains separate)
//            Combine the smaller candidate into the larger candidate
//            Add saddlepoint particle to the larger
//            Method: combineCandidate()
//
//      Otherwise the two candidates must be merged into the tree 
//        with a new branch candidate being created
//        Candidates can be two leaf candidates
//        Candidates can be one leaf candidate and one branch candidate
//        Candidates can be two branch candidates
//
//        Only for leaf candidates set the massive partner
//          of the smaller candidate to the larger candidate
//        If smaller candidate branch is much smaller than the larger
//          cut the smaller candidate and set massive partner to larger
//        If larger candidate branch is much smaller than the smaller
//          cut the larger candidate and set massive partner to smaller
//        Add saddlepoint particle to the uncut candidate
//
/////////////////////////////////////////////////////////////////////////
//
// Make a final candidate which will hold the fuzz which are all particles
//  not bound to any structure
//
/////////////////////////////////////////////////////////////////////////
//
// Note on structures:
//   The "candidates" vector contains all created SubHaloCandidates.
//   An instance of SubHaloCandidate holds the information needed to deduce
//   the tree structure within the "candidates" vector.  It has indices for the
//   parent and each child candidate.  It holds the index of the first
//   particle member of the candidate which can be followed through the
//   "particleList" array to locate all particles in the candidate.
//   It has an index to the top ancestor of the candidate tree which is
//   able to accept new particle members.  This is needed when looking at
//   the two neighbors to see if they are within the same candidate branch.
//   Count of particles in the candidate as well as the count of particles
//   in the entire tree above the candidate are kept.
//
//   The SubHaloCandidate can actually hold particles if it was created by
//   having a new particle placed in it, or it is just a "merge" candidate
//   which holds structure but no actual particles
//
//   The "candidateIndx" array is of particleCount Size.  It indicates which
//   candidate a particle is a member of.  Using this and the "top" variable
//   within the candidate indicates what branch that particle belongs in.
//
//   The "particleList" is of particleCount size.  The SubHaloCandidate points
//   to the index of its first particle member and that position in the 
//   particleList points to the next particle in the candidate.  This allows
//   locating all particles in a single candidate.
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::calculateSubGroups()
{
  vector<SPHParticle*>& sphParticle = this->bhTree->getSPHParticle();

  // Create arrays to hold the subhalo candidate particle information
  this->candidateIndx = new int[this->particleCount];
  this->particleList = new int[this->particleCount];
  for (int p = 0; p < this->particleCount; p++) {
    this->particleList[p] = -1;
    this->candidateIndx[p] = -1;
  }

  // Put the most dense particle into the first subhalo candidate
  makeNewCandidate(0);

  // Iterate over the particles by decreasing density
  for (ID_T p = 1; p < this->particleCount; p++) {

    ID_T particleIndx = this->data[p].particleId;

    // Find neighbors of particle within radius of smoothing length h
    POSVEL_T pos[DIMENSION];
    POSVEL_T h, rho;

    pos[0] = this->xx[particleIndx];
    pos[1] = this->yy[particleIndx];
    pos[2] = this->zz[particleIndx];
    h = sphParticle[particleIndx]->smoothingLength;
    rho = sphParticle[particleIndx]->density;

    vector<ValueInfo> neighborList;
    set<int> possibleGroup;
    set<int>::iterator siter;

    this->bhTree->getClosestNeighbors(this->numberOfCloseNeighbors, 
                                      particleIndx, pos, h, 
                                      this->particleCount, neighborList);

    // Find the neighbors which have already been placed in candidates
    vector<ValueInfo> closeList;
    for (int n = 0; n < this->numberOfCloseNeighbors; n++) {
      int neighbor = neighborList[n].particleId;
      if (rho < sphParticle[neighbor]->density) {
        ValueInfo info;
        info.particleId = neighbor;
        info.value = neighborList[n].value;
        closeList.push_back(info);
      }
    }

    int numNeighbors = closeList.size();
    int cand1 = -1;
    int cand2 = -2;
    int top1 = -1;
    int top2 = -2;

    // If there are two candidates make sure cand1 is the larger
    // Set the candidate index and the top open candidate index
    if (numNeighbors > 0) {
      cand1 = this->candidateIndx[closeList[0].particleId];
      top1 = this->candidates[cand1]->top;
    }
    if (numNeighbors > 1) {
      cand2 = this->candidateIndx[closeList[1].particleId];
      top2 = this->candidates[cand2]->top;

      if (this->candidates[cand1]->count < this->candidates[cand2]->count) {
        int tmp = cand1;
        cand1 = cand2;
        cand2 = tmp;
        tmp = top1;
        top1 = top2;
        top2 = tmp;
      }
    }

    // a) If neither close neighbor is in a candidate make a new candidate
    if (numNeighbors == 0) {
      makeNewCandidate(p);
    }

    // b) Particle has neighbors in one candidate only
    //    Join the particle to that candidate if it is not already cut
    //    Otherwise join to the more massive partner
    else if (numNeighbors == 1 || cand1 == cand2) {

      // Not cut so join directly to the candidate
      if (this->candidates[cand1]->cut == 0) {
        joinCandidate(p, cand1);

      // Candidate is cut so find the massive partner that is still not cut
      } else {
        cand1 = this->candidates[cand1]->partner;
        while (this->candidates[cand1]->cut == 1 && cand1 != -1) {
          cand1 = this->candidates[cand1]->partner;
        }
        joinCandidate(p, cand1);
      }
    }

    // b) Particle has neighbors in two candidates which have been merged
    //    into the same branch of the tree
    //
    //    If neither candidate has been cut, test smaller against the
    //      larger to see if it can be cut, if so cut smaller and set
    //      its massive partner to the larger
    //    Add the particle to the larger candidate
    //    If it has been cut add to the smaller if it was not cut otherwise
    //      search the massive partners
    //
    else if (top1 == top2) {

      // If the smaller candidate is not cut see if it should be cut now
      // and set the massive partner to the larger
      if (this->candidates[cand2]->cut == 0 &&
          this->candidates[cand1]->cut == 0) {
        if (this->candidates[cand1]->count >
            (this->alphaFactor * this->candidates[cand2]->count)) {
          this->candidates[cand2]->cut = 1;
          this->candidates[cand2]->partner = cand1;
        }
      }

      // If the larger is not cut add particle to larger
      if (this->candidates[cand1]->cut == 0) {
        joinCandidate(p, cand1);
      }

      // If larger is cut and smaller is not add particle to smaller
      else if (this->candidates[cand2]->cut == 0) {
        joinCandidate(p, cand2);
      }

      // If both are cut search massive partners of larger
      else {
        cand1 = this->candidates[cand1]->partner;
        while (this->candidates[cand1]->cut == 1 && cand1 != -1) {
          cand1 = this->candidates[cand1]->partner;
        }
        joinCandidate(p, cand1);
      }
    }

    // Particle has neighbors in two candidates
    // Absorb one candidate into the other plus the saddle point
    // or merge the two candidates into a third depending on significance
    else {
      mergeCandidate(p, cand1, cand2, top1, top2);
    }
  }

  // Last candidate holds the fuzz which are unbound particles
  SubHaloCandidate* candidate = new SubHaloCandidate(); 
  this->fuzz = this->candidateCount;
  candidate->top = -1;
  candidate->first = -1;
  candidate->partner = -1;
  candidate->cut = 0;
  candidate->parent = -1;
  candidate->child1 = -1;
  candidate->child2 = -1;
  candidate->count = 1;
  candidate->totalCount = 0;
  this->candidates.push_back(candidate);
}

/////////////////////////////////////////////////////////////////////////
//
// Create a new candidate with the particle
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::makeNewCandidate(int p)
{    
  ID_T particleIndx = this->data[p].particleId;

  SubHaloCandidate* candidate = new SubHaloCandidate(); 
  candidate->top = this->candidateCount;
  candidate->first = particleIndx;
  candidate->partner = -1;
  candidate->cut = 0;
  candidate->parent = -1;
  candidate->child1 = -1;
  candidate->child2 = -1;
  candidate->count = 1;
  candidate->totalCount = 0;

  this->candidates.push_back(candidate);
  candidateIndx[particleIndx] = this->candidateCount;

  this->candidateCount++;
}

/////////////////////////////////////////////////////////////////////////
//
// Join the particle to an existing candidate.
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::joinCandidate(int p, int cand1)
{
  ID_T particleIndx = this->data[p].particleId;

  // Push particle in front of candidate's particles
  // Increment the candidate and set candidate index for this particle
  this->particleList[particleIndx] = this->candidates[cand1]->first;
  this->candidates[cand1]->first = particleIndx;
  this->candidates[cand1]->count++;
  this->candidateIndx[particleIndx] = cand1;
}

/////////////////////////////////////////////////////////////////////////
//
// Add the particle to an existing candidate
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::addParticleToCandidate(int particleIndx, int cIndx)
{
  // Push particle in front of candidate's particles
  // Increment the candidate and set candidate index for this particle
  this->particleList[particleIndx] = this->candidates[cIndx]->first;
  this->candidates[cIndx]->first = particleIndx;
  this->candidates[cIndx]->count++;
  this->candidateIndx[particleIndx] = cIndx;
}

/////////////////////////////////////////////////////////////////////////
//
// Remove the particle from an existing candidate
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::removeParticleFromCandidate(int particleIndx, int cIndx)
{
  // Is it the first particle that must be removed
  int curPart = this->candidates[cIndx]->first;
  int nextPart;
  int nnextPart;

  if (curPart != -1) {
    if (curPart == particleIndx) {
      nextPart = this->particleList[curPart];
      this->candidates[cIndx]->first = nextPart;
      this->candidates[cIndx]->count--;
    } else while (curPart != -1) {
      nextPart = this->particleList[curPart];
      if (nextPart == particleIndx) {
        nnextPart = this->particleList[nextPart];
        this->particleList[curPart] = nnextPart;
        this->candidates[cIndx]->count--;
        curPart = -1;
      }
      curPart = nextPart;
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Particle is a saddle point between two existing candidates
// The candidates either contain particles or are "merge" candidates
// which contain no particles of their own, but their children do
//
// Check the size and significance of smaller candidate to decide whether
// to COMBINE one candidate into the other or to merge two candidates into third
// If merged the candidates can both be allowed to GROW or one may be CUT
// meaning it takes no additional particles but will continue to exist
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::mergeCandidate(int p, 
                                   int cand1, int cand2,
                                   int top1, int top2)
{
  vector<SPHParticle*>& sphParticle = this->bhTree->getSPHParticle();
  ID_T particleIndx = this->data[p].particleId;

  // Is the smaller halo significant, if not absorb its particles into larger
  // Calculate the average density of the small candidate's particles
  int count = candidates[cand2]->count;
  POSVEL_T avgDensity = 0;
  int curPart = this->candidates[cand2]->first;
  while (curPart != -1) {
    avgDensity += sphParticle[curPart]->density;
    curPart = this->particleList[curPart];
  }
  avgDensity /= count;
  POSVEL_T saddleDensity = sphParticle[particleIndx]->density;

  // Use the Poisson Noise beta parameter to decide if smaller is significant
  int significant = 0;
  if (avgDensity > (saddleDensity * (1.0 + betaFactor / sqrt(static_cast<double>(count)))))
    significant = 1;

  // If the smaller candidate has no children or parent it may be absorbed
  int hasChildren2 = 0;
  if (candidates[cand2]->child1 != -1 || candidates[cand2]->parent != -1)
    hasChildren2 = 1;

  // Collect totalCounts of each candidate to decide on CUT or GROW
  int totalCount1 = collectTotal(top1);
  int totalCount2 = collectTotal(top2);
  int cutCandidate1 = 0;
  int cutCandidate2 = 0;

  if (totalCount1 > (this->alphaFactor  * totalCount2))
    cutCandidate2 = 1;
  if (totalCount2 > (this->alphaFactor  * totalCount1))
    cutCandidate1 = 1;

  int removeCandidate2 = 0;
  if (hasChildren2 == 0) {
    if (significant == 0)
      removeCandidate2 = 1;
    else if (cutCandidate2 == 1 && 
             this->candidates[cand2]->count < this->minCandidateSize)
      removeCandidate2 = 1;
  }

  // Small candidate is significant and can't be absorbed
  // c) 2) MERGE two candidates
  //       If one is much smaller than the other CUT it for joins
  //       Any particle to be joined to it should go to the massive partner
  //       If same size, keep both open for GROW
  if (removeCandidate2 == 0) {

    SubHaloCandidate* candidate = new SubHaloCandidate(); 
    candidate->top = this->candidateCount;
    candidate->first = -1;
    candidate->partner = -1;
    candidate->parent = -1;
    candidate->child1 = top1;
    candidate->child2 = top2;
    candidate->count = 0;
    candidate->totalCount = 0;

    // Set the new candidate as the parent of the children
    this->candidates[top1]->parent = this->candidateCount;
    this->candidates[top2]->parent = this->candidateCount;

    // Recurse through all children of children to set new top open candidate
    setTopCandidate(top1, this->candidateCount);
    setTopCandidate(top2, this->candidateCount);

    // Collect totalCounts of each candidate to decide on CUT or GROW
    this->candidates[top1]->totalCount = totalCount1;
    this->candidates[top2]->totalCount = totalCount2;

    // Set the smaller candidate's massive partner to the larger candidate
    if (this->candidates[cand1]->count >
        this->candidates[cand2]->count) {
      if (this->candidates[cand2]->partner == -1)
        this->candidates[cand2]->partner = cand1;
    } else {
      if (this->candidates[cand1]->partner == -1)
        this->candidates[cand1]->partner = cand2;
    }

    // If one branch is smaller, cut the candidate of the other branch
    int saddlePointCand = cand1;
    if (cutCandidate2 == 1) {
      this->candidates[cand2]->cut = 1;
      this->candidates[cand2]->partner = cand1;
      if (this->candidates[cand1]->cut == 0)
        saddlePointCand = cand1;
      else
        saddlePointCand = cand2;
    }

    // If one branch is smaller, cut the candidate of the other branch
    else if (cutCandidate1 == 1) {
      this->candidates[cand1]->cut = 1;
      this->candidates[cand1]->partner = cand2;
      if (this->candidates[cand2]->cut == 0)
        saddlePointCand = cand2;
      else
        saddlePointCand = cand1;
    }

    // Add in saddle point particle
    this->particleList[particleIndx] = this->candidates[saddlePointCand]->first;
    this->candidates[saddlePointCand]->first = particleIndx;
    this->candidates[saddlePointCand]->count++;
    this->candidateIndx[particleIndx] = saddlePointCand;

    // Add new branch candidate to candidate list
    this->candidates.push_back(candidate);
    this->candidateCount++;
  }

  // c) 1) ABSORB smaller candidate into the larger candidate
  else {
    // If the larger candidate has already been cut join to massive partner
    if (this->candidates[cand1]->cut == 1) {
      cand1 = this->candidates[cand1]->partner;
    }

    // Move all particles in small candidate to the large candidate
    combineCandidate(cand1, cand2);

    // Add saddlepoint to cand1
    this->particleList[particleIndx] = this->candidates[cand1]->first;
    this->candidates[cand1]->first = particleIndx;
    this->candidates[cand1]->count++;
    this->candidateIndx[particleIndx] = cand1;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Combine one candidate into another candidate
// All particles in second candidate are moved to the first candidate
// 
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::combineCandidate(int cand1, int cand2)
{
  // Have all particles in cand2 change their candidateIndx to cand1
  int curParticle = this->candidates[cand2]->first;
  while (curParticle != -1) {
    this->candidateIndx[curParticle] = cand1;
    curParticle = this->particleList[curParticle];
  }

  // If the cand1 is empty fill with cand2
  if (this->candidates[cand1]->first == -1) {
    this->candidates[cand1]->first = this->candidates[cand2]->first;
    this->candidates[cand1]->count = this->candidates[cand2]->count;
    this->candidates[cand2]->count = 0;
    this->candidates[cand2]->first = -1;
  }

  else {
    // Find last particle in cand2 (points to -1)
    curParticle = this->candidates[cand2]->first;
    int lastParticle = curParticle;
    while (curParticle != -1) {
      lastParticle = curParticle;
      curParticle = this->particleList[curParticle];
    }

    // Add cand1 list to the end of cand2 list, point to start of cand2 list
    if (lastParticle > -1) {
      this->particleList[lastParticle] = this->candidates[cand1]->first;
      this->candidates[cand1]->first = this->candidates[cand2]->first;
      this->candidates[cand1]->count += this->candidates[cand2]->count;
      this->candidates[cand2]->count = 0;
      this->candidates[cand2]->first = -1;
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Recursively set the enclosing candidate that is open to new particles
// This is so that when a neighbor particle is identified, the group containing
// it can be immediately known.  If two neighbors are in two different
// subcandidates which were joined into a single candidate, we want to know
// they are both in the same one.
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::setTopCandidate(int candidate, int top)
{
  this->candidates[candidate]->top = top;
  int child1 = this->candidates[candidate]->child1;
  int child2 = this->candidates[candidate]->child2;
  if (child1 != -1)
    setTopCandidate(child1, top);

  if (child2 != -1)
    setTopCandidate(child2, top);
}

/////////////////////////////////////////////////////////////////////////
//
// Remove candidates that are too small
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::removeSmallCandidates()
{
  int numberOfCandidates = this->candidates.size();
  for (int cIndx = 0; cIndx < (numberOfCandidates-1); cIndx++) {

    int count = this->candidates[cIndx]->count;
    int partner = this->candidates[cIndx]->partner;

    // Only work with candidates with size, not the merge points
    if (count > 0) {

      if (count < this->minCandidateSize) {
        if (partner >= 0)
          combineCandidate(partner, cIndx);
        else
          combineCandidate(fuzz, cIndx);
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Unbind particles with positive total energy
// For now walk the candidate vector in reverse order building the tree
// Possibly more than one root because not all candidates have to join
// in the end.
//
// total_energy = kinetic_energy + potential_energy
//
// kinetic_energy = (mass * velocity * velocity) / 2	always positive
// potential_energy = mass * potential                  always negative
//
// positive total_energy means the particle is able to escape
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::unbind()
{
  int rootIndx = this->candidates.size() - 2;
  int cIndx = rootIndx;
  while (cIndx >= 0 && this->candidates[cIndx]->parent == -1) {
    unbindCandidate(cIndx);
    cIndx--;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Recursive method to count bound particles on every candidate in tree
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::unbindCandidate(int cIndx)
{
  int child1 = this->candidates[cIndx]->child1;
  int child2 = this->candidates[cIndx]->child2;
  int count = this->candidates[cIndx]->count;

  // Unbind particles in the candidate
  if (count > 0) {
    unbindParticles(cIndx);
  }

  if (child1 != -1 && child2 != -1) {
    if (this->candidates[child1]->totalCount >
        this->candidates[child2]->totalCount) {
      unbindCandidate(child2);
      unbindCandidate(child1);
    } else {
      unbindCandidate(child1);
      unbindCandidate(child2);
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Recursive method to print the subhalo candidate tree with counts
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::printCandidate(int cIndx, int indent)
{
       cout << setw(7) << indent << ": "
       << "Candidate " << setw(7) << cIndx 
       << " total " << setw(8) << this->candidates[cIndx]->totalCount
       << " count " << setw(8) << this->candidates[cIndx]->count
       << " parent " << setw(7) << this->candidates[cIndx]->parent
       << " child1 " << setw(7) << this->candidates[cIndx]->child1
       << " child2 " << setw(7) << this->candidates[cIndx]->child2
       << " partner " << setw(7) << this->candidates[cIndx]->partner
       << " cut " << setw(3) << this->candidates[cIndx]->cut
       << " subhalos " << endl;

  if (this->candidates[cIndx]->child1 != -1)
    printCandidate(this->candidates[cIndx]->child1, indent + 1);

  if (this->candidates[cIndx]->child2 != -1)
    printCandidate(this->candidates[cIndx]->child2, indent + 1);
}

/////////////////////////////////////////////////////////////////////////
//
// Recursive method to print the subhalo candidate tree after unbinding
//
/////////////////////////////////////////////////////////////////////////

void SubHaloFinder::printSubHalo(int cIndx, int indent)
{
  if (this->candidates[cIndx]->count > 0) {
         cout << setw(7) << indent << ": "
         << "Subhalo " << setw(7) << cIndx 
         << " total " << setw(8) << this->candidates[cIndx]->totalCount
         << " count " << setw(8) << this->candidates[cIndx]->count
         << " parent " << setw(7) << this->candidates[cIndx]->parent
         << " child1 " << setw(7) << this->candidates[cIndx]->child1
         << " child2 " << setw(7) << this->candidates[cIndx]->child2
         << " partner " << setw(7) << this->candidates[cIndx]->partner
         << " cut " << setw(3) << this->candidates[cIndx]->cut << endl;
  }

  if (this->candidates[cIndx]->child1 != -1)
    printSubHalo(this->candidates[cIndx]->child1, indent + 1);

  if (this->candidates[cIndx]->child2 != -1)
    printSubHalo(this->candidates[cIndx]->child2, indent + 1);
}

/////////////////////////////////////////////////////////////////////////
//
// Recursive method to gather the total counts for the subhalo candidates
//
/////////////////////////////////////////////////////////////////////////

int SubHaloFinder::collectTotal(int cIndx)
{
  if (this->candidates[cIndx]->child1 == -1)
    return this->candidates[cIndx]->count;

  int totalChild1 = collectTotal(this->candidates[cIndx]->child1);
  int totalChild2 = collectTotal(this->candidates[cIndx]->child2);

  return (totalChild1 + totalChild2);
}

/////////////////////////////////////////////////////////////////////////
//
// Recursive method to gather the total counts for the subhalo candidates
//
/////////////////////////////////////////////////////////////////////////

int SubHaloFinder::collectAllTotals(int cIndx)
{
  int totalChild1 = 0;
  int totalChild2 = 0;

  if (this->candidates[cIndx]->child1 != -1)
    totalChild1 = collectAllTotals(this->candidates[cIndx]->child1);
  if (this->candidates[cIndx]->child2 != -1)
    totalChild2 = collectAllTotals(this->candidates[cIndx]->child2);

  this->candidates[cIndx]->totalCount = this->candidates[cIndx]->count +
                                        totalChild1 + totalChild2;

  return this->candidates[cIndx]->totalCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Count the number of bound particles in a candidate with no children
//
/////////////////////////////////////////////////////////////////////////////

void SubHaloFinder::unbindParticles(int cIndx)
{
  int numberOfParticles = this->candidates[cIndx]->count;
  int numberLeft = numberOfParticles;

  int massivePartner = this->candidates[cIndx]->partner;

  POTENTIAL_T* lpot = new POTENTIAL_T[numberOfParticles];
  int* valid = new int[numberOfParticles];
  int* id = new int[numberOfParticles];
  POSVEL_T* xLoc = new POSVEL_T[numberOfParticles];
  POSVEL_T* yLoc = new POSVEL_T[numberOfParticles];
  POSVEL_T* zLoc = new POSVEL_T[numberOfParticles];
  POSVEL_T* xVel = new POSVEL_T[numberOfParticles];
  POSVEL_T* yVel = new POSVEL_T[numberOfParticles];
  POSVEL_T* zVel = new POSVEL_T[numberOfParticles];
  POSVEL_T* MASS = new POSVEL_T[numberOfParticles];

  // Store the location and velocity information in arrays
  int p = this->candidates[cIndx]->first;
  int indx = 0;
  while (p != -1) {
    xLoc[indx] = this->xx[p];
    yLoc[indx] = this->yy[p];
    zLoc[indx] = this->zz[p];
    xVel[indx] = this->vx[p];
    yVel[indx] = this->vy[p];
    zVel[indx] = this->vz[p];
    MASS[indx] = this->mass[p];
    valid[indx] = 1;
    id[indx] = p;
    p = this->particleList[p];
    indx++;
  }

  int bindDone = 0;
  if (numberLeft > MAX_UNBIND_3)
    bindDone = 1;

  while (numberLeft >= this->minCandidateSize && bindDone == 0) {

    vector<ValueInfo>* totalEnergy = new vector<ValueInfo>[numberLeft];
    
    // Calculate the average velocity of the body of particles
    POSVEL_T xAvg = 0.0;
    POSVEL_T yAvg = 0.0;
    POSVEL_T zAvg = 0.0;
    for (int i = 0; i < numberOfParticles; i++) {
      if (valid[i] == 1) {
        xAvg += xVel[i];
        yAvg += yVel[i];
        zAvg += zVel[i];
      }
    }
    xAvg /= numberLeft;
    yAvg /= numberLeft;
    zAvg /= numberLeft;

    // Calculate the potential of each particle within the body of particles
    for (int i = 0; i < numberOfParticles; i++)
      lpot[i] = 0.0;

    // First particle in halo to calculate minimum potential on
    for (int i = 0; i < numberOfParticles; i++) {

      // Next particle in halo in minimum potential loop
      for (int j = i+1; j < numberOfParticles; j++) {

        if (valid[i] == 1 && valid[j] == 1) {
          POSVEL_T xdist = (POSVEL_T) fabs(xLoc[i] - xLoc[j]);
          POSVEL_T ydist = (POSVEL_T) fabs(yLoc[i] - yLoc[j]);
          POSVEL_T zdist = (POSVEL_T) fabs(zLoc[i] - zLoc[j]);
  
          POSVEL_T r = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
  
          if (r != 0.0) {
            lpot[i] = (POTENTIAL_T)(lpot[i] - (1.0 / r));
            lpot[j] = (POTENTIAL_T)(lpot[j] - (1.0 / r));
          }
        }
      }
    }

    // Calculate total_energy = kinetic_energy + potential+energy
    int positiveTECount = 0;
    for (int i = 0; i < numberOfParticles; i++) {
      if (valid[i] == 1) {
        POSVEL_T XVel = xVel[i] - xAvg;
        POSVEL_T YVel = yVel[i] - yAvg;
        POSVEL_T ZVel = zVel[i] - zAvg;

        POSVEL_T v = sqrt(XVel * XVel + YVel * YVel + ZVel * ZVel);
        POSVEL_T kineticEnergy = (v * v) / 2.0;
        POSVEL_T potentialEnergy = lpot[i] * this->potentialFactor;
        POSVEL_T totEnergy = kineticEnergy + potentialEnergy;

        if (totEnergy > 0.0) {
          positiveTECount++;
          ValueInfo info;
          info.particleId = i;
          info.value = totEnergy;
          (*totalEnergy).push_back(info);
        }
      }
    }

    // Sort the positive total energy along with particle index if any
    // Mark as invalid the top 10% of positive total energy particles
    if (positiveTECount > 0) {
      sort(totalEnergy->begin(), totalEnergy->end(), ValueGT());

      int maxToDelete = 1;
      if (numberLeft > MAX_UNBIND_1 && numberLeft < MAX_UNBIND_2)
        maxToDelete = (positiveTECount / FACTOR_UNBIND_1) + 1;
      else if (numberLeft >= MAX_UNBIND_2 && numberLeft < MAX_UNBIND_3)
        maxToDelete = (positiveTECount / FACTOR_UNBIND_2) + 1;
#ifdef DEBUG
      cout << "Unbind at most " << maxToDelete 
           << " particles numberLeft " << numberLeft 
           << " numberOfParticles " << numberOfParticles
           << " total energy " << (*totalEnergy)[maxToDelete - 1].value << endl;
#endif

      for (int i = 0; i < maxToDelete; i++) {
        int pid = (*totalEnergy)[i].particleId;
        valid[pid] = 0;
      }
      numberLeft -= maxToDelete;

      // If we are almost done and the halo is large enough just quit
      if (numberLeft > MAX_UNBIND_2 && maxToDelete <= MAX_UNBIND_DELETE)
        bindDone = 1;

    } else {
      bindDone = 1;
    }

    totalEnergy->clear();
  }

  // Discard any unbound particles
  int numberOfBoundParticles = numberLeft;

  // Not enough bound particles so move all to the massive partner
  if (numberOfBoundParticles < this->minCandidateSize) {
#ifdef DEBUG
    cout << "\tDISCARD move to partner " << massivePartner 
         << " number of particles " << numberOfBoundParticles << endl;
#endif
    if (massivePartner >= 0)
      combineCandidate(massivePartner, cIndx);
    else
      combineCandidate(this->fuzz, cIndx);
  }

  // Enough bound particles for subhalo, move only the unbound particles
  else {
#ifdef DEBUG
    cout << "UNBIND " << numberOfBoundParticles 
         << " particles to massive partner " << massivePartner << endl;
#endif
    for (int i = 0; i < numberOfParticles; i++) {
      if (valid[i] == 0) {
        removeParticleFromCandidate(id[i], cIndx);

        if (massivePartner != -1) {
          addParticleToCandidate(id[i], massivePartner);
        } else {
          addParticleToCandidate(id[i], this->fuzz);
        }
      }
    }
  }
  delete [] valid;
  delete [] id;
  delete [] xLoc;
  delete [] yLoc;
  delete [] zLoc;
  delete [] xVel;
  delete [] yVel;
  delete [] zVel;
  delete [] MASS;
  delete [] lpot;

#ifdef DEBUG
    cout << "UNBIND CANDIDATE " << setw(7) << cIndx 
         << " totalcount " << setw(8) << this->candidates[cIndx]->totalCount
         << " count " << setw(8) << this->candidates[cIndx]->count
         << " partner " << setw(8) << massivePartner
         << " cut " << this->candidates[cIndx]->cut
         << " #bound " << numberLeft << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
// Write a .cosmo file for ParaView with location, velocity and subhalo
// candidate number in place of the mass variable
//
/////////////////////////////////////////////////////////////////////////////

void SubHaloFinder::writeSubhaloCosmoFile(const string& outFile)
{
  // Collect the candidates with particles for sorting and remapping
  // Fuzz is the last candidate
  int numberOfCandidates = this->candidates.size();
  vector<ValueInfo> groups;

  this->numberOfSubhalos = 0;
  for (int i = 0; i < (numberOfCandidates-1); i++) {
    if (this->candidates[i]->count > 0) {
      this->numberOfSubhalos++;
      ValueInfo info;
      info.particleId = i;
      info.value = (float) this->candidates[i]->count;
      groups.push_back(info);
    }
  }

  // Sort the valid subhalo candidates (not the fuzz candidate) by size
  sort(groups.begin(), groups.end(), ValueGT());

  // Map the actual candidate index into an index based on size
  int* mapCandidate = new int[numberOfCandidates];
  for (int i = 0; i < numberOfCandidates; i++)
    mapCandidate[i] = -1;

  for (int i = 0; i < this->numberOfSubhalos; i++) {
    int oldCandidate = groups[i].particleId;
    mapCandidate[oldCandidate] = i;
#ifdef DEBUG
    cout << "Map candidate " << setw(7) << groups[i].particleId
         << " to subhalo " << setw(7) << i 
         << " with count " << setw(10) << groups[i].value << endl;
#endif
  }

  // Set the fuzz candidate to the last subhalo
  mapCandidate[fuzz] = numberOfSubhalos;
#ifdef DEBUG
  cout << "Map fuzz candidate " << fuzz
       << " to candidate " << numberOfSubhalos 
       << " with count " << this->candidates[fuzz]->count << endl;
#endif

  // Write the particles with mapped candidate numbers
  ofstream cStream(outFile.c_str(), ios::out|ios::binary);

  float fBlock[COSMO_FLOAT];
  int iBlock[COSMO_INT];

  for (int p = 0; p < this->particleCount; p++) {
    fBlock[0] = this->xx[p];
    fBlock[1] = this->vx[p];
    fBlock[2] = this->yy[p];
    fBlock[3] = this->vy[p];
    fBlock[4] = this->zz[p];
    fBlock[5] = this->vz[p];
    fBlock[6] = (float) mapCandidate[this->candidateIndx[p]];
    cStream.write(reinterpret_cast<char*>(fBlock),
                  COSMO_FLOAT * sizeof(POSVEL_T));
    iBlock[0] = this->tag[p];
    cStream.write(reinterpret_cast<char*>(iBlock),
                  COSMO_INT * sizeof(ID_T));
  }
  cStream.close();
  delete [] mapCandidate;
}

/////////////////////////////////////////////////////////////////////////////
//
// Gather candidates that were legal subhalos, renumber starting with 0
// for the largest subhalo, update particleIndx to correspond to the
// found subhalos.  This can be returned such that FOFHaloProperties can
// run on each FOF halo's subhalos.  Also we should be able to put the
// original FOF halos that were too small together with the subhalos of
// other FOF halos into a single structure for display or analysis.
//
/////////////////////////////////////////////////////////////////////////////

void SubHaloFinder::createSubhaloStructure()
{
  // Collect the candidates with particles for sorting and remapping
  // Fuzz is the last candidate and is ignored
  int numberOfCandidates = this->candidates.size() - 1;
  vector<ValueInfo> groups;

  this->numberOfSubhalos = 0;
  for (int i = 0; i < numberOfCandidates; i++) {
    if (this->candidates[i]->count > 0) {
      this->numberOfSubhalos++;
      ValueInfo info;
      info.particleId = i;
      info.value = (float) this->candidates[i]->count;
      groups.push_back(info);
    }
  }

  // Sort the valid subhalo candidates (not the fuzz candidate) by size
  sort(groups.begin(), groups.end(), ValueGT());

  // Map the actual candidate index into an index based on size
  int* mapCandidate = new int[numberOfCandidates];
  for (int i = 0; i < numberOfCandidates; i++)
    mapCandidate[i] = -1;

  for (int i = 0; i < this->numberOfSubhalos; i++) {
    int oldCandidate = groups[i].particleId;
    mapCandidate[oldCandidate] = i;
  }

  // Allocate structures similar to FOF structures for returning answer
  this->subhaloCount = new int[this->numberOfSubhalos];
  this->subhalos = new int[this->numberOfSubhalos];

  // Need array of counts of particles within each subhalo
  for (int cindx = 0; cindx < numberOfCandidates; cindx++) {
    int haloIndx = mapCandidate[cindx];
    if (this->candidates[cindx]->count > 0) {
      this->subhaloCount[haloIndx] = this->candidates[cindx]->count;
      this->subhalos[haloIndx] = this->candidates[cindx]->first;
    }
  }

  // Equivalent of haloList from FOF is the particleList which through
  // indirect addressing locates every particle in a subhalo
  delete [] mapCandidate;
}

}
