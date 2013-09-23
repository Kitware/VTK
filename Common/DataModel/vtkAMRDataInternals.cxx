/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRDataInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRDataInternals.h"
#include "vtkUniformGrid.h"
#include "vtkObjectFactory.h"

#include <cassert>
vtkStandardNewMacro(vtkAMRDataInternals);

vtkAMRDataInternals::Block::Block(unsigned int i, vtkUniformGrid* g)
{
  this->Index = i;
  this->Grid = g;
}

//-----------------------------------------------------------------------------

vtkAMRDataInternals::vtkAMRDataInternals()
  : InternalIndex(NULL)
{
}

void vtkAMRDataInternals::Initialize()
{
  delete this->InternalIndex; this->InternalIndex=NULL;
  this->Blocks.clear();
}

vtkAMRDataInternals::~vtkAMRDataInternals()
{
  this->Blocks.clear();
  delete this->InternalIndex;
}

void vtkAMRDataInternals::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


void vtkAMRDataInternals::Insert(unsigned int index, vtkUniformGrid* grid)
{
  this->Blocks.push_back(Block(index,grid));
  int i = static_cast<int>(this->Blocks.size())-2;
  while( i>=0 && this->Blocks[i].Index > this->Blocks[i+1].Index)
    {
    std::swap(this->Blocks[i],this->Blocks[i+1]);
    i--;
    }
}

vtkUniformGrid* vtkAMRDataInternals::GetDataSet(unsigned int compositeIndex)
{
  unsigned int internalIndex(0);
  if(!this->GetInternalIndex(compositeIndex,internalIndex))
    {
    return NULL;
    }
  return this->Blocks[internalIndex].Grid;
}

bool vtkAMRDataInternals::GetInternalIndex(unsigned int compositeIndex, unsigned int& internalIndex)
{
  this->GenerateIndex();
  if (compositeIndex>=this->InternalIndex->size())
    {
    return false;
    }
  int idx = (*this->InternalIndex)[compositeIndex];
  if(idx<0)
    {
    return false;
    }
  internalIndex= static_cast<unsigned int>(idx);
  return true;
}

void vtkAMRDataInternals::GenerateIndex(bool force)
{
  if(!force && this->InternalIndex)
    {
    return;
    }
  delete this->InternalIndex;
  this->InternalIndex = new std::vector<int>();
  std::vector<int>& internalIndex(*this->InternalIndex);

  for(unsigned i=0; i<this->Blocks.size();i++)
    {
    unsigned int index = this->Blocks[i].Index;
    for(unsigned int j = static_cast<unsigned int>(internalIndex.size()); j<=index; j++)
      {
      internalIndex.push_back(-1);
      }
    internalIndex[index] = static_cast<int>(i);
    }
}

void vtkAMRDataInternals::ShallowCopy(vtkObject *src)
{
  if( src == this )
    {
    return;
    }

  if(vtkAMRDataInternals * hbds = vtkAMRDataInternals::SafeDownCast(src))
    {
    this->Blocks = hbds->Blocks;
    }

  this->Modified();
}
