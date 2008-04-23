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
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelection.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <vtkstd/map>

class vtkExtractArraysOverTime::vtkInternal
{
private:
  class vtkKey
    {
  public:
    unsigned int CompositeID;
    vtkIdType ID;

    vtkKey(vtkIdType id)
      {
      this->CompositeID = 0;
      this->ID = id;
      }
    vtkKey(unsigned int cid, vtkIdType id)
      {
      this->CompositeID = cid;
      this->ID = id;
      }

    bool operator<(const vtkKey& other) const
      {
      if (this->CompositeID == other.CompositeID)
        {
        return (this->ID < other.ID);
        }
      return (this->CompositeID < other.CompositeID);
      }
    };

public: // vtkValue is made public due to a bug in VS 6.0
  class vtkValue
    {
  public:
    vtkSmartPointer<vtkRectilinearGrid> Output;
    vtkSmartPointer<vtkUnsignedCharArray> ValidMaskArray;
    vtkSmartPointer<vtkDoubleArray> PointCoordinatesArray;
    };
private:


  typedef vtkstd::map<vtkKey, vtkValue> MapType;
  MapType OutputGrids;
  int NumberOfTimeSteps;
  int CurrentTimeIndex;
  int FieldType;
  int ContentType;

  void AddTimeStepInternal(unsigned int cid, double time, vtkDataSet* data);
  void AddTimeStepInternalForLocations(unsigned int composite_index, 
    double time, vtkDataSet* input);
  vtkValue* GetOutput(const vtkKey& key, vtkDataSetAttributes* inDSA);

  void RemoveInvalidPoints(vtkUnsignedCharArray* validArray,
    vtkDataSetAttributes* pd)
    {
    vtkIdType numIDs = validArray->GetNumberOfTuples();
    for (vtkIdType cc=0; cc < numIDs; cc++)
      {
      if (validArray->GetValue(cc) != 1)
        {
        //an invalid sample, set all the data values to 0.0
        vtkIdType narrays = pd->GetNumberOfArrays();
        for (vtkIdType a = 0; a < narrays; a++)
          {
          vtkDataArray *da = pd->GetArray(a);
          if (da != validArray && da != this->TimeArray.GetPointer())
            {
            for (vtkIdType j = 0; j < da->GetNumberOfComponents(); j++)
              {
              da->SetComponent(cc, j, 0.0);
              }
            }
          }
        }

      }
    }

  // We use the same time array for all extracted time lines, since that doesn't
  // change.
  vtkSmartPointer<vtkDoubleArray> TimeArray;
public:
  vtkInternal()
    {
    this->NumberOfTimeSteps = 0;
    this->FieldType = 0;
    this->CurrentTimeIndex = 0;
    this->ContentType = -1;
    }

  // Description:
  // Intializes the data structure.
  void Initialize(int numTimeSteps, int contentType, int fieldType)
    {
    this->CurrentTimeIndex = 0;
    this->NumberOfTimeSteps = numTimeSteps;
    this->FieldType = fieldType;
    this->ContentType = contentType;
    this->OutputGrids.clear();

    this->TimeArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->TimeArray->SetNumberOfTuples(this->NumberOfTimeSteps);
    this->TimeArray->FillComponent(0, 0);
    }

  // Description:
  // Add the output of the extract selection filter. 
  void AddTimeStep(double time, vtkDataObject* data);

