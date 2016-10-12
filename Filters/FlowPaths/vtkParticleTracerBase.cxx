/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticleTracerBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkParticleTracerBase.h"

#include "vtkAbstractParticleWriter.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTemporalInterpolatedVelocityField.h"
#include <cassert>

#include <functional>
#include <algorithm>
#ifdef DEBUGPARTICLETRACE
#define Assert(x) assert(x)
#define PRINT(x) cout<<__LINE__<<": "<<x<<endl;
#else
#define PRINT(x)
#define Assert(x)
#endif

const double vtkParticleTracerBase::Epsilon = 1.0E-12;

using namespace vtkParticleTracerBaseNamespace;

vtkCxxSetObjectMacro(vtkParticleTracerBase, ParticleWriter, vtkAbstractParticleWriter);
vtkCxxSetObjectMacro(vtkParticleTracerBase,Integrator,vtkInitialValueProblemSolver);

// this SetMacro is different than the regular vtkSetMacro
// because it resets the cache as well.
#define ParticleTracerSetMacro(name,type)               \
  void vtkParticleTracerBase::Set##name (type _arg)     \
  {                                                     \
    if (this->name == _arg)                             \
    {                                                 \
      return;                                           \
    }                                                 \
    this->name = _arg;                                  \
    this->ResetCache();                                 \
    this->Modified();                                   \
  }
ParticleTracerSetMacro(StartTime, double)
ParticleTracerSetMacro(ComputeVorticity, bool);
ParticleTracerSetMacro(RotationScale, double)
ParticleTracerSetMacro(ForceReinjectionEveryNSteps,int);
ParticleTracerSetMacro(TerminalSpeed, double);

namespace
{
  //return the interval i, such that a belongs to the interval (A[i],A[i+1]]
  inline int FindInterval(double a, const std::vector<double>& A)
  {
    if(A.size() == 0 || a < A[0])
    {
      return -1;
    }

    for(size_t i=0; i< A.size()-1;i++)
    {
      if(a <= A[i+1])
      {
        return static_cast<int>(i);
      }
    }

    return -1;
  }
};

//---------------------------------------------------------------------------
vtkParticleTracerBase::vtkParticleTracerBase()
{
  // by default process active point vectors
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::VECTORS);

  this->CurrentTimeStep             = 0;
  this->CurrentTimeValue            = 0;
  this->ForceReinjectionEveryNSteps = 0;
  this->ReinjectionCounter          = 0;
  this->AllFixedGeometry            = 1;
  this->StaticMesh                  = 0;
  this->StaticSeeds                 = 0;
  this->ComputeVorticity            = 1;
  this->IgnorePipelineTime          = 1;
  this->ParticleWriter              = NULL;
  this->ParticleFileName            = NULL;
  this->EnableParticleWriting       = false;
  this->UniqueIdCounter             = 0;
  this->Integrator                  = NULL;

  this->StartTime = 0.0;
  this->TerminationTime       = 0.0;
  this->FirstIteration = true;
  this->HasCache = false;

  this->RotationScale    = 1.0;
  this->MaximumError         = 1.0e-6;
  this->TerminalSpeed = vtkParticleTracerBase::Epsilon;
  this->IntegrationStep = 0.5;

  this->Interpolator = vtkSmartPointer<vtkTemporalInterpolatedVelocityField>::New();
  this->SetNumberOfInputPorts(2);

#ifdef JB_H5PART_PARTICLE_OUTPUT
#ifdef _WIN32
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
  this->DisableResetCache = 0;
}

//---------------------------------------------------------------------------
vtkParticleTracerBase::~vtkParticleTracerBase()
{
  this->SetParticleWriter(NULL);
  this->SetParticleFileName(NULL);

  this->CachedData[0] = NULL;
  this->CachedData[1] = NULL;

  this->SetIntegrator(0);
  this->SetInterpolatorPrototype(0);
}

