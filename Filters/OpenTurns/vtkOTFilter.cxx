/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOTFilter.h"

#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkOTUtilities.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include "openturns/NumericalSample.hxx"

using namespace OT;

//-----------------------------------------------------------------------------
vtkOTFilter::vtkOTFilter()
{
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

//-----------------------------------------------------------------------------
vtkOTFilter::~vtkOTFilter()
{
}

//-----------------------------------------------------------------------------
int vtkOTFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkOTFilter::AddToOutput(NumericalSample* ns, const std::string& name)
{
  vtkSmartPointer<vtkDataArray> outArray =
    vtkSmartPointer<vtkDataArray>::Take(vtkOTUtilities::NumericalSampleToArray(ns));
  outArray->SetName(name.c_str());
  this->Output->AddColumn(outArray);
}

//-----------------------------------------------------------------------------
int vtkOTFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  this->Output = vtkTable::GetData(outputVector, 0);
  this->Output->Initialize();

  vtkDataArray* dataArray = this->GetInputArrayToProcess(0, inputVector);
  NumericalSample* ns = vtkOTUtilities::ArrayToNumericalSample(dataArray);

  int ret = 1;
  if(ns)
  {
    ret = this->Process(ns);
    delete ns;
  }
  return ret;
}