  // Description:
  // Collect the gathered timesteps into the output.
  void CollectTimesteps(vtkMultiBlockDataSet* output)
    {
    output->Initialize();
    MapType::iterator iter;
    unsigned int cc=0;
    for (iter = this->OutputGrids.begin();
      iter != this->OutputGrids.end(); ++iter)
      {
      if (iter->second.Output.GetPointer())
        {
        vtkValue& value = iter->second;

        // TODO; To add information about where which cell/pt this grid came
        // from.

        value.Output->GetPointData()->RemoveArray(
          value.ValidMaskArray->GetName());
        value.Output->GetPointData()->AddArray(value.ValidMaskArray);

        value.Output->GetPointData()->RemoveArray(
          this->TimeArray->GetName());
        value.Output->GetPointData()->AddArray(this->TimeArray);
 
        if (value.PointCoordinatesArray)
          {
          value.Output->GetPointData()->RemoveArray(
            value.PointCoordinatesArray->GetName());
          value.Output->GetPointData()->AddArray(value.PointCoordinatesArray);
          }

        this->RemoveInvalidPoints(value.ValidMaskArray, value.Output->GetPointData());
        output->SetBlock(cc, iter->second.Output.GetPointer());
        cc++;
        }
      }

    this->OutputGrids.clear();
    }
};

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::vtkInternal::AddTimeStep(
  double time, vtkDataObject* data)
{
  this->TimeArray->SetTuple1(this->CurrentTimeIndex, time);

  if (data && data->IsA("vtkDataSet"))
    {
    this->AddTimeStepInternal(0, time, static_cast<vtkDataSet*>(data));
    }
  else if (data && data->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* cd = reinterpret_cast<vtkCompositeDataSet*>(data);
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
        {
        this->AddTimeStepInternal(iter->GetCurrentFlatIndex(), time, ds);
        }
      }
    iter->Delete();
    }

  this->CurrentTimeIndex++;
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::vtkInternal::AddTimeStepInternalForLocations(
  unsigned int vtkNotUsed(composite_index), double vtkNotUsed(time), vtkDataSet* input)
{
  vtkDataSetAttributes* inDSA = input->GetPointData();
  vtkCharArray* validMask = vtkCharArray::SafeDownCast(
    inDSA->GetArray("vtkValidPointMask"));

  if (!validMask)
    {
    vtkGenericWarningMacro("Missing \"vtkValidPointMask\" in extracted dataset.");
    return;
    }

  vtkIdType numIDs = validMask->GetNumberOfTuples();
  if (numIDs <= 0)
    {
    return;
    }

  for (vtkIdType cc=0; cc < numIDs; cc++)
    {
    char valid = validMask->GetValue(cc);
    if (valid == 0)
      {
      continue;
      }

    // When probing locations, each timeline corresponds to each of the probe
    // locations. Hence, the key is just the index of the probe location and the
    // not the selected cell/point id.
    vtkKey key(0, cc);

    // This will allocate a new vtkRectilinearGrid is none is present
    vtkValue* value = this->GetOutput(key, inDSA);
    vtkRectilinearGrid* output = value->Output;
    output->GetPointData()->CopyData(inDSA, cc, this->CurrentTimeIndex);

    // Mark the entry valid.
    value->ValidMaskArray->SetValue(this->CurrentTimeIndex, 1);

    // Record the point coordinate if we are tracking a point.
    double *point = input->GetPoint(cc);
    value->PointCoordinatesArray->SetTuple(this->CurrentTimeIndex, point);
    }
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::vtkInternal::AddTimeStepInternal(
  unsigned int composite_index, double time, vtkDataSet* input)
{
  if (this->ContentType == vtkSelection::LOCATIONS)
    {
    this->AddTimeStepInternalForLocations(composite_index, time, input);
    return;
    }

  vtkDataSetAttributes* inDSA = 0;
  const char* idarrayname = 0;
  if (this->FieldType == vtkSelection::CELL)
    {
    inDSA = input->GetCellData();
    idarrayname = "vtkOriginalCellIds";
    }
  else
    {
    inDSA = input->GetPointData();
    idarrayname = "vtkOriginalPointIds";
    }


  vtkIdTypeArray* idsArray = 
    vtkIdTypeArray::SafeDownCast(inDSA->GetArray(idarrayname));
  if (!idsArray)
    {
    vtkGenericWarningMacro("Missing \"" << idarrayname << "\" in extracted dataset.");
    return;
    }

  vtkIdType numIDs = idsArray->GetNumberOfTuples();
  if (numIDs <= 0)
    {
    return;
    }

  for (vtkIdType cc=0; cc < numIDs; cc++)
    {
    vtkIdType curid = idsArray->GetValue(cc);
    vtkKey key(composite_index, curid);

    // This will allocate a new vtkRectilinearGrid is none is present
    vtkValue* value= this->GetOutput(key, inDSA);
    vtkRectilinearGrid* output = value->Output;
    output->GetPointData()->CopyData(inDSA, cc, this->CurrentTimeIndex);

    // Mark the entry valid.
    value->ValidMaskArray->SetValue(this->CurrentTimeIndex, 1);

    // Record the point coordinate if we are tracking a point.
    if (value->PointCoordinatesArray)
      {
      double *point = input->GetPoint(cc);
      value->PointCoordinatesArray->SetTuple(this->CurrentTimeIndex, point);
      }
    }
}

//----------------------------------------------------------------------------
vtkExtractArraysOverTime::vtkInternal::vtkValue*
vtkExtractArraysOverTime::vtkInternal::GetOutput(
  const vtkKey& key, vtkDataSetAttributes* inDSA)
{
  MapType::iterator iter = this->OutputGrids.find(key);

  if (iter == this->OutputGrids.end())
    {
    vtkValue value;
    vtkRectilinearGrid *output = vtkRectilinearGrid::New();
    value.Output.TakeReference(output);

    output->SetDimensions(this->NumberOfTimeSteps, 1, 1); 
    
    vtkPointData *opd = output->GetPointData();
    if (this->ContentType == vtkSelection::LOCATIONS)
      {
      opd->InterpolateAllocate(inDSA, this->NumberOfTimeSteps);
      }
    else
      {
      opd->CopyAllocate(inDSA, this->NumberOfTimeSteps);
      }

    // Add an array to hold the time at each step
    vtkDoubleArray *timeArray = this->TimeArray;
    if (inDSA->GetArray("Time"))
      {
      timeArray->SetName("TimeData");
      }
    else
      {
      timeArray->SetName("Time");
      }
    
    // Assign this array as the x-coords
    output->SetXCoordinates(timeArray);

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

    if (this->FieldType == vtkSelection::POINT || 
      this->ContentType == vtkSelection::LOCATIONS)
      {
      // These are the point coordinates of the original data
      vtkDoubleArray* coordsArray = vtkDoubleArray::New();
      coordsArray->SetNumberOfComponents(3);
      coordsArray->SetNumberOfTuples(this->NumberOfTimeSteps);
      if (inDSA->GetArray("Point Coordinates"))
        {
        coordsArray->SetName("Points");
        }
      else
        {
        coordsArray->SetName("Point Coordinates");
        }
      if (this->ContentType == vtkSelection::LOCATIONS)
        {
        coordsArray->SetName("Probe Coordinates");
        }
      value.PointCoordinatesArray.TakeReference(coordsArray);
      }

    // This array is used to make particular samples as invalid.
    // This happens when we are looking at a location which is not contained
    // by a cell or at a cell or point id that is destroyed.
    // It is used in the parallel subclass as well.
    vtkUnsignedCharArray* validPts = vtkUnsignedCharArray::New();
    validPts->SetName("vtkValidPointMask");
    validPts->SetNumberOfComponents(1);
    validPts->SetNumberOfTuples(this->NumberOfTimeSteps);
    validPts->FillComponent(0, 0);
    value.ValidMaskArray.TakeReference(validPts);

    iter = this->OutputGrids.insert(MapType::value_type(key, value)).first;
    }

  return &iter->second;
}

//****************************************************************************
vtkCxxRevisionMacro(vtkExtractArraysOverTime, "1.19");
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

  this->Internal = new vtkInternal;

  this->WaitingForFastPathData = false;
  this->IsExecuting = false;
  this->UseFastPath = false;
  this->SelectedId = -1;
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
    // We can handle composite datasets.
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");    
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
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

  // Check whether there is a fast-path option and if so, set our internal flag
  if ( inInfo->Has(
        vtkStreamingDemandDrivenPipeline::FAST_PATH_FOR_TEMPORAL_DATA()) )
    {
    this->UseFastPath = true;
    }
  else
    {
    this->UseFastPath = false;
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

  /*
   * This filter is no longer producing rectilinear grid, instead it is
   * producing a multiblock of rectilinear grids. That being the case, we do not
   * need any specific extent translation
   */
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::RequestUpdateExtent(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo1 = inputVector[0]->GetInformationObject(0);

  // get the requested update extent
  double *inTimes = inInfo1->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
    {
    double timeReq[1];
    timeReq[0] = inTimes[this->CurrentTimeIndex];
    inInfo1->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), 
                 timeReq, 
                 1);
    }

  if(this->UseFastPath && this->SelectedId>=0 && !this->WaitingForFastPathData)
    {
    // Create a key for the selected id
    inInfo1->Set(vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_ID(),
                  this->SelectedId);

    // Create a key for the data type
    if(this->FieldType == vtkSelection::CELL)
      {
      inInfo1->Set(vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_TYPE(),
                   "CELL");
      }
    else if(this->FieldType == vtkSelection::POINT)
      {
      inInfo1->Set(vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_TYPE(),
                   "POINT");
      }

    // Create a key for the type of id
    if(this->ContentType == vtkSelection::INDICES)
      {
      inInfo1->Set(vtkStreamingDemandDrivenPipeline::FAST_PATH_ID_TYPE(),
                   "INDEX");
      }
    else if(this->ContentType == vtkSelection::GLOBALIDS)
      {
      inInfo1->Set(vtkStreamingDemandDrivenPipeline::FAST_PATH_ID_TYPE(),
                   "GLOBAL");
      }

    this->WaitingForFastPathData = true;
    }

  /* Again, extent related stuff is no longer relevant since we are not
   * producing rectilinear grid as the output, instead it is multiblock.
   */ 

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (this->NumberOfTimeSteps == 0)
    {
    vtkErrorMacro("No time steps in input data!");
    return 0;
    }

  if (!inputVector[1]->GetInformationObject(0))
    {
    return 1;
    }


  // get the output data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // is this the first request
  if (!this->IsExecuting)
    {
    vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);
    vtkSelection* selection = vtkSelection::GetData(inInfo2);
    if (!this->DetermineSelectionType(selection))
      {
      return 0;
      }

    // FIXME: Need to fix the fast path for composite datasets.
    this->UseFastPath = false;

    // Only INDICES and GLOBALIDS based selection support fast path.
    if (this->ContentType != vtkSelection::INDICES &&
      this->ContentType != vtkSelection::GLOBALIDS)
      {
      this->UseFastPath = false;
      }

    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);

    this->Internal->Initialize(this->NumberOfTimeSteps, this->ContentType, this->FieldType);

    this->Error = vtkExtractArraysOverTime::NoError;

    this->IsExecuting = true;
    }

  if (this->UseFastPath)
    {
    // Have we already sent our fast-path information upstream and are waiting
    // for the actual data?
    if(this->WaitingForFastPathData)
      {
      // this->CopyFastPathDataToOutput(input, output);
      this->PostExecute(request, inputVector, outputVector);
      this->WaitingForFastPathData = false;
      return 1;
      }
    else
      {
      // Grab the selected id (either an index, or global id)
      // from the input selection. 
      this->SelectedId = this->GetSelectedId(inputVector, outInfo);
      if(this->SelectedId < 0)
        {
        vtkWarningMacro("Could not find index or global id. Fast path "
                        "option failed. Reverting to standard algorithm.")
        this->UseFastPath = false;
        }
      else
        {
        return 1;
        }
      }
    } 

  // If we get here, there is no fast-path option available.
  this->ExecuteAtTimeStep(inputVector, outInfo);

  // increment the time index
  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
    {
    this->PostExecute(request, inputVector, outputVector);
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
  this->IsExecuting = false;
  this->SelectedId = -1;

  switch (this->Error)
    {
    case vtkExtractArraysOverTime::MoreThan1Indices:
      vtkErrorMacro(<< "This filter can extract only 1 cell or "
                    << " point at a time. Only the first index"
                    << " was extracted");
      
    }

  //Use the vtkValidPointMask array to zero any invalid samples.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::GetData(outInfo);
  this->Internal->CollectTimesteps(output);
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::ExecuteAtTimeStep(
  vtkInformationVector** inputVector, 
  vtkInformation* outInfo)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkSelection* selInput = vtkSelection::GetData(selInfo);

  vtkDataObject* inputClone = input->NewInstance();
  inputClone->ShallowCopy(input);

  vtkSelection* selInputClone = selInput->NewInstance();
  selInputClone->ShallowCopy(selInput);
 
  vtkExtractSelection* filter = vtkExtractSelection::New();
  filter->SetPreserveTopology(0);
  filter->SetUseProbeForLocations(1);
  filter->SetInputConnection(0, inputClone->GetProducerPort());
  filter->SetInputConnection(1, selInputClone->GetProducerPort());

  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      filter->GetExecutive());

  vtkDebugMacro(<< "Preparing subfilter to extract from dataset");
  //pass all required information to the helper filter
  int piece = -1;
  int npieces = -1;
  int *uExtent;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    if (sddp)
      {
      sddp->SetUpdateExtent(0, piece, npieces, 0);
      }
    }

  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
    {
    uExtent = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    if (sddp)
      {
      sddp->SetUpdateExtent(0, uExtent);
      }
    }

  filter->Update();

  vtkDataObject* output = filter->GetOutputDataObject(0)->NewInstance();
  output->ShallowCopy(filter->GetOutputDataObject(0));

  double time_step = input->GetInformation()->Get(
    vtkDataObject::DATA_TIME_STEPS())[0];
  this->Internal->AddTimeStep(time_step, output);

  output->Delete();
  filter->Delete();
  inputClone->Delete();
  selInputClone->Delete();

  this->UpdateProgress(
    static_cast<double>(this->CurrentTimeIndex)/this->NumberOfTimeSteps);
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::CopyFastPathDataToOutput(vtkDataSet *input, 
                                                 vtkRectilinearGrid *output)
{
  vtkDataSetAttributes* inputAttributes = 0;
  vtkDataSetAttributes* outputAttributes = 0;
  vtkFieldData *ifd = input->GetFieldData();
  int numFieldArrays = ifd->GetNumberOfArrays();
  int numArrays = 0;

  // Get the right attribute and number of elements
  switch (this->FieldType)
    {
    case vtkSelection::CELL:
      inputAttributes = input->GetCellData();
      break;
    case vtkSelection::POINT:
      inputAttributes = input->GetPointData();
    }

  outputAttributes = output->GetPointData();
    
  if(!inputAttributes || !outputAttributes)
    {
    vtkErrorMacro("Unsupported field type.");
    return;
    }

  for (int j=0; j<numFieldArrays; j++)
    {  
    vtkDataArray* inFieldArray = ifd->GetArray(j);
    if (inFieldArray && 
        inFieldArray->GetName() && 
        !inFieldArray->IsA("vtkIdTypeArray"))
      {
      vtkStdString fieldName = inFieldArray->GetName();
      vtkStdString::size_type idx = fieldName.find("OverTime",0);
      if(idx != vtkStdString::npos)
        {
        vtkStdString actualName = fieldName.substr(0,idx);
        vtkDataArray *outArray = 
              outputAttributes->GetArray(actualName.c_str());
        outArray->SetNumberOfTuples(inFieldArray->GetNumberOfTuples());
        numArrays++;
        for(vtkIdType i=0; i<inFieldArray->GetNumberOfComponents(); i++)
          {
          outArray->CopyComponent(i,inFieldArray,i);
          }
        }
      }
    }

  // Copy the time steps to the output
  if (inputAttributes->GetArray("Time"))
    {
    for(int m=0; m<this->NumberOfTimeSteps; m++)
      {
      outputAttributes->GetArray("TimeData")->SetTuple1(m,m);
      }
    }
  else
    {
    for(int m=0; m<this->NumberOfTimeSteps; m++)
      {
      outputAttributes->GetArray("Time")->SetTuple1(m,m);
      }
    }
    
  vtkUnsignedCharArray* validPts = 
    vtkUnsignedCharArray::SafeDownCast(
      output->GetPointData()->GetArray("vtkValidPointMask"));

  // if no valid field arrays were found, which would happen if the reader
  // did not have the requested data, set validity to 0, otherwise 1.
  int validity = numArrays ? 1 : 0;
  validPts->FillComponent(0,validity);
}

