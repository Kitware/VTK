// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2007, Los Alamos National Security, LLC
// SPDX-FileCopyrightText: Copyright (c) 2021, Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
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

#include "vtkABINamespace.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
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

VTK_ABI_NAMESPACE_END
#endif
