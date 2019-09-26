/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXSchema.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXSchema.cxx
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VTXSchema.h"
#include "VTXSchema.txx"

#include <stdexcept>

namespace vtx
{
// PUBLIC
VTXSchema::VTXSchema(
  const std::string& type, const std::string& schema, adios2::IO& io, adios2::Engine& engine)
  : Type(type)
  , Schema(schema)
  , IO(io)
  , Engine(engine)
{
}

VTXSchema::~VTXSchema() {}

void VTXSchema::Fill(vtkMultiBlockDataSet* multiBlock, const size_t step)
{
  DoFill(multiBlock, step);
}

// PROTECTED
void VTXSchema::GetTimes(const std::string& variableName)
{
  if (variableName.empty())
  {
    // set default steps as "timesteps"
    const size_t steps = this->Engine.Steps();
    for (size_t step = 0; step < steps; ++step)
    {
      const double timeDbl = static_cast<double>(step);
      this->Times[timeDbl] = step;
    }
    return;
  }

  // if variable is found
  const std::string type = this->IO.VariableType(variableName);

  if (type.empty())
  {
    throw std::invalid_argument("ERROR: time variable " + variableName + " not present " +
      " in Engine " + this->Engine.Name() + " when reading time data\n");
  }
#define declare_type(T)                                                                            \
  else if (type == adios2::GetType<T>()) { GetTimesCommon<T>(variableName); }
  VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type
}

void VTXSchema::GetDataArray(
  const std::string& variableName, types::DataArray& dataArray, const size_t step)
{
  const std::string type = this->IO.VariableType(variableName);

  if (type.empty())
  {
  }
#define declare_type(T)                                                                            \
  else if (type == adios2::GetType<T>())                                                           \
  {                                                                                                \
    adios2::Variable<T> variable = this->IO.InquireVariable<T>(variableName);                      \
    GetDataArrayCommon<T>(variable, dataArray, step);                                              \
  }
  VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type
}

#define declare_type(T)                                                                            \
  void VTXSchema::SetDimensions(adios2::Variable<T> /*variable*/,                                  \
    const types::DataArray& /*dataArray*/, const size_t /*step*/)                                  \
  {                                                                                                \
    throw std::invalid_argument("ERROR: global array not supported for this schema\n");            \
  }                                                                                                \
                                                                                                   \
  void VTXSchema::SetBlocks(                                                                       \
    adios2::Variable<T> /*variable*/, types::DataArray& /*dataArray*/, const size_t /*step*/)      \
  {                                                                                                \
    throw std::invalid_argument("ERROR: local array not supported for this schema\n");             \
  }

VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

} // end namespace vtx
