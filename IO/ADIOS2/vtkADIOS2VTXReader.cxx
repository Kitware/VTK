/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkADIOS2VTXReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 *  vtkADIOS2VTXReader.cxx
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "vtkADIOS2VTXReader.h"

#include "VTX/VTXSchemaManager.h"
#include "VTX/common/VTXHelper.h"

#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkADIOS2VTXReader);

vtkADIOS2VTXReader::vtkADIOS2VTXReader()
  : FileName(nullptr)
  , SchemaManager(new vtx::VTXSchemaManager)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

int vtkADIOS2VTXReader::RequestInformation(vtkInformation* vtkNotUsed(inputVector),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  this->SchemaManager->Update(FileName); // check if FileName changed

  // set time info
  const std::vector<double> vTimes =
    vtx::helper::MapKeysToVector(this->SchemaManager->Reader->Times);

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS(), vTimes.data(), static_cast<int>(vTimes.size()));

  const std::vector<double> timeRange = { vTimes.front(), vTimes.back() };
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange.data(),
    static_cast<int>(timeRange.size()));

  return 1;
}

int vtkADIOS2VTXReader::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  const double newTime = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  this->SchemaManager->Step = this->SchemaManager->Reader->Times[newTime];
  this->SchemaManager->Time = newTime;
  return 1;
}

int vtkADIOS2VTXReader::RequestData(vtkInformation* vtkNotUsed(inputVector),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
  vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(output);

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->SchemaManager->Time);
  this->SchemaManager->Fill(multiBlock, this->SchemaManager->Step);
  return 1;
}

void vtkADIOS2VTXReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}
