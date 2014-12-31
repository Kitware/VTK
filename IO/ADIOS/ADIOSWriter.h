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

#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <adios_mpi.h>

#include "ADIOSDefs.h"
#include "ADIOSUtilities.h"

namespace ADIOS
{

// An array dimension as either a string or integer
struct ArrayDim
{
  ArrayDim(size_t i) : ValueI(i), ValueS("") { }
  ArrayDim(const std::string& var) : ValueI(0), ValueS(var) { }

  size_t ValueI;
  std::string ValueS;
};

class Writer
{
public:
  static bool SetCommunicator(MPI_Comm);

public:
  Writer(ADIOS::TransportMethod transport, const std::string& transportArgs);

  ~Writer();

  // Description:
  // Define scalar attributes
  template<typename TN>
  void DefineAttribute(const std::string& path, const TN& value)
  {
    std::stringstream ss;
    ss << value;
    this->DefineAttribute(path, Type::NativeToADIOS<TN>(), ss.str());
  }

  // Description:
  // Define scalars for later writing
  template<typename TN>
  int DefineScalar(const std::string& path)
  {
    return this->DefineScalar(path, Type::NativeToADIOS<TN>());
  }

  // Define an array for later writing
  template<typename TN>
  int DefineLocalArray(const std::string& path,
    const std::vector<ArrayDim>& dims, Transform xfm = Transform_NONE)
  {
    return this->DefineLocalArray(path, Type::NativeToADIOS<TN>(), dims, xfm);
  }
  int DefineLocalArray(const std::string& path, ADIOS_DATATYPES adiosType,
    const std::vector<ArrayDim>& dims, Transform xfm = Transform_NONE);

  // Description:
  // Enqueue a scalar for writing
  template<typename TN>
  void WriteScalar(const std::string& path, const TN& val)
  {
    this->WriteScalar(path, Type::NativeToADIOS<TN>(), &val);
  }
  void WriteScalar(const std::string& path, ADIOS_DATATYPES adiosType,
    const void* val);

  // Description:
  // Enqueue an array for writing
  void WriteArray(const std::string& path, const void* val);

  // Description:
  // Perform all writes for the current time step
  void Commit(const std::string& fileName, bool append = false);

private:
  struct InitContext;
  InitContext *Ctx;

  struct WriterImpl;
  WriterImpl *Impl;

  void DefineAttribute(const std::string& path, ADIOS_DATATYPES adiosType,
    const std::string& value);
  int DefineScalar(const std::string& path, ADIOS_DATATYPES adiosType);
};

}
#endif // _ADIOSWriter_h
