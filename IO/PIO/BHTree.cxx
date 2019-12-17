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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "BHTree.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// BHLeaf contains information about stored points
//
/////////////////////////////////////////////////////////////////////////

BHLeaf::BHLeaf()
{
  for (int dim = 0; dim < MAX_DIM; dim++)
    this->location[dim] = 0.0;
}

BHLeaf::BHLeaf(int dimension, double* loc)
{
  for (int dim = 0; dim < dimension; dim++)
    this->location[dim] = loc[dim];
}

bool BHLeaf::sameAs(int dimension, double* loc)
{
  bool same = true;
  for (int dim = 0; dim < dimension; dim++)
    if (location[dim] != loc[dim])
      same = false;
  return same;
}

/////////////////////////////////////////////////////////////////////////
//
// BHNode is a region of physical space divided into octants
//
/////////////////////////////////////////////////////////////////////////

BHNode::BHNode()
{
  for (int dim = 0; dim < MAX_DIM; dim++)
  {
    this->length[dim] = 0.0;
    this->center[dim] = 0.0;
  }
  for (int i = 0; i < MAX_CHILD; i++)
    this->child[i] = 0;
}

BHNode::BHNode(int dimension, int numChild, double* minLoc, double* maxLoc)
{
  for (int dim = 0; dim < dimension; dim++)
  {
    this->length[dim] = maxLoc[dim] - minLoc[dim];
    this->center[dim] = minLoc[dim] + this->length[dim] * 0.5;
  }
  for (int i = 0; i < numChild; i++)
    this->child[i] = 0;
}

/////////////////////////////////////////////////////////////////////////
//
// BHNode constructed from an octant of a parent node
//
/////////////////////////////////////////////////////////////////////////

BHNode::BHNode(int dimension, int numChild, BHNode* parent, int oindx)
{
  for (int dim = 0; dim < dimension; dim++)
  {
    this->length[dim] = parent->length[dim] * 0.5;
  }

  if (dimension == 3)
  {
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
  }
  else if (dimension == 2)
  {
    if (oindx & 1)
      this->center[0] = parent->center[0] + this->length[0] * 0.5;
    else
      this->center[0] = parent->center[0] - this->length[0] * 0.5;

    if (oindx & 2)
      this->center[1] = parent->center[1] + this->length[1] * 0.5;
    else
      this->center[1] = parent->center[1] - this->length[1] * 0.5;
  }

  for (int i = 0; i < numChild; i++)
    this->child[i] = 0;
}

/////////////////////////////////////////////////////////////////////////
//
// Barnes Hut Tree
//
/////////////////////////////////////////////////////////////////////////

BHTree::BHTree(int treeDim, int numChild, double* minLoc, double* maxLoc)
{
  this->dimension = treeDim;
  this->numberOfChildren = numChild;

  // Find the grid size of this chaining mesh
  for (int dim = 0; dim < this->dimension; dim++)
  {
    this->minRange[dim] = minLoc[dim];
    this->maxRange[dim] = maxLoc[dim];
  }

  // Instead of using pointers to leaf and node we encode the type by
  // using leaves (1,2,...) and nodes (-1,-2...) to quickly tell if we
  // have a leaf or a node and where to find it
  // To make this work easily, stick a dummy leaf and node in vector[0]
  BHLeaf* leaf = new BHLeaf();
  this->bhLeaf.push_back(leaf);

  BHNode* node = new BHNode();
  this->bhNode.push_back(node);

  // Create the root node of the BH tree which is in vector[1]
  BHNode* root =
    new BHNode(this->dimension, this->numberOfChildren, this->minRange, this->maxRange);
  this->bhNode.push_back(root);

  this->leafIndex = 0;
  this->nodeIndex = 1;
}

BHTree::~BHTree()
{
  for (int i = 0; i < (this->leafIndex + 1); i++)
    delete this->bhLeaf[i];
  for (int i = 0; i < (this->nodeIndex + 1); i++)
    delete this->bhNode[i];
  this->bhLeaf.clear();
  this->bhNode.clear();
}

