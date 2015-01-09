/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME ADIOSReader - The utility class performing ADIOS read operations

#ifndef _ADIOSReader_h
#define _ADIOSReader_h

#include <stdexcept>
#include <string>
#include <vector>

#include <adios_mpi.h>

#include "ADIOSAttribute.h"
#include "ADIOSDefs.h"
#include "ADIOSScalar.h"
#include "ADIOSVarInfo.h"

namespace ADIOS
{

class Reader
{
public:
  static bool SetCommunicator(MPI_Comm);
  static bool SetReadMethod(ReadMethod, const std::string&);

  Reader();
  ~Reader();

  // Description:
  // Open the ADIOS file and cache the variable names and scalar data
  void Open(const std::string &fileName);

  // Description:
  // Close an already open file handle and free it's resources
  void Close();

  // Description:
  // Retrieve the total number of seps
  void GetStepRange(int &tStart, int &tEnd) const;

  // Description:
  // Retrieve a list of attributes
  const std::vector<const Attribute*>& GetAttributes() const;

  // Description:
  // Retrieve a list of scalars and thier associated metadata
  const std::vector<const Scalar*>& GetScalars() const;

  // Description:
  // Retrieve a list of arrays and thier associated metadata
  const std::vector<const VarInfo*>& GetArrays() const;

  // Description:
  // Schedule array data to be read. Data will be read with ReadArrays.
  // step specified the time step index to read and block specifies the
  // write block index to read (-1 means use whatever your current mpi rank is)
  void ScheduleReadArray(int id, void *data, int step, int block);

  // Description:
  // Perform all scheduled array read operations
  void ReadArrays();

  // Description:
  // Whether or not the file / stream is already open
  bool IsOpen() const;

private:
  // Initialization context to manage one-time init and finalize of ADIOS
  struct InitContext;
  InitContext *Ctx;

  // ADIOS specific implementation details (file handles, group sizes, etc.)
  struct ReaderImpl;
  ReaderImpl *Impl;
};

} // End anmespace ADIOS
#endif
// VTK-HeaderTest-Exclude: ADIOSReader.h
