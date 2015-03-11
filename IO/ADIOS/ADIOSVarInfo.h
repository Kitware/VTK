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
// .NAME ADIOSVarInfo - The utility class wrapping the ADIOS_VARINFO struct

#ifndef _ADIOSVarInfo_h
#define _ADIOSVarInfo_h

#include <string>
#include <vector>

#include <adios_read.h>

//----------------------------------------------------------------------------
namespace ADIOS
{

class VarInfo
{
public:
  // Data structure used to hold block index mapping info
  struct StepBlock
  {
    StepBlock() : Step(-1), Block(-1), BlockId(-1) {}
    StepBlock(int s, int b, int i) : Step(s), Block(b), BlockId(i) { }
    int Step;
    int Block;
    int BlockId;
  };

public:
  VarInfo(ADIOS_FILE *f, ADIOS_VARINFO *v);
  virtual ~VarInfo(void);
  void SetName(const std::string& name) { this->Name = name; }

  const int& GetId() const;
  const ADIOS_DATATYPES& GetType() const;
  const std::string& GetName(void) const;
  size_t GetNumSteps(void) const;
  size_t GetNumBlocks(size_t step) const;
  StepBlock* GetNewestBlockIndex(size_t step, size_t pid) const;
  void GetDims(std::vector<size_t>& dims, size_t step, size_t pid) const;

protected:
  int Id;
  ADIOS_DATATYPES Type;
  std::string Name;
  size_t NumSteps;
  size_t NumPids;
  std::vector<std::vector<size_t> > Dims;

  // This maps the absolute time step and process id to a file-local
  // step and block id for reading
  std::vector<StepBlock*> StepBlockIndex;
};

} // End namespace ADIOS
#endif // _ADIOSVarInfo_h
// VTK-HeaderTest-Exclude: ADIOSVarInfo.h
