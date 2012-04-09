/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractTemporalFieldData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractTemporalFieldData.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTable.h"
#include "vtkSelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkGenericCell.h"
#include "vtkStdString.h"
#include <vector>



vtkStandardNewMacro(vtkExtractTemporalFieldData);

//----------------------------------------------------------------------------
vtkExtractTemporalFieldData::vtkExtractTemporalFieldData()
{
  this->NumberOfTimeSteps = 0;
}

//----------------------------------------------------------------------------
vtkExtractTemporalFieldData::~vtkExtractTemporalFieldData()
{
}

//----------------------------------------------------------------------------
void vtkExtractTemporalFieldData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
}

//----------------------------------------------------------------------------
int vtkExtractTemporalFieldData::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");    
  return 1;
}


//----------------------------------------------------------------------------
int vtkExtractTemporalFieldData::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }
  else if(
    request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request,
                            inputVector,
                            outputVector);
    }
  
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}


//----------------------------------------------------------------------------
int vtkExtractTemporalFieldData::RequestInformation(
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

  return 1;
}


//----------------------------------------------------------------------------
int vtkExtractTemporalFieldData::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (this->NumberOfTimeSteps == 0)
    {
    vtkErrorMacro("No time steps in input data!");
    return 0;
    }

  // get the output data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable *output = vtkTable::GetData(outInfo);

  // get the input data object
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::GetData(inInfo);

  this->CopyDataToOutput(inInfo, input, output);

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractTemporalFieldData::CopyDataToOutput(
  vtkInformation* inInfo,
  vtkDataSet *input, vtkTable *output)
{
  vtkDataSetAttributes *opd = output->GetRowData();
  vtkFieldData *ifd = input->GetFieldData();
  int numArrays = 0;
    
  if(!ifd || !opd)
    {
    vtkErrorMacro("Unsupported field type.");
    return;
    }

  for (vtkIdType j=0; j<ifd->GetNumberOfArrays(); j++)
    {  
    vtkDataArray* inFieldArray = ifd->GetArray(j);
    if (inFieldArray && 
        inFieldArray->GetName() && 
        !inFieldArray->IsA("vtkIdTypeArray") && 
        inFieldArray->GetNumberOfTuples() == this->NumberOfTimeSteps)
      {
      vtkDataArray *outPointArray = vtkDataArray::CreateDataArray(inFieldArray->GetDataType());
      //outPointArray->DeepCopy(inFieldArray);
      outPointArray->SetNumberOfComponents(inFieldArray->GetNumberOfComponents());
      outPointArray->SetNumberOfTuples(this->NumberOfTimeSteps);
      for(vtkIdType i=0; i<inFieldArray->GetNumberOfComponents(); i++)
        {
        outPointArray->CopyComponent(i,inFieldArray,i);
        }
      outPointArray->SetName(inFieldArray->GetName());

      opd->AddArray(outPointArray);
      outPointArray->Delete();
      numArrays++;
      }
    }

  double *timesteps = NULL;
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    timesteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }

  // Add an array to hold the time at each step
  vtkDoubleArray *timeArray = vtkDoubleArray::New();
  timeArray->SetNumberOfComponents(1);
  timeArray->SetNumberOfTuples(this->NumberOfTimeSteps);
  if (ifd->GetArray("Time"))
    {
    timeArray->SetName("TimeData");
    }
  else
    {
    timeArray->SetName("Time");
    }
  for(int m=0; m<this->NumberOfTimeSteps; m++)
    {
    timeArray->SetTuple1(m, timesteps[m]);
    }
  opd->AddArray(timeArray);
  timeArray->Delete();

  // This array is used to make particular samples as invalid.
  // This happens when we are looking at a location which is not contained
  // by a cell or at a cell or point id that is destroyed.
  // It is used in the parallel subclass as well.
  vtkUnsignedCharArray* validPts = vtkUnsignedCharArray::New();
  validPts->SetName("vtkValidPointMask");
  validPts->SetNumberOfTuples(this->NumberOfTimeSteps);
  opd->AddArray(validPts);
  // if no valid field arrays were found, which would happen if the reader
  // did not have the requested data, set validity to 0, otherwise 1.
  int validity = numArrays ? 1 : 0;
  validPts->FillComponent(0,validity);
  validPts->Delete();

}
