/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleScalarTree.cxx
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
#include "vtkSimpleScalarTree.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSimpleScalarTree, "1.2");
vtkStandardNewMacro(vtkSimpleScalarTree);

class vtkScalarNode {};

template <class TScalar>
class vtkScalarRange : public vtkScalarNode
{
public:
  TScalar min;
  TScalar max;
};

// Instantiate scalar tree with maximum level of 20 and branching
// factor of 5.
vtkSimpleScalarTree::vtkSimpleScalarTree()
{
  this->DataSet = NULL;
  this->Level = 0;
  this->MaxLevel = 20;
  this->BranchingFactor = 3;
  this->Tree = NULL;
  this->TreeSize = 0;
}

vtkSimpleScalarTree::~vtkSimpleScalarTree()
{
  if ( this->Tree )
    {
    delete [] this->Tree;
    }
}

// Initialize locator. Frees memory and resets object as appropriate.
void vtkSimpleScalarTree::Initialize()
{
  if ( this->Tree )
    {
    delete [] this->Tree;
    }
  this->Tree = NULL;
}

// Construct the scalar tree from the dataset provided. Checks build times
// and modified time from input and reconstructs the tree if necessaery.
void vtkSimpleScalarTree::BuildTree()
{
  vtkIdType numCells, cellId, i, j, numScalars;
  int level, offset, parentOffset, prod;
  vtkIdType numNodes, node, numLeafs, leaf, numParentLeafs;
  vtkCell *cell;
  vtkIdList *cellPts;
  vtkScalarRange<float> *tree, *parent;
  float *s;
  vtkFloatArray *cellScalars;

  // Check input...see whether we have to rebuild
  //
  if ( !this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro( << "No data to build tree with");
    return;
    }

  if ( this->Tree != NULL && this->BuildTime > this->MTime 
    && this->BuildTime > this->DataSet->GetMTime() )
    {
    return;
    }

  vtkDebugMacro( << "Building scalar tree..." );

  this->Scalars = this->DataSet->GetPointData()->GetScalars();
  if ( ! this->Scalars )
    {
    vtkErrorMacro( << "No scalar data to build trees with");
    return;
    }

  this->Initialize();
  cellScalars = vtkFloatArray::New();
  cellScalars->Allocate(100);
  
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
  vtkScalarRange<float> *TTree;
  this->TreeSize = numNodes - (prod - numLeafs);
  this->Tree = TTree = new vtkScalarRange<float>[this->TreeSize];
  for ( i=0; i < this->TreeSize; i++ )
    {
    TTree[i].min = VTK_LARGE_FLOAT;
    TTree[i].max = -VTK_LARGE_FLOAT;
    }

  // Loop over all cells getting range of scalar data and place into leafs
  //
  for ( cellId=0, node=0; node < numLeafs; node++ )
    {
    tree = TTree + offset + node;
    for ( i=0; i < this->BranchingFactor && cellId < numCells; i++, cellId++ )
      {
      cell = this->DataSet->GetCell(cellId);
      cellPts = cell->GetPointIds();
      numScalars = cellPts->GetNumberOfIds();
      cellScalars->SetNumberOfTuples(numScalars);
      this->Scalars->GetTuples(cellPts, cellScalars);
      s = cellScalars->GetPointer(0);

      for ( j=0; j < numScalars; j++ )
        {
        if ( s[j] < tree->min )
          {
          tree->min = s[j];
          }
        if ( s[j] > tree->max )
          {
          tree->max = s[j];
          }
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
      parent = TTree + parentOffset + node;
      for ( i=0; i < this->BranchingFactor && leaf < numLeafs; i++, leaf++ )
        {
        tree = TTree + offset + leaf;
        if ( tree->min < parent->min )
          {
          parent->min = tree->min; 
          }
        if ( tree->max > parent->max )
          {
          parent->max = tree->max;
          }
        }
      }

    numLeafs = numParentLeafs;
    offset = parentOffset;
    }

  this->BuildTime.Modified();
  cellScalars->Delete();
}

// Begin to traverse the cells based on a scalar value. Returned cells
// will have scalar values that span the scalar value specified.
void vtkSimpleScalarTree::InitTraversal(float scalarValue)
{
  vtkScalarRange<float> *TTree = 
    static_cast< vtkScalarRange<float> * > (this->Tree);

  this->BuildTree();
  this->ScalarValue = scalarValue;
  this->TreeIndex = this->TreeSize;

  // Check root of tree for overlap with scalar value
  //
  if ( TTree[0].min > scalarValue || TTree[0].max < scalarValue )
    {
    return;
    }

  else //find leaf that does overlap with scalar value
    {
    this->FindStartLeaf(0,0); //recursive function call
    }
}

int vtkSimpleScalarTree::FindStartLeaf(vtkIdType index, int level)
{
  if ( level < this->Level )
    {
    int i;
    vtkIdType childIndex=this->BranchingFactor*index+1;
    
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
    vtkScalarRange<float> *tree = static_cast< 
      vtkScalarRange<float>*>(this->Tree) + index;

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

int vtkSimpleScalarTree::FindNextLeaf(vtkIdType childIndex, int childLevel)
{
  vtkIdType myIndex=(childIndex-1)/this->BranchingFactor;
  int myLevel=childLevel-1;
  vtkIdType firstChildIndex, childNum, index;

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
    else if ( this->FindStartLeaf(index, childLevel) )
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
    return this->FindNextLeaf(myIndex,myLevel);
    }
}


// Return the next cell that may contain scalar value specified to
// initialize traversal. The value NULL is returned if the list is
// exhausted. Make sure that InitTraversal() has been invoked first or
// you'll get erratic behavior.
vtkCell *vtkSimpleScalarTree::GetNextCell(vtkIdType& cellId, vtkIdList* &cellPts,
                                    vtkDataArray *cellScalars)
{
  vtkFloatArray* array = vtkFloatArray::SafeDownCast(cellScalars);
  if (!array)
    {
    vtkErrorMacro("Expected a float array in scalars, got an array of type:"
                  << cellScalars->GetDataType());
    return 0;
    }

  float *s, min=VTK_LARGE_FLOAT, max=(-VTK_LARGE_FLOAT);
  vtkIdType i, numScalars;
  vtkCell *cell;
  vtkIdType numCells = this->DataSet->GetNumberOfCells();

  while ( this->TreeIndex < this->TreeSize )
    {
    for ( ; this->ChildNumber<this->BranchingFactor && this->CellId<numCells; 
    this->ChildNumber++, this->CellId++ )
      {
      cell = this->DataSet->GetCell(this->CellId);
      cellPts = cell->GetPointIds();
      numScalars = cellPts->GetNumberOfIds();
      array->SetNumberOfTuples(numScalars);
      this->Scalars->GetTuples(cellPts, array);
      s = array->GetPointer(0);
      for (i=0; i < numScalars; i++)
        {
        if ( s[i] < min )
          {
          min = s[i];
          }
        if ( s[i] > max )
          {
          max = s[i];
          }
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
    this->FindNextLeaf(this->TreeIndex, this->Level);
    } //while not all leafs visited

  return NULL;
}

void vtkSimpleScalarTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Level: " << this->Level << "\n" ;
  os << indent << "Max Level: " << this->MaxLevel << "\n" ;
  os << indent << "Branching Factor: " << this->BranchingFactor << "\n" ;
}