//----------------------------------------------------------------------------
// Returns index or global id depending on the selection type.
vtkIdType vtkExtractArraysOverTime::GetSelectedId( vtkInformationVector** inputV, 
  vtkInformation* outInfo)
{
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
  // Get the right number of elements
  switch (this->FieldType)
    {
    case vtkSelection::CELL:
      numElems = input->GetNumberOfCells();
      break;
    case vtkSelection::POINT:
      numElems = input->GetNumberOfPoints();
    }

  vtkInformation* selProperties = selection->GetProperties();
  if (selProperties->Has(vtkSelection::PROCESS_ID()) &&
      piece != selProperties->Get(vtkSelection::PROCESS_ID()))
    {
    vtkDebugMacro("Selection from a different process");
    return -1;
    }

  if ((this->ContentType == vtkSelection::INDICES) ||        
      (this->ContentType == vtkSelection::GLOBALIDS))
    {
    vtkIdTypeArray* idArray = vtkIdTypeArray::SafeDownCast(
      selection->GetSelectionList());
    if (!idArray || idArray->GetNumberOfTuples() == 0)
      {
      vtkDebugMacro(<< "Empty selection");
      return -1;
      }

    if (idArray->GetNumberOfTuples() > 1)
      {
      this->Error = vtkExtractArraysOverTime::MoreThan1Indices;
      }

    // The selection is of size 1, so just grab the first (and only) element
    vtkIdType selectedId = idArray->GetValue(0);

    // If this is an index-based selection,
    // do some boundary checking.
    if (this->ContentType == vtkSelection::INDICES &&
        ( selectedId < 0 || selectedId >= numElems ))
      {
      return -1;
      }
  
    return selectedId;
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::DetermineSelectionType(vtkSelection* sel)
{
  int selContentType = sel->GetContentType();
  if (selContentType == vtkSelection::SELECTIONS)
    {
    int contentType = -1;
    int fieldType = -1;

    unsigned int numChildren = sel->GetNumberOfChildren();
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkSelection* child = sel->GetChild(cc);
      if (child)
        {
        int childFieldType = child->GetFieldType();
        int childContentType = child->GetContentType();

        if ((fieldType != -1 && fieldType != childFieldType) ||
          (contentType != -1 && contentType != childContentType))
          {
          vtkErrorMacro("All vtkSelection instances within a composite vtkSelection"
            " must have the same ContentType and FieldType.");
          return 0;
          }
        fieldType = childFieldType;
        contentType = childContentType;
        }
      }

    this->ContentType = contentType;
    this->FieldType = fieldType;
    }
  else
    {
    this->ContentType = selContentType;
    this->FieldType = sel->GetFieldType();
    }

  return 1;
}
