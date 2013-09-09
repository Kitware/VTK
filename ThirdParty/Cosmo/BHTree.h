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

// .NAME BHTree - Create a Barnes Hut tree from the given particles
//
// .SECTION Description
// BHTree takes particle locations and distributes them recursively in
// a Barnes Hut tree.  The tree is an octree, dividing on the physical
// location such that one particle or one node appears within a child
// so that it is essentially AMR for particles.
//
// After the tree is created it is walked using depth first recursion and
// the nodes are threaded together so that the tree becomes iterative.
// By stringing nodes together rather than maintaining indices into children
// summary information for each node can replace the 8 integer slots that
// were taken up by the children.  Now each node can maintain the mass
// below, the length of the physical box it represents and the center of
// mass of particles within the node.
//
// Each particle and each node maintains an index for the next node and
// also the parent, so that it is possible to represent the recursive tree
// by paying attention to parents.
//
// SPHParticle is indexed from 0 to number of particles - 1 and the created 
// nodes are numbered from (number of particles) within the tree.  Particles
// and Nodes are maintained in separate vectors so the node can be located
// using the index - number of particles.
//

#ifndef BHTree_h
#define BHTree_h

#include "Definition.h"
#include <vector>
#include <algorithm>

using std::vector;

namespace cosmologytools {
/////////////////////////////////////////////////////////////////////////
//
// Structure for sorting particles on a value
//
/////////////////////////////////////////////////////////////////////////

class COSMO_EXPORT ValueInfo {
public:
  POSVEL_T value;
  ID_T particleId;
};

class COSMO_EXPORT ValueLT {
public:
  bool operator() (const ValueInfo& p, const ValueInfo& q) const
  {
  return p.value < q.value;
  }
};

class COSMO_EXPORT ValueGT {
public:
  bool operator() (const ValueInfo& p, const ValueInfo& q) const
  {
  return p.value > q.value;
  }
};

/////////////////////////////////////////////////////////////////////////
//
// SPH (Smoothed Particle Hydrodynamics) Particles
//
/////////////////////////////////////////////////////////////////////////

class COSMO_EXPORT SPHParticle {
public:
  SPHParticle();

  POSVEL_T density;
  POSVEL_T smoothingLength;

  ID_T      parent;		// Parent SPHNode
  ID_T      nextNode;		// Next node in iteration, particle or node
};

/////////////////////////////////////////////////////////////////////////
//
// SPH (Smoothed Particle Hydrodynamics) Nodes
//
// Barnes Hut octree structure for N-body is represented by vector
// of SPHNodes which divide space into octants which are filled with one
// particle or one branching node.  As the tree is built the child[8]
// array is used.  Afterwards the tree is walked linking the nodes and
// replacing the child structure with data about the tree.  When building
// the tree child information is an integer which is the index of the
// halo particle which was put into a vector of SPHParticle, or the index
// of the SPHNode offset by the number of particles
//
/////////////////////////////////////////////////////////////////////////

class COSMO_EXPORT SPHNode {
public:
  SPHNode(POSVEL_T* minLoc, POSVEL_T* maxLoc);
  SPHNode(SPHNode* parent, int child);

  POSVEL_T length[DIMENSION];		// Length of octant on each side
  POSVEL_T center[DIMENSION];		// Physical center of octant

  union {
    ID_T  child[NUM_CHILDREN];		// Index of particle or node
    struct Info {
      POSVEL_T mass;
      POSVEL_T s[DIMENSION];
      ID_T     sibling;
      ID_T     nextNode;
      ID_T     parent;
    } info;
  } node;
};

/////////////////////////////////////////////////////////////////////////
//
// Barnes Hut octree of SPHParticles and SPHNodes threaded
//
/////////////////////////////////////////////////////////////////////////

class COSMO_EXPORT BHTree {
public:
  BHTree(
        POSVEL_T* minLoc,       // Bounding box of halo
        POSVEL_T* maxLoc,       // Bounding box of halo
        ID_T count,             // Number of particles in halo
        POSVEL_T* xLoc,         // Locations of every particle
        POSVEL_T* yLoc,
        POSVEL_T* zLoc,
	POSVEL_T* mass,		// Mass of each particle
	POSVEL_T avgMass);	// Average mass for estimation

  ~BHTree();

  void createBHTree();

  void threadBHTree(
	ID_T curIndx,
	ID_T sibling,
	ID_T parent,
	ID_T* lastIndx);

  void printBHTree();
	
  void calculateInitialSmoothingLength(
	int numSPHNeighbors);	// Number for local density

  void calculateDensity(
	int numSPHNeighbors);	// Number for local density

  void getClosestNeighbors(
	int numberOfClosest,
	ID_T me,
	POSVEL_T pos[DIMENSION],
	POSVEL_T hsml,
	ID_T startNode,
	vector<ValueInfo>& hsmlList);

  void getNeighborList(
	int me,
	POSVEL_T searchcenter[DIMENSION],
	POSVEL_T hsml,
	ID_T startNode,
	vector<int>& neighborList);

  vector<SPHParticle*>& getSPHParticle()	{ return this->sphParticle; }
  vector<SPHNode*>& getSPHNode()		{ return this->sphNode; }
  ID_T getParticleCount()			{ return this->particleCount; }

  int getChildIndex(SPHNode* node, ID_T pindx);

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  POSVEL_T boxSize;             // Physical box size of the data set
  POSVEL_T deadSize;            // Border size for dead particles
  POSVEL_T bb;			// Interparticle distance for halos

  ID_T   particleCount;         // Total particles
  ID_T   nodeCount;             // Total nodes
  POSVEL_T particleMass;	// Average particle mass

  POSVEL_T* xx;                 // X location for particles on this processor
  POSVEL_T* yy;                 // Y location for particles on this processor
  POSVEL_T* zz;                 // Z location for particles on this processor
  POSVEL_T* mass;               // Mass for particles on this processor

  POSVEL_T* minRange;           // Physical range of data
  POSVEL_T* maxRange;           // Physical range of data

  vector<SPHParticle*> sphParticle;
  vector<SPHNode*>     sphNode;
};

} /* end namespace cosmology tools */
#endif
