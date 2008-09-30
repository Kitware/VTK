/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrain.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=============================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkObjectFactory.h"
#include "vtkGeoCamera.h"
#include "vtkGeoTerrainGlobeSource.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoTerrainSource.h"
#include "vtkGeoTerrain.h"
#include "vtkPolyDataWriter.h"
#include "vtkPolyData.h"
#include "vtkMutexLock.h"

// For vtkSleep
#include "vtkWindows.h"
#include <ctype.h>
#include <time.h>

vtkCxxRevisionMacro(vtkGeoTerrain, "1.5");
vtkStandardNewMacro(vtkGeoTerrain);
#if _WIN32
#include "windows.h"
#endif
#include "vtkTimerLog.h"

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

//-----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkGeoTerrainThreadStart( void *arg )
{
//   int threadId = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
//   int threadCount = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;
  
  vtkGeoTerrain* self;
  self = static_cast<vtkGeoTerrain *>
    (static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);

 self->ThreadStart();
  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
vtkGeoTerrain::vtkGeoTerrain() 
{
  // It is OK to have a default, 
  // but the use should be able to change the .
  vtkSmartPointer<vtkGeoTerrainSource> source;
  source = vtkSmartPointer<vtkGeoTerrainGlobeSource>::New();
  this->SetTerrainSource(source);

  this->Threader = vtkSmartPointer<vtkMultiThreader>::New();
  this->WaitForRequestMutex1 = vtkSmartPointer<vtkMutexLock>::New();
  this->WaitForRequestMutex1->Lock();
  
  this->TreeMutex = vtkSmartPointer<vtkMutexLock>::New();
  this->TreeLock = 0;

  // Spawn a thread to update the tree.
  this->ThreadId = this->Threader->SpawnThread( vtkGeoTerrainThreadStart, this);
}

//-----------------------------------------------------------------------------
vtkGeoTerrain::~vtkGeoTerrain() 
{
  this->RequestTerminate();
  this->Threader->TerminateThread(this->ThreadId);
  this->ThreadId = -1;
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::SetOrigin(double x, double y, double z)
{
  this->GetTerrainSource()->SetOrigin(x, y, z);

  // We need to get rid of terrain pathces generated so far.
  this->Initialize(); // from cache
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::StartEdit()
{
  this->NewNodes.clear();
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::AddNode(vtkGeoTerrainNode* node)
{
  this->NewNodes.push_back(node);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::FinishEdit()
{
  this->Nodes = this->NewNodes;
  this->NewNodes.clear();
}

//-----------------------------------------------------------------------------
int vtkGeoTerrain::GetNumberOfNodes()
{
  return static_cast<int>(this->Nodes.size());
}

//-----------------------------------------------------------------------------
vtkGeoTerrainNode* vtkGeoTerrain::GetNode(int idx)
{
  return this->Nodes[idx];
}

//-----------------------------------------------------------------------------
bool vtkGeoTerrain::Update(vtkGeoCamera* camera)
{
  bool returnValue = this->Update(this, camera);
  // I am putting the request second so that it will not block the Update.
  this->Request(camera);
  
  return returnValue;
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::SetTerrainSource(vtkGeoTerrainSource* source)
{
  if ( !source )
    {
    return;
    }
  
  this->TerrainSource = source;
  this->Initialize();
}


//-----------------------------------------------------------------------------
// This could be done in a separate thread.
int vtkGeoTerrain::RefineNode(vtkGeoTerrainNode* node)
{
  // Create the four children.
  if (node->GetChild(0))
    { // This node is already refined.
    return VTK_OK;
    }

  if (node->CreateChildren() == VTK_ERROR)
    {
    return VTK_ERROR;
    }

  this->TerrainSource->GenerateTerrainForNode(node->GetChild(0));
  this->TerrainSource->GenerateTerrainForNode(node->GetChild(1));
  this->TerrainSource->GenerateTerrainForNode(node->GetChild(2));
  this->TerrainSource->GenerateTerrainForNode(node->GetChild(3));

  return VTK_OK;
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::InitializeTerrain(vtkGeoTerrain* terrain)
{
  terrain->StartEdit();
  terrain->AddNode(this->WesternHemisphere);
  terrain->AddNode(this->EasternHemisphere);
  terrain->FinishEdit();
}

void vtkGeoTerrain::Initialize()
{
  if (this->TerrainSource == 0)
    {
    vtkErrorMacro("Missing terrain source.");
    return;
    }

  this->WesternHemisphere = vtkSmartPointer<vtkGeoTerrainNode>::New();
  this->EasternHemisphere = vtkSmartPointer<vtkGeoTerrainNode>::New();
  this->WesternHemisphere->SetId(0);
  this->EasternHemisphere->SetId(1);

  // Id is a bitmap representation of the branch trace.
  this->WesternHemisphere->SetLongitudeRange(-180.0,0.0);
  this->WesternHemisphere->SetLatitudeRange(-90.0,90.0);
  this->TerrainSource->GenerateTerrainForNode(this->WesternHemisphere);
  this->EasternHemisphere->SetLongitudeRange(0.0,180.0);
  this->EasternHemisphere->SetLatitudeRange(-90.0,90.0);
  this->TerrainSource->GenerateTerrainForNode(this->EasternHemisphere);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::Request(vtkGeoTerrainNode* node, vtkGeoCamera* cam)
{
  int evaluation = this->EvaluateNode(node, cam);
  if (evaluation > 0)
    { // refine the node.  Add the 4 children.
    // For simplicity, lets just refine one level per update.
    if ( node->GetChild(0) == 0)
      { // Temporarily refine here.  Later we will have asynchronous refinement.
      this->RefineNode(node);
      }
    else
      {
      this->Request(node->GetChild(0), cam);
      this->Request(node->GetChild(1), cam);
      this->Request(node->GetChild(2), cam);
      this->Request(node->GetChild(3), cam);
      }
    } 
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::ThreadStart()
{
  // Use mutex to avoid a busy loop.  Select on a socket will be better.
  while (1)
    {
    // Stupid gating a thread via mutex guntlet.
    this->WaitForRequestMutex1->Lock();
    this->WaitForRequestMutex1->Unlock();

    if (this->Camera == 0)
      { // terminate
      return;
      }

    // Variable to manage whoe has access to reading and changing tree.
    // This thread never keeps this lock for long.
    // We do not want to block the client.
    this->GetWriteLock();
    this->Request(this->WesternHemisphere, this->Camera);
    this->Request(this->EasternHemisphere, this->Camera);
    this->ReleaseWriteLock();
    }
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::Request(vtkGeoCamera* camera)
{
  if (camera == 0)
    {
    return;
    }

  double t = vtkTimerLog::GetUniversalTime();

  this->TreeMutex->Lock();
  // If a request is already in progress.  I do not want to block.
  if (this->TreeLock == 0)
    { // The request thread is idle.
    this->Camera = camera;
    this->WaitForRequestMutex1->Unlock();
    vtkSleep(0.01);
    this->WaitForRequestMutex1->Lock();
    }
  this->TreeMutex->Unlock();
  
  t = vtkTimerLog::GetUniversalTime() - t;
  if (t > 0.1)
    {
    cerr << "request took : " << t << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::RequestTerminate()
{
  this->Camera = 0;
  this->WaitForRequestMutex1->Unlock();
  vtkSleep(0.01);
  this->WaitForRequestMutex1->Lock();
}


//-----------------------------------------------------------------------------
bool vtkGeoTerrain::GetReadLock()
{
  this->TreeMutex->Lock();
  if (this->TreeLock)
    { // The background thread is writeing to the tree..
    this->TreeMutex->Unlock();
    return false;
    }
    
  // Keep the mutex lock until we are finished.
  return true;
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::ReleaseReadLock()
{
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::GetWriteLock()
{
  this->TreeMutex->Lock();
  this->TreeLock = 1;
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
void vtkGeoTerrain::ReleaseWriteLock()
{
  this->TreeMutex->Lock();
  this->TreeLock = 0;
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
bool vtkGeoTerrain::Update(vtkGeoTerrain* terrain, vtkGeoCamera* camera)
{
  vtkGeoTerrainNode* node;
  bool changedFlag = false;
  int maxLevel = 0;
  
  int hackCount = 0; // Do not change too many tiles at once.

  double t1 = vtkTimerLog::GetUniversalTime();
  
  // If we cannot get a lock, we should return immediately and not wait.
  // Since the cache owns the terrain tree, it is responsible
  // For managing the access to the tree from multiple threads.
  if (this->GetReadLock() == false)
    {
    return false;
    }
  if (terrain->GetNumberOfNodes() == 0 )
    {
    // The terrain always covers the entire globe.  If we have no nodes,
    // then we need to initialize this terrain to have the lowest two
    // nodes (East and west hemisphere).
    changedFlag = true;
    this->InitializeTerrain(terrain);
    }

  // Start a new list of nodes.
  terrain->StartEdit();
  
  // Create a new list of nodes and copy/refine old list to the new list.
  // The order of nodes traces a strict depth first search.
  // This makes merging nodes to a lower resolution simpler.
  int numInNodes = terrain->GetNumberOfNodes();
  int evaluation;
  int outIdx = 0;
  int inIdx = 0;
  while (inIdx < numInNodes)
    {
    node = terrain->GetNode(inIdx);
    vtkGeoTerrainNode* parent = node->GetParent();
    evaluation = this->EvaluateNode(node, camera);
    
    // Just a test to even out model changes over multiple renders.
    //if (hackCount > 20)
    //  {
    //  evaluation = 0;
    //  }
    
    if (evaluation < 0)
      { // Do not merge the children if the parent would want to split;
      // I am trying to avoid oscilations.
      if (parent && this->EvaluateNode(parent, camera) > 0)
        {
        evaluation = 0;
        }
      }
    // We cannot split if there are no children.
    if (evaluation > 0 && node->GetChild(0) == 0)
      {
      evaluation = 0;
      }
      
    if (evaluation > 0)
      { // refine the node.  Add the 4 children.
      // For simplicity, lets just refine one level per update.
      if ( node->GetChild(0) == 0)
        { // sanity check.  We checked for this above.
        terrain->AddNode(node);
        ++outIdx;
        if (node->GetLevel() > maxLevel) { maxLevel = node->GetLevel();}
        }
      else
        {
        //newList[outIdx++] = node->GetChild(0);
        vtkGeoTerrainNode* child;
        child = node->GetChild(0);
        terrain->AddNode(child);
        child = node->GetChild(1);
        terrain->AddNode(child);
        child = node->GetChild(2);
        terrain->AddNode(child);
        child = node->GetChild(3);
        terrain->AddNode(child);
        // Just for debugging.
        if (child->GetLevel() > maxLevel) { maxLevel = child->GetLevel();}
        hackCount += 4;

        changedFlag = true;
        }
      ++inIdx;
      }
    else if (evaluation < 0 && node->GetLevel() > 0 && node->GetWhichChildAreYou() == 0)
      { // Only merge if the first child wants to.
      // TODO: Change this to use the "IsDescendantOf" method.
      unsigned long parentId = parent->GetId();
      // Now remove all nodes until we get to a node that is not a 
      // decendant of the parent node.
      // All decendents will have the first N bits in their Id.
      unsigned long mask = ((node->GetLevel() * 2) - 1);
      mask = (1 << mask) - 1;
      // No need to test heritage of first child.
      unsigned long tmp = parentId;
      // This leaves the inIdx point to the next node so
      // other paths need to increment the inIdx at their end.
      while ((tmp == parentId) && inIdx < numInNodes)
        {
        ++inIdx;
        // This while structure of this loop makes termination sort of complicated.
        // We can go right past the end of the list here.
        if (inIdx < numInNodes)
          {
          node = terrain->GetNode(inIdx);
          tmp = node->GetId();
          tmp = tmp & mask;
          }
        }
      // Just add the parent for all the nodes we skipped.
      if (parent->GetLevel() > maxLevel) { maxLevel = parent->GetLevel();} // Just for debugging
      terrain->AddNode(parent);
      hackCount += 1;
      ++outIdx;
      changedFlag = true;
      }
    else
      { // Just pass the node through unchanged.
      //newList[outIdx++] = node;
      if (node->GetLevel() > maxLevel) { maxLevel = node->GetLevel();} // Just for debugging
      terrain->AddNode(node);
      ++inIdx;
      }
    }
    
  if (changedFlag)
    {
    terrain->FinishEdit();
    }

  this->ReleaseReadLock();

  t1 = vtkTimerLog::GetUniversalTime() - t1;
  if (t1 > 0.1)
    {
    cerr << "Update took : " << t1 << endl;
    }
    
  return changedFlag;
}

//-----------------------------------------------------------------------------
// Returns 0 if there should be no change, -1 if the node resolution is too
// high, and +1 if the nodes resolution is too low.
int vtkGeoTerrain::EvaluateNode(vtkGeoTerrainNode* node, vtkGeoCamera* cam)
{
  double sphereViewSize;
  
  if (cam == 0)
    {
    return 0;
    }
  
  // Size of the sphere in view area units (0 -> 1)
  sphereViewSize = cam->GetNodeCoverage(node);
  
  // Arbitrary tresholds
  if (sphereViewSize > 0.2)
    {
    return 1;
    }
  if (sphereViewSize < 0.05)
    {
    return -1;
    }
  // Do not change the node.
  return 0;
}
