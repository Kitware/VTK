/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleScalarTree.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkSimpleScalarTree - organize data according to scalar values (used to accelerate contouring operations)
// .SECTION Description
// vtkSimpleScalarTree creates a pointerless binary tree that helps search for
// cells that lie within a particular scalar range. This object is used to
// accelerate some contouring (and other scalar-based techniques).
// 
// The tree consists of an array of (min,max) scalar range pairs per node in
// the tree. The (min,max) range is determined from looking at the range of
// the children of the tree node. If the node is a leaf, then the range is
// determined by scanning the range of scalar data in n cells in the
// dataset. The n cells are determined by arbitrary selecting cell ids from
// id(i) to id(i+n), and where n is specified using the BranchingFactor
// ivar. Note that leaf node i=0 contains the scalar range computed from
// cell ids (0,n-1); leaf node i=1 contains the range from cell ids (n,2n-1);
// and so on. The implication is that there are no direct lists of cell ids
// per leaf node, instead the cell ids are implicitly known.

#ifndef __vtkSimpleScalarTree_h
#define __vtkSimpleScalarTree_h

#include "vtkScalarTree.h"

//BTX
class vtkFloatArray;
class vtkScalarNode;
//ETX

class VTK_FILTERING_EXPORT vtkSimpleScalarTree : public vtkScalarTree
{
public:
  // Description:
  // Instantiate scalar tree with maximum level of 20 and branching
  // factor of 5.
  static vtkSimpleScalarTree *New();

  // Description:
  // Standard type related macros and PrintSelf() method.
  vtkTypeRevisionMacro(vtkSimpleScalarTree,vtkScalarTree);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the branching factor for the tree. This is the number of
  // children per tree node. Smaller values (minimum is 2) mean deeper
  // trees and more memory overhead. Larger values mean shallower
  // trees, less memory usage, but worse performance.
  vtkSetClampMacro(BranchingFactor,int,2,VTK_LARGE_INTEGER);
  vtkGetMacro(BranchingFactor,int);

  // Description:
  // Get the level of the scalar tree. This value may change each time the
  // scalar tree is built and the branching factor changes.
  vtkGetMacro(Level,int);

  // Description:
  // Set the maximum allowable level for the tree. 
  vtkSetClampMacro(MaxLevel,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(MaxLevel,int);

  // Description:
  // Construct the scalar tree from the dataset provided. Checks build times
  // and modified time from input and reconstructs the tree if necessary.
  virtual void BuildTree();
  
  // Description:
  // Initialize locator. Frees memory and resets object as appropriate.
  virtual void Initialize();

  // Description:
  // Begin to traverse the cells based on a scalar value. Returned cells
  // will have scalar values that span the scalar value specified.
  virtual void InitTraversal(float scalarValue);

  // Description:
  // Return the next cell that may contain scalar value specified to
  // initialize traversal. The value NULL is returned if the list is
  // exhausted. Make sure that InitTraversal() has been invoked first or
  // you'll get erratic behavior.
  virtual vtkCell *GetNextCell(vtkIdType &cellId, vtkIdList* &ptIds,
                               vtkDataArray *cellScalars);

protected:
  vtkSimpleScalarTree();
  ~vtkSimpleScalarTree();

  vtkDataArray *Scalars;
  int MaxLevel;
  int Level;
  int BranchingFactor; //number of children per node
  vtkScalarNode *Tree; //pointerless scalar range tree
  int TreeSize; //allocated size of tree

private:
  vtkIdType TreeIndex; //traversal location within tree
  vtkIdType LeafOffset; //offset to leaf nodes of tree
  int       ChildNumber; //current child in traversal
  vtkIdType CellId; //current cell id being examined
  int       FindStartLeaf(vtkIdType index, int level);
  int       FindNextLeaf(vtkIdType index,int level);

private:
  vtkSimpleScalarTree(const vtkSimpleScalarTree&);  // Not implemented.
  void operator=(const vtkSimpleScalarTree&);  // Not implemented.
};

#endif


