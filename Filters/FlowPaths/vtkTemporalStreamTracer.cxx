/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkTemporalStreamTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalStreamTracer.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSmartPointer.h"
#include "vtkTemporalInterpolatedVelocityField.h"
#include "vtkOutputWindow.h"
#include "vtkAbstractParticleWriter.h"
#include "vtkToolkits.h" // For VTK_USE_MPI
#include "assert.h"

#ifdef WIN32
  #undef JB_H5PART_PARTICLE_OUTPUT
#else
//  #define JB_H5PART_PARTICLE_OUTPUT
#endif

#ifdef JB_H5PART_PARTICLE_OUTPUT
// #include "vtkH5PartWriter.h"
  #include "vtkXMLParticleWriter.h"
#endif

#include <functional>
#include <algorithm>

using namespace vtkTemporalStreamTracerNamespace;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//#define JB_DEBUG__
#if defined JB_DEBUG__
  #ifdef WIN32
      #define OUTPUTTEXT(a) vtkOutputWindowDisplayText(a);
  #else
  #endif

    #undef vtkDebugMacro
    #define vtkDebugMacro(a)  \
    { \
      vtkOStreamWrapper::EndlType endl; \
      vtkOStreamWrapper::UseEndl(endl); \
      vtkOStrStreamWrapper vtkmsg; \
      vtkmsg << "P(" << this->UpdatePiece << "): " a << "\n"; \
      OUTPUTTEXT(vtkmsg.str()); \
      vtkmsg.rdbuf()->freeze(0); \
    }

  #undef vtkErrorMacro
  #define vtkErrorMacro(a) vtkDebugMacro(a)

