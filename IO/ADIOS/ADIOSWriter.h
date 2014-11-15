/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME ADIOSWriter - The utility class performing ADIOS Write operations

#ifndef _ADIOSWriter_h
#define _ADIOSWriter_h

#include <string>
#include <vector>

#include <adios_mpi.h>

#include "ADIOSDefs.h"

class ADIOSWriter
{
public:
  static bool SetCommunicator(MPI_Comm);
  static MPI_Comm GetCommunicator();
  static bool ResizeBuffer(size_t bufSize);

protected:
  struct Context;
  Context* WriterContext;

public:
  ADIOSWriter(ADIOS::TransportMethod, const std::string&);

  ~ADIOSWriter();

  // Description:
  // Initialize the underlying ADIOS subsystem
  static bool Initialize(MPI_Comm comm);

  // Description
  // Define scalar attributes
  template<typename TN>
  void DefineAttribute(const std::string& path, const TN& value);

  // Description
  // Define scalars for later writing
  template<typename TN>
  int DefineScalar(const std::string& path);

  // Description
  // Define scalars for later writing
  int DefineScalar(const std::string& path, const std::string& v);

  // Description
  // Define arrays for later writing.  Return the local block id used for
  // writing
  template<typename TN>
  int DefineArray(const std::string& path, const std::vector<size_t>& dims,
    ADIOS::Transform xfm=ADIOS::Transform_NONE);

  // Description
  // Define arrays for later writing.  Return the local block id used for
  // writing
  int DefineArray(const std::string& path, const std::vector<size_t>& dims,
    int vtkType, ADIOS::Transform xfm=ADIOS::Transform_NONE);

  // Description
  // Define arrays for later writing.  Return the local block id used for
  // writing
  int DefineGlobalArray(const std::string& path, const size_t *dimsLocal,
    const size_t *dimsGlobal, const size_t *dimsOffset, size_t nDims,
    int vtkType, ADIOS::Transform xfm=ADIOS::Transform_NONE);

  // Description:
  // Open the vtk group in the ADIOS file for writing one timestep
  void Open(const std::string &fileName, bool append = false);

  // Description:
  // Close the VTK group for the current time step in the ADIOS file
  void Close();

  // Description
  // Schedule scalars for writing
  template<typename TN>
  void WriteScalar(const std::string& path, const TN& value);

  // Description
  // Schedule arrays for writing
  template<typename TN>
  void WriteArray(const std::string& path, const TN* value);

protected:
  struct ADIOSWriterImpl;

  ADIOSWriterImpl *Impl;
};

#endif
// VTK-HeaderTest-Exclude: ADIOSWriter.h
