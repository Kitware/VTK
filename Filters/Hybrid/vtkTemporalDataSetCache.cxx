/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalDataSetCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalDataSetCache.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkFeatures.h" // for VTK_USE_MEMKIND
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <vector>

// A helper class to to turn on memkind, if enabled, while ensuring it always is restored
class vtkTDSCMemkindRAII
{
#ifdef VTK_USE_MEMKIND
  bool OriginalValue = false;
#endif

public:
  vtkTDSCMemkindRAII(vtkTemporalDataSetCache* owner)
  {
#ifdef VTK_USE_MEMKIND
    this->OriginalValue = vtkObjectBase::GetUsingMemkind();
    if (owner->GetCacheInMemkind())
    {
      vtkObjectBase::SetUsingMemkind(true);
    }
#else
    (void)owner;
#endif
  }
#ifdef VTK_USE_MEMKIND
  ~vtkTDSCMemkindRAII() { vtkObjectBase::SetUsingMemkind(this->OriginalValue); }
#else
  ~vtkTDSCMemkindRAII() = default;
#endif
  vtkTDSCMemkindRAII(vtkTDSCMemkindRAII const&) = default;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalDataSetCache);

//------------------------------------------------------------------------------
vtkTemporalDataSetCache::vtkTemporalDataSetCache()
{
  this->CacheSize = 10;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->CacheInMemkind = false;
  this->IsASource = false;
  this->Ejected = nullptr;
}

//------------------------------------------------------------------------------
vtkTemporalDataSetCache::~vtkTemporalDataSetCache()
{
  CacheType::iterator pos = this->Cache.begin();
  for (; pos != this->Cache.end();)
  {
    pos->second.second->UnRegister(this);
    this->Cache.erase(pos++);
  }
  this->SetEjected(nullptr);
}

