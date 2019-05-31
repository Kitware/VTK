/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2Schema.txx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2Schema.txx
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_SCHEMA_ADIOS2SCHEMA_TCC_
#define VTK_IO_ADIOS2_SCHEMA_ADIOS2SCHEMA_TCC_

#include "ADIOS2Schema.h"

#include <stdexcept>

#include "ADIOS2Helper.h"

namespace adios2vtk
{

template<class T>
void ADIOS2Schema::GetDataArrayCommon(adios2::Variable<T> variable, types::DataArray& dataArray,
  const size_t step, const std::string mode)
{
  if (!variable)
  {
    throw std::runtime_error(
      "ERROR: variable " + variable.Name() + " not found in " + m_Engine->Name() + "\n");
  }

  if (variable.ShapeID() != adios2::ShapeID::GlobalArray &&
    variable.ShapeID() != adios2::ShapeID::LocalArray)
  {
    return;
  }

  SetDimensions(variable, dataArray, step);

  variable.SetStepSelection({ step, 1 });
  const size_t nElements = helper::TotalElements(dataArray.m_Count);

  dataArray.m_vtkDataArray = helper::NewDataArray<T>();
  dataArray.m_vtkDataArray->Allocate(nElements);
  dataArray.m_vtkDataArray->SetNumberOfValues(nElements);
  dataArray.m_vtkDataArray->SetNumberOfTuples(nElements);
  dataArray.m_vtkDataArray->SetNumberOfComponents(1);
  dataArray.m_vtkDataArray->SetName(variable.Name().c_str());
  T* ptr = reinterpret_cast<T*>(dataArray.m_vtkDataArray->GetVoidPointer(0));

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
