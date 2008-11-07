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
#include <vtksys/stl/vector>

// For vtkSleep
#include "vtkWindows.h"
#include <ctype.h>
#include <time.h>
#if _WIN32
#include "windows.h"
#endif

//-----------------------------------------------------------------------------
// Cross platform sleep
inline void vtkSleep(double duration)
{
  duration = duration; // avoid warnings
  // sleep according to OS preference
#ifdef _WIN32
  Sleep((int)(1000*duration));
#elif defined(__FreeBSD__) || defined(__linux__) || defined(sgi)
  struct timespec sleep_time, dummy;
  sleep_time.tv_sec = static_cast<int>(duration);
  sleep_time.tv_nsec = static_cast<int>(1000000000*(duration-sleep_time.tv_sec));
  nanosleep(&sleep_time,&dummy);
#endif
}

VTK_THREAD_RETURN_TYPE vtkGeoSourceThreadStart(void* arg)
{
  vtkGeoSource* self;
  self = static_cast<vtkGeoSource *>(static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);
  self->WorkerThread();
  return VTK_THREAD_RETURN_VALUE;
}

class vtkGeoSource::implementation {
public:
  vtksys_stl::map<vtkSmartPointer<vtkGeoTreeNode>, vtkSmartPointer<vtkCollection> > OutputMap;
  vtksys_stl::vector<int> ThreadIds;
};

vtkCxxRevisionMacro(vtkGeoSource, "1.2");
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
  this->Implementation = new implementation();
  this->Threader = vtkMultiThreader::New();
  int maxThreads = this->Threader->GetGlobalDefaultNumberOfThreads();
  maxThreads = 1;
  for(int i = 0; i < maxThreads; ++i)
    {
    this->Implementation->ThreadIds.push_back(
      this->Threader->SpawnThread(vtkGeoSourceThreadStart, this));
    }
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

void vtkGeoSource::ShutDown()
{
  if (this->Implementation->ThreadIds.size() > 0)
    {
    this->Lock->Lock();
    this->StopThread = true;
    this->Condition->Broadcast();
    this->Lock->Unlock();

    cout << "Signal Exit" << endl;

    vtkstd::vector<int>::iterator iter;
    for(iter = this->Implementation->ThreadIds.begin();
        iter != this->Implementation->ThreadIds.end();
        ++iter)
      {
      this->Threader->TerminateThread(*iter);
      }
    }
}

vtkCollection* vtkGeoSource::GetRequestedNodes(vtkGeoTreeNode* node)
{
  //vtkTimerLog * timer = vtkTimerLog::New();
  //timer->StartTimer();
  vtkCollection* c = 0;
  //this->Lock->Lock();
  this->OutputSetLock->Lock();
  if (this->Implementation->OutputMap.count(node) > 0)
    {
    c = this->Implementation->OutputMap[node];
    }
  //this->Lock->Unlock();
  this->OutputSetLock->Unlock();

  //timer->StopTimer();
  //cout << "GetRequestedNodes: " << timer->GetElapsedTime() << endl;
  //timer->Delete();
  return c;
}

void vtkGeoSource::RequestChildren(vtkGeoTreeNode* node)
{
  //vtkTimerLog * timer = vtkTimerLog::New();
  //timer->StartTimer();
  //this->Lock->Lock();
  this->InputSetLock->Lock();
  this->InputSet->AddItem(node);
  this->Condition->Broadcast();
  //this->Lock->Unlock();
  this->InputSetLock->Unlock();
  //timer->StopTimer();
  //cout << "RequestChildrenTime: " << timer->GetElapsedTime() << endl;
  //timer->Delete();
}

void vtkGeoSource::WorkerThread()
{
  while (true)
    {
    this->Lock->Lock();

    if (this->StopThread)
      {
      //cout << "Thread Exit" << endl;
      //cout.flush();
      this->Lock->Unlock();
      return;
      }

    this->Lock->Unlock();

    this->InputSetLock->Lock();

    // Try to find something to work on.
    if (this->InputSet->GetNumberOfItems() > 0)
      {

      //cout << "Found Work" << endl;
      //cout.flush();
      // Move from input set to processing set
      //this->ProcessingSetLock->Lock();
      vtkGeoTreeNode* node = vtkGeoTreeNode::SafeDownCast(this->InputSet->GetItemAsObject(0));
      this->InputSet->RemoveItem(0);
      //this->ProcessingSet->AddItem(node);
      //this->ProcessingSetLock->Unlock();
      //this->Condition->Broadcast();
      this->InputSetLock->Unlock();
      //this->Lock->Unlock();

      // if there is more than 1 item send a signal incase
      // we have other threads waiting.
      //if(this->InputSet->GetNumberOfItems() > 1)
      //  {
      //  this->Condition->Broadcast();
      //  }

      // Create appropriate child instances
      vtkGeoTreeNode* child[4];
      if (vtkGeoTerrainNode::SafeDownCast(node))
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
      //this->Lock->Lock();
      this->OutputSetLock->Lock();
      //this->ProcessingSetLock->Lock();
      //this->ProcessingSet->RemoveItem(node);
      this->Implementation->OutputMap[node] =
        vtkSmartPointer<vtkCollection>::New();
      if (success)
        {
        for (int i = 0; i < 4; ++i)
          {
          this->Implementation->OutputMap[node]->AddItem(child[i]);
          }
        }
      //this->ProcessingSetLock->Unlock();
      this->OutputSetLock->Unlock();
      //this->Lock->Unlock();

      // Clean up
      for (int i = 0; i < 4; ++i)
        {
        child[i]->Delete();
        }

      //cout << "Work Complete" << endl;
      //cout.flush();
      }
    else
      {
      //cout << "No Work" << endl;
      //cout.flush();
      //this->Lock->Unlock();

      this->InputSetLock->Unlock();

      this->Lock->Lock();

      // No Work, so lets wait till we're signaled again.
      this->Condition->Wait( this->Lock );

      this->Lock->Unlock();

      //cout << "Stop Waiting" << endl;
      //cout.flush();
      }
    }
}

