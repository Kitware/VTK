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
#include "vtkGenericCell.h"
#include <vtkstd/vector>

class vtkExtractArraysOverTimeInternal
{
  //storage for the set of cell data arrays that must be copied in 
  //ExtractLocationOverTime
public:
  vtkExtractArraysOverTimeInternal()
  {
    this->OutArrays.reserve(256);
  }

  ~vtkExtractArraysOverTimeInternal()
  {
    if (this->OutArrays.size() != 0)
      {
      this->OutArrays.clear();
      }
  }

  void CreateArrays(int n)
  {
    if (this->OutArrays.size() != 0)
      {
      this->OutArrays.clear();
      }
    this->OutArrays.resize(n);
  }

  int GetSize()
  {
    return this->OutArrays.size();
  }

  vtkstd::vector<vtkDataArray*> OutArrays;
};

vtkCxxRevisionMacro(vtkExtractArraysOverTime, "1.11");
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

  this->Internal = new vtkExtractArraysOverTimeInternal;
}

//----------------------------------------------------------------------------
vtkExtractArraysOverTime::~vtkExtractArraysOverTime()
{
  delete this->Internal;
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
    
    if ((this->ContentType == vtkSelection::INDICES) ||        
        (this->ContentType == vtkSelection::GLOBALIDS))
      {
      this->ExecuteIdAtTimeStep(inputVector, outInfo);
      }
    if (this->ContentType == vtkSelection::LOCATIONS)
      {     
      this->ExecuteLocationAtTimeStep(inputVector, outInfo);
      }

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

  //Use the vtkEAOTValidity array to zero any invalid samples.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkRectilinearGrid *output = vtkRectilinearGrid::GetData(outInfo);
  vtkRectilinearGrid *cleanOut = this->RemoveInvalidPoints(output);
  output->DeepCopy(cleanOut);
  cleanOut->Delete();
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::AllocateOutputData(vtkDataSet *input, 
                                                 vtkRectilinearGrid *output)
{
  output->SetDimensions(this->NumberOfTimeSteps, 1, 1);

  // now the point data
  vtkDataSetAttributes* attr = 0;
  if (this->ContentType == vtkSelection::LOCATIONS)
    {
    attr = input->GetPointData();
    }
  else
    {
    switch (this->FieldType)
      {
      case vtkSelection::CELL:
        attr = input->GetCellData();
        break;
      case vtkSelection::POINT:
        attr = input->GetPointData();
      }
    }
  output->GetPointData()->CopyAllOn();
  if (this->ContentType == vtkSelection::LOCATIONS)
    {
    output->GetPointData()->InterpolateAllocate(
      input->GetPointData(), 
      this->NumberOfTimeSteps);
    }
  else
    {
    output->GetPointData()->CopyAllocate(attr, 
                                         this->NumberOfTimeSteps);
    }

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

  if (this->FieldType == vtkSelection::POINT)
    {
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
    }

  // Create an array of point ids to record what pts makeup the found cells.
  if (this->FieldType == vtkSelection::CELL)
    {
    vtkIdType nPtIds = input->GetMaxCellSize();
    vtkIdTypeArray *ptIdsArray = vtkIdTypeArray::New();
    ptIdsArray->SetName("Cell's Point Ids");
    ptIdsArray->SetNumberOfComponents(nPtIds);
    ptIdsArray->SetNumberOfTuples(this->NumberOfTimeSteps);
    for (vtkIdType i=0; i<this->NumberOfTimeSteps; i++)
      {
      for (vtkIdType j=0; j<nPtIds; j++)
        {
        ptIdsArray->SetComponent(i,j,-1);
        }
      }
    output->GetPointData()->AddArray(ptIdsArray);
    ptIdsArray->Delete();
    }

  if (this->ContentType == vtkSelection::LOCATIONS)
    {
    //the output point data already has arrays for the input point data
    //add another set of arrays for the input cell data,
    //or at least the arrays in it that don't have name collisions with 
    //in the point data arrays.
    //this->Internal keeps a record of it all so that we can reuse it later
    int numArrays = input->GetCellData()->GetNumberOfArrays();
    this->Internal->CreateArrays(numArrays);
      
    vtkPointData *opd = output->GetPointData();
    for (int i=0; i<numArrays; i++)
      {      
      vtkDataArray* inArray = input->GetCellData()->GetArray(i);
      if (inArray && inArray->GetName() && !opd->GetArray(inArray->GetName()))
        {
        vtkDataArray* newArray = inArray->NewInstance();
        newArray->SetName(inArray->GetName());
        newArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
        newArray->Allocate(this->NumberOfTimeSteps);
        opd->AddArray(newArray);
        this->Internal->OutArrays[i] = newArray;
        newArray->Delete();
        }
      else
        {
        this->Internal->OutArrays[i] = 0;
        }
      }
    }

  // This array is used to make particular samples as invalid.
  // This happens when we are looking at a location which is not contained
  // by a cell or at a cell or point id that is destroyed.
  // It is used in the parallel subclass as well.
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
            return i;
            }
          }
        }
      }    
    }
  return -1;
}

