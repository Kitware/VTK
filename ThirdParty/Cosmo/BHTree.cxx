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

#ifdef _WIN32
// this is needed before math.h to get M_PI.
# define _USE_MATH_DEFINES
#endif

#include <math.h>

#include "Partition.h"
#include "BHTree.h"

using namespace std;

namespace cosmologytools {


/////////////////////////////////////////////////////////////////////////
//
// SPHParticle contains information about FOF halo particles
//
/////////////////////////////////////////////////////////////////////////

SPHParticle::SPHParticle()
{
  this->density = 0.0;
  this->smoothingLength = 0.0;
  this->parent = -1;
  this->nextNode = -1;
}

/////////////////////////////////////////////////////////////////////////
//
// SPHNode is a region of physical space divided into octants
//
/////////////////////////////////////////////////////////////////////////

SPHNode::SPHNode(POSVEL_T* minLoc, POSVEL_T* maxLoc)
{
  for (int dim = 0; dim < DIMENSION; dim++) {
    this->length[dim] = maxLoc[dim] - minLoc[dim];
    this->center[dim] = minLoc[dim] + this->length[dim] * 0.5;

  }
  for (int i = 0; i < NUM_CHILDREN; i++)
    this->node.child[i] = -1;
}

/////////////////////////////////////////////////////////////////////////
//
// SPHNode constructed from an octant of a parent node
//
/////////////////////////////////////////////////////////////////////////

SPHNode::SPHNode(SPHNode* parent, int oindx)
{
  for (int dim = 0; dim < DIMENSION; dim++) {
    this->length[dim] = parent->length[dim] * 0.5;
  }

  if (oindx & 1)
    this->center[0] = parent->center[0] + this->length[0] * 0.5;
  else
    this->center[0] = parent->center[0] - this->length[0] * 0.5;

  if (oindx & 2)
    this->center[1] = parent->center[1] + this->length[1] * 0.5;
  else
    this->center[1] = parent->center[1] - this->length[1] * 0.5;

  if (oindx & 4)
    this->center[2] = parent->center[2] + this->length[2] * 0.5;
  else
    this->center[2] = parent->center[2] - this->length[2] * 0.5;

  for (int i = 0; i < NUM_CHILDREN; i++)
    this->node.child[i] = -1;
}

/////////////////////////////////////////////////////////////////////////
//
// Barnes Hut Tree
//
/////////////////////////////////////////////////////////////////////////

BHTree::BHTree(
		POSVEL_T* minLoc,
		POSVEL_T* maxLoc,
		ID_T count,
		POSVEL_T* xLoc,
		POSVEL_T* yLoc,
		POSVEL_T* zLoc,
		POSVEL_T* ms,
		POSVEL_T avgMass)
{
  this->minRange = new POSVEL_T[DIMENSION];
  this->maxRange = new POSVEL_T[DIMENSION];

  // Extract the contiguous data block from a vector pointer
  this->particleCount = count;
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->mass = ms;
  this->particleMass = avgMass;

  // Find the grid size of this chaining mesh
  for (int dim = 0; dim < DIMENSION; dim++) {
    this->minRange[dim] = minLoc[dim];
    this->maxRange[dim] = maxLoc[dim];
  }

  // Create the recursive BH tree from the particle locations
  createBHTree();

  // Thread the recursive tree turning it into an iterative tree
  ID_T rootIndx = this->particleCount;
  ID_T sibling = -1;
  ID_T parent = -1;
  ID_T lastIndx = -1;
  threadBHTree(rootIndx, sibling, parent, &lastIndx);
}

BHTree::~BHTree()
{
  for (ID_T i = 0; i < this->particleCount; i++)
    delete this->sphParticle[i];
  for (ID_T i = 0; i < this->nodeCount; i++)
    delete this->sphNode[i];
}

/////////////////////////////////////////////////////////////////////////
//
// Find the subhalos of the FOF halo using SUBFIND algorithm which
// requires subhalos to be locally overdense and self-bound
//
/////////////////////////////////////////////////////////////////////////

void BHTree::createBHTree()
{
  // Create the SPHParticles
  for (ID_T p = 0; p < this->particleCount; p++) {
    SPHParticle* particle = new SPHParticle;
    this->sphParticle.push_back(particle);
  }

  // Create the root node of the BH tree
  SPHNode* root = new SPHNode(this->minRange, this->maxRange);
  this->sphNode.push_back(root);
  ID_T nodeIndex = 0;

  // Iterate on all particles placing them in the BH tree
  // Child slots in the tree contain the index of the SPHParticle or
  // the index of the SPHNode offset by the number of particles
  // This is so we can use an integer instead of pointers
  // Otherwise we would need a generic pointer to cast as node or particle
  // and a field to indicate type
  //
  for (ID_T pindx = 0; pindx < this->particleCount; pindx++) {

    // Start at root of tree for insertion of a new particle
    // pindx is index into the halo particles where location is stored
    // tindx is index into the BH tree nodes
    // oindx is index into the octant of the tree node
    ID_T tindx = 0;
    int oindx = getChildIndex(this->sphNode[tindx], pindx);

    while (this->sphNode[tindx]->node.child[oindx] != -1) {

      // Child slot in tree contains another SPHNode so go there
      if (this->sphNode[tindx]->node.child[oindx] > this->particleCount) {
        tindx = this->sphNode[tindx]->node.child[oindx] - this->particleCount;
        oindx = getChildIndex(this->sphNode[tindx], pindx);
      }

      // Otherwise there is a particle in the slot and we make a new SPHNode
      else {

        // Get the particle index of particle already in the node
        ID_T pindx2 = this->sphNode[tindx]->node.child[oindx];
        if (this->xx[pindx] == this->xx[pindx2] &&
            this->yy[pindx] == this->yy[pindx2] &&
            this->zz[pindx] == this->zz[pindx2]) {
          cout << "Same particle encountered - SHOULD NOT HAPPEN " << pindx << " and " << pindx2 << endl;
          break;
        }
        
        SPHNode* node = new SPHNode(this->sphNode[tindx], oindx);
        this->sphNode.push_back(node);
        nodeIndex++;
        ID_T tindx2 = nodeIndex;
        
        // Place the node that was sitting there already
        int oindx2 = getChildIndex(this->sphNode[tindx2], pindx2);
        this->sphNode[tindx2]->node.child[oindx2] = pindx2;

        // Add the new SPHNode to the BHTree
        this->sphNode[tindx]->node.child[oindx] = tindx2 + this->particleCount;

        // Set to new node
        tindx = tindx2;
        oindx = getChildIndex(this->sphNode[tindx], pindx);
      }
    }
    // Place the current particle in the BH tree
    this->sphNode[tindx]->node.child[oindx] = pindx;
  }
  this->nodeCount = this->sphNode.size();
}

/////////////////////////////////////////////////////////////////////////
//
// Update the SPHNode vector by walking using a depth first recursion
// Set parent and sibling indices which can replace the child[8] already
// there, and supply extra information about center of mass and avg velocity
// Enters recursion with the root sphNode and walks depth first through child
//
/////////////////////////////////////////////////////////////////////////

void BHTree::threadBHTree(
			ID_T curIndx,
			ID_T sibling,
			ID_T parent,
			ID_T* lastIndx)
{
  ID_T offset = this->particleCount;

  // Set the next index in the threading for node or particle
  // Particles and nodes are threaded together so all are touched in iteration
  if (*lastIndx >= 0) {
    if (*lastIndx >= offset) {
      this->sphNode[(*lastIndx - offset)]->node.info.nextNode = curIndx;
    } else {
      this->sphParticle[*lastIndx]->nextNode = curIndx;
    }
  }
  *lastIndx = curIndx;
 
  // SPHParticle saves only the parent SPHNode
  if (curIndx < offset) {
      this->sphParticle[curIndx]->parent = parent;

  // SPHNode recurses on each of the children
  } else {
    ID_T child[NUM_CHILDREN];
    for (int j = 0; j < NUM_CHILDREN; j++) {
      child[j] = this->sphNode[curIndx-offset]->node.child[j];
    }

    POSVEL_T totalMass = 0.0;
    POSVEL_T s[DIMENSION];
    for (int dim = 0; dim < DIMENSION; dim++) {
      s[dim] = 0.0;
    }

    // Recurse on each of the children, recording information on the way up
    for (int j = 0; j < NUM_CHILDREN; j++) {

      // Process children which have either particle or node in them
      ID_T childIndx, childIndxNext, nextSibling;
      if ((childIndx = child[j]) >= 0) {

        // Check for a sibling on the same level
        int jj;
        for (jj = j + 1; jj < NUM_CHILDREN; jj++)

          if ((childIndxNext = child[jj]) >= 0)
            break;

        // Set sibling to node on this level, or the sibling from the last node
        if (jj < NUM_CHILDREN)
          nextSibling = childIndxNext;
        else
          nextSibling = sibling;

        // Recursion
        threadBHTree(childIndx, nextSibling, curIndx, lastIndx);

        // Return from recursion on childIndx which is a particle or a node
        if (childIndx >= offset) {

          // SPHNode
          totalMass += this->sphNode[childIndx-offset]->node.info.mass;
          for (int dim = 0; dim < DIMENSION; dim++) {
            s[dim] += this->sphNode[childIndx-offset]->node.info.mass * 
                      this->sphNode[childIndx-offset]->node.info.s[dim];
          }

        } else {
          // SPHParticle
          totalMass += this->particleMass;
          s[0] += this->particleMass * this->xx[childIndx];
          s[1] += this->particleMass * this->yy[childIndx];
          s[2] += this->particleMass * this->zz[childIndx];
        }
      }
    }

    if (totalMass) {
      for (int dim = 0; dim < DIMENSION; dim++) {
        s[dim] /= totalMass;
      }
    } else {
      for (int dim = 0; dim < DIMENSION; dim++) {
        s[dim] = this->sphNode[curIndx-offset]->center[dim];
      }
    }
    for (int dim = 0; dim < DIMENSION; dim++) {
      this->sphNode[curIndx-offset]->node.info.s[dim] = s[dim];
    }

    this->sphNode[curIndx-offset]->node.info.mass = totalMass;
    this->sphNode[curIndx-offset]->node.info.sibling = sibling;
    this->sphNode[curIndx-offset]->node.info.parent = parent;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Print BH tree with indentations indicating levels
// Since the tree has been threaded changing the recursive tree with children
// into an iterative tree with next nodes and parents, walk the tree
// iteratively keeping track of parents to indicate when levels change
//
/////////////////////////////////////////////////////////////////////////

void BHTree::printBHTree()
{
  ID_T offset = this->particleCount;
  ID_T curIndex = offset;
  vector<ID_T> parents;
  parents.push_back(-1);
  ID_T parentIndex = 0;

  while (curIndex != -1) {

    // Get the parent of the current index
    ID_T parent;
    if (curIndex >= offset)
      parent = this->sphNode[curIndex - offset]->node.info.parent;
    else
      parent = this->sphParticle[curIndex]->parent;

    // Pop the stack of parents until the level is right
    while (parent != parents[parentIndex]) {
      parents.pop_back();
      parentIndex--;
    }

    // Print SPHNode
    if (curIndex >= offset) {
      cout << parentIndex << ":" << setw(parentIndex) << " ";
      cout << "N " << curIndex 
           << " next " << this->sphNode[curIndex-offset]->node.info.nextNode 
           << " parent " << this->sphNode[curIndex-offset]->node.info.parent 
           << " (" << this->sphNode[curIndex-offset]->node.info.s[0] 
           << " ," << this->sphNode[curIndex-offset]->node.info.s[1] 
           << " ," << this->sphNode[curIndex-offset]->node.info.s[2]
           << ") MASS " << this->sphNode[curIndex-offset]->node.info.mass
           << endl;
        
      // Push back the new SPHNode which will have children
      parents.push_back(curIndex);
      parentIndex++;

      // Walk to next node (either particle or node)
      curIndex = this->sphNode[curIndex-offset]->node.info.nextNode;
    }

    // Print SPHParticle
    else {
      cout << parentIndex << ":" << setw(parentIndex) << " ";
      cout << "P " << curIndex 
           << " next " << this->sphParticle[curIndex]->nextNode 
           << " parent " << this->sphParticle[curIndex]->parent
           << " (" << xx[curIndex]
           << " ," << yy[curIndex]
           << " ," << zz[curIndex] << ")" << endl;

      // Walk to next node (either particle or node)
      curIndex = this->sphParticle[curIndex]->nextNode;
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the initial smoothing length for each SPH particle
// SUBFIND uses the entropy formulation of SPH and not the thermal energy
//
// Some formulations will choose h such that a constant # particles is within
// This formulation chooses h such that
//   (4*PI)/3 * h^3 * est_density = DesNumNgb * particleMass
//   (4*PI)/3 * h^3 * (sphNode.mass / sphNode.len^3) = DesNumNgb * particleMass
//
// Initial guess is found by walking up the parent nodes until finding a
// SPHNode that has at least the minimum number of neighbors in it.  The
// estimated density is based on the cube which is an sphNode.  We just want
// the initial guess to be larger than the actual smoothing length which
// will be calculated in calculateDensity().
//
// h = cube_root(3/(4*PI) * DesNumNgb * particleMass / sphNode.mass) * 
//               sphNode.len
//
/////////////////////////////////////////////////////////////////////////

void BHTree::calculateInitialSmoothingLength(int numberOfNeighbors)
{
  // Walk up parent tree looking for a node of minNeighborMass for
  // calculating the smoothing length guess.  Multiply the requested number
  // of neighbors by a factor to make sure the guess includes the actual.
  POSVEL_T EstimateFactor = 10;
  POSVEL_T minMass = EstimateFactor * numberOfNeighbors * this->particleMass;
  POSVEL_T maxMass = (POSVEL_T) this->particleCount * this->particleMass;
  minMass = min(minMass, maxMass);

  // SPHNodes start numbering after the last particle index number
  POSVEL_T factor1 = 3.0 / (4.0 * M_PI) * 
                     numberOfNeighbors * this->particleMass;
  POSVEL_T onethird = 1.0 / 3.0;
  
  // Calculate smoothing length guess h_i for each particle p_i
  for (ID_T p = 0; p < this->particleCount; p++) {
    ID_T parent = this->sphParticle[p]->parent;

    // Move up parent tree until we have enough neighbors for smoothing
    // Find more neighbors than we actually need
    ID_T node = parent;
    while (node >= this->particleCount && node != -1 &&
         minMass > this->sphNode[parent-this->particleCount]->node.info.mass) { 
      parent = node;
      node = this->sphNode[parent-this->particleCount]->node.info.parent;
    }

    // Get the mass and volume of the parent containing enough particles
    POSVEL_T pLen = max(
                      max(
                        this->sphNode[parent-this->particleCount]->length[0],
                        this->sphNode[parent-this->particleCount]->length[1]),
                      this->sphNode[parent-this->particleCount]->length[2]);
    POSVEL_T pMass = this->sphNode[parent-this->particleCount]->node.info.mass;

    sphParticle[p]->smoothingLength = pow((factor1 / pMass), onethird) * pLen;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the local density for each particle i in the halo
// Guess of smoothing length h_i is provided by calculateInitialSmoothingLength
//
// Density of a particle is found by locating j closest neighbors
//   density_i = Sum_over_j (mass_j * W(r_ij, h_i)
//
// Smoothing kernel is defined as
//   if distance to the neighbor is less than half the smoothing length
//     if 0 <= r/h <= 0.5
//       W(r,h) = (8/(PI*h^3)) * (1 - 6 * (r/h)^2 + 6 * (r/h)^3)
//   if distance to the neighbor is between half and full smoothing length
//     if 0.5 < r/h <= 1.0
//       W(r,h) = (8/(PI*h^3)) * (2 * (1 + (r/h)^3)
//   if distance to the neighbor is more than the smoothing length
//     if r > h
//       W(r,h) = 0
//
/////////////////////////////////////////////////////////////////////////

void BHTree::calculateDensity(int numberOfClosest)
{
  // When calculating density must have a constant mass within the
  // smoothing length sphere.  Use a number slightly bigger than the
  // number of neighbors required for creating subgroups
  ID_T startNode = this->particleCount;

  // Coefficients for cubic spline smoothing kernel for density
  POSVEL_T KERNEL_1 = 8.0 / M_PI;
  POSVEL_T KERNEL_2 = 6.0 * KERNEL_1;
  POSVEL_T KERNEL_3 = 3.0 * KERNEL_2;
  POSVEL_T KERNEL_4 = 2.0 * KERNEL_2;
  POSVEL_T KERNEL_5 = 2.0 * KERNEL_1;
  POSVEL_T KERNEL_6 = -1.0 * KERNEL_2;

  POSVEL_T h0, h, h2, hinv, hinv3, hinv4;
  POSVEL_T rho, divv, weighted_numngb, dhsmlrho;

  // Calculate the density for every particle using smoothing length to
  // locate enough neighbor particles in the BH tree
  for (ID_T p = 0; p < this->particleCount; p++) {

    POSVEL_T pos[DIMENSION];
    pos[0] = this->xx[p];
    pos[1] = this->yy[p];
    pos[2] = this->zz[p];

    // Initial guess at smoothing length which will be refined
    h0 = this->sphParticle[p]->smoothingLength;

    // Find the neighbors of particle within radius of smoothing length h
    // which are ordered by increasing distance
    vector<ValueInfo> neighborList;
    getClosestNeighbors(numberOfClosest, p, pos, h0, startNode, 
                        neighborList);

    // Reset the smoothing length of this particle
    this->sphParticle[p]->smoothingLength = 
      neighborList[numberOfClosest-1].value;

    h = this->sphParticle[p]->smoothingLength;
    h2 = h * h;
    hinv = 1.0 / h;
    hinv3 = hinv * hinv * hinv;
    hinv4 = hinv3 * hinv;

    rho = 0.0;
    divv = 0.0;
    weighted_numngb = 0;
    dhsmlrho = 0;

    // Iterate over the closest neighbors, distance was already calculated
    for (int n = 0; n < numberOfClosest; n++) {
  
      POSVEL_T r = neighborList[n].value;

      // Only do particles within smoothing length
      if (r <= h) {
        POSVEL_T u = r * hinv;
        POSVEL_T wk, dwk;
  
        // Smoothing kernel for cubic spline based on r and h
        // Derivative of smoothing kernel 
        if (u < 0.5) {
          // Neighbor distance is less than half of smoothing length
          wk = hinv3 * (KERNEL_1 + (KERNEL_2 * u * u * (u - 1.0)));
          dwk = hinv4 * u * (KERNEL_3 * u - KERNEL_4);
        }
  
        else {
          // Neighbor distance is greater than half of smoothing length
          wk = hinv3 * KERNEL_5 * (1.0 - u) * (1.0 - u) * (1.0 - u);
          dwk = hinv4 * KERNEL_6 * (1.0 - u) * (1.0 - u);
        }
  
        // Density is accumulated sum of mass * smoothing kernel function
        rho += particleMass * wk;

        dhsmlrho += -particleMass * (DIMENSION * hinv * wk + u * dwk);
      }
    }
    this->sphParticle[p]->density = rho;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Fetch the closest N neighbors of a particle and return ordered
// Use the smoothing length hsml of the particle for an initial guess and
// call getNeighborList to return all particles within a box of
// hsml in each direction from the particle.
//
// Since hsml marks a sphere and not a box, not all particles returned 
// will meet the criteria and will be discarded.  We must return the
// requested N neighbors.  If the initial hsml does not get enough neighbors
// multiply it by a factor and try again until at least N particles are
// within hsml.
//
// We want no more than N neighbors so calculate the distances to each
// neighbor, sort by distance and take the N closest to return.  The
// hsml for the given particle will be set to the distance to the Nth 
// neighbor in the calling calculateDensity() method.
//
// This code can also be used by the subhalo grouping method with a
// smaller N and in this case hsml will not be reset.  Might think about
// saving the closest N neighbors to save recalculation is memory exists.
//
/////////////////////////////////////////////////////////////////////////////

void BHTree::getClosestNeighbors(
			int numberOfClosest,
			ID_T me,
			POSVEL_T pos[DIMENSION],
			POSVEL_T hsml,
			ID_T startNode,
			vector<ValueInfo>& hsmlList)
{
  // Loop until number of neighbors is greater than required amount
  while ((int) hsmlList.size() < numberOfClosest) {

    // Find the neighbors of particle within radius of smoothing length h
    // Can return too many or not enough
    vector<int> neighborList;
    getNeighborList(me, pos, hsml, startNode, neighborList);

    // Collect neigbors from hsml box which are also inside hsml sphere
    for (int n = 0; n < (int) neighborList.size(); n++) {
  
      int neighbor = neighborList[n];
      POSVEL_T dx = pos[0] - this->xx[neighbor];
      POSVEL_T dy = pos[1] - this->yy[neighbor];
      POSVEL_T dz = pos[2] - this->zz[neighbor];
      POSVEL_T r = sqrt(dx * dx + dy * dy + dz * dz);
  
      // Neighbor has to be within the smoothing distance of the particle
      // in order to contribute to density of particle
      if (r < hsml) {
        ValueInfo info;
        info.value = r;
        info.particleId = neighbor;
        hsmlList.push_back(info); 
      }
    }
    // If there aren't enough neighbors widen the smoothing length
    if ((int) hsmlList.size() < numberOfClosest) {
      hsml *= 1.25;
      hsmlList.clear();
    }
  }

  // Sort the neighbors within hsml by increasing distance
  // Return with closest neighbor in vector
  sort(hsmlList.begin(), hsmlList.end(), ValueLT());
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns neighbors with distance <= hsml and returns them in Ngblist. 
// Actually, particles in a box of half side length hsml are
// returned, i.e. the reduction to a sphere still needs to be done in the
// calling routine.
//
/////////////////////////////////////////////////////////////////////////////

void BHTree::getNeighborList(
			int me,
			POSVEL_T searchcenter[DIMENSION],
			POSVEL_T hsml,
			ID_T startNode,
			vector<int>& neighborList)
{
  ID_T no = startNode;
  ID_T offset = this->particleCount;

  POSVEL_T searchmin[DIMENSION], searchmax[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    searchmin[dim] = searchcenter[dim] - hsml;
    searchmax[dim] = searchcenter[dim] + hsml;
  }

  while (no >= 0) {
    if (no < this->particleCount) {
      // SPHParticles
      ID_T p = no;
      no = this->sphParticle[no]->nextNode;
  
      if (p != me &&
          this->xx[p] >= searchmin[0] && this->xx[p] <= searchmax[0] &&
          this->yy[p] >= searchmin[1] && this->yy[p] <= searchmax[1] &&
          this->zz[p] >= searchmin[2] && this->zz[p] <= searchmax[2]) {
        neighborList.push_back(p);
      }
    }

    else {
      // SPHNode
      ID_T nodeIndx = no - offset;
      // Follow the sibling if the entire tree under this node is out of range
      no = this->sphNode[nodeIndx]->node.info.sibling;

      if ((this->sphNode[nodeIndx]->center[0] + 
             0.5 * this->sphNode[nodeIndx]->length[0]) >= searchmin[0] &&
          (this->sphNode[nodeIndx]->center[0] - 
             0.5 * this->sphNode[nodeIndx]->length[0]) <= searchmax[0] &&

          (this->sphNode[nodeIndx]->center[1] + 
             0.5 * this->sphNode[nodeIndx]->length[1]) >= searchmin[1] &&
          (this->sphNode[nodeIndx]->center[1] - 
             0.5 * this->sphNode[nodeIndx]->length[1]) <= searchmax[1] &&

          (this->sphNode[nodeIndx]->center[2] + 
             0.5 * this->sphNode[nodeIndx]->length[2]) >= searchmin[2] &&
          (this->sphNode[nodeIndx]->center[2] - 
             0.5 * this->sphNode[nodeIndx]->length[2]) <= searchmax[2]) {

        // Node has area which intersects the search area
        no = this->sphNode[nodeIndx]->node.info.nextNode;
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Get the index of the child which should contain this particle
//
/////////////////////////////////////////////////////////////////////////

int BHTree::getChildIndex(SPHNode* node, ID_T pindx)
{
  int index = 0;
  if (this->xx[pindx] > node->center[0]) 
    index += 1;
  if (this->yy[pindx] > node->center[1]) 
    index += 2;
  if (this->zz[pindx] > node->center[2]) 
    index += 4;
  return index;
}

} /* end namespace cosmologytools */