//----------------------------------------------------------------------------
int vtkParticleTracerBase::FillInputPortInformation(
  int port, vtkInformation* info)
{
  // port 0 must be a temporal collection of any type
  // the executive should put a temporal collection in when
  // we request multiple time steps.
  if (port==0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  }
  else if (port==1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkParticleTracerBase::AddSourceConnection(vtkAlgorithmOutput* input)
{
  this->AddInputConnection(1, input);
}

//----------------------------------------------------------------------------
void vtkParticleTracerBase::RemoveAllSources()
{
  this->SetInputConnection(1, 0);
}

//----------------------------------------------------------------------------
int vtkParticleTracerBase::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    if(this->FirstIteration)
    {
      return this->RequestInformation(request, inputVector, outputVector);
    }
  }
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkParticleTracerBase::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  vtkInformation *inInfo  = inputVector[0]->GetInformationObject(0);

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
  {
    unsigned int numberOfInputTimeSteps =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    vtkDebugMacro(<<"vtkParticleTracerBase "
                  "inputVector TIME_STEPS " << numberOfInputTimeSteps);
    //
    // Get list of input time step values
    this->InputTimeValues.resize(numberOfInputTimeSteps);
    inInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 &this->InputTimeValues[0] );
    if (numberOfInputTimeSteps==1 && this->DisableResetCache==0)
    {  //warning would be skipped in coprocessing work flow
      vtkWarningMacro(<<"Not enough input time steps for particle integration");
    }

    //clamp the default start time to be within the data time range
    if(this->StartTime < this->InputTimeValues[0])
    {
      this->StartTime = this->InputTimeValues[0];
    }
    else if (this->StartTime > this->InputTimeValues.back())
    {
      this->StartTime = this->InputTimeValues.back();
    }
  }
  else
  {
    vtkErrorMacro(<<"Input information has no TIME_STEPS set");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkParticleTracerBase::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  PRINT("RUE: "<<this->HasCache<<" "<<this->FirstIteration
        <<" "<<this->StartTime<<" "<<this->TerminationTime<<" "<<this->CurrentTimeStep);
  //
  // The output has requested a time value, what times must we ask from our input
  // do this only for the first time
  if(this->FirstIteration)
  {
    if(this->InputTimeValues.size()==1)
    {
      this->StartTimeStep = this->InputTimeValues[0]==this->StartTime? 0 : -1;
    }
    else
    {
      this->StartTimeStep = FindInterval(this->StartTime, this->InputTimeValues);
    }

    if(this->StartTimeStep<0)
    {
      vtkErrorMacro("Start time not in time range");
      return 0;
    }

    if (!this->IgnorePipelineTime && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      double terminationTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      PRINT("Pipeline has set termination time: "<< terminationTime);
      this->SetTerminationTimeNoModify(terminationTime);
    }

    if(this->TerminationTime> this->InputTimeValues.back())
    {
      this->TerminationTime = this->InputTimeValues.back();
    }

    if(this->InputTimeValues.size()==1)
    {
      this->TerminationTimeStep = this->TerminationTime == this->InputTimeValues[0]? 0 : -1;
    }
    else
    {
      this->TerminationTimeStep = FindInterval(this->TerminationTime, this->InputTimeValues)+1;
    }

    if(this->TerminationTimeStep<0)
    {
      vtkErrorMacro("Termination time not in time range");
      return 0;
    }

    for(int i=0; i<this->GetNumberOfInputPorts(); i++)
    {
      vtkInformation* info = this->GetInputPortInformation(i);
      if(info->Get(vtkAlgorithm::INPUT_IS_OPTIONAL()) &&
         this->GetNumberOfInputConnections(i) == 0)
      {
        continue;
      }
      vtkAlgorithm* inputAlgorithm = this->GetInputAlgorithm(i,0);
      vtkStreamingDemandDrivenPipeline* sddp = vtkStreamingDemandDrivenPipeline::SafeDownCast(inputAlgorithm->GetExecutive());
      if(sddp)
      {
        sddp->UpdatePipelineMTime();
        vtkMTimeType pmt = sddp->GetPipelineMTime();
        if(pmt>this->ExecuteTime.GetMTime())
        {
          PRINT("Reset cache of because upstream is newer")
            this->ResetCache();
        }
      }
    }
    if(!this->HasCache)
    {
      this->CurrentTimeStep = this->StartTimeStep;
      this->CurrentTimeValue = -DBL_MAX;
    }
  }

  for (int i=0; i<numInputs; i++)
  {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(i);
    if(this->CurrentTimeStep < static_cast<int>(this->InputTimeValues.size()))
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->InputTimeValues[this->CurrentTimeStep]);
    }
    else
    {
      Assert(this->CurrentTimeValue == this->InputTimeValues.back());
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
int vtkParticleTracerBase::InitializeInterpolator()
{
  if (!this->CachedData[0] || !this->CachedData[1])
  {
    vtkErrorMacro("Missing data set to process.");
    return VTK_ERROR;
  }
  //
  // When Multiblock arrays are processed, some may be empty
  // if the first is empty, we won't find the correct vector name
  // so scan until we get one
  //
  vtkSmartPointer<vtkCompositeDataIterator> iterP;
  iterP.TakeReference(this->CachedData[0]->NewIterator());
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
    vtkErrorMacro(<< "Couldn't find vector array " << vecname);
    return VTK_ERROR;
  }

  vtkDebugMacro(<< "Interpolator using array " << vecname);
  this->Interpolator->SelectVectors(vecname);

  this->AllFixedGeometry = 1;

  int numValidInputBlocks[2] = {0, 0};
  int numTotalInputBlocks[2] = {0, 0};
  this->DataReferenceT[0] = this->DataReferenceT[1] = 0;
  for (int T=0; T<2; T++)
  {
    this->CachedBounds[T].clear();
    int index = 0;
    // iterate over all blocks of input and cache the bounds information
    // and determine fixed/dynamic mesh status.

    vtkSmartPointer<vtkCompositeDataIterator> anotherIterP;
    anotherIterP.TakeReference(this->CachedData[T]->NewIterator());
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
          bool static_dataset = (this->StaticMesh != 0);
          this->AllFixedGeometry = this->AllFixedGeometry && static_dataset;
          // add the dataset to the interpolator
          this->Interpolator->SetDataSetAtTime(index++, T, this->GetCacheDataTime(T), inp, static_dataset);
          if (!this->DataReferenceT[T])
          {
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
    vtkErrorMacro("Not enough inputs have been found. Can not execute."
                  << numValidInputBlocks[0] << " " << numValidInputBlocks[1]);
    return VTK_ERROR;
  }
  if (numValidInputBlocks[0] != numValidInputBlocks[1] && this->StaticMesh)
  {
    vtkErrorMacro("StaticMesh is set to True but the number of datasets is different between time steps "
                  << numValidInputBlocks[0] << " " << numValidInputBlocks[1]);
    return VTK_ERROR;
  }
  //
  vtkDebugMacro("Number of Valid input blocks is " << numValidInputBlocks[0] << " from " << numTotalInputBlocks[0]);
  vtkDebugMacro("AllFixedGeometry " << this->AllFixedGeometry);

  // force optimizations if StaticMesh is set.
  if (this->StaticMesh)
  {
    vtkDebugMacro("Static Mesh optimizations Forced ON");
    this->AllFixedGeometry = 1;
  }

  //
  return VTK_OK;
}

//---------------------------------------------------------------------------
std::vector<vtkDataSet*> vtkParticleTracerBase::GetSeedSources(
  vtkInformationVector* inputVector, int vtkNotUsed(timeStep))
{
  int numSources = inputVector->GetNumberOfInformationObjects();
  std::vector<vtkDataSet*> seedSources;
  for (int idx=0; idx<numSources; ++idx)
  {
    if(vtkInformation *inInfo = inputVector->GetInformationObject(idx))
    {
      vtkDataObject* dobj   = inInfo->Get(vtkDataObject::DATA_OBJECT());
      seedSources.push_back(vtkDataSet::SafeDownCast(dobj));
    }
  }
  return seedSources;
}

//---------------------------------------------------------------------------
int vtkParticleTracerBase::UpdateDataCache(vtkDataObject *data)
{
  double dataTime = data->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());

  Assert(dataTime>=this->GetCacheDataTime());
  if(dataTime== this->GetCacheDataTime())
  {
    return 1;
  }

  int i = 0;
  if(this->CurrentTimeStep==this->StartTimeStep)
  {
    i = 0;
  }
  else if (this->CurrentTimeStep==this->StartTimeStep+1)
  {
    i = 1;
  }
  else
  {
    i = 1;
    this->CachedData[0] = this->CachedData[1];
    this->CachedData[1] = NULL;
  }


  this->CachedData[i] = vtkSmartPointer<vtkMultiBlockDataSet>::New();

  // if simple dataset, add to our list, otherwise, add blocks
  vtkDataSet           *dsInput = vtkDataSet::SafeDownCast(data);
  vtkMultiBlockDataSet *mbInput = vtkMultiBlockDataSet::SafeDownCast(data);

  if (dsInput)
  {
    vtkSmartPointer<vtkDataSet> copy;
    copy.TakeReference(dsInput->NewInstance());
    copy->ShallowCopy(dsInput);
    this->CachedData[i]->SetBlock(this->CachedData[i]->GetNumberOfBlocks(), copy);
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
        this->CachedData[i]->SetBlock(this->CachedData[i]->GetNumberOfBlocks(), copy);
      }
    }
  }
  else
  {
    vtkDebugMacro("This filter cannot handle inputs of type: "
                  << (data?data->GetClassName():"(none)"));
    return 0;
  }

  this->CachedData[i]->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dataTime);
  if(this->CurrentTimeStep==this->StartTimeStep)
  {
    this->CachedData[1] = this->CachedData[0];
  }
  return 1;
}