//----------------------------------------------------------------------------
// This is executed once at every time step
void vtkExtractArraysOverTime::ExecuteIdAtTimeStep(
  vtkInformationVector** inputV, 
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

  // Record the time to plot with later
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
    
  // Extract the selected point/cell data at this time step
  vtkIdType index = this->GetIndex(idArray->GetValue(0), input);
  
  if (index >= 0 && index < numElems)
    {
    //record that their is good data at this sample time
    vtkUnsignedCharArray* validPts = 
      vtkUnsignedCharArray::SafeDownCast(
        output->GetPointData()->GetArray("vtkEAOTValidity"));
    if (validPts)
      {
      validPts->SetValue(this->CurrentTimeIndex, 1);
      }
    
    if (this->FieldType == vtkSelection::POINT)
      {
      //save the location of our chosen point
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

    if (this->FieldType == vtkSelection::CELL)
      {
      //save the ids of the points that make up the cell
      vtkIdTypeArray* ptIdsArray = 
        vtkIdTypeArray::SafeDownCast(
          output->GetPointData()->GetArray("Cell's Point Ids"));
      if (ptIdsArray)
        {
        vtkCell *cell = input->GetCell(index);
        vtkIdType npts = cell->GetNumberOfPoints();
        for (vtkIdType j=0; j<npts; j++)
          {
          ptIdsArray->SetComponent(this->CurrentTimeIndex,j,
                                  cell->GetPointId(j));
          }
        }
      }

    // extract the actual data
    output->GetPointData()->CopyData(attr, 
                                     index,
                                     this->CurrentTimeIndex);

    }
  
  this->UpdateProgress(
    (double)this->CurrentTimeIndex/this->NumberOfTimeSteps);
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::ExecuteLocationAtTimeStep(
  vtkInformationVector** inputV, 
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

  vtkDataSetAttributes* ipd = input->GetPointData();
  vtkDataSetAttributes* icd = input->GetCellData();
  vtkDataSetAttributes* opd = output->GetPointData();

  vtkInformation* selProperties = selection->GetProperties();
  if (selProperties->Has(vtkSelection::PROCESS_ID()) &&
      piece != selProperties->Get(vtkSelection::PROCESS_ID()))
    {
    vtkDebugMacro("Selection from a different process");
    return;
    }

  vtkDoubleArray *locArray = vtkDoubleArray::SafeDownCast(
    selection->GetSelectionList());
  if (!locArray || locArray->GetNumberOfTuples() == 0)
    {
    vtkDebugMacro(<< "Empty selection");
    return;
    }
  
  if (locArray->GetNumberOfTuples() > 1)
    {
    this->Error = vtkExtractArraysOverTime::MoreThan1Indices;
    }

  // Record the time to plot with later
  if (ipd->GetArray("Time"))
    {
    opd->GetArray("TimeData")->SetTuple1(
      this->CurrentTimeIndex, 
      input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEPS())[0]);
    }
  else
    {
    opd->GetArray("Time")->SetTuple1(
      this->CurrentTimeIndex, 
      input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEPS())[0]);
    }
  
  // Find the cell that contains this location
  double *location = locArray->GetTuple(0);

  vtkGenericCell* cell = vtkGenericCell::New();
  vtkIdList *idList = vtkIdList::New();
  int subId;
  double pcoords[3];
  double *weights;
  double fastweights[256];
  int mcs = input->GetMaxCellSize();
  if (mcs<=256)
    {
    weights = fastweights;
    }
  else
    {
    weights = new double[mcs];
    }
  double tol2 = input->GetLength();
  tol2 = tol2 ? tol2*tol2 / 1000.0 : 0.001;

  //find the cell that contains the search location
  vtkIdType cellId = input->FindCell(location, NULL, cell,
                           0, 0.0, subId, pcoords, weights);
  if (cellId >= 0)
    {
    vtkCell *inCell = input->GetCell(cellId);
      
    //record that their is good data at this sample time
    vtkUnsignedCharArray* validPts = 
      vtkUnsignedCharArray::SafeDownCast(
        output->GetPointData()->GetArray("vtkEAOTValidity"));
    if (validPts)
      {
      validPts->SetValue(this->CurrentTimeIndex, 1);
      }
    
    //save the ids of the points that make up the cell
    vtkIdTypeArray* ptIdsArray = 
      vtkIdTypeArray::SafeDownCast(
        output->GetPointData()->GetArray("Cell's Point Ids"));
    if (ptIdsArray)
      {
      vtkIdType npts = inCell->GetNumberOfPoints();
      for (vtkIdType j=0; j<npts; j++)
        {
        ptIdsArray->SetComponent(this->CurrentTimeIndex,j,
                                 inCell->GetPointId(j));
        }
      }
    
    // extract the actual data
    //interpolate the point data from the input cell and put it in the output
    opd->InterpolatePoint(ipd, 
                          this->CurrentTimeIndex, 
                          inCell->PointIds,
                          weights);
    //copy the cell data over (as long as the names don't collide)
    for (int i=0; i<this->Internal->GetSize(); i++)
      {
      vtkDataArray* inArray = icd->GetArray(i);
      if (this->Internal->OutArrays[i])
        {
        opd->CopyTuple(inArray, 
                       this->Internal->OutArrays[i], 
                       cellId, this->CurrentTimeIndex);
        }
      }    
    }
    
  if (mcs>256)
    {
    delete[] weights;
    }
  cell->Delete();
  idList->Delete();
  
  this->UpdateProgress(
    (double)this->CurrentTimeIndex/this->NumberOfTimeSteps);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid *vtkExtractArraysOverTime::RemoveInvalidPoints(
  vtkRectilinearGrid *source
  )
{
  vtkRectilinearGrid *dest = vtkRectilinearGrid::New();
  dest->GetPointData()->CopyAllOn();

  vtkUnsignedCharArray* validPts = vtkUnsignedCharArray::SafeDownCast(
    source->GetPointData()->GetArray("vtkEAOTValidity"));
  if (!validPts)
    {
    dest->ShallowCopy(source);
    return dest;
    }

  dest->SetDimensions(this->NumberOfTimeSteps,1,1);

  // Assign dummy y and z coordinates
  vtkDoubleArray* yCoords = vtkDoubleArray::New();
  yCoords->SetNumberOfComponents(1);
  yCoords->SetNumberOfTuples(1);
  yCoords->SetTuple1(0, 0.0);
  dest->SetYCoordinates(yCoords);
  yCoords->Delete();
  vtkDoubleArray* zCoords = vtkDoubleArray::New();
  zCoords->SetNumberOfComponents(1);
  zCoords->SetNumberOfTuples(1);
  zCoords->SetTuple1(0, 0.0);
  dest->SetZCoordinates(zCoords);
  zCoords->Delete();
  
  vtkDataArray *oxc = source->GetXCoordinates();
  vtkDataArray *cxc = oxc->NewInstance();
  dest->SetXCoordinates(cxc);
  
  vtkPointData *opd = source->GetPointData();
  vtkPointData *cpd = dest->GetPointData();
  cpd->CopyAllOn();
  cpd->CopyAllocate(opd); 
  
  for (vtkIdType i=0; i<this->NumberOfTimeSteps; i++)
    {
    if (validPts->GetValue(i) == 1)
      {
      cxc->InsertNextTuple(i, oxc);
      cpd->CopyData(opd, i, i);
      }
    else
      {
      //an invalid sample, set all the data values to 0.0
      vtkIdType narrays = cpd->GetNumberOfArrays();

      for (vtkIdType a = 0; a < narrays; a++)
        {
        vtkDataArray *da = cpd->GetArray(a);
        for (vtkIdType j = 0; j < da->GetNumberOfComponents(); j++)
          {
          da->InsertComponent(i, j, 0.0);
          }    
        }

      //keep the time value, it is valid
      //cerr << "sample " << i << " is invalid" << endl;
      cxc->InsertNextTuple(i, oxc);
     
      vtkDoubleArray *itimeArray = vtkDoubleArray::SafeDownCast(
        source->GetPointData()->GetArray("TimeData"));
      vtkDoubleArray *otimeArray = vtkDoubleArray::SafeDownCast(
        dest->GetPointData()->GetArray("TimeData"));
      if (!itimeArray)
        {
        itimeArray = vtkDoubleArray::SafeDownCast(
          source->GetPointData()->GetArray("Time"));
        otimeArray = vtkDoubleArray::SafeDownCast(
          dest->GetPointData()->GetArray("Time"));
        }
      if (itimeArray)
        {
        otimeArray->SetValue(i, itimeArray->GetValue(i));
        }

      }
    }
  cxc->Delete();

  //This array is for internal use only (by this class and it's parallel 
  //descendent) so we remove it here.

  //Actually it will be useful to use it in the graph code so we keep it.
  //dest->GetPointData()->RemoveArray("vtkEAOTValidity");
  return dest;
}
