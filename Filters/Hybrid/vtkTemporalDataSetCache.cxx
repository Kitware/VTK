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

#include "vtkTemporalDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalDataSetCache);

//----------------------------------------------------------------------------
vtkTemporalDataSetCache::vtkTemporalDataSetCache()
{
  this->CacheSize = 10;
}

//----------------------------------------------------------------------------
vtkTemporalDataSetCache::~vtkTemporalDataSetCache()
{
  CacheType::iterator pos = this->Cache.begin();
  for (; pos != this->Cache.end();)
    {
    pos->second.second->UnRegister(this);
    this->Cache.erase(pos++);
    }
}
//----------------------------------------------------------------------------
int vtkTemporalDataSetCache::FillInputPortInformation(
  int port, 
  vtkInformation* info)
{
  // port 0 must be temporal data, but port 1 can be any dataset
  if (port==0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTemporalDataSet");
  }
  return 1;
}
//----------------------------------------------------------------------------
void vtkTemporalDataSetCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CacheSize: " << this->CacheSize << endl;
}
//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
int vtkTemporalDataSetCache
::RequestUpdateExtent (vtkInformation * vtkNotUsed(request),
                       vtkInformationVector **inputVector,
                       vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // First look through the cached data to see if it is still valid.
  CacheType::iterator pos;
  vtkDemandDrivenPipeline *ddp = 
    vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (!ddp)
    {
    return 1;
    }

  unsigned long pmt = ddp->GetPipelineMTime();
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

  // are there any times that we are missing from the request? e.g. times
  // that are not cached?
  std::vector<double> reqTimeSteps;
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
      // no time steps were passed in the update request, so just request
      // something to keep the pipeline happy.
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
      {
      int NumberOfInputTimeSteps = inInfo->Length( 
        vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
      //
      // Get list of input time step values
      std::vector<double> InputTimeValues;
      InputTimeValues.resize(NumberOfInputTimeSteps);
      inInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
        &InputTimeValues[0] );

      // this should be the same, just checking for debug purposes
      reqTimeSteps.push_back(InputTimeValues[0]);
      }
    else return 0;
    }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    double *upTimes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int numTimes = 
      outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int i;
    for (i = 0; i < numTimes; ++i)
      {
      // do we have this time step?
      pos = this->Cache.find(upTimes[i]);
      if (pos == this->Cache.end())
        {
        reqTimeSteps.push_back(upTimes[i]);
        }
      }

    // if we need any data
    if (reqTimeSteps.size())
      {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(),
                  &reqTimeSteps[0],static_cast<int>(reqTimeSteps.size()));
      }
    // otherwise leave the input with what it already has 
    else
      {
      vtkDataObject *dobj = inInfo->Get(vtkDataObject::DATA_OBJECT());
      if (dobj)
        {
        double *its = dobj->GetInformation()->Get
          (vtkDataObject::DATA_TIME_STEPS());
        int itsSize = dobj->GetInformation()->Length
          (vtkDataObject::DATA_TIME_STEPS());
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(),
                    its,itsSize);
        }
      }
    }

/*
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
    {
    // well really, the least they could do is pass something in
    int updateExtent[6];
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
      {
      inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);
      int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent, len);
      }
    else return 1;
    }
  else 
    {
    // just forward the request up the pipeline
    int updateExtent[6];
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);
    int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent, len);
    vtkDebugMacro(<<"vtkTemporalDataSetCache Extent : " 
      << updateExtent[0] << " " << updateExtent[1] << " " 
      << updateExtent[2] << " " << updateExtent[3] << " " 
      << updateExtent[4] << " " << updateExtent[5]);

    }
*/

  return 1;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkTemporalDataSetCache::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation      *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation     *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject       *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkTemporalDataSet *outData = vtkTemporalDataSet::SafeDownCast(output);
  
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkTemporalDataSet *temporal = vtkTemporalDataSet::SafeDownCast(input);

  // get some time information
  double *upTimes =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
  int numUpTimes = 
    outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

  int inLength = 
    input->GetInformation()->Length(vtkDataObject::DATA_TIME_STEPS());
  double *inTimes = 
    input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEPS());
  
  // fill in the request by using the cached data and input data
  outData->Initialize();  
  int i;
  for (i = 0; i < numUpTimes; ++i)
    {
    // a time should either be in the Cache or in the input
    CacheType::iterator pos = this->Cache.find(upTimes[i]);
    if (pos != this->Cache.end())
      {
      outData->SetTimeStep(i, pos->second.second);
      // update the m time in the cache
      pos->second.first = outData->GetUpdateTime();
      }
    // otherwise it better be in the input
    else
      {
      int j;
      int found = 0;
      for (j = 0; j < inLength; ++j)
        {
        if (inTimes[j] == upTimes[i])
          {
          if (temporal) 
            {
            outData->SetTimeStep(i, temporal->GetTimeStep(j));
            if (input->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()))
              {
              vtkDebugMacro(<<"Dataset has DATA_GEOMETRY_UNMODIFIED");
              }
            }
          else
            {
            vtkDebugMacro(<<"Cache : Should not be here 1");
            outData->SetTimeStep(i, input);
            }
          found = 1;
          break;
          }
        }
      if (!found)
        {
/*
        for (tt = 0; tt< inLength; ++tt)
          {
          temp << inTimes[tt] << " ";
        }
        vtkErrorMacro("Unable to find proper time step : requested " 
          << upTimes[i] << " : available " << temp.str().c_str());
//        return 1;
*/
        }
      }
    }
  // set the data times
  outData->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(),
                                 upTimes, numUpTimes);

  // now we need to update the cache, based on the new data and the cache
  // size add the requested data to the cache first
  int j;
  for (j = 0; j < inLength; ++j)
    {
    // is the input time not already in the cache?
    CacheType::iterator pos = this->Cache.find(inTimes[j]);
    if (pos == this->Cache.end())
      {
      // if we have room in the Cache then just add the new data
      if (this->Cache.size() < static_cast<unsigned long>(this->CacheSize))
        {
        if (temporal) 
          {
          this->Cache[inTimes[j]] = 
            std::pair<unsigned long, vtkDataObject *>
            (outData->GetUpdateTime(), temporal->GetTimeStep(j));
          vtkDataObject *dobj = temporal->GetTimeStep(j);
          if (!dobj)
            {
            vtkErrorMacro(<<"The dataset is invalid");
            return 0;
            }
          temporal->GetTimeStep(j)->Register(this);
          }
        else
          {
          vtkDebugMacro(<<"Cache : Should not be here 2");
          this->Cache[inTimes[j]] = 
            std::pair<unsigned long, vtkDataObject *>
            (outData->GetUpdateTime(), input);
          input->Register(this);
          }
        }
      // no room in the cache, we need to get rid of something
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
        //was there old data?
        if (oldestpos->second.first < outData->GetUpdateTime())
          {
          oldestpos->second.second->UnRegister(this);
          this->Cache.erase(oldestpos);
          --j;
          }
        // if no old data and no room then we are done
        else
          {
          break;
          }
        }
      }
    }
  return 1;
}
