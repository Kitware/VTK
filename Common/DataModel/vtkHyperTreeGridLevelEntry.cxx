/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridLevelEntry.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridLevelEntry.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkHyperTreeGridLevelEntry::vtkHyperTreeGridLevelEntry(
  vtkHyperTreeGrid* grid,
  vtkIdType treeIndex,
  bool create
) : Tree( grid->GetTree( treeIndex, create ) ), Level( 0 ), Index( 0 )
{
}

//-----------------------------------------------------------------------------

vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>
vtkHyperTreeGridLevelEntry::GetHyperTreeGridNonOrientedCursor( vtkHyperTreeGrid* grid )
{
  //JB assert ( "pre: level==0" && this->Level == 0 );
  vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor = vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::New();
  cursor->Initialize( grid, this->GetTree(), this->Level, this->Index );
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::PrintSelf( ostream& os, vtkIndent indent )
{
  os << indent << "--vtkHyperTreeGridLevelEntry--" << endl;
  this->Tree->PrintSelf(os, indent);
  os << indent << "Level:" << this->Level << endl;
  os << indent << "Index:" << this->Index << endl;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::Dump( ostream& os )
{
  os << "Level:" << this->Level << endl;
  os << "Index:" << this->Index << endl;
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridLevelEntry::Initialize( vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create )
{
  this->Tree = grid->GetTree( treeIndex, create );
  this->Level = 0;
  this->Index = 0;
  return this->Tree;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridLevelEntry::GetGlobalNodeIndex() const
{
  //JB BAD assert( "pre: not_tree" &&
  //     JB BAD     this->Tree );
  if ( this->Tree )
  {
    return this->Tree->GetGlobalIndexFromLocal( this->Index );
  }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::SetGlobalIndexStart( vtkIdType index )
{
  assert( "pre: not_tree" &&
          this->Tree );
  this->Tree->SetGlobalIndexStart( index );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::SetGlobalIndexFromLocal( vtkIdType index )
{
  assert( "pre: not_tree" &&
          this->Tree );
  this->Tree->SetGlobalIndexFromLocal( this->Index, index );
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridLevelEntry::IsLeaf() const
{
  assert( "pre: not_tree" &&
          this->Tree );
  if ( this->Index )
  {
    return this->Tree->IsLeaf( this->Index );
  }
  return ( this->Tree->GetNumberOfVertices() == 1 );
}

//---------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::SubdivideLeaf()
{
  assert( "pre: not_tree" &&
          this->Tree );
  if ( this->IsLeaf() )
  {
    this->Tree->SubdivideLeaf( this->Index, this->Level );
  }
}

//---------------------------------------------------------------------------
bool vtkHyperTreeGridLevelEntry::IsTerminalNode() const
{
  assert( "pre: not_tree" &&
          this->Tree );
  bool result = ! this->IsLeaf();
  if ( result )
  {
    result = this->Tree->IsTerminalNode( this->Index );
  }
  // A=>B: notA or B
  assert( "post: compatible" &&
          ( ! result || ! this->IsLeaf() ) );
  return result;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::ToChild( unsigned char ichild )
{
  assert( "pre: not_tree" &&
          this->Tree );
  assert( "pre: not_leaf" &&
          ! this->IsLeaf() );
  assert( "pre: valid_child" &&
          ichild < this->Tree->GetNumberOfChildren() );
  this->Index = this->Tree->GetElderChildIndex( this->Index ) + ichild;

  this->Level ++;
}