//------------------------------------------------------------------------------
vtkTypeBool vtkTemporalDataSetCache::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // generate the data
  if (request->Has(vtkCompositeDataPipeline::REQUEST_DATA()))
  {
    int retVal = this->RequestData(request, inputVector, outputVector);
    return retVal;
  }

  // set update extent
  if (request->Has(vtkCompositeDataPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // when acting as a source, provide extents
  if (request->Has(vtkCompositeDataPipeline::REQUEST_INFORMATION()))
  {
    if (this->IsASource)
    {
      return this->RequestInformation(request, inputVector, outputVector);
    }
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkTemporalDataSetCache::FillInputPortInformation(int port, vtkInformation* info)
{
  // port 0 must be temporal data, but port 1 can be any dataset
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  return 1;
}

int vtkTemporalDataSetCache::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}
//------------------------------------------------------------------------------
void vtkTemporalDataSetCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CacheSize: " << this->CacheSize << endl;
}

//------------------------------------------------------------------------------
void vtkTemporalDataSetCache::SetCacheSize(int size)
{
  if (size < 1)
  {
    vtkErrorMacro("Attempt to set cache size to less than 1");
    return;
  }

  // if growing the cache, there is no need to do anything
  this->CacheSize = size;
  if (this->Cache.size() <= static_cast<unsigned long>(size))
  {
    return;
  }

  // skrinking, have to get rid of some old data, to be easy just chuck the
  // first entries
  int i = static_cast<int>(this->Cache.size()) - size;
  CacheType::iterator pos = this->Cache.begin();
  for (; i > 0; --i)
  {
    pos->second.second->UnRegister(this);
    this->Cache.erase(pos++);
  }
}

//------------------------------------------------------------------------------
int vtkTemporalDataSetCache::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* dobj = inInfo->Get(vtkDataObject::DATA_OBJECT());
  bool hasInTime = false;
  double inTime = 0.0;
  if (dobj)
  {
    if (dobj->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      inTime = dobj->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
      hasInTime = true;
    }
  }

  this->TimeStepValues.clear();
  size_t numTimeStepValues = this->Cache.size();
  if (numTimeStepValues <= 0)
  {
    return 1;
  }

  CacheType::iterator pos = this->Cache.begin();
  double min = pos->first;
  double max = pos->first;
  for (; pos != this->Cache.end();)
  {
    this->TimeStepValues.push_back(pos->first);
    if (pos->first < min)
    {
      min = pos->first;
    }
    if (pos->first > max)
    {
      max = pos->first;
    }
    if (hasInTime && pos->first == inTime)
    {
      hasInTime = false;
    }
    pos++;
  }
  if (hasInTime)
  {
    // cache doesn't contain our input, but it will when we are asked later
    // so announce it's time too
    if (inTime < min)
    {
      min = inTime;
    }
    if (inTime > max)
    {
      max = inTime;
    }
    this->TimeStepValues.push_back(inTime);
  }
  std::sort(this->TimeStepValues.begin(), this->TimeStepValues.end());
  vtkInformation* info = outputVector->GetInformationObject(0);

  // tell the caller that I can provide time varying data and
  // tell it what range of times I can deal with
  double tRange[2];
  tRange[0] = this->TimeStepValues[0];
  tRange[1] = this->TimeStepValues[this->TimeStepValues.size() - 1];
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);

  // tell the caller what the specific values are
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->TimeStepValues[0],
    static_cast<int>(this->TimeStepValues.size()));

  // if we are caching structured data, we need to provide topological extents
  pos = this->Cache.begin();
  if (pos != this->Cache.end())
  {
    vtkImageData* id = vtkImageData::SafeDownCast(pos->second.second);
    if (id)
    {
      // as an ID
      double* origin = id->GetOrigin();
      int* extent = id->GetExtent();
      double* spacing = id->GetSpacing();
      info->Set(vtkDataObject::ORIGIN(), origin, 3);
      info->Set(vtkDataObject::SPACING(), spacing, 3);
      info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalDataSetCache::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->GetNumberOfInputPorts() == 0 || this->GetNumberOfOutputPorts() == 0)
  {
    return 1;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkTDSCMemkindRAII(this);
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkTemporalDataSetCache ::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // First look through the cached data to see if it is still valid.
  CacheType::iterator pos;
  vtkDemandDrivenPipeline* ddp = vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (!ddp)
  {
    return 1;
  }

  if (!this->IsASource)
  {
    vtkMTimeType pmt = ddp->GetPipelineMTime();
    for (pos = this->Cache.begin(); pos != this->Cache.end();)
    {
      if (pos->second.first < pmt)
      {
        pos->second.second->Delete();
        this->Cache.erase(pos++);
      }
      else
      {
        ++pos;
      }
    }
  }

  // are there any times that we are missing from the request? e.g. times
  // that are not cached?
  std::vector<double> reqTimeSteps;
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // no time steps were passed in the update request, so just request
    // something to keep the pipeline happy.
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      int NumberOfInputTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      //
      // Get list of input time step values
      std::vector<double> InputTimeValues;
      InputTimeValues.resize(NumberOfInputTimeSteps);
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &InputTimeValues[0]);

      // this should be the same, just checking for debug purposes
      reqTimeSteps.push_back(InputTimeValues[0]);
    }
    else
      return 0;
  }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double upTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // do we have this time step?
    pos = this->Cache.find(upTime);
    if (pos == this->Cache.end())
    {
      reqTimeSteps.push_back(upTime);
    }

    // if we need any data
    if (!reqTimeSteps.empty())
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), reqTimeSteps[0]);
    }
    // otherwise leave the input with what it already has
    else
    {
      vtkDataObject* dobj = inInfo->Get(vtkDataObject::DATA_OBJECT());
      if (dobj)
      {
        double it = dobj->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
        if (dobj->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
        {
          inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), it);
        }
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkTemporalDataSetCache::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkMTimeType outputUpdateTime = outInfo->Get(vtkDataObject::DATA_OBJECT())->GetUpdateTime();

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  // get some time informationX
  double upTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double inTime = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());

  vtkSmartPointer<vtkDataObject> output;

  // a time should either be in the Cache or in the input
  CacheType::iterator pos = this->Cache.find(upTime);
  if (pos != this->Cache.end())
  {
    vtkTDSCMemkindRAII(this);
    vtkDataObject* cachedData = pos->second.second;
    output.TakeReference(cachedData->NewInstance());
    output->ShallowCopy(cachedData);
    // update the m time in the cache
    pos->second.first = outputUpdateTime;
  }
  // otherwise it better be in the input
  else
  {
    if (input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      if (inTime == upTime)
      {
        vtkTDSCMemkindRAII(this);
        output.TakeReference(input->NewInstance());
        output->ShallowCopy(input);
      }
      else
      {
        output = this->GetEjected()->NewInstance();
        output->ShallowCopy(this->GetEjected());
      }
    }
    else
    {
      // just shallow copy input to output
      output.TakeReference(input->NewInstance());
      output->ShallowCopy(input);
    }
  }
  // set the data times
  outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), upTime);

  // now we need to update the cache, based on the new data and the cache
  // size add the requested data to the cache first
  if (input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    // nothing to do if the input time is already in the cache
    CacheType::iterator pos1 = this->Cache.find(inTime);
    if (pos1 == this->Cache.end())
    {
      // if we have room in the Cache then just add the new data
      if (this->Cache.size() < static_cast<unsigned long>(this->CacheSize))
      {
        this->ReplaceCacheItem(input, inTime, outputUpdateTime);
      }
      // if there is no room in the cache, we need to get rid of something
      else
      {
        // get rid of the oldest data in the cache
        CacheType::iterator pos2 = this->Cache.begin();
        CacheType::iterator oldestpos = this->Cache.begin();
        for (; pos2 != this->Cache.end(); ++pos2)
        {
          if (pos2->second.first < oldestpos->second.first)
          {
            oldestpos = pos2;
          }
        }
        // was there old data?
        if (oldestpos->second.first < outputUpdateTime)
        {
          this->SetEjected(oldestpos->second.second);
          oldestpos->second.second->UnRegister(this);
          this->Cache.erase(oldestpos);

          this->ReplaceCacheItem(input, inTime, outputUpdateTime);
        }
        else
        {
          // if no old data and no room then we are done
        }
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkTemporalDataSetCache::ReplaceCacheItem(
  vtkDataObject* input, double inTime, vtkMTimeType outputUpdateTime)
{
  vtkTDSCMemkindRAII(this);
  vtkDataObject* cachedData = input->NewInstance();
  if (vtkDataObject::GetUsingMemkind() && !this->IsASource)
  {
    cachedData->ShallowCopy(input);
  }
  else
  {
    if (this->GetCacheInMemkind())
    {
      cachedData->DeepCopy(input);
    }
    else
    {
      cachedData->ShallowCopy(input);
    }
  }
  this->Cache[inTime] = std::pair<unsigned long, vtkDataObject*>(outputUpdateTime, cachedData);
}

//------------------------------------------------------------------------------
void vtkTemporalDataSetCache::SetEjected(vtkDataObject* victim)
{
  if (this->Ejected != victim)
  {
    vtkDataObject* tmp = this->Ejected;
    this->Ejected = victim;
    if (this->Ejected != nullptr)
    {
      this->Ejected->Register(this);
    }
    if (tmp != nullptr)
    {
      tmp->UnRegister(this);
    }
    // this->Modified(); //this is only thing we are changing from the macro
  }
}