//---------------------------------------------------------------------------
bool vtkParticleTracerBase::InsideBounds(double point[])
{
  double delta[3] = { 0.0, 0.0, 0.0 };
  for (int t=0; t<2; ++t)
  {
    for (size_t i=0; i<(this->CachedBounds[t].size()); ++i)
    {
      if (vtkMath::PointIsWithinBounds(point, &((this->CachedBounds[t])[i].b[0]), delta))
      {
        return true;
      }
    }
  }
  return false;
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::TestParticles(
  ParticleVector &candidates, ParticleVector &passed, int &count)
{
  std::vector<int> passedIndices;
  this->TestParticles(candidates, passedIndices);
  count = static_cast<int>(passedIndices.size());

  for(size_t i=0; i<passedIndices.size();i++)
  {
    passed.push_back(candidates[passedIndices[i]]);
  }
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::TestParticles(
  vtkParticleTracerBaseNamespace::ParticleVector &candidates,
  std::vector<int> &passed)
{
  int i = 0;
  for (ParticleIterator it=candidates.begin(); it!=candidates.end(); ++it, ++i)
  {
    ParticleInformation &info = (*it);
    double *pos = &info.CurrentPosition.x[0];
    // if outside bounds, reject instantly
    if (this->InsideBounds(pos))
    {
      // since this is first test, avoid bad cache tests
      this->Interpolator->ClearCache();
      info.LocationState = this->Interpolator->TestPoint(pos);
      if (info.LocationState==ID_OUTSIDE_ALL /*|| location==ID_OUTSIDE_T0*/)
      {
        // can't really use this particle.
        vtkDebugMacro(<< "TestParticles rejected particle");
      }
      else
      {
        // get the cached ids and datasets from the TestPoint call
        this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
        passed.push_back(i);
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::AssignSeedsToProcessors(
  double time, vtkDataSet *source, int sourceID, int ptId,
  ParticleVector &localSeedPoints, int &localAssignedCount)
{
  ParticleVector candidates;
  //
  // take points from the source object and create a particle list
  //
  int numSeeds = source->GetNumberOfPoints();
  candidates.resize(numSeeds);
  //
  for (int i=0; i<numSeeds; i++)
  {
    ParticleInformation &info = candidates[i];
    memcpy(&(info.CurrentPosition.x[0]), source->GetPoint(i), sizeof(double)*3);
    info.CurrentPosition.x[3] = time;
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
    info.SimulationTime       = this->GetCurrentTimeValue();
    info.PointId              = -1;
    info.TailPointId          = -1;
  }
  //
  // Gather all Seeds to all processors for classification
  //
  this->TestParticles(candidates, localSeedPoints, localAssignedCount);

  // Assign unique identifiers taking into account uneven distribution
  // across processes and seeds which were rejected
  this->AssignUniqueIds(localSeedPoints);
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::AssignUniqueIds(
  vtkParticleTracerBaseNamespace::ParticleVector &localSeedPoints)
{
  vtkIdType particleCountOffset = 0;
  vtkIdType numParticles = localSeedPoints.size();
  for (vtkIdType i=0; i<numParticles; i++)
  {
    localSeedPoints[i].UniqueParticleId =
      this->UniqueIdCounter + particleCountOffset + i;
  }
  this->UniqueIdCounter += numParticles;
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::UpdateParticleList(ParticleVector &candidates)
{
  int numSeedsNew = static_cast<int>(candidates.size());
  //
  for (int i=0; i<numSeedsNew; i++)
  {
    // allocate a new particle on the list and get a reference to it
    this->ParticleHistories.push_back(candidates[i]);
  }
  vtkDebugMacro(<< "UpdateParticleList completed with " << this->NumberOfParticles() << " particles");
}

//---------------------------------------------------------------------------
int vtkParticleTracerBase::ProcessInput(vtkInformationVector** inputVector)
{
  Assert(this->CurrentTimeStep>=StartTimeStep  && this->CurrentTimeStep<=TerminationTimeStep);
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

  vtkInformation    *inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo)
  {
    vtkDataObject* input    = inInfo->Get(vtkDataObject::DATA_OBJECT());
    this->UpdateDataCache(input);
  }
  return 1;
}

//---------------------------------------------------------------------------
vtkPolyData* vtkParticleTracerBase::Execute(vtkInformationVector** inputVector)
{
  Assert(this->CurrentTimeStep>=this->StartTimeStep);

  double from = this->CurrentTimeStep==this->StartTimeStep? this->StartTime : this->GetCacheDataTime(0);
  this->CurrentTimeValue =
    this->CurrentTimeStep==this->StartTimeStep? this->StartTime:
    (this->CurrentTimeStep==this->TerminationTimeStep? this->TerminationTime : this->GetCacheDataTime(1));

  //set up the output
  vtkPolyData *output = vtkPolyData::New();
  //
  // Add the datasets to an interpolator object
  //
  if (this->InitializeInterpolator() != VTK_OK)
  {
    if (this->CachedData[0])
    {
      this->CachedData[0] = NULL;
    }
    if (this->CachedData[1])
    {
      this->CachedData[1] = NULL;
    }
    vtkErrorMacro(<<"InitializeInterpolator failed");
    return output;
  }

  vtkDebugMacro(<< "About to allocate point arrays ");
  this->ParticleAge         = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleIds         = vtkSmartPointer<vtkIntArray>::New();
  this->ParticleSourceIds   = vtkSmartPointer<vtkCharArray>::New();
  this->InjectedPointIds    = vtkSmartPointer<vtkIntArray>::New();
  this->InjectedStepIds     = vtkSmartPointer<vtkIntArray>::New();
  this->ErrorCode           = vtkSmartPointer<vtkIntArray>::New();
  this->ParticleVorticity   = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleRotation    = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleAngularVel  = vtkSmartPointer<vtkFloatArray>::New();
  this->CellVectors         = vtkSmartPointer<vtkDoubleArray>::New();
  this->ParticleCells       = vtkSmartPointer<vtkCellArray>::New();
  this->OutputCoordinates   = vtkSmartPointer<vtkPoints>::New();

  this->OutputPointData     = output->GetPointData();
  this->OutputPointData->Initialize();
  vtkDebugMacro(<< "About to Interpolate allocate space");
  this->OutputPointData->InterpolateAllocate(this->DataReferenceT[0]->GetPointData());
  this->ParticleAge->SetName("ParticleAge");
  this->ParticleIds->SetName("ParticleId");
  this->ParticleSourceIds->SetName("ParticleSourceId");
  this->InjectedPointIds->SetName("InjectedPointId");
  this->InjectedStepIds->SetName("InjectionStepId");
  this->ErrorCode->SetName("ErrorCode");

  if (this->ComputeVorticity)
  {
    this->CellVectors->SetNumberOfComponents(3);
    this->CellVectors->Allocate(3*VTK_CELL_SIZE);
    this->ParticleVorticity->SetName("Vorticity");
    this->ParticleRotation->SetName("Rotation");
    this->ParticleAngularVel->SetName("AngularVelocity");
  }
  this->InitializeExtraPointDataArrays(output->GetPointData());

  output->SetPoints(this->OutputCoordinates);
  output->SetVerts(this->ParticleCells);
  vtkDebugMacro(<< "Finished allocating point arrays ");

  // How many Seed point sources are connected?
  // Copy the sources into a vector for later use

  std::vector<vtkDataSet*> seedSources = this->GetSeedSources(inputVector[1], this->CurrentTimeStep);

  //
  // Setup some variables
  //
  vtkSmartPointer<vtkInitialValueProblemSolver> integrator;
  integrator.TakeReference(this->GetIntegrator()->NewInstance());
  integrator->SetFunctionSet(this->Interpolator);

  //
  // Make sure the Particle Positions are initialized with Seed particles
  //
  if (this->StartTimeStep==this->CurrentTimeStep)
  {
    Assert(!this->HasCache); //shouldn't have cache if restarting
    int seedPointId=0;
    if (!(this->StaticSeeds && this->AllFixedGeometry && this->LocalSeeds.size()==0))
    {
      // wipe the list and reclassify for each injection
      this->LocalSeeds.clear();
    }

    this->AddRestartSeeds(inputVector);

    for (size_t i=0; i<seedSources.size(); i++)
    {
      this->AssignSeedsToProcessors(this->CurrentTimeValue,seedSources[i], static_cast<int>(i),
                                    0, this->LocalSeeds, seedPointId);
    }

    this->ParticleInjectionTime.Modified();

    // Now update our main list with the ones we are keeping
    vtkDebugMacro(<< "Reinjection about to update candidates (" << this->LocalSeeds.size() << " particles)");
    this->UpdateParticleList(this->LocalSeeds);
    //this->ReinjectionCounter += this->ForceReinjectionEveryNSteps;
  }

  if(this->CurrentTimeStep==this->StartTimeStep) //just add all the particles
  {
    for(ParticleListIterator itr = this->ParticleHistories.begin();  itr!=this->ParticleHistories.end();itr++)
    {
      ParticleInformation& info(*itr);
      this->Interpolator->TestPoint(info.CurrentPosition.x);
      double velocity[3];
      this->Interpolator->GetLastGoodVelocity(velocity);
      info.speed = vtkMath::Norm(velocity);
      this->AddParticle(*itr,velocity);
    }
  }
  else
  {
    ParticleListIterator  it_first = this->ParticleHistories.begin();
    ParticleListIterator  it_last  = this->ParticleHistories.end();
    ParticleListIterator  it_next;

    //
    // Perform mulitple passes. The number of passes is equal to one more than
    // the maximum times a particle gets migrated between processes.
    bool continueExecuting = true;
    int pass = 0; // really just for debugging
    while(continueExecuting)
    {
      vtkDebugMacro(<<"Begin Pass " << pass << " with " << this->ParticleHistories.size() << " Particles");
      for (ParticleListIterator it=it_first; it!=it_last;)
      {
        // Keep the 'next' iterator handy because if a particle is terminated
        // or leaves the domain, the 'current' iterator will be deleted.
        it_next = it;
        it_next++;
        this->IntegrateParticle(it, from, this->CurrentTimeValue, integrator);
        if (this->GetAbortExecute())
        {
          break;
        }
        it = it_next;
      }
      // Particles might have been deleted during the first pass as they move
      // out of domain or age. Before adding any new particles that are sent
      // to us, we must know the starting point ready for the next pass
      bool list_valid = (this->ParticleHistories.size()>0);
      if (list_valid)
      {
        // point to one before the end
        it_first = --this->ParticleHistories.end();
      }
      // Send and receive any particles which exited/entered the domain
      continueExecuting = this->UpdateParticleListFromOtherProcesses();
      it_last = this->ParticleHistories.end();
      if (list_valid)
      {
        // increment to point to first new entry
        it_first++;
      }
      else
      {
        it_first = this->ParticleHistories.begin();
      }
      pass++;
    }//end of pass
  }

  bool injectionFlag(false);
  if (this->CurrentTimeValue!=this->StartTime && this->ForceReinjectionEveryNSteps>0)
  {
    injectionFlag = (this->CurrentTimeStep - this->StartTimeStep)%this->ForceReinjectionEveryNSteps==0;
  }

  if(injectionFlag) //reinject again in the last step
  {
    this->ReinjectionCounter = this->CurrentTimeStep - this->StartTimeStep;

    ParticleListIterator lastParticle = this->ParticleHistories.end();
    if (!this->ParticleHistories.empty())
    {
      lastParticle--;
    }
    int seedPointId=0;
    this->LocalSeeds.clear();
    for (size_t i=0; i<seedSources.size(); i++)
    {
      this->AssignSeedsToProcessors(this->CurrentTimeValue,seedSources[i], static_cast<int>(i), 0, this->LocalSeeds, seedPointId);
    }
    this->ParticleInjectionTime.Modified();
    this->UpdateParticleList(this->LocalSeeds);

    ParticleListIterator itr = lastParticle;
    if(itr!=this->ParticleHistories.end())
    {
      itr++;
    }
    else
    {
      itr = this->ParticleHistories.begin();
    }

    for(; itr!=this->ParticleHistories.end(); ++itr)
    {
      this->Interpolator->TestPoint(itr->CurrentPosition.x);
      double velocity[3];
      this->Interpolator->GetLastGoodVelocity(velocity);
      itr->speed = vtkMath::Norm(velocity);
      this->AddParticle(*itr,velocity);
    }
  }

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

  this->ParticlePointData = vtkSmartPointer<vtkPointData>::New();
  this->ParticlePointData->ShallowCopy(this->OutputPointData);

  // save some locator building, by re-using them as time progresses
  this->Interpolator->AdvanceOneTimeStep();

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->CurrentTimeValue);
  this->ExecuteTime.Modified();
  this->HasCache = true;
  PRINT("Output "<<output->GetNumberOfPoints()<<" particles, "<<this->ParticleHistories.size()<<" in cache");

  //Check post condition
  // To do:  verify here that the particles in ParticleHistories are consistent with CurrentTime

  // These hold reference to the inputs. Release them.
  this->DataReferenceT[0] = this->DataReferenceT[1] = 0;
  return output;
}

//---------------------------------------------------------------------------
int vtkParticleTracerBase::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  PRINT("RD start: "<<this->CurrentTimeStep<<" "<<this->CurrentTimeValue<<" "<<this->StartTimeStep<<" "<<this->TerminationTimeStep);
  if(this->StartTimeStep<0)
  {
    return 0;
  }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation    *inInfo = inputVector[0]->GetInformationObject(0);

  if(this->HasCache && this->CurrentTimeValue == this->TerminationTime)
  {
    vtkDataObject* out = outInfo->Get(vtkDataObject::DATA_OBJECT());
    out->ShallowCopy(this->Output);
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->FirstIteration = true;
    return 1; //nothing to be done
  }

  bool finished = this->CurrentTimeStep==this->TerminationTimeStep;
  this->ProcessInput(inputVector);

  if(this->FirstIteration)
  {
    vtkDataObject* input    = inInfo->Get(vtkDataObject::DATA_OBJECT());
    // first check if the point data is consistent on all blocks of a multiblock
    // and over all processes.
    if(this->IsPointDataValid(input) == false)
    {
      vtkErrorMacro("Point data arrays are not consistent across all data sets. Cannot do flow paths.");
      return 0;
    }
    this->CreateProtoPD(input);
  }

  vtkSmartPointer<vtkPolyData> particles;
  particles.TakeReference(this->Execute(inputVector));
  this->OutputParticles(particles);


  if(this->CurrentTimeStep<this->TerminationTimeStep)
  {
    this->CurrentTimeStep++;
  }
  else //we are at the last step
  {
    if(this->TerminationTime == this->InputTimeValues[this->CurrentTimeStep])
    {
      this->CurrentTimeStep++;
    }
  }

  if(!finished)
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    this->FirstIteration = false;
  }
  else
  {
    this->Finalize();
    this->Output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->TerminationTime);
    PRINT("RD: "<<this->Output->GetNumberOfLines()<<" lines");
    vtkDataObject* out = outInfo->Get(vtkDataObject::DATA_OBJECT());
    out->ShallowCopy(this->Output);
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->FirstIteration = true;
  }

  PRINT("RD: "<<this->CurrentTimeStep<<" "<<this->StartTimeStep<<" "<<this->TerminationTimeStep);
  return 1;
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::IntegrateParticle(
  ParticleListIterator &it, double currenttime, double targettime,
  vtkInitialValueProblemSolver* integrator)
{
  double epsilon = (targettime-currenttime)/100.0;
  double velocity[3], point1[4], point2[4] = {0.0, 0.0, 0.0, 0.0};
  double minStep=0, maxStep=0;
  double stepWanted, stepTaken=0.0;
  int substeps = 0;

  ParticleInformation &info = (*it);
  ParticleInformation previous = (*it);
  bool particle_good = true;

  info.ErrorCode = 0;

  // Get the Initial point {x,y,z,t}
  memcpy(point1, &info.CurrentPosition, sizeof(Position));

  if(currenttime==targettime)
  {
    Assert(point1[3]==currenttime);
  }
  else
  {
    Assert (point1[3]>=(currenttime-epsilon) && point1[3]<=(targettime+epsilon));

    //
    // begin interpolation between available time values, if the particle has
    // a cached cell ID and dataset - try to use it,
    //
    if(this->AllFixedGeometry)
    {
      this->Interpolator->SetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
    }
    else
    {
      this->Interpolator->ClearCache();
    }

    double delT = (targettime-currenttime) * this->IntegrationStep;
    epsilon = delT*1E-3;

    while (point1[3] < (targettime-epsilon))
    {
      //
      // Here beginneth the real work
      //
      double error = 0;

      // If, with the next step, propagation will be larger than
      // max, reduce it so that it is (approximately) equal to max.
      stepWanted = delT;
      if ( (point1[3] + stepWanted) > targettime )
      {
        stepWanted = targettime - point1[3];
        maxStep = stepWanted;
      }

      // Calculate the next step using the integrator provided.
      // If the next point is out of bounds, send it to another process
      if (integrator->ComputeNextStep(
            point1, point2, point1[3], stepWanted,
            stepTaken, minStep, maxStep,
            this->MaximumError, error) != 0)
      {
        // if the particle is sent, remove it from the list
        info.ErrorCode = 1;
        if (!this->RetryWithPush(info, point1, delT, substeps))
        {
          if(previous.PointId <0 && previous.TailPointId < 0)
          {
            vtkErrorMacro("the particle should have been added");
          }
          else
          {
            this->SendParticleToAnotherProcess(info,previous, this->ParticlePointData);
          }
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
        info.SimulationTime += stepTaken;

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

    if (particle_good)
    {
      // The integration succeeded, but check the computed final position
      // is actually inside the domain (the intermediate steps taken inside
      // the integrator were ok, but the final step may just pass out)
      // if it moves out, we can't interpolate scalars, so we must send it away
      info.LocationState = this->Interpolator->TestPoint(info.CurrentPosition.x);
      if (info.LocationState==ID_OUTSIDE_ALL)
      {
        info.ErrorCode = 2;
        // if the particle is sent, remove it from the list
        if (this->SendParticleToAnotherProcess(info,previous,this->OutputPointData))
        {
          this->ParticleHistories.erase(it);
          particle_good = false;
        }
      }
    }

    // Has this particle stagnated
    //
    if (particle_good)
    {
      this->Interpolator->GetLastGoodVelocity(velocity);
      info.speed = vtkMath::Norm(velocity);
      if (it->speed <= this->TerminalSpeed)
      {
        this->ParticleHistories.erase(it);
        particle_good = false;
      }
    }
  }

  //
  // We got this far without error :
  // Insert the point into the output
  // Create any new scalars and interpolate existing ones
  // Cache cell ids and datasets
  //
  if (particle_good)
  {
    //
    // store the last Cell Ids and dataset indices for next time particle is updated
    //
    this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
    //
    info.TimeStepAge += 1;
    //
    // Now generate the output geometry and scalars
    //
    this->AddParticle(info,velocity);
  }
  else
  {
    this->Interpolator->ClearCache();
  }

#ifdef DEBUGPARTICLETRACE
  double eps = (this->GetCacheDataTime(1)-this->GetCacheDataTime(0))/100;
  Assert (point1[3]>=(this->GetCacheDataTime(0)-eps) && point1[3]<=(this->GetCacheDataTime(1)+eps));
#endif
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ParticleWriter: " << this->ParticleWriter << endl;
  os << indent << "ParticleFileName: " <<
    (this->ParticleFileName ? this->ParticleFileName : "None") << endl;
  os << indent << "ForceReinjectionEveryNSteps: "
     << this->ForceReinjectionEveryNSteps << endl;
  os << indent << "EnableParticleWriting: " << this->EnableParticleWriting << endl;
  os << indent << "IgnorePipelineTime: " << this->IgnorePipelineTime << endl;
  os << indent << "StaticMesh: " << this->StaticMesh << endl;
  os << indent << "TerminationTime: " << this->TerminationTime << endl;
  os << indent << "StaticSeeds: " << this->StaticSeeds << endl;
}

//---------------------------------------------------------------------------
bool vtkParticleTracerBase::ComputeDomainExitLocation(
  double pos[4], double p2[4], double intersection[4],
  vtkGenericCell *cell)
{
  double t, pcoords[3];
  int subId;
  if (cell->IntersectWithLine(pos, p2, 1E-3, t, intersection, pcoords, subId)==0)
  {
    vtkDebugMacro(<< "No cell/domain exit was found");
    return 0;
  }
  else
  {
    // We found an intersection on the edge of the cell.
    // Shift it by a small amount to ensure that it crosses over the edge
    // into the adjoining cell.
    for (int i=0; i<3; i++)
    {
      intersection[i] = pos[i] + (t+0.01)*(p2[i]-pos[i]);
    }
    // intersection stored, compute T for intersection
    intersection[3] = pos[3] + (t+0.01)*(p2[3]-pos[3]);
    return 1;
  }
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::SetIntegratorType(int type)
{
  vtkInitialValueProblemSolver* ivp=0;
  switch (type)
  {
    case RUNGE_KUTTA2:
      ivp = vtkRungeKutta2::New();
      break;
    case RUNGE_KUTTA4:
      ivp = vtkRungeKutta4::New();
      break;
    case RUNGE_KUTTA45:
      ivp = vtkRungeKutta45::New();
      break;
    default:
      vtkWarningMacro("Unrecognized integrator type. Keeping old one.");
      break;
  }
  if (ivp)
  {
    this->SetIntegrator(ivp);
    ivp->Delete();
  }
}

//---------------------------------------------------------------------------
int vtkParticleTracerBase::GetIntegratorType()
{
  if (!this->Integrator)
  {
    return NONE;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta2"))
  {
    return RUNGE_KUTTA2;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta4"))
  {
    return RUNGE_KUTTA4;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta45"))
  {
    return RUNGE_KUTTA45;
  }
  return UNKNOWN;
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::CalculateVorticity(
  vtkGenericCell* cell, double pcoords[3],
  vtkDoubleArray* cellVectors, double vorticity[3] )
{
  double* cellVel;
  double derivs[9];

  cellVel = cellVectors->GetPointer(0);
  cell->Derivatives(0, pcoords, cellVel, 3, derivs);
  vorticity[0] = derivs[7] - derivs[5];
  vorticity[1] = derivs[2] - derivs[6];
  vorticity[2] = derivs[3] - derivs[1];
}

//---------------------------------------------------------------------------
double vtkParticleTracerBase::GetCacheDataTime(int i)
{
  return this->CachedData[i]->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
}

//---------------------------------------------------------------------------
double vtkParticleTracerBase::GetCacheDataTime()
{
  double currentTime = CachedData[1]? this->GetCacheDataTime(1):
    ( CachedData[0]? this->GetCacheDataTime(0) :  -DBL_MAX);
  return currentTime;

}

//---------------------------------------------------------------------------
unsigned int vtkParticleTracerBase::NumberOfParticles()
{
  return static_cast<unsigned int>(this->ParticleHistories.size());
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::ResetCache()
{
  if(this->DisableResetCache==0)
  {
    PRINT("Reset cache");
    this->LocalSeeds.clear();
    this->ParticleHistories.clear();
    this->ReinjectionCounter = 0;
    this->UniqueIdCounter    = 0; ///check

    this->CachedData[0] = NULL;
    this->CachedData[1] = NULL;

    this->Output = NULL;
    this->HasCache = false;
  }
}

//---------------------------------------------------------------------------
bool vtkParticleTracerBase::SetTerminationTimeNoModify(double t)
{
  if(t==this->TerminationTime)
  {
    return false;
  }

  if (t < this->TerminationTime)
  {
    this->ResetCache();
  }

  if( t < this->StartTime)
  {
    vtkWarningMacro("Can't go backward");
    t = this->StartTime;
  }

  this->TerminationTime = t;

  return true;
}

//---------------------------------------------------------------------------
vtkTemporalInterpolatedVelocityField* vtkParticleTracerBase::GetInterpolator()
{
  return this->Interpolator;
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::SetTerminationTime(double t)
{
  if (this->SetTerminationTimeNoModify(t))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::CreateProtoPD(vtkDataObject* input)
{
  this->ProtoPD = NULL;
  vtkDataSet* inputData(NULL);

  if(vtkCompositeDataSet::SafeDownCast(input))
  {
    vtkSmartPointer<vtkCompositeDataIterator> inputIter;
    inputIter.TakeReference(vtkCompositeDataSet::SafeDownCast(input)->NewIterator());
    inputIter->GoToFirstItem();
    inputData = vtkDataSet::SafeDownCast(inputIter->GetCurrentDataObject());
  }
  else
  {
    inputData = vtkDataSet::SafeDownCast(input);
  }
  if(!inputData)
  {
    return;
  }

  this->ProtoPD  = vtkSmartPointer<vtkPointData>::New();
  this->ProtoPD->InterpolateAllocate(inputData->GetPointData());
}

//---------------------------------------------------------------------------
bool vtkParticleTracerBase::RetryWithPush(
  ParticleInformation &info,  double* point1,double delT, int substeps)
{
  double velocity[3];
  this->Interpolator->ClearCache();

  info.LocationState = this->Interpolator->TestPoint(point1);

  if (info.LocationState==ID_OUTSIDE_ALL)
  {
    // something is wrong, the particle has left the building completely
    // we can't get the last good velocity as it won't be valid
    // send the particle 'as is' and hope it lands in another process
    if (substeps>0)
    {
      this->Interpolator->GetLastGoodVelocity(velocity);
    }
    else
    {
      velocity[0] = velocity[1] = velocity[2] = 0.0;
    }
    info.ErrorCode = 3;
  }
  else if (info.LocationState==ID_OUTSIDE_T0)
  {
    // the particle left the volume but can be tested at T2, so use the velocity at T2
    this->Interpolator->GetLastGoodVelocity(velocity);
    info.ErrorCode = 4;
  }
  else if (info.LocationState==ID_OUTSIDE_T1)
  {
    // the particle left the volume but can be tested at T1, so use the velocity at T1
    this->Interpolator->GetLastGoodVelocity(velocity);
    info.ErrorCode = 5;
  }
  else
  {
    // The test returned INSIDE_ALL, so test failed near start of integration,
    this->Interpolator->GetLastGoodVelocity(velocity);
  }

  // try adding a one increment push to the particle to get over a rotating/moving boundary
  for (int v=0; v<3; v++)
  {
    info.CurrentPosition.x[v] += velocity[v]*delT;
  }

  info.CurrentPosition.x[3] += delT;
  info.LocationState = this->Interpolator->TestPoint(info.CurrentPosition.x);
  info.age += delT;
  info.SimulationTime += delT; // = this->GetCurrentTimeValue();

  if (info.LocationState!=ID_OUTSIDE_ALL)
  {
    // a push helped the particle get back into a dataset,
    info.ErrorCode = 6;
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::AddParticle(
  vtkParticleTracerBaseNamespace::ParticleInformation &info, double* velocity)
{
  const double    *coord = info.CurrentPosition.x;
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
  this->AppendToExtraPointDataArrays(info);
  info.PointId = tempId;
  info.TailPointId = -1;

  //
  // Interpolate all existing point attributes
  // In principle we always integrate the particle until it reaches Time2
  // - so we don't need to do any interpolation of the scalars
  // between T0 and T1, just fetch the values
  // of the spatially interpolated scalars from T1.
  //
  if (info.LocationState==ID_OUTSIDE_T1)
  {
    this->Interpolator->InterpolatePoint(0, this->OutputPointData, tempId);
  }
  else
  {
    this->Interpolator->InterpolatePoint(1, this->OutputPointData, tempId);
  }
  //
  // Compute vorticity
  //
  if (this->ComputeVorticity)
  {
    vtkGenericCell *cell(NULL);
    double pcoords[3], vorticity[3], weights[256];
    double rotation, omega;
    // have to use T0 if particle is out at T1, otherwise use T1
    if (info.LocationState==ID_OUTSIDE_T1)
    {
      this->Interpolator->GetVorticityData(
        0, pcoords, weights, cell, this->CellVectors);
    }
    else
    {
      this->Interpolator->GetVorticityData(
        1, pcoords, weights, cell, this->CellVectors);
    }

    this->CalculateVorticity(cell, pcoords, CellVectors, vorticity);
    this->ParticleVorticity->InsertNextTuple(vorticity);
    // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
    if (info.speed != 0.0)
    {
      omega = vtkMath::Dot(vorticity, velocity);
      omega /= info.speed;
      omega *= this->RotationScale;
    }
    else
    {
      omega = 0.0;
    }
    vtkIdType index = this->ParticleAngularVel->InsertNextValue(omega);
    if (index>0)
    {
      rotation     = info.rotation + (info.angularVel + omega)/2 * (info.CurrentPosition.x[3] - info.time);
    }
    else
    {
      rotation     = 0.0;
    }
    this->ParticleRotation->InsertNextValue(rotation);
    info.rotation   = rotation;
    info.angularVel = omega;
    info.time       = info.CurrentPosition.x[3];
  }

}

//---------------------------------------------------------------------------
bool vtkParticleTracerBase::IsPointDataValid(vtkDataObject* input)
{
  if(vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input))
  {
    std::vector<std::string> arrayNames;
    return this->IsPointDataValid(cdInput, arrayNames);
  }
  // a single data set on a single process will always have consistent point data
  return true;
}

//---------------------------------------------------------------------------
bool vtkParticleTracerBase::IsPointDataValid(vtkCompositeDataSet* input,
                                             std::vector<std::string>& arrayNames)
{
  arrayNames.clear();
  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->SkipEmptyNodesOn();
  iter->GoToFirstItem();
  this->GetPointDataArrayNames(vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()),
                               arrayNames);
  for(iter->GoToNextItem();!iter->IsDoneWithTraversal();iter->GoToNextItem())
  {
    std::vector<std::string> tempNames;
    this->GetPointDataArrayNames(vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()),
                                 tempNames);
    if(std::equal(tempNames.begin(), tempNames.end(), arrayNames.begin()) == false)
    {
      iter->Delete();
      return false;
    }
  }
  iter->Delete();
  return true;
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::GetPointDataArrayNames(
  vtkDataSet* input, std::vector<std::string>& names)
{
  if(input == NULL)
  {
    names.clear();
    return;
  }
  names.resize(input->GetPointData()->GetNumberOfArrays());
  for(vtkIdType i=0;i<input->GetPointData()->GetNumberOfArrays();i++)
  {
    names[i] = input->GetPointData()->GetArrayName(i);
  }
}

//---------------------------------------------------------------------------
vtkFloatArray*  vtkParticleTracerBase::GetParticleAge(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkFloatArray>(pd->GetArray("ParticleAge"));
}

//---------------------------------------------------------------------------
vtkIntArray* vtkParticleTracerBase::GetParticleIds(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkIntArray>(pd->GetArray("ParticleId"));
}

//---------------------------------------------------------------------------
vtkCharArray* vtkParticleTracerBase::GetParticleSourceIds(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkCharArray>(pd->GetArray("ParticleSourceId"));
}

//---------------------------------------------------------------------------
vtkIntArray* vtkParticleTracerBase::GetInjectedPointIds(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkIntArray>(pd->GetArray("InjectedPointId"));
}

//---------------------------------------------------------------------------
vtkIntArray* vtkParticleTracerBase::GetInjectedStepIds(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkIntArray>(pd->GetArray("InjectionStepId"));
}

//---------------------------------------------------------------------------
vtkIntArray* vtkParticleTracerBase::GetErrorCodeArr(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkIntArray>(pd->GetArray("ErrorCode"));
}

//---------------------------------------------------------------------------
vtkFloatArray*  vtkParticleTracerBase::GetParticleVorticity(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkFloatArray>(pd->GetArray("Vorticity"));
}

//---------------------------------------------------------------------------
vtkFloatArray*  vtkParticleTracerBase::GetParticleRotation(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkFloatArray>(pd->GetArray("Rotation"));
}

//---------------------------------------------------------------------------
vtkFloatArray*  vtkParticleTracerBase::GetParticleAngularVel(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkFloatArray>(pd->GetArray("AngularVelocity"));
}

//---------------------------------------------------------------------------
void vtkParticleTracerBase::PrintParticleHistories()
{
  cout<<"Particle id, ages: "<<endl;
  for(ParticleListIterator itr = this->ParticleHistories.begin();
      itr!=this->ParticleHistories.end();itr++)
  {
    ParticleInformation& info(*itr);
    cout<<info.InjectedPointId<<" "<<info.age<<" "<<endl;
  }
  cout<<endl;
}