#endif
//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalStreamTracer);
vtkCxxSetObjectMacro(vtkTemporalStreamTracer, ParticleWriter, vtkAbstractParticleWriter);
//---------------------------------------------------------------------------
vtkTemporalStreamTracer::vtkTemporalStreamTracer()
{
  this->IntegrationDirection        = FORWARD;
  this->TimeStep                    = 0;
  this->ActualTimeStep              = 0;
  this->NumberOfInputTimeSteps      = 0;
  this->ForceReinjectionEveryNSteps = 1;
  this->ReinjectionFlag             = 0;
  this->ReinjectionCounter          = 0;
  this->UpdatePiece                 = 0;
  this->UpdateNumPieces             = 0;
  this->AllFixedGeometry            = 1;
  this->StaticMesh                  = 1;
  this->StaticSeeds                 = 1;
  this->ComputeVorticity            = 1;
  this->IgnorePipelineTime          = 0;
  this->ParticleWriter              = NULL;
  this->ParticleFileName            = NULL;
  this->EnableParticleWriting       = false;
  this->UniqueIdCounter             = 0;
  this->UniqueIdCounterMPI          = 0;
  this->InterpolationCount          = 0;
  //
  this->NumberOfParticles     = 0;
  this->TimeStepResolution    = 1.0;
  this->TerminationTime       = 0.0;
  this->TerminationTimeUnit   = TERMINATION_STEP_UNIT;
  this->EarliestTime          =-1E6;
  // we are not actually using these for now

  this->MaximumPropagation = 1.0;

  this->IntegrationStepUnit    = LENGTH_UNIT;
  this->MinimumIntegrationStep = 1.0E-2;
  this->MaximumIntegrationStep = 1.0;
  this->InitialIntegrationStep = 0.5;
  //
  this->Interpolator = vtkSmartPointer<vtkTemporalInterpolatedVelocityField>::New();
  //
  this->SetNumberOfInputPorts(2);

#ifdef JB_H5PART_PARTICLE_OUTPUT
#ifdef WIN32
  vtkDebugMacro(<<"Setting vtkH5PartWriter");
  vtkH5PartWriter *writer = vtkH5PartWriter::New();
#else
  vtkDebugMacro(<<"Setting vtkXMLParticleWriter");
  vtkXMLParticleWriter *writer = vtkXMLParticleWriter::New();
#endif
  this->SetParticleWriter(writer);
  writer->Delete();
#endif

  this->SetIntegratorType(RUNGE_KUTTA4);
  this->RequestIndex = 0;
}
//---------------------------------------------------------------------------
vtkTemporalStreamTracer::~vtkTemporalStreamTracer()
{
  this->SetParticleWriter(NULL);
  if (this->ParticleFileName)
  {
    delete []this->ParticleFileName;
    this->ParticleFileName = NULL;
  }
}
//----------------------------------------------------------------------------
int vtkTemporalStreamTracer::FillInputPortInformation(
  int port,
  vtkInformation* info)
{
  // port 0 must be a temporal collection of any type
  // the executive should put a temporal collection in when
  // we request multiple time steps.
  if (port==0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
//    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else if (port==1) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
//    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}
//----------------------------------------------------------------------------
void vtkTemporalStreamTracer::AddSourceConnection(vtkAlgorithmOutput* input)
{
  this->AddInputConnection(1, input);
}
//----------------------------------------------------------------------------
void vtkTemporalStreamTracer::RemoveAllSources()
{
  this->SetInputConnection(1, 0);
}
//----------------------------------------------------------------------------
int vtkTemporalStreamTracer::ProcessRequest(
  vtkInformation* request,
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
int vtkTemporalStreamTracer::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo  = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfInputTimeSteps = inInfo->Length(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    vtkDebugMacro(<<"vtkTemporalStreamTracer "
      "inputVector TIME_STEPS " << this->NumberOfInputTimeSteps);
    //
    // Get list of input time step values
    this->InputTimeValues.resize(this->NumberOfInputTimeSteps);
    inInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
      &this->InputTimeValues[0] );
    if (this->NumberOfInputTimeSteps==1)
      {
      vtkErrorMacro(<<"Not enough input time steps for particle integration");
      return 0;
      }
    //
    // We only output T-1 time steps
    //
    this->OutputTimeValues.resize(this->NumberOfInputTimeSteps-1);
    this->OutputTimeValues.clear();
    this->OutputTimeValues.insert(
      this->OutputTimeValues.begin(),
      this->InputTimeValues.begin()+1, this->InputTimeValues.end());
  }
  else
    {
    vtkErrorMacro(<<"Input information has no TIME_STEPS set");
    return 0;
    }

  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
               &this->OutputTimeValues[0],
               static_cast<int>(this->OutputTimeValues.size()));

return 1;
}
//----------------------------------------------------------------------------
class WithinTolerance: public std::binary_function<double, double, bool>
{
public:
    result_type operator()(first_argument_type a, second_argument_type b) const
    {
      bool result = (fabs(a-b)<=(a*1E-6));
      return (result_type)result;
    }
};
//----------------------------------------------------------------------------
int vtkTemporalStreamTracer::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  //
  // The output has requested a time value, what times must we ask from our input
  //
  if (this->IgnorePipelineTime || !outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
    double requestedTimeValue;
    //
    // ideally we want the output information to be requesting a time step,
    // but since it isn't we must use the SetTimeStep value as a Time request
    //
    if (this->TimeStep<this->OutputTimeValues.size()) {
      requestedTimeValue = this->OutputTimeValues[this->TimeStep];
    } else {
      requestedTimeValue = this->OutputTimeValues.back();
    }
    this->ActualTimeStep = this->TimeStep;

    vtkDebugMacro(<< "SetTimeStep       : requestedTimeValue "
      << requestedTimeValue << " ActualTimeStep " << this->ActualTimeStep);
    (void) requestedTimeValue; //silence unused variable warning
    }
  else
    {
//
    // Get the requested time step.
    //
    double requestedTimeValue = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    this->ActualTimeStep = std::find_if(
      this->OutputTimeValues.begin(),
      this->OutputTimeValues.end(),
      std::bind2nd( WithinTolerance( ), requestedTimeValue ))
      - this->OutputTimeValues.begin();
    if (this->ActualTimeStep>=this->OutputTimeValues.size())
    {
      this->ActualTimeStep = 0;
    }
    vtkDebugMacro(<< "UPDATE_TIME_STEPS : requestedTimeValue "
      << requestedTimeValue << " ActualTimeStep " << this->ActualTimeStep);
    }

  if (this->ActualTimeStep<this->OutputTimeValues.size())
    {
    for (int i=0; i<numInputs; i++)
      {
      vtkInformation *inInfo = inputVector[0]->GetInformationObject(i);
      // our output timestep T is timestep T+1 in the source
      // so output inputTimeSteps[T], inputTimeSteps[T+1]
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
                  this->InputTimeValues[this->ActualTimeStep + this->RequestIndex]);
      vtkDebugMacro(<< "requested 1 time values : "
                    << this->InputTimeValues[this->ActualTimeStep + this->RequestIndex]);
      }
    }
  else
  {
    vtkDebugMacro(<< "UPDATE_TIME_STEPS : Error getting requested time step");
    return 0;
  }

  return 1;
}
//---------------------------------------------------------------------------
int vtkTemporalStreamTracer::InitializeInterpolator()
{
  if (!this->InputDataT[0] || !this->InputDataT[1]) {
    return 0;
  }
  //
  // When Multiblock arrays are processed, some may be empty
  // if the first is empty, we won't find the correct vector name
  // so scan until we get one
  //
  vtkSmartPointer<vtkCompositeDataIterator> iterP;
  iterP.TakeReference(this->InputDataT[0]->NewIterator());
  iterP->GoToFirstItem();
  char *vecname = NULL;
  while (!iterP->IsDoneWithTraversal())
    {
    vtkDataArray *vectors = this->GetInputArrayToProcess(
      0,iterP->GetCurrentDataObject());
    if (vectors)
      {
      vecname = vectors->GetName();
      break;
      }
    iterP->GoToNextItem();
    }
  if (!vecname)
    {
    vtkDebugMacro(<< "Couldn't find vector array " << vecname);
    return VTK_ERROR;
    }

  vtkDebugMacro(<< "Interpolator using array " << vecname);
  this->Interpolator->SelectVectors(vecname);

  this->AllFixedGeometry = 1;

  int numValidInputBlocks[2] = {0, 0};
  int numTotalInputBlocks[2] = {0, 0};
  this->DataReferenceT[0] = this->DataReferenceT[1] = 0;
  for (int T=0; T<2; T++) {
    this->CachedBounds[T].clear();
    int index = 0;
    // iterate over all blocks of input and cache the bounds information
    // and determine fixed/dynamic mesh status.

    vtkSmartPointer<vtkCompositeDataIterator> anotherIterP;
    anotherIterP.TakeReference(this->InputDataT[T]->NewIterator());
    anotherIterP->GoToFirstItem();
    while (!anotherIterP->IsDoneWithTraversal())
      {
      numTotalInputBlocks[T]++;
      vtkDataSet* inp = vtkDataSet::SafeDownCast(anotherIterP->GetCurrentDataObject());
      if (inp)
        {
        if (inp->GetNumberOfCells()==0)
          {
          vtkDebugMacro("Skipping an empty dataset");
          }
        else if (!inp->GetPointData()->GetVectors(vecname) && inp->GetNumberOfPoints()>0)
          {
          vtkDebugMacro("One of the input datasets has no velocity vector.");
          }
        else
          {
          // vtkDebugMacro("pass " << i << " Found dataset with " << inp->GetNumberOfCells() << " cells");
          //
          // store the bounding boxes of each local dataset for faster 'point-in-dataset' testing
          //
          bounds bbox;
          inp->ComputeBounds();
          inp->GetBounds(&bbox.b[0]);
          this->CachedBounds[T].push_back(bbox);
          bool static_dataset = this->StaticMesh || inp->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED());
          this->AllFixedGeometry = this->AllFixedGeometry && static_dataset;
          // add the dataset to the interpolator
          this->Interpolator->SetDataSetAtTime(index++, T, this->CurrentTimeSteps[T], inp, static_dataset);
          if (!this->DataReferenceT[T]) {
            this->DataReferenceT[T] = inp;
          }
          //
          numValidInputBlocks[T]++;
          }
        }
      anotherIterP->GoToNextItem();
      }
  }
  if (numValidInputBlocks[0]==0 || numValidInputBlocks[1]==0)
    {
    vtkDebugMacro("Not enough inputs have been found. Can not execute." << numValidInputBlocks[0] << " " << numValidInputBlocks[1]);
    return VTK_ERROR;
    }
  if (numValidInputBlocks[0] != numValidInputBlocks[1])
    {
    vtkDebugMacro("The number of datasets is different between time steps " << numValidInputBlocks[0] << " " << numValidInputBlocks[1]);
    return VTK_ERROR;
    }
  //
  vtkDebugMacro("Number of Valid input blocks is " << numValidInputBlocks[0] << " from " << numTotalInputBlocks[0]);
  vtkDebugMacro("AllFixedGeometry " << this->AllFixedGeometry);

  // force optimizations if StaticMesh is set.
  if (this->StaticMesh) {
    vtkDebugMacro("Static Mesh optimizations Forced ON");
    this->AllFixedGeometry = 1;
  }

  //
  return VTK_OK;
}
int vtkTemporalStreamTracer::SetTemporalInput(vtkDataObject *data, int i)
{
  // if not set, create a multiblock dataset to hold all input blocks
  if (!this->InputDataT[i])
    {
    this->InputDataT[i] = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    }
  // if simple dataset, add to our list, otherwise, add blocks
  vtkDataSet           *dsInput = vtkDataSet::SafeDownCast(data);
  vtkMultiBlockDataSet *mbInput = vtkMultiBlockDataSet::SafeDownCast(data);

  if (dsInput)
    {
    vtkSmartPointer<vtkDataSet> copy;
    copy.TakeReference(dsInput->NewInstance());
    copy->ShallowCopy(dsInput);
    this->InputDataT[i]->SetBlock(this->InputDataT[i]->GetNumberOfBlocks(), copy);
    }
  else if (mbInput)
    {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(mbInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataSet *ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
        {
        vtkSmartPointer<vtkDataSet> copy;
        copy.TakeReference(ds->NewInstance());
        copy->ShallowCopy(ds);
        if (ds->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()))
          {
          copy->GetInformation()->Set(vtkDataObject::DATA_GEOMETRY_UNMODIFIED(),1);
          }
        this->InputDataT[i]->SetBlock(this->InputDataT[i]->GetNumberOfBlocks(), copy);
        }
      }
    }
  else
    {
    vtkDebugMacro("This filter cannot handle inputs of type: "
                  << (data?data->GetClassName():"(none)"));
    return 0;
    }

  return 1;
}


