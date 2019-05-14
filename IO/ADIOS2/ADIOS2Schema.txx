/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2VTK.txx
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */
#ifndef VTK_IO_ADIOS2_ADIOS2SCHEMA_TCC_
#define VTK_IO_ADIOS2_ADIOS2SCHEMA_TCC_

#include "ADIOS2Schema.h"

#include <stdexcept>

#include "ADIOS2Helper.h"
#include <adios2.h>

namespace adios2vtk
{

template<class T>
void ADIOS2Schema::GetDataArrayCommon(const std::string& variableName,
  vtkSmartPointer<vtkDataArray>& dataArray, const size_t step, const std::string mode)
{
  adios2::Variable<T> variable = m_IO->InquireVariable<T>(variableName);
  if (variable.ShapeID() != adios2::ShapeID::GlobalArray &&
    variable.ShapeID() != adios2::ShapeID::LocalArray)
  {
    return;
  }

  if (!variable)
  {
    throw std::runtime_error(
      "ERROR: variable " + variable.Name() + " not found in " + m_Engine->Name() + "\n");
  }

  const adios2::Dims shape = variable.Shape(step);
  // TODO: 1 block for now, MPI partition later
  variable.SetSelection({ adios2::Dims(shape.size(), 0), shape });
  variable.SetStepSelection({ step, 1 });

  const size_t nElements = helper::TotalElements(shape);

  dataArray = helper::NewDataArray<T>();
  dataArray->Allocate(nElements);
  dataArray->SetNumberOfValues(nElements);
  dataArray->SetNumberOfTuples(nElements);
  dataArray->SetNumberOfComponents(1);
  dataArray->SetName(variableName.c_str());
  T* ptr = reinterpret_cast<T*>(dataArray->GetVoidPointer(0));

  if (mode == "deferred")
  {
    m_Engine->Get(variable, ptr);
  }
  else if (mode == "sync")
  {
    m_Engine->Get(variable, ptr, adios2::Mode::Sync);
  }
}

template<class T>
void ADIOS2Schema::GetTimesCommon(const std::string& variableName)
{
  adios2::Variable<T> varTime = m_IO->InquireVariable<T>(variableName);
  varTime.SetStepSelection({ 0, varTime.Steps() });
  std::vector<T> timeValues;
  m_Engine->Get(varTime, timeValues, adios2::Mode::Sync);

  size_t currentStep = 0;
  for (const T timeValue : timeValues)
  {
    const double timeDbl = static_cast<double>(timeValue);
    m_Times[timeDbl] = currentStep;
    ++currentStep;
  }
}

} // end namespace adios2vtk

#endif
