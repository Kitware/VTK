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

// .NAME SubHaloFinder - calculate subhalos within individual FOF halos
//
// SubHaloFinder takes data from CosmoHaloFinderP about individual halos
// and data from all particles and calculates subhalos.  The output is the
// same form as that from CosmoHaloFinderP so that FOFHaloProperties can
// also be run on subhalos.
//

#ifndef SubHaloFinder_h
#define SubHaloFinder_h

#include "Definition.h"
#include "BHTree.h"
#include <string>
#include <vector>
#include <algorithm>

using std::string;
using std::vector;

namespace cosmologytools {
/////////////////////////////////////////////////////////////////////////
//
// SubHaloCandidate holds information about a possible subhalo
// When combined with a structure holding parent and child information
// allows subhalos within other subhalos, and the location of all particles
// within any level of candidate
//
// Particles are added to candidates based on density and neighbors already
// placed in a candidate.  When two candidates are joined at a saddle point
// those two candidates are closed to new members, and a new candidate is
// created containing those two, but taking new members.  By using this tree
// any arbitrary candidate level can be chosen as a subhalo, and all particles
// belonging to that candidate and child candidates can be retrieved.
//
// If a particle to be placed identifies a neighbor which is in a closed
// candidate, the parent links are followed up to find the open candidate
// This allows neighbors in two subcandidates to note that they are members
// of the same parent candidate.
//
/////////////////////////////////////////////////////////////////////////

class COSMO_EXPORT SubHaloCandidate {
public:
  ID_T first;		// Index into linked list of all particles for
			// retrieving particles belonging to candidate
  int top;		// Top most candidate which is open to new particles
  int partner;
  int cut;
  int parent;		// Index into vector of candidates to find
			// new group made after merge of two
			// candidates at a saddle point
  int child1;		// Index into vector fo candidates
  int child2;		// Index into vector fo candidates
  int count;		// Number of particles in candidate
  int totalCount;	// Number of particles in candidate and children
};

/////////////////////////////////////////////////////////////////////////
//
// SubHaloFinder takes the FOF halos and particle information, builds
// a Barnes Hut octree from the particles of each halo, calculates
// smoothing length and density for each particle, and rearranges those
// particle into subhalo based on local over-dense regions.
//
/////////////////////////////////////////////////////////////////////////

class COSMO_EXPORT SubHaloFinder {
public:
  SubHaloFinder();
  ~SubHaloFinder();

  // Set parameters
  void setParameters(
	POSVEL_T avgMass,
	POSVEL_T G,
	POSVEL_T alpha,
	POSVEL_T beta,
	int minCandSize,
	int numSPH,
	int numClosest);

  // Set alive particle vectors which were created elsewhere
  void setParticles(
	ID_T count,
        POSVEL_T* xLoc,
        POSVEL_T* yLoc,
        POSVEL_T* zLoc,
        POSVEL_T* xVel,
        POSVEL_T* yVel,
        POSVEL_T* zVel,
        POSVEL_T* pmass,
        ID_T* id);

  // Create the subhalos found within each FOF halo
  void findSubHalos();

  // Smoothing length and particle density are calculated within BHTree class
  // Using that information calculate the subhalo candidates
  void calculateSubGroups();

  void makeNewCandidate(int p);
  void joinCandidate(int p, int cand1);
  void mergeCandidate(int p, int cand1, int cand2, int top1, int top2);

  void setTopCandidate(int cIndx, int top);
  void combineCandidate(int cand1, int cand2);
  void addParticleToCandidate(int p, int cand);
  void removeParticleFromCandidate(int p, int cand);

  // Remove candidates that are too small even before unbinding
  void removeSmallCandidates();

  // Unbind particles from subhalo candidates
  void unbind();
  void unbindCandidate(int cIndx);
  void unbindParticles(int cIndx);

  // Utilities
  void writeSubhaloCosmoFile(const string& outFile);
  int  collectTotal(int cIndx);
  int  collectAllTotals(int cIndx);
  void printCandidate(int cIndx, int indent);
  void printSubHalo(int cIndx, int indent);
  void createSubhaloStructure();

  int  getNumberOfSubhalos()	{ return this->numberOfSubhalos; }
  int* getSubhaloCount()	{ return this->subhaloCount; }
  int* getSubhalos()		{ return this->subhalos; }
  int* getSubhaloList()		{ return this->particleList; }

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  ID_T   particleCount;         // Total particles on this processor

  POSVEL_T particleMass;	// Average particle mass
  POSVEL_T gravityConstant;	// Gravitational constant
  POSVEL_T potentialFactor;	// Factor in calculating potential energy

  POSVEL_T alphaFactor;		// Factor for cut/grow
  POSVEL_T betaFactor;		// Factor for Poisson noise
  int minCandidateSize;		// Less than this count is absorbed

  int    numberOfSPHNeighbors;	// Number of neighbors for local density
  int    numberOfCloseNeighbors;// Number for subgroup inclusion

  POSVEL_T* xx;                 // X location for particles on this processor
  POSVEL_T* yy;                 // Y location for particles on this processor
  POSVEL_T* zz;                 // Z location for particles on this processor
  POSVEL_T* vx;                 // X velocity for particles on this processor
  POSVEL_T* vy;                 // Y velocity for particles on this processor
  POSVEL_T* vz;                 // Z velocity for particles on this processor
  POSVEL_T* mass;		// Mass of particles on this processor
  ID_T* tag;                    // Id tag for particles on this processor

  // Sorted SPH particle densities and indices
  vector<ValueInfo> data;

  // Information about subhalo candidates found within FOF halos
  vector<SubHaloCandidate*> candidates;
  int candidateCount;		// Index of last candidate created
  int fuzz;			// Index of fuzz candidate
  int* particleList;		// Indices of next particle in candidate
  int* candidateIndx;		// Lowest level candidate particle belongs to

  // Barnes Hut Tree
  BHTree* bhTree;		// Particles organized by location

  int numberOfSubhalos;		// Candidates with valid subhalos
  int* subhaloCount;		// Counts in subhalos found
  int* subhalos;		// Index of first particle in subhalo
};

}
#endif
