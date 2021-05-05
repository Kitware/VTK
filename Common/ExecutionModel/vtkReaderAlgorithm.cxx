/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReaderAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReaderAlgorithm.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//------------------------------------------------------------------------------
vtkReaderAlgorithm::vtkReaderAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkReaderAlgorithm::~vtkReaderAlgorithm() = default;

//------------------------------------------------------------------------------
vtkTypeBool vtkReaderAlgorithm::ProcessRequest(
  vtkInformation* request, vtkInformationVector** vtkNotUsed(inInfo), vtkInformationVector* outInfo)
{
  using vtkSDDP = vtkStreamingDemandDrivenPipeline;
  vtkInformation* reqs = outInfo->GetInformationObject(0);
  const int hasTime = reqs->Has(vtkSDDP::UPDATE_TIME_STEP());
  const double* steps = reqs->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int timeIndex = 0;
  if (hasTime && steps)
  {
    double requestedTimeStep = reqs->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    int length = reqs->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < length - 1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    timeIndex = cnt;
  }

  int result = 1;
  if (request->Has(vtkSDDP::REQUEST_DATA_OBJECT()))
  {
    vtkDataObject* currentOutput = vtkDataObject::GetData(outInfo);
    vtkDataObject* output = this->CreateOutput(currentOutput);
    if (output)
    {
      result = 1;
      if (output != currentOutput)
      {
        outInfo->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
        output->Delete();
      }
    }
  }
  else if (request->Has(vtkSDDP::REQUEST_INFORMATION()))
  {
    try
    {
      result = this->ReadMetaData(outInfo->GetInformationObject(0));
    }
    catch (const std::exception&)
    {
      result = 0;
    }
  }
  else if (request->Has(vtkSDDP::REQUEST_TIME_DEPENDENT_INFORMATION()))
  {
    try
    {
      result = this->ReadTimeDependentMetaData(timeIndex, outInfo->GetInformationObject(0));
    }
    catch (const std::exception&)
    {
      result = 0;
    }
  }
  else if (request->Has(vtkSDDP::REQUEST_DATA()))
  {
    int piece =
      reqs->Has(vtkSDDP::UPDATE_PIECE_NUMBER()) ? reqs->Get(vtkSDDP::UPDATE_PIECE_NUMBER()) : 0;
    int npieces = reqs->Has(vtkSDDP::UPDATE_NUMBER_OF_PIECES())
      ? reqs->Get(vtkSDDP::UPDATE_NUMBER_OF_PIECES())
      : 1;
    int nghosts = reqs->Get(vtkSDDP::UPDATE_NUMBER_OF_GHOST_LEVELS());
    vtkDataObject* output = vtkDataObject::GetData(outInfo);

    try
    {
      result = this->ReadMesh(piece, npieces, nghosts, timeIndex, output);
      if (result)
      {
        result = this->ReadPoints(piece, npieces, nghosts, timeIndex, output);
      }
      if (result)
      {
        result = this->ReadArrays(piece, npieces, nghosts, timeIndex, output);
      }
    }
    catch (const std::exception&)
    {
      result = 0;
    }

    if (!result && output != nullptr)
    {
      // cleanup output so we don't end up producing partial results.
      output->Initialize();
    }
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkReaderAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
