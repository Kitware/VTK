/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalDataSetInterpolationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalDataSetInterpolationFilter.h"

#include "vtkObjectFactory.h"
#include "vtkInstantiator.h"
#include "vtkPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkTupleInterpolator.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkTemporalDataSet.h"
#include "vtkSimpleInterpolator.h"
#include "vtkDataObjectCollection.h"

#include <functional>
#include <vtkstd/algorithm>

// It would be unlikely to use more than this number of timesteps for an interpolation
// we can save a great deal of time by allocating static arrays for calculations
#define MAX_INTERPOLATION_POINTS 16
#define MAX_TUPLE_COMPONENTS     16
#define MAX_WORKSPACE_SIZE       MAX_INTERPOLATION_POINTS*4

//----------------------------------------------------------------------------
// This is obsolete and I shall remove it and use a simple DataObjectCollection
// or even a temporal dataset. Leave it for now until I can tidy it
//----------------------------------------------------------------------------
class vtkDataSetCache : public vtkDataObjectCollection
{
  public:
    vtkTypeRevisionMacro(vtkDataSetCache, vtkDataObjectCollection);
    static vtkDataSetCache *New();
    void AddItem(vtkDataObject *data);
    void PutCacheItem(int i, vtkDataObject *data);
    void PutCacheItemCopy(int i, vtkDataObject *data);
    void ReplaceItem(int i, vtkDataObject *data);
    bool GetIsValid(int i);
    vtkDataSet *GetItemAsDataSet(int i);
  protected:
};
//---------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkDataSetCache, "1.1");
vtkStandardNewMacro(vtkDataSetCache);
//---------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkTemporalDataSetInterpolationFilter, "1.1");
vtkStandardNewMacro(vtkTemporalDataSetInterpolationFilter); 
//----------------------------------------------------------------------------
vtkTemporalDataSetInterpolationFilter::vtkTemporalDataSetInterpolationFilter()
{
  this->NumberOfOutputTimeSteps             = 0;
  this->TimeStep                            = 0;
  this->TimeValue                           = 0;
  this->ActualTimeStep                      = 0;
  this->TimeStepInterval                    = 0.25;
  this->NumberOfSplineInterpolationPoints   = 5;
  this->DataCache                           = vtkDataSetCache::New();
  this->InterpolationType                   = INTERPOLATION_TYPE_LINEAR;
  this->RequestedInputTimeStep              =-1;
  this->FirstLoopIndex                      = 0;
  this->LastLoopIndex                       = 0;
  this->SuppressDataUpdate                  = 0;
  this->SuppressedDataUpdate                = 0;
}
//----------------------------------------------------------------------------
vtkTemporalDataSetInterpolationFilter::~vtkTemporalDataSetInterpolationFilter()
{
  this->DataCache->Delete();
}
//----------------------------------------------------------------------------
int vtkTemporalDataSetInterpolationFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkTemporalDataSet");
  return 1;
  return 1;
}
//----------------------------------------------------------------------------
int vtkTemporalDataSetInterpolationFilter::ProcessRequest(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
      return this->RequestUpdateExtent(request, inputVector, outputVector);
    }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);      
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
//----------------------------------------------------------------------------
vtkExecutive* vtkTemporalDataSetInterpolationFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkTemporalDataSetInterpolationFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo  = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
  {
    this->NumberOfInputTimeSteps = inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    vtkDebugMacro(<<"vtkTemporalDataSetInterpolationFilter inputVector TIME_STEPS " << this->NumberOfInputTimeSteps);
    //
    // We know the input has got N time steps, how many output steps are we capable of
    // producing, and what are they.
    //
    // we should not recompute all this every time RequestInformation is called
    //
    if (this->NumberOfInputTimeSteps>1) {
      // Get list of input time step values
      this->InputTimeValues.resize(this->NumberOfInputTimeSteps);
      inInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->InputTimeValues[0] );
      double first = this->InputTimeValues[0];
      double last  = this->InputTimeValues[this->NumberOfInputTimeSteps-1];
      this->NumberOfOutputTimeSteps = 1 + static_cast<int>(0.5+((last-first)/this->TimeStepInterval));
      //
      this->TimeStepRange[0] = 0;
      this->TimeStepRange[1] = this->NumberOfOutputTimeSteps-1;

      // Generate list of new output time step values
      this->OutputTimeValues.clear();
      for (int i=0; i<this->NumberOfOutputTimeSteps; i++) {
        this->OutputTimeValues.push_back((double)(i)*this->TimeStepInterval + first);
      }
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
                    &this->OutputTimeValues[0], 
                    this->NumberOfOutputTimeSteps);
    }
    else { 
      this->NumberOfOutputTimeSteps = 0;
      this->TimeStepRange[0]  = 0;
      this->TimeStepRange[1]  = 0;
      vtkErrorMacro(<<"Not enough input time steps for interpolation");
      return 0;
    }
  }
  else if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()) )
  {
    // Since the input is providing no TIME_STEPS, but is providing a TIME_RANGE
    // we shall assume it is capable of producing continuous data over all T
    // so there's really no need to interpolate it at all. 
    //
    // For fun, we will, interpolate it anyway as we don't have many Temporal
    // Producers to play with yet.
    //
    // We will output 1 step and set the input steps to integral values
    // one before and after the requested values so that when we issue a request 
    // for data, we have two input timesteps available to us.
    //
    vtkDebugMacro(<<"vtkTemporalDataSetInterpolationFilter inputVector continuous TIME_RANE ");
    double Trange[2];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), Trange);
    this->NumberOfOutputTimeSteps = (Trange[1]-Trange[0]);
    this->NumberOfOutputTimeSteps = 1 + static_cast<int>(0.5+((Trange[1]-Trange[0])/this->TimeStepInterval));
    //
    this->TimeStepRange[0] = 0;
    this->TimeStepRange[1] = this->NumberOfOutputTimeSteps-1;
    double first = Trange[0];

    // Generate list of new output time step values
    this->OutputTimeValues.clear();
    for (int i=0; i<this->NumberOfOutputTimeSteps; i++) {
      this->OutputTimeValues.push_back((double)(i)*this->TimeStepInterval + first);
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
                  &this->OutputTimeValues[0], 
                  this->NumberOfOutputTimeSteps);

    // Set the input steps to -1 to tell our request update extent that the 
    // input is a continuous T producer
    this->NumberOfInputTimeSteps = -1;
  }
  else 
  {
    this->NumberOfOutputTimeSteps = 0;
    this->TimeStepRange[0]  = 0;
    this->TimeStepRange[1]  = 0;
    vtkErrorMacro(<<"Input information has no TIME_STEPS set");
    return 0;
  }
  if ( outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
      int NumberOfOutputTimeSteps = outInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
      vtkDebugMacro(<<"vtkTemporalDataSetInterpolationFilter outputVector TIME_STEPS " << NumberOfOutputTimeSteps);
   }

  return 1;
}
//----------------------------------------------------------------------------
int vtkTemporalDataSetInterpolationFilter::RequestUpdateExtent(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation  *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject   *output = vtkDataObject::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  //
  // The output has requesed a time value, what times must we ask from our input
  //
  this->ComputeInputTimeValues(request, inputVector, outputVector);

  //
  // set the required time value(s) on the input request 
  // For temporal inputs we can request multiple steps
  //
  if (vtkTemporalDataSet::SafeDownCast(this->GetInput(0))) {
    int Ni = this->LastLoopIndex - this->FirstLoopIndex + 1;
    double *timeReq = new double[Ni];
    // ask for the same one twice for now because the fractal generator changes structure between time steps
    for (int i=0; i<Ni; ++i) timeReq[i] = this->InputTimeValues[this->FirstLoopIndex /*+ i*/]; 
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), timeReq, Ni);
    delete []timeReq;
  } 
  //
  // For non-temporal inputs we must loop the pipeline
  //
  else {
    double timeReq[1];
    timeReq[0] = this->InputTimeValues[this->RequestedInputTimeStep];
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), timeReq, 1);
    //
    // Save the output time value in the output data information.
    // Snap the value to the nearest actual time value we said we could generate
    // (we created the list originally in RequestInformation)
    //
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())) {
      output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), &OutputTimeValues[this->ActualTimeStep], 1);
    }
  }
  return 1;
}  
//----------------------------------------------------------------------------
int vtkTemporalDataSetInterpolationFilter::ComputeInputTimeValues(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  this->SuppressDataUpdate = 0;

  // This is the actual time value we will be generating
  double requestedTimeValue;

  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())) {
    //
    // ideally we want the output information to be requesting a time step,
    // but since it isn't we must use the SetTimeStep value as a Time request
    //
    requestedTimeValue = this->OutputTimeValues[this->TimeStep];
    // this should be the same, just checking for debug purposes
    this->ActualTimeStep = vtkstd::find_if(OutputTimeValues.begin(), OutputTimeValues.end(), 
        vtkstd::bind2nd( vtkstd::greater_equal<double>( ), requestedTimeValue )) - OutputTimeValues.begin();
    vtkDebugMacro(<< "From SetTimeStep       : requestedTimeValue " << requestedTimeValue << " ActualTimeStep " << this->ActualTimeStep);
  } 
  else {
    //
    // Get the requested time step. 
    // Might be multiple steps requested in future
    //
    double *requestedTimeValues = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    requestedTimeValue = requestedTimeValues[0];
    // This might not be the same
    this->ActualTimeStep = vtkstd::find_if(OutputTimeValues.begin(), OutputTimeValues.end(), 
        vtkstd::bind2nd( vtkstd::greater_equal<double>( ), requestedTimeValue )) - OutputTimeValues.begin();
    vtkDebugMacro(<< "From UPDATE_TIME_STEPS : requestedTimeValue " << requestedTimeValue << " ActualTimeStep " << this->ActualTimeStep);
  }

  if (this->NumberOfInputTimeSteps == -1) {
    //
    // special case, the input is a continupus T producer, should not be interpolating, 
    // but we're doing it here. Use next value below and above for interpolation.
    //
    this->NumberOfInputTimeSteps = 2;
    this->InputTimeValues.resize(2);
    this->InputTimeValues[0] = static_cast<int>(requestedTimeValue);
    this->InputTimeValues[1] = this->InputTimeValues[0] + 1.0;
  }

  //
  // In order to generate the requested time value, what input time values do we need
  //
  if (this->InterpolationType==INTERPOLATION_TYPE_SPLINE) {
    // find the TimeSteps on the input that are N before and N after our requestedTimeValue
    int Nbefore    = this->NumberOfSplineInterpolationPoints/2;
    int Nafter     = this->NumberOfSplineInterpolationPoints - Nbefore;
    int FirstAbove = vtkstd::find_if(InputTimeValues.begin(), InputTimeValues.end(), 
      vtkstd::bind2nd( vtkstd::greater_equal<double>( ), requestedTimeValue )) - InputTimeValues.begin();
    //
    this->FirstLoopIndex = FirstAbove - Nbefore;
    this->LastLoopIndex  = FirstAbove + Nafter - 1;
    //
    if (this->FirstLoopIndex<0) this->FirstLoopIndex = 0;
    if (this->LastLoopIndex>(this->NumberOfInputTimeSteps-1)) this->LastLoopIndex = this->NumberOfInputTimeSteps-1;
  }
  else { // INTERPOLATION_TYPE_LINEAR
    int FirstAbove = vtkstd::find_if(InputTimeValues.begin(), InputTimeValues.end(), 
      vtkstd::bind2nd( vtkstd::greater<double>( ), requestedTimeValue )) - InputTimeValues.begin();
    if (FirstAbove<this->NumberOfInputTimeSteps) {
      this->LastLoopIndex  = FirstAbove;
      this->FirstLoopIndex = this->LastLoopIndex - 1;
      if (this->FirstLoopIndex<0) {
        this->FirstLoopIndex = 0;
        this->LastLoopIndex  = 1;
      }
    } 
    else {
      this->LastLoopIndex  = this->NumberOfInputTimeSteps-1;
      this->FirstLoopIndex = this->LastLoopIndex - 1;
    }
  }

  vtkDebugMacro(<< "Computed Timestep indices : " << this->FirstLoopIndex << " " << this->LastLoopIndex);

  //
  // Clear any Cached Datasets that we might have that we are not using
  //
  for (int i=0; i<this->NumberOfInputTimeSteps; ++i) {
    if ((i<this->FirstLoopIndex || i>this->LastLoopIndex) && this->DataCache->GetIsValid(i)) {
      this->DataCache->PutCacheItem(i, NULL);
    }
  }

  //
  // Check if the timesteps we need are available in the cache, 
  // set the actual required input index to the lowest missing one
  //
  this->RequestedInputTimeStep = -1;
  for (int i=this->FirstLoopIndex; i<=this->LastLoopIndex; ++i) {
    if (!this->DataCache->GetIsValid(i)) {
      this->RequestedInputTimeStep = i;
      vtkDebugMacro(<< "Interpolation needs : " << i );
      break;
    }
    else {
      vtkDebugMacro(<< "Interpolation has   : " << i );
    }
  }
  if (this->RequestedInputTimeStep == -1) {
    vtkDebugMacro(<< "All Cached for Interpolation Algorithm ");
    this->RequestedInputTimeStep = this->LastLoopIndex;
    this->SuppressDataUpdate = 1;
    this->SuppressedDataUpdate = 0;
  };
  return 1;
}
//----------------------------------------------------------------------------
int vtkTemporalDataSetInterpolationFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation    *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject      *output = vtkDataObject::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataObject       *input = vtkDataObject::SafeDownCast(this->GetInput(0));

