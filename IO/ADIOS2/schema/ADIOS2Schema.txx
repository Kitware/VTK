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
  const size_t step, const std::string /*mode*/)
{
  // TODO support variable.ShapeID() != adios2::ShapeID::LocalArray
  if (variable.ShapeID() != adios2::ShapeID::GlobalArray)
  {
    throw std::invalid_argument("ERROR: ADIOS2VTK only supporting Global Arrays\n");
  }

  SetDimensions(variable, dataArray, step);

  variable.SetStepSelection({ step, 1 });
  const size_t nElements = helper::TotalElements(dataArray.Count);

  dataArray.Data = helper::NewDataArray<T>();
  dataArray.Data->Allocate(nElements);
  dataArray.Data->SetNumberOfValues(nElements);
  dataArray.Data->SetNumberOfTuples(nElements);
  dataArray.Data->SetNumberOfComponents(1);
  dataArray.Data->SetName(variable.Name().c_str());
  T* ptr = reinterpret_cast<T*>(dataArray.Data->GetVoidPointer(0));
  this->Engine.Get(variable, ptr);
}

template<class T>
void ADIOS2Schema::GetTimesCommon(const std::string& variableName)
{
  adios2::Variable<T> varTime = this->IO.InquireVariable<T>(variableName);
  varTime.SetStepSelection({ 0, varTime.Steps() });
  std::vector<T> timeValues;
  this->Engine.Get(varTime, timeValues, adios2::Mode::Sync);

  size_t currentStep = 0;
  for (const T timeValue : timeValues)
  {
    const double timeDbl = static_cast<double>(timeValue);
    this->Times[timeDbl] = currentStep;
    ++currentStep;
  }
}

} // end namespace adios2vtk

#endif
