/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridEntry.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridEntry.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"

#include <cassert>

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::PrintSelf( ostream& os, vtkIndent indent )
{
  os << indent << "--vtkHyperTreeGridEntry--" << endl;
  os << indent << "Index:" << this->Index << endl;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::Dump( ostream& os )
{
  os << "Index:" << this->Index << endl;
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridEntry::Initialize( vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create )
{
  this->Index = 0;
  return grid->GetTree( treeIndex, create );
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridEntry::GetGlobalNodeIndex( const vtkHyperTree* tree ) const
{
  assert( "pre: not_tree" &&
          tree );
  return tree->GetGlobalIndexFromLocal( this->Index );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::SetGlobalIndexStart( vtkHyperTree* tree, vtkIdType index )
{
  assert( "pre: not_tree" &&
          tree );
  tree->SetGlobalIndexStart( index );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::SetGlobalIndexFromLocal( vtkHyperTree* tree, vtkIdType index )
{
  assert( "pre: not_tree" &&
          tree );
  tree->SetGlobalIndexFromLocal( this->Index, index );
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridEntry::IsLeaf( const vtkHyperTree* tree ) const
{
  assert( "pre: not_tree" &&
          tree );
  if ( this->Index )
  {
    return tree->IsLeaf( this->Index );
  }
  return ( tree->GetNumberOfVertices() == 1 );
}

//---------------------------------------------------------------------------
void vtkHyperTreeGridEntry::SubdivideLeaf( vtkHyperTree* tree, unsigned int level)
{
  assert( "pre: not_tree" &&
          tree );
  if ( this->IsLeaf( tree ) )
  {
    tree->SubdivideLeaf( this->Index, level );
  }
}

//---------------------------------------------------------------------------
bool vtkHyperTreeGridEntry::IsTerminalNode( const vtkHyperTree* tree ) const
{
  assert( "pre: not_tree" &&
          tree );
  bool result = ! this->IsLeaf( tree );
  if ( result )
  {
    result = tree->IsTerminalNode( this->Index );
  }
  assert( "post: compatible" &&
          ( ! result || ! this->IsLeaf( tree ) ) );
  return result;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::ToChild( const vtkHyperTree* tree, unsigned char ichild )
{
  assert( "pre: not_tree" &&
          tree );
  assert( "pre: not_leaf" &&
          ! this->IsLeaf( tree ) );
  assert( "pre: valid_child" &&
          ichild < tree->GetNumberOfChildren() );
  this->Index = tree->GetElderChildIndex( this->Index ) + ichild;
}