//  int updatePiece     = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
//  int updateNumPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  int Ni = this->LastLoopIndex - this->FirstLoopIndex + 1;
  //
  if (vtkTemporalDataSet::SafeDownCast(input)) {
    //
    // if the input is a temporal dataset, then it should have all the required 
    // timesteps inside it. Let's use them if we can.
    //
    vtkTemporalDataSet *tinput = vtkTemporalDataSet::SafeDownCast(input);
    if (tinput->GetNumberOfGroups()==Ni) {
      vtkDebugMacro(<<"Temporal input provided required datasets");
      // now we can interpolate them 
      vtkstd::vector<vtkDataObject*> indata;
      for (int i=0; i<Ni; ++i) {
        indata.push_back(tinput->GetDataSet(i,0));
      }
      vtkDataObject *result = this->InterpolateDataObject(&indata[0], Ni);
      if (result) {
        vtkTemporalDataSet *Toutput = vtkTemporalDataSet::SafeDownCast(output);
        Toutput->SetDataSet(0, 0, result); // timestep(0), datasetnum(0), dataset
        return 1;
      }
      else {
        vtkErrorMacro(<<"Unexpected error during interpolation");
        return 0;
      }
    }
    else {
      vtkErrorMacro(<<"Temporal input had stuff, but not exactly what we asked for");
      return 0;
    }
  }
  else {
    //
    // The input is simple, we must loop the pipeline and cache datasets
    //
    if (this->RequestedInputTimeStep==this->FirstLoopIndex) {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

    if (this->RequestedInputTimeStep==this->LastLoopIndex)
    {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    }

    //
    // If we are looping, copy input to cache
    //
    if (!this->DataCache->GetIsValid(this->RequestedInputTimeStep)) {
      this->DataCache->PutCacheItemCopy(this->RequestedInputTimeStep, input);
      vtkDebugMacro(<< "Cached index " << this->RequestedInputTimeStep );
    }

    if (this->RequestedInputTimeStep==this->LastLoopIndex) {
      //
      // Do the interpolation
      //
      vtkstd::vector<vtkDataObject*> indatasets;
      for (int i=0; i<Ni; ++i) {
        indatasets.push_back(this->DataCache->GetItem(i+this->FirstLoopIndex));
      }
      vtkDataObject *result = this->InterpolateDataObject(&indatasets[0], Ni);
      if (result) {
        vtkTemporalDataSet *Toutput = vtkTemporalDataSet::SafeDownCast(output);
        Toutput->SetDataSet(0, 0, result); // timestep(0), datasetnum(0), dataset
        return 1;
      }
      else {
        vtkErrorMacro(<<"Unexpected error during interpolation");
        return 0;
      }
    }
  }
  return 1;
}
//----------------------------------------------------------------------------
vtkDataObject *vtkTemporalDataSetInterpolationFilter::InterpolateDataObject(vtkDataObject **input, int Ni)
{
  if (vtkDataSet::SafeDownCast(input[0])) {
    //
    // if we have reached the Leaf/DataSet level, we can interpolate directly
    //
    vtkDataSet **indata = reinterpret_cast<vtkDataSet**>(input);
    return this->InterpolateDataSet(indata, Ni);
  }
  else if (vtkMultiGroupDataSet::SafeDownCast(input[0])) {
    vtkMultiGroupDataSet **mgds = reinterpret_cast<vtkMultiGroupDataSet**>(input);
    //
    // We need to loop over blocks etc and build up a new dataset
    //
    vtkMultiGroupDataSet *output = mgds[0]->NewInstance();
    int numGroups = mgds[0]->GetNumberOfGroups();
    output->SetNumberOfGroups(numGroups);
    //
    for (int g=0; g<numGroups; ++g) {
      int numDataSets = mgds[0]->GetNumberOfDataSets(g);
      for (int d=0; d<numDataSets; ++d) {
        vtkstd::vector<vtkDataObject*> indata;
        bool abort = false;
        for (int i=0; i<Ni; ++i) {
          // These multigroup dataset can have null data, it's bad, but
          // we'll just skip the rest of that bundle
          vtkDataObject *dataobj = mgds[i]->GetDataSet(g,d);
          if (!dataobj) abort = true;
          indata.push_back(dataobj);
        }
        if (abort) {
          vtkWarningMacro("The MultiGroup datasets were not identical in structure : Group " 
            << g << " Dataset " << d << " was skipped");
          continue;
        }
        vtkDataObject *result = this->InterpolateDataObject(&indata[0], Ni);
        if (result) {
          output->SetDataSet(g, d, result); 
        }
        else {
          vtkErrorMacro(<<"Unexpected error during interpolation");
          // nned to clear up memory we may have allocated and lost :(
          return NULL;
        }
      }
    }
    return output;
  }
  else {
    vtkErrorMacro(<<"We cannot yet interpolate this type of dataset");
    return NULL;
  }
}
//----------------------------------------------------------------------------
vtkDataSet *vtkTemporalDataSetInterpolationFilter::InterpolateDataSet(vtkDataSet **input, int Ni)
{
  double T[MAX_INTERPOLATION_POINTS];
  //
  for (int i=0; i<Ni; ++i) T[i] = this->InputTimeValues[i+this->FirstLoopIndex];
  //
  vtkDataSet *output = input[0]->NewInstance();
  output->CopyStructure(input[0]);
  //
  // Interpolate points if the dataset is a vtkPointSet
  //
  if (vtkPointSet::SafeDownCast(input[0])) {
    vtkstd::vector<vtkDataArray*> arrays;
    for (int i=0; i<Ni; ++i) {
      arrays.push_back(vtkPointSet::SafeDownCast(input[i])->GetPoints()->GetData());
    }
    // allocate double for output if input is double - otherwise float
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(&arrays[0], Ni)) {
      vtkWarningMacro(<<"Interpolation aborted for points " 
        << " because the number of tuples/components in each time step are different");
    }
    vtkDataArray *outarray = InterpolateDataArray(T, &arrays[0], Ni, arrays[0]->GetNumberOfTuples());
    vtkPoints *outpoints = vtkPointSet::SafeDownCast(output)->GetPoints();
    if (vtkDoubleArray::SafeDownCast(outarray)) outpoints->SetDataTypeToDouble();
    else outpoints->SetDataTypeToFloat();
    outpoints->SetNumberOfPoints(arrays[0]->GetNumberOfTuples());
    outpoints->SetData(outarray);
    outarray->Delete();
  }
  //
  // Interpolate pointdata if present
  //
  for (int s=0; s<input[0]->GetPointData()->GetNumberOfArrays(); ++s) {
    vtkstd::vector<vtkDataArray*> arrays;
    char *scalarname = NULL;
    for (int i=0; i<Ni; ++i) {
      //
      // On some data, the scalar arrays are consistent but ordered differently 
      // on each time step, so we will fetch them by name if possible.
      //
      if (i==0 || (scalarname==NULL)) {
        vtkDataArray *dataarray = input[i]->GetPointData()->GetArray(s);
        scalarname = dataarray->GetName();
        arrays.push_back(dataarray);
      }
      else {
        vtkDataArray *dataarray = input[i]->GetPointData()->GetArray(scalarname);
        arrays.push_back(dataarray);
      }
    }
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(&arrays[0], Ni)) {
      vtkWarningMacro(<<"Interpolation aborted for array " 
        << (scalarname ? scalarname : "(unnamed array)") 
        << " because the number of tuples/components in each time step are different");
    }
    // allocate double for output if input is double - otherwise float
    vtkDataArray *outarray = InterpolateDataArray(T, &arrays[0], Ni, arrays[0]->GetNumberOfTuples());
    output->GetPointData()->AddArray(outarray);
    outarray->Delete();
  }
  //
  // Interpolate celldata if present
  //
  for (int s=0; s<input[0]->GetCellData()->GetNumberOfArrays(); ++s) {
    vtkstd::vector<vtkDataArray*> arrays;
    char *scalarname = NULL;
    for (int i=0; i<Ni; ++i) {
      //
      // On some data, the scalar arrays are consistent but ordered differently 
      // on each time step, so we will fetch them by name if possible.
      //
      if (i==0 || (scalarname==NULL)) {
        vtkDataArray *dataarray = input[i]->GetCellData()->GetArray(s);
        scalarname = dataarray->GetName();
        arrays.push_back(dataarray);
      }
      else {
        vtkDataArray *dataarray = input[i]->GetCellData()->GetArray(scalarname);
        arrays.push_back(dataarray);
      }
    }
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(&arrays[0], Ni)) {
      vtkWarningMacro(<<"Interpolation aborted for array " 
        << (scalarname ? scalarname : "(unnamed array)") 
        << " because the number of tuples/components in each time step are different");
    }
    // allocate double for output if input is double - otherwise float
    vtkDataArray *outarray = InterpolateDataArray(T, &arrays[0], Ni, arrays[0]->GetNumberOfTuples());
    output->GetCellData()->AddArray(outarray);
    outarray->Delete();
  }
  return output;
}
//----------------------------------------------------------------------------
vtkDataArray *vtkTemporalDataSetInterpolationFilter::InterpolateDataArray(double *T, vtkDataArray **arrays, vtkIdType Ni, vtkIdType N)
{
  double work[MAX_INTERPOLATION_POINTS];
  double coeffs[MAX_WORKSPACE_SIZE];
  double data[MAX_TUPLE_COMPONENTS][MAX_INTERPOLATION_POINTS]; 
  vtkstd::vector<vtkSimpleInterpolator*> ScalarInterpolators;
  //
  // Create the output
  //
  vtkDataArray *output;
  if (vtkDoubleArray::SafeDownCast(arrays[0])) {
    output = vtkDoubleArray::New();
  }
  else {
    output = vtkFloatArray::New();
  }
  //
  // One interpolator per component of the tuple
  //
  int Nc = arrays[0]->GetNumberOfComponents();
  for (int c=0; c<Nc; ++c) {
    ScalarInterpolators.push_back(vtkSimpleInterpolator::New());
  }
  //
  // initialize the output
  //
  output->SetNumberOfComponents(Nc);
  output->SetNumberOfTuples(N);
  output->SetName(arrays[0]->GetName());
  //
  // Loop over the tuples and components doing the interpolation
  //
  double timeout = this->OutputTimeValues[this->ActualTimeStep];
  double tuple[MAX_TUPLE_COMPONENTS];
  for (int p=0; p<N; ++p) {
    for (int i=0; i<Ni; ++i) {
      double *tuple = arrays[i]->GetTuple(p);
      for (int c=0; c<Nc; ++c) {
        data[c][i] = tuple[c];
      }
    }
    if (this->InterpolationType==INTERPOLATION_TYPE_SPLINE) {
      for (int c=0; c<Nc; ++c) {
        ScalarInterpolators[c]->SetArrays(Ni, T, data[c], work, coeffs);
        tuple[c] = ScalarInterpolators[c]->EvaluateSpline(timeout);
      }
    } 
    else {
      for (int c=0; c<Nc; ++c) {
        ScalarInterpolators[c]->SetArrays(Ni, T, data[c], work, coeffs);
        tuple[c] = ScalarInterpolators[c]->EvaluateLinear(timeout);
      }
    }
    output->SetTuple(p, tuple); 
  }
  //
  // Cleanup our interpolators
  //
  for (unsigned int i=0; i<ScalarInterpolators.size(); ++i) {
    ScalarInterpolators[i]->Delete();
  }
  //
  return output;
}
//----------------------------------------------------------------------------
bool vtkTemporalDataSetInterpolationFilter::VerifyArrays(vtkDataArray **arrays, int N)
{
  vtkIdType Nt = arrays[0]->GetNumberOfTuples();
  vtkIdType Nc = arrays[0]->GetNumberOfComponents();
  for (int i=1; i<N; ++i) {
    if (arrays[i]->GetNumberOfTuples()!=Nt) return false;
    if (arrays[i]->GetNumberOfComponents()!=Nc) return false;
  }
  return true;
}
//----------------------------------------------------------------------------
// The Algorithm receives this ModifyRequest from the executive before sending
// REQUEST_DATA. 
// If all the timesteps we want are already cached - then stop the REQUEST_DATA 
// from being sent Upstream - otherwise the filter upstream will update a second
// time and we don't need it to. Once we have interpolated, put the request back
// otherwise the pipeline will fall over next time around (the request is a
// static vtkInformation object essentially)
//
// Note : I don't like doing this, but it works.
//----------------------------------------------------------------------------
int vtkTemporalDataSetInterpolationFilter::ModifyRequest(vtkInformation* request, int when)
{
  if (!this->SuppressDataUpdate) return 1;
  //
  if (when==vtkExecutive::BeforeForward &&
    request->Has(vtkDemandDrivenPipeline::REQUEST_DATA())) {
    request->Remove(vtkDemandDrivenPipeline::REQUEST_DATA());
    this->SuppressedDataUpdate = 1;
  }
  else if (this->SuppressedDataUpdate && when==vtkExecutive::AfterForward) {
    request->Set(vtkDemandDrivenPipeline::REQUEST_DATA());
    this->SuppressedDataUpdate = 0;
  }
  return 1;
}


