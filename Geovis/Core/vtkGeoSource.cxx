/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkGeoSource.h"

#include "vtkCollection.h"
#include "vtkConditionVariable.h"
#include "vtkDataObject.h"
#include "vtkGeoImageNode.h"
#include "vtkGeoTerrainNode.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

#include <vtksys/stl/map>
#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

VTK_THREAD_RETURN_TYPE vtkGeoSourceThreadStart(void* arg)
{
  vtkGeoSource* self;
  self = static_cast<vtkGeoSource *>(static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);
  self->WorkerThread();
  return VTK_THREAD_RETURN_VALUE;
}

class vtkGeoSource::implementation {
public:
  vtksys_stl::map<vtksys_stl::pair<unsigned long, int>, vtkSmartPointer<vtkCollection> > OutputMap;
  vtksys_stl::vector<int> ThreadIds;
};

vtkGeoSource::vtkGeoSource()
{
  this->InputSet = vtkCollection::New();
  this->InputSetLock = vtkMutexLock::New();
  this->ProcessingSet = vtkCollection::New();
  this->ProcessingSetLock = vtkMutexLock::New();
  this->OutputSetLock = vtkMutexLock::New();
  this->Lock = vtkMutexLock::New();
  this->Condition = vtkConditionVariable::New();
  this->StopThread = false;
  this->Initialized = false;
  this->Implementation = new implementation();
  this->Threader = vtkMultiThreader::New();
}

vtkGeoSource::~vtkGeoSource()
{
  this->ShutDown();
  this->InputSet->Delete();
  this->ProcessingSet->Delete();
  this->Threader->Delete();
  delete this->Implementation;
  this->InputSetLock->Delete();
  this->ProcessingSetLock->Delete();
  this->OutputSetLock->Delete();
  this->Condition->Delete();
  this->Lock->Delete();
}

void vtkGeoSource::Initialize(int numThreads)
{
  if(this->Initialized)
    {
    return;
    }
  
  int maxThreads = this->Threader->GetGlobalDefaultNumberOfThreads();
  maxThreads = numThreads < maxThreads ? numThreads : maxThreads;

  for(int i = 0; i < maxThreads; ++i)
    {
    this->Implementation->ThreadIds.push_back(
      this->Threader->SpawnThread(vtkGeoSourceThreadStart, this));
    }
  this->Initialized = true;
}

void vtkGeoSource::ShutDown()
{
  if (this->Implementation->ThreadIds.size() > 0)
    {
    this->Lock->Lock();
    this->StopThread = true;
    this->Condition->Broadcast();
    this->Lock->Unlock();

    std::vector<int>::iterator iter;
    for(iter = this->Implementation->ThreadIds.begin();
        iter != this->Implementation->ThreadIds.end();
        ++iter)
      {
      this->Threader->TerminateThread(*iter);
      }
    this->Implementation->ThreadIds.clear();
    this->Implementation->OutputMap.clear();
    }
  this->Initialized = false;
}

vtkCollection* vtkGeoSource::GetRequestedNodes(vtkGeoTreeNode* node)
{
  vtkCollection* c = 0;
  this->OutputSetLock->Lock();
  vtksys_stl::pair<unsigned long, int> p(node->GetId(), node->GetLevel());
  if (this->Implementation->OutputMap.count(p) > 0)
    {
    c = this->Implementation->OutputMap[p];
    if (c)
      {
      c->Register(0);
      this->Implementation->OutputMap[p] = 0;
      }
    }
  this->OutputSetLock->Unlock();

  return c;
}

void vtkGeoSource::RequestChildren(vtkGeoTreeNode* node)
{
  if(!this->Initialized)
    {
    vtkErrorMacro("Call Initialize() first in order to spawn worker threads.");
    return;
    }

  this->InputSetLock->Lock();
  this->InputSet->AddItem(node);
  // Reference Count is 2 at this point. Decrease so that
  // we can delete this copy of the node from the worker
  // thread.
  node->UnRegister(this);
  this->Condition->Broadcast();
  this->InputSetLock->Unlock();
}

void vtkGeoSource::WorkerThread()
{
  bool isTerrainNode = false;
  while (true)
    {
    this->Lock->Lock();

    if (this->StopThread)
      {

      this->Lock->Unlock();
      return;
      }

    this->Lock->Unlock();

    this->InputSetLock->Lock();

    // Try to find something to work on.
    if (this->InputSet->GetNumberOfItems() > 0)
      {
      // Move from input set to processing set
      vtkGeoTreeNode* node = vtkGeoTreeNode::SafeDownCast(this->InputSet->GetItemAsObject(0));
      node->Register(this);
      this->InputSet->RemoveItem(0);
      this->InputSetLock->Unlock();

      // Create appropriate child instances
      vtkGeoTreeNode* child[4];
      isTerrainNode = vtkGeoTerrainNode::SafeDownCast(node) != NULL ? true : false;
      if (isTerrainNode)
        {
        for (int i = 0; i < 4; ++i)
          {
          child[i] = vtkGeoTerrainNode::New();
          }
        }
      else
        {
        for (int i = 0; i < 4; ++i)
          {
          child[i] = vtkGeoImageNode::New();
          }
        }

      // Fetch the children
      bool success = true;
      for (int i = 0; i < 4; ++i)
        {
        if (!this->FetchChild(node, i, child[i]))
          {
          success = false;
          break;
          }
        }

      // Move from processing set to output
      this->OutputSetLock->Lock();
      vtksys_stl::pair<unsigned long, int> p(node->GetId(), node->GetLevel());
      this->Implementation->OutputMap[p] =
        vtkSmartPointer<vtkCollection>::New();
      if (success)
        {
        for (int i = 0; i < 4; ++i)
          {
          this->Implementation->OutputMap[p]->AddItem(child[i]);
          }
        }
      this->OutputSetLock->Unlock();


      node->Delete();
      node = NULL;

      for (int i = 0; i < 4; ++i)
        {
        child[i]->Delete();
        }
      }
    else
      {

      this->InputSetLock->Unlock();

      this->Lock->Lock();

      // No Work, so lets wait till we're signaled again.
      this->Condition->Wait( this->Lock );

      this->Lock->Unlock();
      }
    }
}

