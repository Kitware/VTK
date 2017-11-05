/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridCursor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridCursor.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeCursor.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkHyperTreeGridCursor);
vtkCxxSetObjectMacro(vtkHyperTreeGridCursor,Grid,vtkHyperTreeGrid);
vtkCxxSetObjectMacro(vtkHyperTreeGridCursor,Tree,vtkHyperTree);

//-----------------------------------------------------------------------------
vtkHyperTreeGridCursor::vtkHyperTreeGridCursor()
{
  // No grid is given by default
  this->Grid  = 0;

  // No tree is given by default
  this->Tree  = 0;

  // Default cursor level
  this->Level = 0;

  // Default index
  this->Index = 0;

  // Cursor is not at leaf by default
  this->Leaf  = false;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridCursor::~vtkHyperTreeGridCursor()
{
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridCursor* vtkHyperTreeGridCursor::Clone()
{
  vtkHyperTreeGridCursor* clone = this->NewInstance();
  assert( "post: clone_exists" && clone != 0 );

  // Copy iVars
  clone->Grid = this->Grid;
  clone->Tree  = this->Tree;
  clone->Level = this->Level;
  clone->Index = this->Index;
  clone->Leaf  = this->Leaf;

  // Return clone
  return clone;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridCursor::Initialize( vtkHyperTreeGrid* grid, vtkIdType index )
{
  // Assigned grid instance variable
  this->Grid = grid;

  // Retrieve specified tree from grid
  vtkHyperTree* tree = grid->GetTree( index );
  this->Tree = tree;

  // Initialize other iVars at default values
  this->Level = 0;
  this->Index = 0;

  // Empty trees and trees with only a root cell appear like a leaf so recursion stop
  this->Leaf = ( ! tree || tree->GetNumberOfVertices() == 1 );
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridCursor::GetVertexId()
{
  return this->Index;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridCursor::GetDimension()
{
  // Retrieve tree
  vtkHyperTree* tree = this->Tree;

  // An empty tree has dimension 0
  return tree ? tree->GetDimension() : 0;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridCursor::GetNumberOfChildren()
{
  // Retrieve tree
  vtkHyperTree* tree = this->Tree;

  // An empty tree does not have children
  return tree ? tree->GetNumberOfChildren() : 0;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridCursor::IsLeaf()
{
  // See particular cases in Initialize()
  return this->Leaf;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridCursor::IsRoot()
{
  // No special null cursor exists with this object
  return ( this->Index == 0 );
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridCursor::GetChildIndex()
{
  if ( ! this->Tree )
  {
    // Index in a null grid is always 0
    return 0;
  }

  int child = 0;
  this->Tree->FindChildParameters( child, this->Index, this->Leaf );

  return child;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridCursor::ToRoot()
{
  if ( ! this->Tree )
  {
    // In an empty tree there is nothing to be done
    return;
  }

  // Return to root level
  this->Level = 0;
  this->Index = 0;

  // Handle special case of root cells with no children
  this->Leaf = ( this->Tree->GetNumberOfVertices() == 1 );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridCursor::ToParent()
{
  if ( ! this->Tree || this->Level )
  {
    // In an empty tree or at root there is nothing to be done
    return;
  }

  // Update current vertex index to parent index
  this->Tree->FindParentIndex( this->Index );

  // Move one level higher
  -- this->Level;

  // Cursor can no longer be at a leaf
  this->Leaf = false;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridCursor::ToChild( int child )
{
  // In an empty tree or at a leaf there is nothing to be done
  if ( ! this->Tree || this->Leaf )
  {
    return;
  }

  this->Tree->FindChildParameters( child, this->Index, this->Leaf );

  // Move one level deeper
  ++ this->Level;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridCursor::GetGlobalNodeIndex()
{
  // Retrieve tree
  vtkHyperTree* tree = this->Tree;

  // Global index in a null grid is always 0
  return tree ? tree->GetGlobalIndexFromLocal( this->Index ) : 0;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );

  if ( this->Tree )
  {
    os << indent << "Tree:" << this->Tree << endl;
  }
  else
  {
    os << indent << "Tree: (None)" << endl;
  }

  os << indent << "Level=" << this->Level << endl;
  os << indent << "Index=" << this->Index << endl;
  os << indent << "Leaf: " << ( this->Leaf ? "true" : "false" ) << endl;
}

//-----------------------------------------------------------------------------
