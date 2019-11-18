/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkReaderExecutive.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReaderExecutive.h"

#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkReaderAlgorithm.h"

vtkStandardNewMacro(vtkReaderExecutive);

//----------------------------------------------------------------------------
vtkReaderExecutive::vtkReaderExecutive() {}

//----------------------------------------------------------------------------
vtkReaderExecutive::~vtkReaderExecutive() {}

//----------------------------------------------------------------------------
void vtkReaderExecutive::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkReaderExecutive::CallAlgorithm(vtkInformation* request, int direction,
  vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  // Copy default information in the direction of information flow.
  this->CopyDefaultInformation(request, direction, inInfo, outInfo);

  // Invoke the request on the algorithm.
  this->InAlgorithm = 1;
  int result = 1; // this->Algorithm->ProcessRequest(request, inInfo, outInfo);
  vtkReaderAlgorithm* reader = vtkReaderAlgorithm::SafeDownCast(this->Algorithm);
  if (!reader)
  {
    return 0;
  }

  using vtkSDDP = vtkStreamingDemandDrivenPipeline;
  vtkInformation* reqs = outInfo->GetInformationObject(0);
  int hasTime = reqs->Has(vtkSDDP::UPDATE_TIME_STEP());
  double* steps = reqs->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
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

  if (request->Has(REQUEST_DATA_OBJECT()))
  {
    vtkDataObject* currentOutput = vtkDataObject::GetData(outInfo);
    vtkDataObject* output = reader->CreateOutput(currentOutput);
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
  else if (request->Has(REQUEST_INFORMATION()))
  {
    result = reader->ReadMetaData(outInfo->GetInformationObject(0));
  }
  else if (request->Has(REQUEST_TIME_DEPENDENT_INFORMATION()))
  {
    result = reader->ReadTimeDependentMetaData(timeIndex, outInfo->GetInformationObject(0));
  }
  else if (request->Has(REQUEST_DATA()))
  {
    int piece =
      reqs->Has(vtkSDDP::UPDATE_PIECE_NUMBER()) ? reqs->Get(vtkSDDP::UPDATE_PIECE_NUMBER()) : 0;
    int npieces = reqs->Has(vtkSDDP::UPDATE_NUMBER_OF_PIECES())
      ? reqs->Get(vtkSDDP::UPDATE_NUMBER_OF_PIECES())
      : 1;
    int nghosts = reqs->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
    vtkDataObject* output = vtkDataObject::GetData(outInfo);
    result = reader->ReadMesh(piece, npieces, nghosts, timeIndex, output);
    if (result)
    {
      result = reader->ReadPoints(piece, npieces, nghosts, timeIndex, output);
    }
    if (result)
    {
      result = reader->ReadArrays(piece, npieces, nghosts, timeIndex, output);
    }
  }
  this->InAlgorithm = 0;

  // If the algorithm failed report it now.
  if (!result)
  {
    vtkErrorMacro("Algorithm " << this->Algorithm->GetClassName() << "(" << this->Algorithm
                               << ") returned failure for request: " << *request);
  }

  return result;
}
