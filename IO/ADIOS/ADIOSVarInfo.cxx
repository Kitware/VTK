/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSVarInfo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <iterator>

#include "ADIOSUtilities.h"

#include "ADIOSVarInfo.h"

namespace ADIOS
{

//----------------------------------------------------------------------------
VarInfo::VarInfo(ADIOS_FILE *f, ADIOS_VARINFO *v)
: Id(v->varid), Type(v->type), Name(f->var_namelist[v->varid])
{
  int err;

  // Get extra metadata
  err = adios_inq_var_stat(f, v, 1, 1);
  ReadError::TestEq(0, err);

  err = adios_inq_var_blockinfo(f, v);
  ReadError::TestEq(0, err);

  // Calculate block ids
  size_t bid = 0;
  this->BlockId.resize(v->nsteps);
  this->Dims.resize(v->nsteps);
  for(size_t s = 0; s < v->nsteps; ++s)
    {
    this->Dims[s].resize(v->nblocks[s]);
    for(size_t b = 0; b < v->nblocks[s]; ++b)
      {
      this->Dims[s][b].reserve(v->ndim);
      std::copy(v->blockinfo[bid].count, v->blockinfo[bid].count+v->ndim,
        std::back_inserter(this->Dims[s][b]));

      this->BlockId[s].push_back(bid++);
      }
    }
}

//----------------------------------------------------------------------------
const int& VarInfo::GetId() const
{
  return this->Id;
}

//----------------------------------------------------------------------------
const ADIOS_DATATYPES& VarInfo::GetType() const
{
  return this->Type;
}

//----------------------------------------------------------------------------
const std::string& VarInfo::GetName(void) const
{
  return this->Name;
}

//----------------------------------------------------------------------------
size_t VarInfo::GetNumSteps(void) const
{
  return this->BlockId.size();
}

//----------------------------------------------------------------------------
size_t VarInfo::GetNumBlocks(size_t step) const
{
  return this->BlockId[step].size();
}

//----------------------------------------------------------------------------
size_t VarInfo::GetBlockId(size_t step, size_t block) const
{
  ReadError::TestEq(true, step < this->BlockId.size(), "Invalid step");
  ReadError::TestEq(true, block < this->BlockId[step].size(),
    "Invalid block");
  return static_cast<int>(this->BlockId[step][block]);
}

//----------------------------------------------------------------------------
void VarInfo::GetDims(std::vector<size_t>& dims, size_t step,
  size_t block) const
{
  ReadError::TestEq(true, step < this->BlockId.size(), "Invalid step");
  ReadError::TestEq(true, block < this->BlockId[step].size(),
    "Invalid block");

  dims.clear();
  dims = this->Dims[step][block];
}

} // End namespace ADIOS
