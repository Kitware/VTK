/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRDataInternals2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRDataInternals2.h"
#include "vtkUniformGrid.h"
#include "vtkObjectFactory.h"

#include <cassert>
vtkStandardNewMacro(vtkAMRDataInternals2);

vtkAMRDataInternals2::Block::Block(unsigned int i, vtkUniformGrid* g)
{
  this->Index = i;
  this->Grid = g;
}

//-----------------------------------------------------------------------------

vtkAMRDataInternals2::vtkAMRDataInternals2()
{
  this->HasBlocksInserted = false;
}

void vtkAMRDataInternals2::Initialize(unsigned int size)
{
  this->ClearSparseBlocks();
  this->Blocks.clear();
  if (size)
    {
    this->SparseBlocks.resize(size*8,NULL);
    }
}

vtkAMRDataInternals2::~vtkAMRDataInternals2()
{
  this->ClearSparseBlocks();
  this->Blocks.clear();
}

void vtkAMRDataInternals2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


void vtkAMRDataInternals2::Insert(unsigned int index, vtkUniformGrid* grid)
{
  if(index < this->SparseBlocks.size())
    {
    this->SparseBlocks[index*8] = new Block(index,grid);
    this->HasBlocksInserted = true;
    return;
    }
  // Old behavior for consistency
  this->Blocks.push_back(Block(index,grid));
  int i = static_cast<int>(this->Blocks.size())-2;
  while( i>=0 && this->Blocks[i].Index > this->Blocks[i+1].Index)
    {
    std::swap(this->Blocks[i],this->Blocks[i+1]);
    i--;
    }
}

vtkUniformGrid* vtkAMRDataInternals2::GetDataSet(unsigned int compositeIndex)
{
  unsigned int i = compositeIndex*8;
  if(i >= this->SparseBlocks.size() || !this->SparseBlocks[i])
    {
    return NULL;
    }
  return this->SparseBlocks[i]->Grid;
}

void vtkAMRDataInternals2::ShallowCopy(vtkObject *src)
{
  if( src == this )
    {
    return;
    }

  if(vtkAMRDataInternals2 * hbds = vtkAMRDataInternals2::SafeDownCast(src))
    {
    this->Blocks = hbds->Blocks;
    this->ClearSparseBlocks();
    this->SparseBlocks = hbds->SparseBlocks;
    this->HasBlocksInserted = hbds->HasBlocksInserted;
    }

  this->Modified();
}

void vtkAMRDataInternals2::ClearSparseBlocks()
{
  std::vector<Block*>::iterator it = this->SparseBlocks.begin();
  while(it < this->SparseBlocks.end())
    {
    if(*it) delete (*it);
    it += 8;
    }
  this->SparseBlocks.clear();
  this->HasBlocksInserted = false;
}

void vtkAMRDataInternals2::CompactBlocks() const
{
  this->Blocks.clear();
  std::vector<Block*>::const_iterator it = this->SparseBlocks.begin();
  while(it < this->SparseBlocks.end())
    {
    if(*it) this->Blocks.push_back(**it);
    it += 8;
    }
  this->HasBlocksInserted = false;
}
