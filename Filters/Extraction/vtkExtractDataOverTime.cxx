/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractDataOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractDataOverTime.h"

#include "vtkPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkExtractDataOverTime);

//----------------------------------------------------------------------------
vtkExtractDataOverTime::vtkExtractDataOverTime()
{
  this->NumberOfTimeSteps = 0;
  this->CurrentTimeIndex = 0;
  this->PointIndex = 0;

}

//----------------------------------------------------------------------------
void vtkExtractDataOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Point Index: " << this->PointIndex << endl;
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
}


//----------------------------------------------------------------------------
int vtkExtractDataOverTime::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
  {
    this->NumberOfTimeSteps =
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }
  // The output of this filter does not contain a specific time, rather
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  return 1;
}


//----------------------------------------------------------------------------
int vtkExtractDataOverTime::ProcessRequest(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  else if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    // get the requested update extent
    double *inTimes = inputVector[0]->GetInformationObject(0)
      ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (inTimes)
    {
      double timeReq = inTimes[this->CurrentTimeIndex];
      inputVector[0]->GetInformationObject(0)->Set
        ( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
          timeReq);
    }
    return 1;
  }

  // generate the data
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (this->NumberOfTimeSteps == 0)
    {
      vtkErrorMacro("No Time steps in input time data!");
      return 0;
    }

    // get the output data object
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPointSet *output =
      vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // and input data object
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkPointSet *input =
      vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

    // is this the first request
    if (!this->CurrentTimeIndex)
    {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      this->AllocateOutputData(input, output);
    }

    // extract the actual data
    output->GetPoints()->SetPoint( this->CurrentTimeIndex,
      input->GetPoints()->GetPoint(this->PointIndex) );
    output->GetPointData()->CopyData(input->GetPointData(), this->PointIndex,
      this->CurrentTimeIndex);
    if (input->GetPointData()->GetArray("Time"))
    {
      output->GetPointData()->GetArray("TimeData")->SetTuple1
        (this->CurrentTimeIndex,
         input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP()));
    }
    else
    {
      output->GetPointData()->GetArray("Time")->SetTuple1
        (this->CurrentTimeIndex,
         input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP()));
    }


    // increment the time index
    this->CurrentTimeIndex++;
    if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
    }

    return 1;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkExtractDataOverTime::AllocateOutputData(vtkPointSet *input, vtkPointSet *output)
{
  // by default vtkPointSetAlgorithm::RequestDataObject already
  // created an output of the same type as the input
  if (!output)
  {
    vtkErrorMacro("Output not created as expected!");
    return 0;
  }

  // 1st the points
  vtkPoints *points = output->GetPoints();
  if (!points)
  {
    points = vtkPoints::New();
    output->SetPoints( points );
    points->Delete();
  }
  points->SetNumberOfPoints( this->NumberOfTimeSteps );

  // now the point data
  output->GetPointData()->CopyAllocate(input->GetPointData(), this->NumberOfTimeSteps);

  // and finally add an array to hold the time at each step
  vtkDoubleArray *timeArray = vtkDoubleArray::New();
  timeArray->SetNumberOfComponents(1);
  timeArray->SetNumberOfTuples(this->NumberOfTimeSteps);
  if (input->GetPointData()->GetArray("Time"))
  {
    timeArray->SetName("TimeData");
  }
  else
  {
    timeArray->SetName("Time");
  }
  output->GetPointData()->AddArray(timeArray);
  timeArray->Delete();

  return 1;
}


