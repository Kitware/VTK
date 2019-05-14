/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2VTK.cxx
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2Schema.h"
#include "ADIOS2Schema.txx"

#include "ADIOS2Helper.h"
#include "ADIOS2Types.h"

#include "vtkDataArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSmartPointer.h"

namespace adios2vtk
{

ADIOS2Schema::ADIOS2Schema(
  const std::string type, const std::string& schema, adios2::IO* io, adios2::Engine* engine)
  : m_Type(type)
  , m_Schema(schema)
  , m_IO(io)
  , m_Engine(engine)
{
}

ADIOS2Schema::~ADIOS2Schema() {}

void ADIOS2Schema::Fill(vtkMultiBlockDataSet* multiBlock, const size_t step)
{
  DoFill(multiBlock, step);
}

void ADIOS2Schema::GetDataArray(const std::string& variableName,
  vtkSmartPointer<vtkDataArray>& dataArray, const size_t step, const std::string mode)
{
  const std::string type = m_IO->VariableType(variableName);

  if (type.empty())
  {
  }
#define declare_type(T)                                                                            \
  else if (type == adios2::GetType<T>())                                                           \
  {                                                                                                \
    GetDataArrayCommon<T>(variableName, dataArray, step, mode);                                    \
  }
  ADIOS2_VTK_ARRAY_TYPE(declare_type)
#undef declare_type
}

void ADIOS2Schema::GetTimes(const std::string& variableName)
{
  if (m_Engine == nullptr)
  {
    throw std::runtime_error(
      "ERROR: Engine is null when populating time variable " + variableName + " \n");
  }

  if (variableName.empty())
  {
    // TODO: implements steps function in Engine
    //    while (m_Engine->BeginStep() != adios2::StepStatus::EndOfStream)
    //    {
    //      const size_t currentStep = m_Engine->CurrentStep();
    //      const double currentStepDbl = static_cast<double>(currentStep);
    //      m_Times[currentStepDbl] = currentStep;
    //      m_Engine->EndStep();
    //    }
    return;
  }

  // if variable is found
  const std::string type = m_IO->VariableType(variableName);

  if (type.empty())
  {
    throw std::invalid_argument("ERROR: time variable " + variableName + " not present " +
      " in Engine " + m_Engine->Name() + " when reading time data\n");
  }
#define declare_type(T)                                                                            \
  else if (type == adios2::GetType<T>()) { GetTimesCommon<T>(variableName); }
  ADIOS2_VTK_TIME_TYPE(declare_type)
#undef declare_type
}

} // end namespace adios2vtk
