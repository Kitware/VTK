/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkADIO2ReaderMultiBlock.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "vtkADIOS2ReaderMultiBlock.h"

#include <iostream>
#include <memory>

#include "ADIOS2Helper.h"
#include "ADIOS2SchemaManager.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkADIOS2ReaderMultiBlock);

vtkADIOS2ReaderMultiBlock::vtkADIOS2ReaderMultiBlock()
  : FileName(nullptr)
  , m_SchemaManager(new adios2vtk::ADIOS2SchemaManager)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

int vtkADIOS2ReaderMultiBlock::RequestInformation(vtkInformation* vtkNotUsed(inputVector),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  m_SchemaManager->Update(FileName); // check if FileName changed

  // set time info
  const std::vector<double> vTimes =
    adios2vtk::helper::MapKeysToVector(m_SchemaManager->m_Reader->m_Times);

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS(), vTimes.data(), static_cast<int>(vTimes.size()));

  const std::vector<double> timeRange = { vTimes.front(), vTimes.back() };
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange.data(),
    static_cast<int>(timeRange.size()));

  return 1;
}

int vtkADIOS2ReaderMultiBlock::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  const double newTime = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  m_SchemaManager->m_Step = m_SchemaManager->m_Reader->m_Times[newTime];
  m_SchemaManager->m_Time = newTime;
  return 1;
}

int vtkADIOS2ReaderMultiBlock::RequestData(vtkInformation* vtkNotUsed(inputVector),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
  vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(output);

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), m_SchemaManager->m_Time);
  m_SchemaManager->Fill(multiBlock, m_SchemaManager->m_Step);
  return 1;
}

void vtkADIOS2ReaderMultiBlock::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}
