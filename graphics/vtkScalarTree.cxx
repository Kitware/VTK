/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarTree.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkScalarTree.h"

// Description:
// Instantiate scalar tree with maximum level of 20 and branching
// factor of 5.
vtkScalarTree::vtkScalarTree()
{
  this->DataSet = NULL;
  this->Level = 0;
  this->MaxLevel = 20;
  this->BranchingFactor = 3;
  this->Tree = NULL;
  this->TreeSize = 0;
}

vtkScalarTree::~vtkScalarTree()
{
  if ( this->Tree ) delete [] this->Tree;
}

// Description:
// Initialize locator. Frees memory and resets object as appropriate.
void vtkScalarTree::Initialize()
{
  if ( this->Tree ) delete [] this->Tree;
  this->Tree = NULL;
}

// Description:
// Construct the scalar tree from the dataset provided. Checks build times
// and modified time from input and reconstructs the tree if necessaery.
void vtkScalarTree::BuildTree()
{
  int numCells, numLeafs, level;
  int offset, parentOffset, prod, numNodes;
  int i, j, node, cellId, numScalars, leaf, numParentLeafs;
  vtkCell *cell;
  vtkFloatScalars cellScalars(100); cellScalars.ReferenceCountingOff();
  vtkIdList *cellPts;
  vtkScalarRange *tree, *parent;
  float *s;

  // Check input...see whether we have to rebuild
  //
  if ( !this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro( << "No data to build tree with");
    return;
    }

  if ( this->Tree != NULL && this->BuildTime > this->MTime 
  && this->BuildTime > this->DataSet->GetMTime() ) return;

  vtkDebugMacro( << "Building scalar tree..." );

  this->Scalars = this->DataSet->GetPointData()->GetScalars();
  if ( ! this->Scalars )
    {
    vtkErrorMacro( << "No scalar data to build trees with");
    return;
    }

  this->Initialize();

  // Compute the number of levels in the tree
  //
  numLeafs = (int) ceil((double)numCells/this->BranchingFactor);
  for (prod=1, numNodes=1, this->Level=0; 
  prod < numLeafs && this->Level <= this->MaxLevel; this->Level++ )
    {
    prod *= this->BranchingFactor;
    numNodes += prod;
    }

  this->LeafOffset = offset = numNodes - prod;
  this->TreeSize = numNodes - (prod - numLeafs);
  this->Tree = new vtkScalarRange[this->TreeSize];
  for ( i=0; i < this->TreeSize; i++ )
    {
    this->Tree[i].min = VTK_LARGE_FLOAT;
    this->Tree[i].max = -VTK_LARGE_FLOAT;
    }

  // Loop over all cells getting range of scalar data and place into leafs
  //
  for ( cellId=0, node=0; node < numLeafs; node++ )
    {
    tree = this->Tree + offset + node;
    for ( i=0; i < this->BranchingFactor && cellId < numCells; i++, cellId++ )
      {
      cell = this->DataSet->GetCell(cellId);
      cellPts = cell->GetPointIds();
      this->Scalars->GetScalars(*cellPts,cellScalars);
      s = cellScalars.GetPointer(0);
      numScalars = cellScalars.GetNumberOfScalars();

      for ( j=0; j < numScalars; j++ )
        {
        if ( s[j] < tree->min ) tree->min = s[j];
        if ( s[j] > tree->max ) tree->max = s[j];
        }
      }
    }

  // Now build top levels of tree in bottom-up fashion
  //
  for ( level=this->Level; level > 0; level-- )
    {
    parentOffset = offset - prod/this->BranchingFactor;
    prod /= this->BranchingFactor;
    numParentLeafs = (int) ceil((double)numLeafs/this->BranchingFactor);

    for ( leaf=0, node=0; node < numParentLeafs; node++ )
      {
      parent = this->Tree + parentOffset + node;
      for ( i=0; i < this->BranchingFactor && leaf < numLeafs; i++, leaf++ )
        {
        tree = this->Tree + offset + leaf;
        if ( tree->min < parent->min ) parent->min = tree->min; 
        if ( tree->max > parent->max ) parent->max = tree->max;
        }
      }

    numLeafs = numParentLeafs;
    offset = parentOffset;
    }

  this->BuildTime.Modified();
}

