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

  // Polulate dimensions and determine block step ranges
  size_t pidMax = 0;
  int nd = v->ndim;
  this->Dims.resize(v->sum_nblocks);
  for(size_t bid = 0; bid < v->sum_nblocks; ++bid)
  {
    ADIOS_VARBLOCK &bi = v->blockinfo[bid];
    if(bi.process_id > pidMax)
    {
      pidMax = bi.process_id;
    }

    if(nd > 0)
    {
      std::vector<size_t> &dimsBid = this->Dims[bid];
      dimsBid.reserve(nd);
      for(size_t n = 0; n < nd; ++n)
      {
        dimsBid.push_back(bi.count[n]);
      }
    }
  }

  // Construct the block index
  this->NumPids = pidMax + 1;
  this->NumSteps = f->last_step+1;
  this->StepBlockIndex.clear();
  this->StepBlockIndex.resize(this->NumSteps*this->NumPids, NULL);
  size_t bid = 0;
  for(size_t s = 0; s < v->nsteps; ++s)
  {
    for(size_t b = 0; b < v->nblocks[s]; ++b)
    {
      ADIOS_VARBLOCK &bi = v->blockinfo[bid];
      this->StepBlockIndex[(bi.time_index-1)*this->NumPids+bi.process_id] =
        new StepBlock(s, b, bid++);
    }
  }
}

//----------------------------------------------------------------------------
VarInfo::~VarInfo()
{
  // Cleanup the block step index
  for(std::vector<StepBlock*>::iterator i = this->StepBlockIndex.begin();
    i != this->StepBlockIndex.end(); ++i)
  {
    delete *i;
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
  return this->NumSteps;
}

//----------------------------------------------------------------------------
size_t VarInfo::GetNumBlocks(size_t step) const
{
  return this->NumPids;
}

//----------------------------------------------------------------------------
VarInfo::StepBlock* VarInfo::GetNewestBlockIndex(size_t step, size_t pid) const
{
  ReadError::TestEq(true, step < this->NumSteps, "Invalid step");
  ReadError::TestEq(true, pid < this->NumPids, "Invalid block");

  StepBlock* idx = NULL;
  for(int curStep = step; !idx && curStep >= 0; --curStep)
  {
    idx = this->StepBlockIndex[curStep*this->NumPids+pid];
  }

  return idx;
}

//----------------------------------------------------------------------------
void VarInfo::GetDims(std::vector<size_t>& dims, size_t step, size_t pid) const
{
  StepBlock* idx = this->GetNewestBlockIndex(step, pid);
  ReadError::TestNe<VarInfo::StepBlock*>(NULL, idx, "Variable not available");

  dims.clear();
  dims = this->Dims[idx->BlockId];
}

} // End namespace ADIOS