/////////////////////////////////////////////////////////////////////////
//
// Look to see if the leaf was already entered into the tree
// If so return the index of that leaf (minus 1 to match ParaView)
// Otherwise create the leaf, insert in tree
//
/////////////////////////////////////////////////////////////////////////

int BHTree::insertLeaf(double* loc)
{
  // Start at root of tree for insertion of a new leaf
  //   tindx is index into the tree nodes where root is at pos 1 for id -1
  //   oindx is index into the octant of the tree node
  int tindx = 1;
  int oindx = getChildIndex(this->bhNode[tindx], loc);

  // Child octant is either another node or a leaf or it is empty
  while (this->bhNode[tindx]->child[oindx] != 0)
  {

    // Child slot in tree contains another BHNode so go there
    if (this->bhNode[tindx]->child[oindx] < 0)
    {
      tindx = (this->bhNode[tindx]->child[oindx] * -1);
      oindx = getChildIndex(this->bhNode[tindx], loc);
    }

    // Otherwise there is a leaf in the slot
    // If it matches the location we want, return the point index
    // If not create a new node, move the old leaf, add the new leaf
    else
    {

      // Get the index of leaf already in the node
      int pindx = this->bhNode[tindx]->child[oindx];
      // double* oldloc = this->bhLeaf[pindx]->location;

      // If it is the same this is the index we return
      if (this->bhLeaf[pindx]->sameAs(this->dimension, loc))
      {
        return (pindx);
      }

      // There is a leaf in the slot but it isn't the one we are looking for
      BHNode* node =
        new BHNode(this->dimension, this->numberOfChildren, this->bhNode[tindx], oindx);
      this->bhNode.push_back(node);
      this->nodeIndex++;
      int tindx2 = this->nodeIndex;

      // Place the node that was sitting there already
      int oindx2 = getChildIndex(this->bhNode[tindx2], bhLeaf[pindx]->location);
      this->bhNode[tindx2]->child[oindx2] = pindx;

      // Add the new BHNode to the BHTree
      this->bhNode[tindx]->child[oindx] = tindx2 * -1;

      // Set to new node
      tindx = tindx2;
      oindx = getChildIndex(this->bhNode[tindx], loc);
    }
  }

  // Place the current particle in the BH tree
  this->leafIndex++;
  BHLeaf* leaf = new BHLeaf(this->dimension, loc);
  this->bhLeaf.push_back(leaf);
  this->bhNode[tindx]->child[oindx] = this->leafIndex;
  return leafIndex;
}

void BHTree::print()
{
  cout << "Number of leaves " << leafIndex << " Number of nodes " << nodeIndex << endl;
  int i, j;
  cout << "LEAVES" << endl;
  for (i = 1; i <= this->leafIndex; i++)
    cout << "   Leaf " << i << " index " << i << " loc " << bhLeaf[i]->location[0] << "     "
         << bhLeaf[i]->location[1] << endl;

  cout << "NODES" << endl;
  for (i = 1; i <= this->nodeIndex; i++)
  {
    cout << "   Node " << i << " index " << (i * -1) << "   children ";
    for (j = 0; j < this->numberOfChildren; j++)
      cout << "  " << bhNode[i]->child[j];
    cout << endl;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Get the index of the child which should contain this particle
//
/////////////////////////////////////////////////////////////////////////

int BHTree::getChildIndex(BHNode* node, double* loc)
{
  int index = 0;

  if (this->dimension == 3)
  {
    if (loc[0] > node->center[0])
      index += 1;
    if (loc[1] > node->center[1])
      index += 2;
    if (loc[2] > node->center[2])
      index += 4;
  }
  else if (this->dimension == 2)
  {
    if (loc[0] > node->center[0])
      index += 1;
    if (loc[1] > node->center[1])
      index += 2;
  }
  return index;
}