// Description:
// Begin to traverse the cells based on a scalar value. Returned cells
// will have scalar values that span the scalar value specified.
void vtkScalarTree::InitTraversal(float scalarValue)
{
  this->BuildTree();
  this->ScalarValue = scalarValue;
  this->TreeIndex = this->TreeSize;

  // Check root of tree for overlap with scalar value
  //
  if ( this->Tree[0].min > scalarValue || this->Tree[0].max < scalarValue )
    {
    return;
    }

  else //find leaf that does overlap with scalar value
    {
    FindStartLeaf(0,0); //recursive function call
    }
}

int vtkScalarTree::FindStartLeaf(int index, int level)
{
  if ( level < this->Level )
    {
    int i, childIndex=this->BranchingFactor*index+1;
    level++;
    for ( i=0; i < this->BranchingFactor; i++ )  
      {
      index = childIndex + i;
      if ( index >= this->TreeSize )
        {
        this->TreeIndex = this->TreeSize;
        return 0;
        }
      else if ( this->FindStartLeaf(childIndex+i, level) ) 
        {
        return 1;
        }
      }

    return 0;
    }

  else //recursion terminated
    {
    vtkScalarRange *tree=this->Tree+index;

    if ( tree->min > this->ScalarValue || tree->max < this->ScalarValue )
      {
      return 0;
      }
    else
      {
      this->ChildNumber = 0;
      this->TreeIndex = index;
      this->CellId = (index - this->LeafOffset) * this->BranchingFactor;
      return 1;
      }
    }
}

int vtkScalarTree::FindNextLeaf(int childIndex, int childLevel)
{
  int myIndex=(childIndex-1)/this->BranchingFactor;
  int myLevel=childLevel-1;
  int firstChildIndex, childNum, index;

  //Find which child invoked this method
  firstChildIndex = myIndex*this->BranchingFactor + 1;
  childNum = childIndex - firstChildIndex;

  for ( childNum++; childNum < this->BranchingFactor; childNum++ )
    {
    index = firstChildIndex + childNum;
    if ( index >= this->TreeSize ) 
      {
      this->TreeIndex = this->TreeSize;
      return 0;
      }
    else if ( FindStartLeaf(index, childLevel) )
      {
      return 1;
      }
    }

  //If here, didn't find anything yet
  if ( myLevel <= 0 ) //at root, can't go any higher in tree
    {
    this->TreeIndex = this->TreeSize;
    return 0;
    }
  else
    {
    return FindNextLeaf(myIndex,myLevel);
    }
}


// Description:
// Return the next cell that may contain scalar value specified to
// initialize traversal. The value NULL is returned if the list is
// exhausted. Make sure that InitTraversal() has been invoked first or
// you'll get erratic behavior.
vtkCell *vtkScalarTree::GetNextCell(int& cellId, vtkIdList* &cellPts,
                                    vtkFloatScalars& cellScalars)
{
  float *s, min=VTK_LARGE_FLOAT, max=(-VTK_LARGE_FLOAT);
  int i, numScalars;
  vtkCell *cell;

  while ( this->TreeIndex < this->TreeSize )
    {
    for ( ; this->ChildNumber < this->BranchingFactor; 
    this->ChildNumber++, this->CellId++ )
      {
      cell = this->DataSet->GetCell(this->CellId);
      cellPts = cell->GetPointIds();
      this->Scalars->GetScalars(*cellPts,cellScalars);
      s = cellScalars.GetPointer(0);
      numScalars = cellScalars.GetNumberOfScalars();
      for (i=0; i < numScalars; i++)
        {
        if ( s[i] < min ) min = s[i];
        if ( s[i] > max ) max = s[i];
        }
      if ( this->ScalarValue >= min && this->ScalarValue <= max )
        {
        cellId = this->CellId;
        this->ChildNumber++; //prepare for next time
        this->CellId++;
        return cell;
        }
      } //for each cell in this leaf

    // If here, must have not found anything in this leaf
    FindNextLeaf(this->TreeIndex, this->Level);
    } //while not all leafs visited

  return NULL;
}


void vtkScalarTree::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->DataSet )
    {
    os << indent << "DataSet: " << this->DataSet << "\n";
    }
  else
    {
    os << indent << "DataSet: (none)\n";
    }

  os << indent << "Level: " << this->Level << "\n" ;
  os << indent << "MaxLevel: " << this->MaxLevel << "\n" ;
  os << indent << "Branching Factor: " << this->BranchingFactor << "\n" ;
  os << indent << "Build Time: " << this->BuildTime.GetMTime() << "\n";
}

