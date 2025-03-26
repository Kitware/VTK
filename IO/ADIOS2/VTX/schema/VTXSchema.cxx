// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
VTK_ABI_NAMESPACE_BEGIN
// PUBLIC
VTXSchema::VTXSchema(
  const std::string& type, const std::string& schema, adios2::IO& io, adios2::Engine& engine)
  : Type(type)
  , Schema(schema)
  , IO(io)
  , Engine(engine)
{
}

VTXSchema::~VTXSchema() = default;

void VTXSchema::Fill(vtkMultiBlockDataSet* multiBlock, size_t step)
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
  const std::string& variableName, types::DataArray& dataArray, size_t step)
{
  const std::string type = this->IO.VariableType(variableName);

  if (type.empty())
  {
    throw std::invalid_argument(
      "ERROR: variable `" + variableName + "` not present " + "in Engine " + this->Engine.Name());
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
  void VTXSchema::SetDimensions(                                                                   \
    adios2::Variable<T> /*variable*/, const types::DataArray& /*dataArray*/, size_t /*step*/)      \
  {                                                                                                \
    throw std::invalid_argument("ERROR: global array not supported for this schema\n");            \
  }                                                                                                \
                                                                                                   \
  void VTXSchema::SetBlocks(                                                                       \
    adios2::Variable<T> /*variable*/, types::DataArray& /*dataArray*/, size_t /*step*/)            \
  {                                                                                                \
    throw std::invalid_argument("ERROR: local array not supported for this schema\n");             \
  }

VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

VTK_ABI_NAMESPACE_END
} // end namespace vtx
