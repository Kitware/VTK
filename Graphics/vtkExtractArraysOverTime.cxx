/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractArraysOverTime.h"

#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkExtractArraysOverTime, "1.1");
vtkStandardNewMacro(vtkExtractArraysOverTime);

//----------------------------------------------------------------------------
vtkExtractArraysOverTime::vtkExtractArraysOverTime()
{
  this->NumberOfTimeSteps = 0;
  this->CurrentTimeIndex = 0;
  
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::RequestInformation(
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

  int wholeExtent[6] = {0, 0, 0, 0, 0, 0};
  wholeExtent[1] = this->NumberOfTimeSteps - 1;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               wholeExtent, 6);

  // Setup ExtentTranslator so that all downstream piece requests are
  // converted to whole extent update requests, as need by this filter.
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (strcmp(
      sddp->GetExtentTranslator(outInfo)->GetClassName(), 
      "vtkOnePieceExtentTranslator") != 0)
    {
    vtkExtentTranslator* et = vtkOnePieceExtentTranslator::New();
    sddp->SetExtentTranslator(outInfo, et);
    et->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::ProcessRequest(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }
  else if(
    request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    // get the requested update extent
    double *inTimes = inputVector[0]->GetInformationObject(0)
      ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (inTimes)
      {
      double timeReq[1];
      timeReq[0] = inTimes[this->CurrentTimeIndex];
      inputVector[0]->GetInformationObject(0)->Set
        ( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), 
          timeReq, 1);
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
    vtkRectilinearGrid *output = vtkRectilinearGrid::GetData(outInfo);

    // and input data object
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkDataSet *input = vtkDataSet::GetData(inInfo);

    // is this the first request
    if (!this->CurrentTimeIndex)
      {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      this->AllocateOutputData(input, output);
      }

    // extract the actual data
    //output->GetPoints()->SetPoint( this->CurrentTimeIndex, 
    //input->GetPoints()->GetPoint(this->PointIndex) );
    output->GetPointData()->CopyData(input->GetPointData(), 
                                     0, // only node 0 for now
                                     this->CurrentTimeIndex);
    if (input->GetPointData()->GetArray("Time"))
      {
      output->GetPointData()->GetArray("TimeData")->SetTuple1(
        this->CurrentTimeIndex, 
        input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEPS())[0]);
      }
    else
      {
      output->GetPointData()->GetArray("Time")->SetTuple1 (
        this->CurrentTimeIndex, 
        input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEPS())[0]);
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
int vtkExtractArraysOverTime::AllocateOutputData(vtkDataSet *input, 
                                                 vtkRectilinearGrid *output)
{
  output->SetDimensions(this->NumberOfTimeSteps, 1, 1);

  // now the point data
  output->GetPointData()->CopyAllocate(input->GetPointData(), 
                                       this->NumberOfTimeSteps);

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


