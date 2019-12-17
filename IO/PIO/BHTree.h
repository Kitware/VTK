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

// .NAME BHTree - Create a Barnes Hut tree from the given points
//
// .SECTION Description
// BHTree takes point locations and distributes them recursively in
// a Barnes Hut tree.  The tree is a quadtree or octree, dividing on physical
// location such that one point or one node appears within a child
// so that it is essentially AMR.
//
// This is used to ensure unique points in the vtk data set and so the
// succeeding steps of threading the tree for iteration is not done.
//
// BHLeaf is the actual point with the index matching the vtkPoints index.
// BHNode are negative numbers.  This allows a quick recognition of a leaf
// or a node.  Children numbered with 0 are empty.
//

#ifndef BHTree_h
#define BHTree_h

#include <vector>

const int MAX_DIM = 3;
const int MAX_CHILD = 8;

/////////////////////////////////////////////////////////////////////////
//
// Leaf information
//
/////////////////////////////////////////////////////////////////////////

class BHLeaf
{
public:
  BHLeaf(int dim, double* loc);
  BHLeaf();

  bool sameAs(int dim, double* loc);

  double location[MAX_DIM];
};

/////////////////////////////////////////////////////////////////////////
//
// BH node information
//
// Barnes Hut octree structure for N-body is represented by vector
// of BHNode which divide space into octants which are filled with one
// particle or one branching node.  As the tree is built the child[8]
// array is used.  Afterwards the tree is walked linking the nodes and
// replacing the child structure with data about the tree.  When building
// the tree child information is an integer which is the index of the
// halo particle which was put into a vector of BHLeaf, or the index
// of the BHNode offset by the number of particles
//
/////////////////////////////////////////////////////////////////////////

class BHNode
{
public:
  BHNode();
  BHNode(int dim, int numChild, double* minLoc, double* maxLoc);
  BHNode(int dim, int numChild, BHNode* parent, int child);

  double length[MAX_DIM]; // Length of octant on each side
  double center[MAX_DIM]; // Physical center of octant
  int child[MAX_CHILD];   // Index of leaf or node
};

/////////////////////////////////////////////////////////////////////////
//
// Barnes Hut Tree
//
/////////////////////////////////////////////////////////////////////////

class BHTree
{
public:
  BHTree(int dimension, int numChild,
    double* minLoc,  // Bounding box of tree
    double* maxLoc); // Bounding box of tree
  ~BHTree();

  int insertLeaf(double* loc);
  int getChildIndex(BHNode* node, double* loc);
  void print();

private:
  int dimension;
  int numberOfChildren;
  int leafIndex;
  int nodeIndex;

  double minRange[MAX_DIM]; // Physical range of data
  double maxRange[MAX_DIM]; // Physical range of data

  std::vector<BHLeaf*> bhLeaf;
  std::vector<BHNode*> bhNode;
};

#endif
