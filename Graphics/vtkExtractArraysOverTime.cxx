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

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkExtractArraysOverTime, "1.4");
vtkStandardNewMacro(vtkExtractArraysOverTime);

//----------------------------------------------------------------------------
vtkExtractArraysOverTime::vtkExtractArraysOverTime()
{
  this->NumberOfTimeSteps = 0;
  this->CurrentTimeIndex = 0;

  this->SetNumberOfInputPorts(2);

  this->ContentType = -1;
  this->FieldType = vtkSelection::CELL;

  this->Error = vtkExtractArraysOverTime::NoError;
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");    
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }
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
int vtkExtractArraysOverTime::ProcessRequest(
  vtkInformation* request,
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
    return this->RequestUpdateExtent(request,
                                     inputVector,
                                     outputVector);
    }
  
  // generate the data
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    if (this->NumberOfTimeSteps == 0)
      {
      vtkErrorMacro("No time steps in input data!");
      return 0;
      }

    // get the output data object
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkRectilinearGrid *output = vtkRectilinearGrid::GetData(outInfo);

    // get the input data object
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkDataSet *input = vtkDataSet::GetData(inInfo);

    // is this the first request
    if (!this->CurrentTimeIndex)
      {
      vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);
      vtkSelection* selection = vtkSelection::GetData(inInfo2);

      vtkInformation* properties = selection->GetProperties();
      if (properties->Has(vtkSelection::CONTENT_TYPE()))
        {
        this->ContentType = properties->Get(vtkSelection::CONTENT_TYPE());
        }
      else
        {
        this->ContentType = -1;
        }
  
      this->FieldType = vtkSelection::CELL;
      if (properties->Has(vtkSelection::FIELD_TYPE()))
        {
        this->FieldType = properties->Get(vtkSelection::FIELD_TYPE());
        }

      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      this->AllocateOutputData(input, output);

      this->Error = vtkExtractArraysOverTime::NoError;
      }

    this->ExecuteTimeStep(inputVector, outInfo);

    // increment the time index
    this->CurrentTimeIndex++;
    if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
      {
      this->PostExecute(request, inputVector, outputVector);
      }
    
    return 1;
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::RequestUpdateExtent(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo1 = inputVector[0]->GetInformationObject(0);
  vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);

  // get the requested update extent
  double *inTimes = inputVector[0]->GetInformationObject(0)->Get(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
    {
    double timeReq[1];
    timeReq[0] = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set( 
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), 
      timeReq, 
      1);
    }

  // This filter changes the ExtentTranslator on the output
  // to always update whole extent on this filter, irrespective of
  // what piece the downstream filter is requesting. Hence, we need to
  // propagate the actual extents upstream. If upstream is structured
  // data we need to use the ExtentTranslator of the input, otherwise
  // we just set the piece information. All this is taken care of
  // by SetUpdateExtent().

  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (outInfo->Has(sddp->UPDATE_NUMBER_OF_PIECES()) &&
      outInfo->Has(sddp->UPDATE_PIECE_NUMBER()) &&
      outInfo->Has(sddp->UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    int piece = outInfo->Get(sddp->UPDATE_PIECE_NUMBER());
    int numPieces = outInfo->Get(sddp->UPDATE_NUMBER_OF_PIECES());
    int ghostLevel = outInfo->Get(sddp->UPDATE_NUMBER_OF_GHOST_LEVELS());

    sddp->SetUpdateExtent(inInfo1, piece, numPieces, ghostLevel);
    sddp->SetUpdateExtent(inInfo2, piece, numPieces, ghostLevel);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::PostExecute(
  vtkInformation* request,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  // Tell the pipeline to stop looping.
  request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  this->CurrentTimeIndex = 0;
  switch (this->Error)
    {
    case vtkExtractArraysOverTime::MoreThan1Indices:
      vtkErrorMacro(<< "This filter can extract only 1 cell or "
                    << " point at a time. Only the first index"
                    << " was extracted");
      
    }

  // The vtkEAOTValidity array is for internal use only. Remove it
  // before returning control to the executive.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkRectilinearGrid *output = vtkRectilinearGrid::GetData(outInfo);
  if (output->GetPointData()->GetArray("vtkEAOTValidity"))
    {
    output->GetPointData()->RemoveArray("vtkEAOTValidity");
    }
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::AllocateOutputData(vtkDataSet *input, 
                                                 vtkRectilinearGrid *output)
{
  output->SetDimensions(this->NumberOfTimeSteps, 1, 1);

  // now the point data
  vtkDataSetAttributes* attr = 0;
  switch (this->FieldType)
    {
    case vtkSelection::CELL:
      attr = input->GetCellData();
      break;
    case vtkSelection::POINT:
      attr = input->GetPointData();
    }
  output->GetPointData()->CopyAllocate(attr, 
                                       this->NumberOfTimeSteps);

  // Add an array to hold the time at each step
  vtkDoubleArray *timeArray = vtkDoubleArray::New();
  timeArray->SetNumberOfComponents(1);
  timeArray->SetNumberOfTuples(this->NumberOfTimeSteps);
  if (attr->GetArray("Time"))
    {
    timeArray->SetName("TimeData");
    }
  else
    {
    timeArray->SetName("Time");
    }
  output->GetPointData()->AddArray(timeArray);
  // Assign this array as the x-coords
  output->SetXCoordinates(timeArray);
  timeArray->Delete();

  // Assign dummy y and z coordinates
  vtkDoubleArray* yCoords = vtkDoubleArray::New();
  yCoords->SetNumberOfComponents(1);
  yCoords->SetNumberOfTuples(1);
  yCoords->SetTuple1(0, 0.0);
  output->SetYCoordinates(yCoords);
  yCoords->Delete();

  vtkDoubleArray* zCoords = vtkDoubleArray::New();
  zCoords->SetNumberOfComponents(1);
  zCoords->SetNumberOfTuples(1);
  zCoords->SetTuple1(0, 0.0);
  output->SetZCoordinates(zCoords);
  zCoords->Delete();

  // These are the point coordinates of the original data
  vtkDoubleArray* coordsArray = vtkDoubleArray::New();
  coordsArray->SetNumberOfComponents(3);
  coordsArray->SetNumberOfTuples(this->NumberOfTimeSteps);
  if (attr->GetArray("Point Coordinates"))
    {
    coordsArray->SetName("Points");
    }
  else
    {
    coordsArray->SetName("Point Coordinates");
    }
  output->GetPointData()->AddArray(coordsArray);
  coordsArray->Delete();

  // This array is here for the sake of a parallel sub-class.
  // It is removed in PostExecute().
  vtkUnsignedCharArray* validPts = vtkUnsignedCharArray::New();
  validPts->SetName("vtkEAOTValidity");
  validPts->SetNumberOfTuples(this->NumberOfTimeSteps);
  output->GetPointData()->AddArray(validPts);
  for (vtkIdType i=0; i<this->NumberOfTimeSteps; i++)
    {
    validPts->SetValue(i, 0);
    }
  validPts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Returns index based on the selection time.
vtkIdType vtkExtractArraysOverTime::GetIndex(vtkIdType selIndex,
                                             vtkDataSet* input)
{
  // If selection type is indices, return the selIndex itself
  if (this->ContentType == vtkSelection::INDICES)
    {
    return selIndex;
    }
  // If selection type is global ids, find the right index
  else if (this->ContentType == vtkSelection::GLOBALIDS)
    {
    vtkDataSetAttributes* attr = 0;
    switch (this->FieldType)
      {
      case vtkSelection::CELL:
        attr = input->GetCellData();
        break;
      case vtkSelection::POINT:
        attr = input->GetPointData();
      }
    if (attr)
      {
      // Get the global id array
      vtkIdTypeArray* globalIds = vtkIdTypeArray::SafeDownCast(
        attr->GetGlobalIds());
      if (globalIds)
        {
        // Find the point/cell that has the given global id
        vtkIdType numVals = globalIds->GetNumberOfTuples()*
          globalIds->GetNumberOfComponents();
        for (vtkIdType i=0; i<numVals; i++)
          {
          vtkIdType idx = globalIds->GetValue(i);
          if (idx == selIndex)
            {
            return idx;
            }
          }
        }
      }    
    }
  return -1;
}

//----------------------------------------------------------------------------
// This is executed once at every time step
void vtkExtractArraysOverTime::ExecuteTimeStep(vtkInformationVector** inputV, 
                                               vtkInformation* outInfo)
{
  vtkRectilinearGrid *output = vtkRectilinearGrid::GetData(outInfo);
  int piece = 0;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }

  vtkInformation* inInfo1 = inputV[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::GetData(inInfo1);
  vtkInformation* inInfo2 = inputV[1]->GetInformationObject(0);
  vtkSelection* selection = vtkSelection::GetData(inInfo2);

  vtkIdType numElems = 0;
  vtkDataSetAttributes* attr = 0;
  // Get the right attribute and number of elements
  switch (this->FieldType)
    {
    case vtkSelection::CELL:
      numElems = input->GetNumberOfCells();
      attr = input->GetCellData();
      break;
    case vtkSelection::POINT:
      numElems = input->GetNumberOfPoints();
      attr = input->GetPointData();
    }

  // This is the time array
  if (attr->GetArray("Time"))
    {
    output->GetPointData()->GetArray("TimeData")->SetTuple1(
      this->CurrentTimeIndex, 
      input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEPS())[0]);
    }
  else
    {
    output->GetPointData()->GetArray("Time")->SetTuple1(
      this->CurrentTimeIndex, 
      input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEPS())[0]);
    }

  if (this->ContentType < 0)
    {
    return;
    }
  
  vtkInformation* selProperties = selection->GetProperties();
  if (selProperties->Has(vtkSelection::PROCESS_ID()) &&
      piece != selProperties->Get(vtkSelection::PROCESS_ID()))
    {
    vtkDebugMacro("Selection from a different process");
    return;
    }

  vtkIdTypeArray* idArray = vtkIdTypeArray::SafeDownCast(
    selection->GetSelectionList());
  if (!idArray || idArray->GetNumberOfTuples() == 0)
    {
    vtkDebugMacro(<< "Empty selection");
    return;
    }

  if (idArray->GetNumberOfTuples() > 1)
    {
    this->Error = vtkExtractArraysOverTime::MoreThan1Indices;
    }

  if (this->ContentType != vtkSelection::INDICES &&
      this->ContentType != vtkSelection::GLOBALIDS)
    {
    vtkErrorMacro("Only selections of type INDICES and GLOBALIDS are "
                  "supported. Cannot extract.");
    return;
    }

  // Extract the selected point/cell data at this time step
  vtkIdType index = this->GetIndex(idArray->GetValue(0), input);

  if (index >= 0 && index < numElems)
    {
    vtkUnsignedCharArray* validPts = 
      vtkUnsignedCharArray::SafeDownCast(
        output->GetPointData()->GetArray("vtkEAOTValidity"));
    if (validPts)
      {
      validPts->SetValue(this->CurrentTimeIndex, 1);
      }

    // extract the actual data
    output->GetPointData()->CopyData(attr, 
                                     index,
                                     this->CurrentTimeIndex);
    double* point = input->GetPoint(index);
    if (attr->GetArray("Point Coordinates"))
      {
      output->GetPointData()->GetArray("Points")->SetTuple(
        this->CurrentTimeIndex,
        point);
      }
    else
      {
      output->GetPointData()->GetArray("Point Coordinates")->SetTuple(
        this->CurrentTimeIndex,
        point);
      }
    }
}

