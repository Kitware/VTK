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
  VarInfo(ADIOS_FILE *f, ADIOS_VARINFO *v);
  virtual ~VarInfo(void) { }
  void SetName(const std::string& name) { this->Name = name; }

  const int& GetId() const;
  const ADIOS_DATATYPES& GetType() const;
  const std::string& GetName(void) const;
  size_t GetNumSteps(void) const;
  size_t GetNumBlocks(size_t step) const;
  size_t GetBlockId(size_t step, size_t block) const;
  void GetDims(std::vector<size_t>& dims, size_t step, size_t block) const;

protected:
  int Id;
  ADIOS_DATATYPES Type;
  std::string Name;
  std::vector<std::vector<size_t> > BlockId;
  std::vector<std::vector<std::vector<size_t> > > Dims;
};

} // End namespace ADIOS
#endif // _ADIOSVarInfo_h
// VTK-HeaderTest-Exclude: ADIOSVarInfo.h