//----------------------------------------------------------------------------
// This is obsolete and I shall remove it and use a simple DataObjectCollection
// or even a temporal dataset. Leave it for now until I can tidy it
//----------------------------------------------------------------------------
bool vtkDataSetCache::GetIsValid(int i) 
{ 
  return (i<this->GetNumberOfItems()) ? (this->GetItem(i)!=NULL) : false;
}
//---------------------------------------------------------------------------
void vtkDataSetCache::PutCacheItem(int i, vtkDataObject *data)
{
  int N = this->GetNumberOfItems();
  if (i>=N) {
    for (int d=N; d<=i; ++d) {
      if (d==i) {
        this->AddItem(data);
      }
      else {
        this->AddItem((vtkDataObject*)NULL);
      }
    }
  }
  else {
    this->ReplaceItem(i, data);
  }
}
//---------------------------------------------------------------------------
// Replace the i'th item in the collection with a
void vtkDataSetCache::AddItem(vtkDataObject *data)
{
  vtkCollectionElement *elem;
  elem = new vtkCollectionElement;
  if (!this->Top) {
    this->Top = elem;
  }
  else {
    this->Bottom->Next = elem;
  }
  this->Bottom = elem;

  if (data) data->Register(this);
  elem->Item = data;
  elem->Next = NULL;

  this->Modified();
  this->NumberOfItems++;
}
//---------------------------------------------------------------------------
// Replace the i'th item in the collection with a
void vtkDataSetCache::ReplaceItem(int i, vtkDataObject *data)
{
  if( i < 0 || i >= this->NumberOfItems ) {
    return;
  }
  vtkCollectionElement *elem = this->Top;
  if (i == this->NumberOfItems - 1) {
    elem = this->Bottom;
  }
  else {
    for (int j = 0; j < i; j++, elem = elem->Next ) {}
  }

  // Take care of reference counting
  if (elem->Item != NULL) elem->Item->UnRegister(this);
  if (data!=NULL) data->Register(this);

  // j == i
  elem->Item = data;

  this->Modified();
}
//---------------------------------------------------------------------------
void vtkDataSetCache::PutCacheItemCopy(int i, vtkDataObject *data)
{
  if (data==NULL) {
    vtkErrorMacro(<<"Can't put NULL in Deep Cache");
    return;
  }
  vtkObject *obj2 = vtkInstantiator::CreateInstance(data->GetClassName());
  vtkDataObject *datacopy = vtkDataObject::SafeDownCast(obj2);
  datacopy->CopyInformation(data);
  datacopy->ShallowCopy(data);
  datacopy->SetSource(NULL);
  this->PutCacheItem(i, datacopy);
  datacopy->Delete();
}
//---------------------------------------------------------------------------
vtkDataSet *vtkDataSetCache::GetItemAsDataSet(int i)
{
  return static_cast<vtkDataSet*>(this->GetItemAsObject(i));
}
//---------------------------------------------------------------------------