//---------------------------------------------------------------------------
bool vtkTemporalStreamTracer::InsideBounds(double point[])
{
  double delta[3] = { 0.0, 0.0, 0.0 };
  for (int t=0; t<2; ++t) {
    for (unsigned int i=0; i<(this->CachedBounds[t].size()); ++i) {
      if (vtkMath::PointIsWithinBounds(point,
        &((this->CachedBounds[t])[i].b[0]), delta)) return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::TestParticles(
  ParticleVector &candidates, ParticleVector &passed, int &count)
{
  int i = 0;
  int div = static_cast<int>(candidates.size()/10);
  if(div==0)
    {
      div=1;
    }
  count = 0;
  for (ParticleIterator it=candidates.begin(); it!=candidates.end(); ++it, ++i) {
    ParticleInformation &info = (*it);
    double *pos = &info.CurrentPosition.x[0];
    // if outside bounds, reject instantly
    if (this->InsideBounds(pos)) {
      if (info.UniqueParticleId==602) {
        vtkDebugMacro(<< "TestParticles got 602");
      }
      // since this is first test, avoid bad cache tests
      this->Interpolator->ClearCache();
      info.LocationState = this->Interpolator->TestPoint(pos);
      if (info.LocationState==ID_OUTSIDE_ALL /*|| location==ID_OUTSIDE_T0*/) {
        // can't really use this particle.
        vtkDebugMacro(<< "TestParticles rejected particle");
      }
      else {
        // get the cached ids and datasets from the TestPoint call
        this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
        passed.push_back(info);
        count++;
      }
    }
    if (i%div==0) {
//      vtkDebugMacro(<< "TestParticles " << i);
    }
  }
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::AssignSeedsToProcessors(
  vtkDataSet *source, int sourceID, int ptId,
  ParticleVector &LocalSeedPoints, int &LocalAssignedCount)
{
  ParticleVector candidates;
  //
  // take points from the source object and create a particle list
  //
  int numSeeds = source->GetNumberOfPoints();
#ifndef NDEBUG
  int numTested = numSeeds;
#endif
  candidates.resize(numSeeds);
  //
  for (int i=0; i<numSeeds; i++) {
    ParticleInformation &info = candidates[i];
    memcpy(&(info.CurrentPosition.x[0]), source->GetPoint(i), sizeof(double)*3);
    info.CurrentPosition.x[3] = this->CurrentTimeSteps[0];
    info.LocationState        = 0;
    info.CachedCellId[0]      =-1;
    info.CachedCellId[1]      =-1;
    info.CachedDataSetId[0]   = 0;
    info.CachedDataSetId[1]   = 0;
    info.SourceID             = sourceID;
    info.InjectedPointId      = i+ptId;
    info.InjectedStepId       = this->ReinjectionCounter;
    info.TimeStepAge          = 0;
    info.UniqueParticleId     =-1;
    info.rotation             = 0.0;
    info.angularVel           = 0.0;
    info.time                 = 0.0;
    info.age                  = 0.0;
    info.speed                = 0.0;
    info.ErrorCode            = 0;
  }
  //
  // Gather all Seeds to all processors for classification
  //

#ifndef NDEBUG
  numTested = static_cast<int>(candidates.size());
#endif
  this->TestParticles(candidates, LocalSeedPoints, LocalAssignedCount);
  int TotalAssigned = LocalAssignedCount; (void)TotalAssigned;

  // Assign unique identifiers taking into account uneven distribution
  // across processes and seeds which were rejected
  this->AssignUniqueIds(LocalSeedPoints);

#ifndef NDEBUG
  vtkDebugMacro(<< "Tested " << numTested << " LocallyAssigned " << LocalAssignedCount);
  if (this->UpdatePiece==0) {
    vtkDebugMacro(<< "Total Assigned to all processes " << TotalAssigned);
  }
#endif
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::AssignUniqueIds(
  vtkTemporalStreamTracerNamespace::ParticleVector &LocalSeedPoints)
{
  vtkIdType ParticleCountOffset = 0;
  vtkIdType numParticles = LocalSeedPoints.size();
  for (vtkIdType i=0; i<numParticles; i++)
    {
    LocalSeedPoints[i].UniqueParticleId =
    this->UniqueIdCounter + ParticleCountOffset + i;
    }
  this->UniqueIdCounter += numParticles;
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::TransmitReceiveParticles(
  ParticleVector &, ParticleVector &, bool )
{
}

//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::UpdateParticleList(ParticleVector &candidates)
{
  int numSeedsNew = static_cast<int>(candidates.size());
  //
  for (int i=0; i<numSeedsNew; i++) {
    // allocate a new particle on the list and get a reference to it
    this->ParticleHistories.push_back(candidates[i]);
  }
  this->NumberOfParticles = static_cast<int>(ParticleHistories.size());

  vtkDebugMacro(<< "UpdateParticleList completed with " << this->NumberOfParticles << " particles");
}

int vtkTemporalStreamTracer::ProcessInput(vtkInformationVector** inputVector)
{
  assert(this->RequestIndex>=0 && this->RequestIndex<2);
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if(numInputs!=1)
    {
    if(numInputs==0)
      {
      vtkErrorMacro(<< "No input found.");
      return 0;
      }
    vtkWarningMacro(<< "Multiple inputs founds. Use only the first one.");
    }

  // inherited from streamtracer, make sure it is null
  this->InputData = NULL;
  this->InputDataT[this->RequestIndex] = NULL;

  vtkInformation    *inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo)
    {
    vtkDataObject* input    = inInfo->Get(vtkDataObject::DATA_OBJECT());
    SetTemporalInput(input,this->RequestIndex);
    //
    // Get the timestep information for this instant
    //
    std::vector<double> timesteps;
    if (inInfo->Has(vtkDataObject::DATA_TIME_STEP()))
      {
      timesteps.resize(1);
      timesteps[0] = inInfo->Get(vtkDataObject::DATA_TIME_STEP());
      }
    else
      {
      vtkErrorMacro(<<"No time step info");
      return 1;
      }
    this->CurrentTimeSteps[this->RequestIndex] = timesteps[0]*this->TimeStepResolution;
    }
  return 1;
}

int vtkTemporalStreamTracer::GenerateOutput(vtkInformationVector** inputVector,
                                            vtkInformationVector* outputVector)

{

  //
  // Parallel/Piece information
  //
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  this->UpdatePiece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->UpdateNumPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  //
  // How many Seed point sources are connected?
  // Copy the sources into a vector for later use
  //

  int numSources = inputVector[1]->GetNumberOfInformationObjects();
  std::vector<vtkDataSet*> SeedSources;
  for (int idx=0; idx<numSources; ++idx)
    {
    vtkDataObject     *dobj   = 0;
    vtkInformation    *inInfo = inputVector[1]->GetInformationObject(idx);
    if (inInfo)
      {
      dobj   = inInfo->Get(vtkDataObject::DATA_OBJECT());
      SeedSources.push_back(vtkDataSet::SafeDownCast(dobj));
      }
    }

  if (this->IntegrationDirection != FORWARD)
    {
    vtkErrorMacro(<<"We can only handle forward time particle tracking at the moment");
    return 1;
    }

  //
  // Add the datasets to an interpolator object
  //
  if (this->InitializeInterpolator() != VTK_OK)
    {
    if (this->InputDataT[0]) this->InputDataT[0] = NULL;
    if (this->InputDataT[1]) this->InputDataT[1] = NULL;
    vtkErrorMacro(<<"InitializeInterpolator failed");
    return 1;
    }

  //
  // Setup some variables
  //
  vtkSmartPointer<vtkInitialValueProblemSolver> integrator;
  integrator.TakeReference(this->GetIntegrator()->NewInstance());
  integrator->SetFunctionSet(this->Interpolator);

  //
  // Make sure the Particle Positions are initialized with Seed particles
  //
  this->ReinjectionFlag = 0;
  if (this->ForceReinjectionEveryNSteps>0) {
  if ((this->ActualTimeStep%this->ForceReinjectionEveryNSteps)==0)
    {
    this->ReinjectionFlag = 1;
    }
  }
  //
  // If T=0 reset everything to allow us to setup stuff then start an animation
  // with a clean slate
  //
  if (this->ActualTimeStep==0) //XXX: what if I start from some other time?
    {
    this->LocalSeeds.clear();
    this->ParticleHistories.clear();
    this->EarliestTime       =-1E6;
    this->ReinjectionFlag    = 1;
    this->ReinjectionCounter = 0;
    this->UniqueIdCounter    = 0;
    this->UniqueIdCounterMPI = 0;
    }
  else if (this->CurrentTimeSteps[0]<this->EarliestTime)
    {
    //
    // We don't want to go back in time, so just reuse whatever we have
    //
    vtkDebugMacro("skipping particle tracking because we have seen this timestep before");
    outInfo->Set(vtkDataObject::DATA_TIME_STEP(), this->OutputTimeValues[this->ActualTimeStep]);
    if (this->InputDataT[0]) this->InputDataT[0] = NULL;
    if (this->InputDataT[1]) this->InputDataT[1] = NULL;
    return 1;
    }
  this->EarliestTime = (this->CurrentTimeSteps[0]>this->EarliestTime)
    ? this->CurrentTimeSteps[0] : this->EarliestTime;
  //
  //
  //
  for (unsigned int i=0; i<SeedSources.size(); i++)
    {
    if (SeedSources[i]->GetMTime()>this->ParticleInjectionTime)
      {
      //    this->ReinjectionFlag = 1;
      }
    }

  //
  // Lists for seed particles
  //
  ParticleVector candidates;
  ParticleVector received;
  //

  if (this->ReinjectionFlag)
    {
    int seedPointId=0;
    if (this->StaticSeeds && this->AllFixedGeometry && this->LocalSeeds.size()==0)
      {
      for (unsigned int i=0; i<SeedSources.size(); i++)
        {
        this->AssignSeedsToProcessors(SeedSources[i], i, 0, this->LocalSeeds, seedPointId);
        }
      }
    else
      {
      // wipe the list and reclassify for each injection
      this->LocalSeeds.clear();
      for (unsigned int i=0; i<SeedSources.size(); i++)
        {
        this->AssignSeedsToProcessors(SeedSources[i], i, 0, this->LocalSeeds, seedPointId);
        }
      }
    this->ParticleInjectionTime.Modified();

    // Now update our main list with the ones we are keeping
    vtkDebugMacro(<< "Reinjection about to update candidates (" << this->LocalSeeds.size() << " particles)");
    this->UpdateParticleList(this->LocalSeeds);
    this->ReinjectionCounter += 1;
  }

  //
  // setup all our output arrays
  //
  vtkDebugMacro(<< "About to allocate point arrays ");
  vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->ParticleAge         = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleIds         = vtkSmartPointer<vtkIntArray>::New();
  this->ParticleSourceIds   = vtkSmartPointer<vtkCharArray>::New();
  this->InjectedPointIds    = vtkSmartPointer<vtkIntArray>::New();
  this->InjectedStepIds     = vtkSmartPointer<vtkIntArray>::New();
  this->ErrorCode           = vtkSmartPointer<vtkIntArray>::New();
  this->ParticleVorticity   = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleRotation    = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleAngularVel  = vtkSmartPointer<vtkFloatArray>::New();
  this->cellVectors         = vtkSmartPointer<vtkDoubleArray>::New();
  this->ParticleCells       = vtkSmartPointer<vtkCellArray>::New();
  this->OutputCoordinates   = vtkSmartPointer<vtkPoints>::New();
  this->OutputPointData     = output->GetPointData();
  this->OutputPointData->Initialize();
  this->InterpolationCount  = 0;
  vtkDebugMacro(<< "About to Interpolate allocate space");
  this->OutputPointData->InterpolateAllocate(this->DataReferenceT[1]->GetPointData());
  //
  this->ParticleAge->SetName("ParticleAge");
  this->ParticleIds->SetName("ParticleId");
  this->ParticleSourceIds->SetName("ParticleSourceId");
  this->InjectedPointIds->SetName("InjectedPointId");
  this->InjectedStepIds->SetName("InjectionStepId");
  this->ErrorCode->SetName("ErrorCode");

  if (this->ComputeVorticity)
    {
    this->cellVectors->SetNumberOfComponents(3);
    this->cellVectors->Allocate(3*VTK_CELL_SIZE);
    this->ParticleVorticity->SetName("Vorticity");
    this->ParticleRotation->SetName("Rotation");
    this->ParticleAngularVel->SetName("AngularVelocity");
    }

  output->SetPoints(this->OutputCoordinates);
  output->SetVerts(this->ParticleCells);
  vtkDebugMacro(<< "Finished allocating point arrays ");

  //
  // Perform 2 passes
  // Pass 0 : Integration of particles created by a source in this process
  // or received at start from a source in another process.
  //
  // Pass 1 : Particles that were sent in mid integration from another process
  // are added in and their integration continued here. In actual fact, the process
  // should be repeated until all particles are finished, but the chances of
  // a particle stepping inside and out again through a single domain
  // in one time step are small (hopefully!)

  vtkDebugMacro(<< "Clear MPI send list ");
  this->MPISendList.clear();

#ifndef NDEBUG
  int Number = static_cast<int>(this->ParticleHistories.size());
#endif
  ParticleListIterator  it_first = this->ParticleHistories.begin();
  ParticleListIterator  it_last  = this->ParticleHistories.end();
  ParticleListIterator  it_next;
#define PASSES 2
  for (int pass=0; pass<PASSES; pass++) {
  vtkDebugMacro(<<"Begin Pass " << pass << " with " << Number << " Particles");
  for (ParticleListIterator it=it_first; it!=it_last;)
    {
    // Keep the 'next' iterator handy because if a particle is terminated
    // or leaves the domain, the 'current' iterator will be deleted.
    it_next = it;
    it_next++;
    //
    // Shall we terminate this particle
    //
    double interval = (this->CurrentTimeSteps[1]-this->CurrentTimeSteps[0]);
    bool terminated = false;
    if (this->TerminationTime>0)
      {
      if (this->TerminationTimeUnit == TERMINATION_TIME_UNIT &&
          (it->age+interval)>this->TerminationTime) {
      terminated = true;
      }
      else if (this->TerminationTimeUnit == TERMINATION_STEP_UNIT &&
               (it->TimeStepAge+1)>this->TerminationTime) {
      terminated = true;
      }
      }
    if (terminated) {
    this->ParticleHistories.erase(it);
    }
    else {
    this->IntegrateParticle(it, this->CurrentTimeSteps[0], this->CurrentTimeSteps[1], integrator);
    }
    //
    if (this->GetAbortExecute()) {
    break;
    }
    it = it_next;
    }
  // Particles might have been deleted during the first pass as they move
  // out of domain or age. Before adding any new particles that are sent
  // to us, we must know the starting point ready for the second pass
  bool list_valid = (this->ParticleHistories.size()>0);
  if (list_valid) {
  // point to one before the end
  it_first = --this->ParticleHistories.end();
  }
  // Send and receive any particles which exited/entered the domain
  if (this->UpdateNumPieces>1 && pass<(PASSES-1)) {
  // the Particle lists will grow if any are received
  // so we must be very careful with our iterators
  vtkDebugMacro(<<"End of Pass " << pass << " with "
                << this->ParticleHistories.size() << " "
                << " about to Transmit/Receive " << this->MPISendList.size());
  this->TransmitReceiveParticles(this->MPISendList, received, true);
  // don't want the ones that we sent away
  this->MPISendList.clear();
  int assigned;
  // classify all the ones we received
  if (received.size()>0) {
  this->TestParticles(received, candidates, assigned);
  vtkDebugMacro(<<"received " << received.size() << " : assigned locally " << assigned);
  received.clear();
  }
  // Now update our main list with the ones we are keeping
  this->UpdateParticleList(candidates);
  // free up unwanted memory
#ifndef NDEBUG
  Number = static_cast<int>(candidates.size());
#endif
  candidates.clear();
  }
  it_last = this->ParticleHistories.end();
  if (list_valid) {
  // increment to point to first new entry
  it_first++;
  }
  else {
  it_first = this->ParticleHistories.begin();
  }
  }
  if (this->MPISendList.size()>0) {
  // If a particle went out of domain on the second pass, it should be sent
  // can it really pass right through a domain in one step?
  // what about grazing the edge of rotating zone?
  vtkDebugMacro(<< "MPISendList not empty " << this->MPISendList.size());
  this->MPISendList.clear();
  }

  //
  // We must only add these scalar arrays at the end because the
  // existing scalars on the input get interpolated during iteration
  // over the particles
  //
  this->OutputPointData->AddArray(this->ParticleIds);
  this->OutputPointData->AddArray(this->ParticleSourceIds);
  this->OutputPointData->AddArray(this->InjectedPointIds);
  this->OutputPointData->AddArray(this->InjectedStepIds);
  this->OutputPointData->AddArray(this->ErrorCode);
  this->OutputPointData->AddArray(this->ParticleAge);
  if (this->ComputeVorticity)
    {
    this->OutputPointData->AddArray(this->ParticleVorticity);
    this->OutputPointData->AddArray(this->ParticleRotation);
    this->OutputPointData->AddArray(this->ParticleAngularVel);
    }

  if (this->InterpolationCount!=this->OutputCoordinates->GetNumberOfPoints()) {
  vtkErrorMacro(<< "Mismatch in point array/data counts");
  }
  //
  outInfo->Set(vtkDataObject::DATA_TIME_STEP(), this->OutputTimeValues[this->ActualTimeStep]);

  // save some locator building, by re-using them as time progresses
  this->Interpolator->AdvanceOneTimeStep();

  //
  // Let go of inputs
  //
  if (this->InputDataT[0]) this->InputDataT[0] = NULL;;
  if (this->InputDataT[1]) this->InputDataT[1] = NULL;;

  //
  // Write Particles out if necessary
  //
  // NB. We don't want our writer to trigger any updates,
  // so shallow copy the output
  if (this->ParticleWriter && this->EnableParticleWriting) {
  vtkSmartPointer<vtkPolyData> polys = vtkSmartPointer<vtkPolyData>::New();
  polys->ShallowCopy(output);
  int N = polys->GetNumberOfPoints(); (void)N;
  this->ParticleWriter->SetFileName(this->ParticleFileName);
  this->ParticleWriter->SetTimeStep(this->ActualTimeStep);
  this->ParticleWriter->SetTimeValue(this->CurrentTimeSteps[1]);
  this->ParticleWriter->SetInputData(polys);
  this->ParticleWriter->Write();
  this->ParticleWriter->CloseFile();
  this->ParticleWriter->SetInputData(NULL);
  vtkDebugMacro(<< "Written " << N);
  }
  return 1;
}

int vtkTemporalStreamTracer::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  //
  // Inputs information
  //
  bool result(true);
  if(RequestIndex<2)
    {
    result = (this->ProcessInput(inputVector)==1);
    if(result && RequestIndex==1)
      {
      this->GenerateOutput(inputVector,outputVector);
      }
    }

  this->RequestIndex++;
  if(result && this->RequestIndex<2)
    {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
  else
    {
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->RequestIndex=0;
    }


//  this->Interpolator->ShowCacheResults();
//  vtkErrorMacro(<<"RequestData done");
  return 1;
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::IntegrateParticle(
  ParticleListIterator &it,
  double currenttime, double targettime,
  vtkInitialValueProblemSolver* integrator)
{
  double epsilon = (targettime-currenttime)/100.0;
  double velocity[3], point1[4], point2[4] = {0.0, 0.0, 0.0, 0.0};
  double minStep=0, maxStep=0;
  double stepWanted, stepTaken=0.0;
  substeps = 0;

  ParticleInformation &info = (*it);
  // Get the Initial point {x,y,z,t}
  memcpy(point1, &info.CurrentPosition, sizeof(Position));

  if (point1[3]<(currenttime-epsilon) || point1[3]>(targettime+epsilon)) {
    vtkDebugMacro(<< "Bad particle time : expected ("
      << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1]
      << ") got " << point1[3]);
  }

  IntervalInformation delT;
  delT.Unit     = LENGTH_UNIT;
  delT.Interval = (targettime-currenttime) * this->InitialIntegrationStep;
  epsilon = delT.Interval*1E-3;

  //
  // begin interpolation between available time values, if the particle has
  // a cached cell ID and dataset - try to use it,
  //
  this->Interpolator->SetCachedCellIds(info.CachedCellId, info.CachedDataSetId);

  bool particle_good = true;
  info.ErrorCode = 0;
  while (point1[3] < (targettime-epsilon)) {
    //
    // Here beginneth the real work
    //
    double error = 0;

    // If, with the next step, propagation will be larger than
    // max, reduce it so that it is (approximately) equal to max.
    stepWanted = delT.Interval;
    if ( (point1[3] + stepWanted) > targettime )
      {
      stepWanted = targettime - point1[3];
      maxStep = stepWanted;
      }
    this->LastUsedStepSize = stepWanted;

    // Calculate the next step using the integrator provided.
    // If the next point is out of bounds, send it to another process
    if (integrator->ComputeNextStep(
          point1, point2, point1[3], stepWanted,
          stepTaken, minStep, maxStep,
          this->MaximumError, error) != 0)
    {
      // if the particle is sent, remove it from the list
      info.ErrorCode = 1;
      if (this->SendParticleToAnotherProcess(info, point1, this->LastUsedStepSize)) {
        this->ParticleHistories.erase(it);
        particle_good = false;
        break;
      }
      else
      {
        // particle was not sent, retry saved it, so copy info back
        substeps++;
        memcpy(point1, &info.CurrentPosition, sizeof(Position));
      }
    }
    else // success, increment position/time
    {
      substeps++;

      // increment the particle time
      point2[3] = point1[3] + stepTaken;
      info.age += stepTaken;

      // Point is valid. Insert it.
      memcpy(&info.CurrentPosition, point2, sizeof(Position));
      memcpy(point1, point2, sizeof(Position));
    }

    // If the solver is adaptive and the next time step (delT.Interval)
    // that the solver wants to use is smaller than minStep or larger
    // than maxStep, re-adjust it. This has to be done every step
    // because minStep and maxStep can change depending on the Cell
    // size (unless it is specified in time units)
    if (integrator->IsAdaptive())
      {
        // code removed. Put it back when this is stable
      }
  }
  if (particle_good) {
    // The integration succeeded, but check the computed final position
    // is actually inside the domain (the intermediate steps taken inside
    // the integrator were ok, but the final step may just pass out)
    // if it moves out, we can't interpolate scalars, so we must send it away
    info.LocationState = this->Interpolator->TestPoint(info.CurrentPosition.x);
    if (info.LocationState==ID_OUTSIDE_ALL)
    {
      info.ErrorCode = 2;
      // if the particle is sent, remove it from the list
      if (this->SendParticleToAnotherProcess(info, point1, this->LastUsedStepSize)) {
        this->ParticleHistories.erase(it);
        particle_good = false;
      }
    }
  }

  //
  // Has this particle stagnated
  //
  if (particle_good) {
    this->Interpolator->GetLastGoodVelocity(velocity);
    info.speed = vtkMath::Norm(velocity);
    if (it->speed <= this->TerminalSpeed)
    {
      this->ParticleHistories.erase(it);
      particle_good = false;
    }
  }

  //
  // We got this far without error :
  // Insert the point into the output
  // Create any new scalars and interpolate existing ones
  // Cache cell ids and datasets
  //
  if (particle_good) {
    //
    // store the last Cell Ids and dataset indices for next time particle is updated
    //
    this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
    //
    info.TimeStepAge += 1;
    //
    // Now generate the output geometry and scalars
    //
    double    *coord = &info.CurrentPosition.x[0];
    vtkIdType tempId = this->OutputCoordinates->InsertNextPoint(coord);
    // create the cell
    this->ParticleCells->InsertNextCell(1, &tempId);
    // set the easy scalars for this particle
    this->ParticleIds->InsertNextValue(info.UniqueParticleId);
    this->ParticleSourceIds->InsertNextValue(info.SourceID);
    this->InjectedPointIds->InsertNextValue(info.InjectedPointId);
    this->InjectedStepIds->InsertNextValue(info.InjectedStepId);
    this->ErrorCode->InsertNextValue(info.ErrorCode);
    this->ParticleAge->InsertNextValue(info.age);
    //
    // Interpolate all existing point attributes
    // In principle we always integrate the particle until it reaches Time2
    // - so we don't need to do any interpolation of the scalars
    // between T0 and T1, just fetch the values
    // of the spatially interpolated scalars from T1.
    //
    if (info.LocationState==ID_OUTSIDE_T1) {
      this->Interpolator->InterpolatePoint(0, this->OutputPointData, tempId);
    }
    else {
      this->Interpolator->InterpolatePoint(1, this->OutputPointData, tempId);
    }
    this->InterpolationCount++;
    //
    // Compute vorticity
    //
    if (this->ComputeVorticity)
    {
      vtkGenericCell *cell;
      double pcoords[3], vorticity[3], weights[256];
      double rotation, omega;
      // have to use T0 if particle is out at T1, otherwise use T1
      if (info.LocationState==ID_OUTSIDE_T1) {
        this->Interpolator->GetVorticityData(
          0, pcoords, weights, cell, this->cellVectors);
      }
      else {
        this->Interpolator->GetVorticityData(
          1, pcoords, weights, cell, this->cellVectors);
      }
      vtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vorticity);
      this->ParticleVorticity->InsertNextTuple(vorticity);
      // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
      if (info.speed != 0.0) {
        omega = vtkMath::Dot(vorticity, velocity);
        omega /= info.speed;
        omega *= this->RotationScale;
      }
      else {
        omega = 0.0;
      }
      vtkIdType index = this->ParticleAngularVel->InsertNextValue(omega);
      if (index>0) {
        rotation     = info.rotation + (info.angularVel + omega)/2 * (info.CurrentPosition.x[3] - info.time);
      }
      else {
        rotation     = 0.0;
      }
      this->ParticleRotation->InsertNextValue(rotation);
      info.rotation   = rotation;
      info.angularVel = omega;
      info.time       = info.CurrentPosition.x[3];
    }
  }
  else this->Interpolator->ClearCache();

  double eps = (this->CurrentTimeSteps[1]-this->CurrentTimeSteps[0])/100;
  if (point1[3]<(this->CurrentTimeSteps[0]-eps) || point1[3]>(this->CurrentTimeSteps[1]+eps)) {
    vtkDebugMacro(<< "Unexpected time ending IntegrateParticle - expected ("
      << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got " << point1[3]);
  }
}
//---------------------------------------------------------------------------
bool vtkTemporalStreamTracer::RetryWithPush(ParticleInformation &info, double velocity[3], double delT)
{
  // try adding a one increment push to the particle to get over a rotating/moving boundary
  for (int v=0; v<3; v++) info.CurrentPosition.x[v] += velocity[v]*delT;
  info.CurrentPosition.x[3] += delT;
  info.LocationState = this->Interpolator->TestPoint(info.CurrentPosition.x);
  if (info.LocationState!=ID_OUTSIDE_ALL) {
    // a push helped the particle get back into a dataset,
    info.age += delT;
    info.ErrorCode = 6;
   return 1;
  }
  return 0;
}
//---------------------------------------------------------------------------
bool vtkTemporalStreamTracer::SendParticleToAnotherProcess(ParticleInformation &info, double point1[4], double delT)
{
//  return 1;
  double velocity[3];
  this->Interpolator->ClearCache();
  if (info.UniqueParticleId==3) {
    vtkDebugMacro(<< "3 is about to be sent");
  }
  info.LocationState = this->Interpolator->TestPoint(point1);
  if (info.LocationState==ID_OUTSIDE_ALL) {
    // something is wrong, the particle has left the building completely
    // we can't get the last good velocity as it won't be valid
    // send the particle 'as is' and hope it lands in another process
    if (substeps>0) {
      this->Interpolator->GetLastGoodVelocity(velocity);
    } else {
      velocity[0] = velocity[1] = velocity[2] = 0.0;
    }
    info.ErrorCode = 3;
  }
  else if (info.LocationState==ID_OUTSIDE_T0) {
    // the particle left the volume but can be tested at T2, so use the velocity at T2
    this->Interpolator->GetLastGoodVelocity(velocity);
    info.ErrorCode = 4;
  }
  else if (info.LocationState==ID_OUTSIDE_T1) {
    // the particle left the volume but can be tested at T1, so use the velocity at T1
    this->Interpolator->GetLastGoodVelocity(velocity);
    info.ErrorCode = 5;
  }
  else {
    // The test returned INSIDE_ALL, so test failed near start of integration,
    this->Interpolator->GetLastGoodVelocity(velocity);
  }
  if (this->RetryWithPush(info, velocity, delT)) return 0;
  this->AddParticleToMPISendList(info);
  return 1;
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TimeStepResolution: " << this->TimeStepResolution << endl;
  os << indent << "ParticleWriter: " << this->ParticleWriter << endl;
  os << indent << "ParticleFileName: " <<
    (this->ParticleFileName ? this->ParticleFileName : "None") << endl;
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "ForceReinjectionEveryNSteps: "
     << this->ForceReinjectionEveryNSteps << endl;
  os << indent << "EnableParticleWriting: " << this->EnableParticleWriting << endl;
  os << indent << "IgnorePipelineTime: " << this->IgnorePipelineTime << endl;
  os << indent << "StaticMesh: " << this->StaticMesh << endl;
  os << indent << "TerminationTime: " << this->TerminationTime << endl;
  os << indent << "TerminationTimeUnit: " << this->TerminationTimeUnit << endl;
  os << indent << "StaticSeeds: " << this->StaticSeeds << endl;
}
//---------------------------------------------------------------------------
bool vtkTemporalStreamTracer::ComputeDomainExitLocation(
  double pos[4], double p2[4], double intersection[4],
  vtkGenericCell *cell)
{
  double t, pcoords[3];
  int subId;
  if (cell->IntersectWithLine(pos, p2, 1E-3, t, intersection, pcoords, subId)==0) {
    vtkDebugMacro(<< "No cell/domain exit was found");
    return 0;
  }
  else {
    // We found an intersection on the edge of the cell.
    // Shift it by a small amount to ensure that it crosses over the edge
    // into the adjoining cell.
    for (int i=0; i<3; i++) intersection[i] = pos[i] + (t+0.01)*(p2[i]-pos[i]);
    // intersection stored, compute T for intersection
    intersection[3] = pos[3] + (t+0.01)*(p2[3]-pos[3]);
    return 1;
  }
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::AddParticleToMPISendList(ParticleInformation &info)
{
  double eps = (this->CurrentTimeSteps[1]-this->CurrentTimeSteps[0])/100;
  if (info.CurrentPosition.x[3]<(this->CurrentTimeSteps[0]-eps) ||
      info.CurrentPosition.x[3]>(this->CurrentTimeSteps[1]+eps)) {
    vtkDebugMacro(<< "Unexpected time value in MPISendList - expected ("
      << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got "
      << info.CurrentPosition.x[3]);
  }
#ifdef VTK_USE_MPI
  if (this->MPISendList.capacity()<(this->MPISendList.size()+1)) {
    this->MPISendList.reserve(static_cast<int>(this->MPISendList.size()*1.5));
  }
  this->MPISendList.push_back(info);
#endif
}
//---------------------------------------------------------------------------
/*
  // Now try to compute the trajectory exit location from the cell on the edge
  if (this->Interpolator->GetLastValidCellId(0)!=-1) {
    vtkDataSet *hitdata = this->Interpolator->GetLastDataSet(0);
    hitdata->GetCell(this->Interpolator->GetLastValidCellId(0), this->GenericCell);
    double intersection[4];
    if (this->ComputeDomainExitLocation(point1, point2, intersection, this->GenericCell))
    {
      vtkDebugMacro(<< "SendParticleToAnotherProcess : Sending Particle " << particleId << " Time " << intersection[3]);
      this->AddParticleToMPISendList(intersection, velocity);
      vtkIdType nextPoint = ParticleCoordinates->InsertNextPoint(point2);
      this->ParticleHistories[particleId].push_back(nextPoint);
      this->LiveParticleIds[particleId] = 0;
      return 1;
    }
    else {
      vtkDebugMacro(<< "Domain-Exit aborted : Domain Intersection failed" << particleId );
      this->LiveParticleIds[particleId] = 0;
      return 0;
    }
  }
  else {
    vtkDebugMacro(<< "Domain-Exit aborted : Couldn't copy cell from earlier test" << particleId );
    this->LiveParticleIds[particleId] = 0;
    return 0;
  }
*/
