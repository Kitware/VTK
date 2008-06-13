/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageCache.cxx

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
#include "vtkGeoAlignedImageCache.h"
#include "vtkGeoTerrainNode.h"
#include "vtkMutexLock.h"

vtkCxxRevisionMacro(vtkGeoAlignedImageCache, "1.2");
vtkStandardNewMacro(vtkGeoAlignedImageCache);

//-----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkGeoAlignedImageCacheThreadStart( void *arg )
{
  int threadId, threadCount;
  threadId = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
  threadCount = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;
  
  vtkGeoAlignedImageCache* self;
  self = (vtkGeoAlignedImageCache*)
    (((vtkMultiThreader::ThreadInfo *)(arg))->UserData);

 self->ThreadStart();
  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
vtkGeoAlignedImageCache::vtkGeoAlignedImageCache() 
{
  this->Threader = vtkSmartPointer<vtkMultiThreader>::New();
  //this->WaitForRequestMutex1 = vtkSmartPointer<vtkMutexLock>::New();
  //this->WaitForRequestMutex1->Lock();
  //this->WaitForRequestMutex2 = vtkSmartPointer<vtkMutexLock>::New();
  //this->WaitForRequestMutex3 = vtkSmartPointer<vtkMutexLock>::New();
  
  //this->TreeMutex = vtkSmartPointer<vtkMutexLock>::New();
  this->TreeLock = 0;

  // Spawn a thread to update the tree.
  //this->ThreadId = this->Threader->SpawnThread(
  //  vtkGeoAlignedImageCacheThreadStart, this);
}

//-----------------------------------------------------------------------------
vtkGeoAlignedImageCache::~vtkGeoAlignedImageCache() 
{
  
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageCache::ThreadStart()
{
  this->WaitForRequestMutex2->Lock();
  // Use mutex to avoid a busy loop.  Select on a socket will be better.
  while (1)
    {
    // Stupid gating a thread via mutex guntlet.
    this->WaitForRequestMutex1->Lock();    // Waits for a long time.
    this->WaitForRequestMutex1->Unlock();  // We only keep lock two in theis process.
    this->WaitForRequestMutex2->Unlock();
    this->WaitForRequestMutex3->Lock();    // Gives other thread a change to lock 1.
    this->WaitForRequestMutex2->Lock();
    this->WaitForRequestMutex3->Unlock();

    if (this->Terrain == 0)
      { // terminate
      this->WaitForRequestMutex2->Unlock();
      return;
      }

    // Variable to manage whoe has access to reading and changing tree.
    // This thread never keeps this lock for long.
    // We do not want to block the client.
    this->GetWriteLock();
    //this->Request(this->Terrain);
    //this->Request(this->Terrain);
    this->ReleaseWriteLock();
    }
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageCache::RequestTerminate()
{
  this->Terrain = 0;
  this->WaitForRequestMutex3->Lock();
  this->WaitForRequestMutex1->Unlock();
  this->WaitForRequestMutex2->Lock(); // Force control to other thread
  this->WaitForRequestMutex1->Lock();
  this->WaitForRequestMutex2->Unlock();
  this->WaitForRequestMutex3->Unlock();
}

//-----------------------------------------------------------------------------
bool vtkGeoAlignedImageCache::GetReadLock()
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
void vtkGeoAlignedImageCache::ReleaseReadLock()
{
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageCache::GetWriteLock()
{
  this->TreeMutex->Lock();
  this->TreeLock = 1;
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageCache::ReleaseWriteLock()
{
  this->TreeMutex->Lock();
  this->TreeLock = 0;
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageCache::SetSource(vtkGeoAlignedImageSource* source)
{
  // For now just grab the whole tree on intialization.
  // API for requesting tiles comes later.
  
  this->WesternHemisphere = source->WesternHemisphere;
  this->EasternHemisphere = source->EasternHemisphere;
  this->Source = source;
}

//-----------------------------------------------------------------------------
vtkGeoImageNode* vtkGeoAlignedImageCache::GetBestImageNode(
  vtkGeoTerrainNode* newTerrainNode)
{
  unsigned long id = newTerrainNode->GetId();
  int childIdx;
  vtkGeoImageNode* imageNode;
  if (id & 1)
    {
    imageNode = this->EasternHemisphere;
    }
  else
    {
    imageNode = this->WesternHemisphere;
    }
  id = id >> 1;
  
  while (imageNode->GetChild(0) && 
         imageNode->GetLevel() < newTerrainNode->GetLevel())
    {
    childIdx = id & 3;
    imageNode = imageNode->GetChild(childIdx);
    id = id >> 2;
    }

  if (this->Source->GetUseTileDatabase())
    {
    if (!imageNode->GetChild(0) &&
        imageNode->GetLevel() < newTerrainNode->GetLevel() &&
        imageNode->GetLevel() < this->Source->GetTileDatabaseDepth())
      {
      imageNode->CreateChildren();
      for (int i = 0; i < 4; ++i)
        {
        imageNode->GetChild(i)->LoadAnImage(
          this->Source->GetTileDatabaseLocation());
        }
      childIdx = id & 3;
      imageNode = imageNode->GetChild(childIdx);
      }
    }
  
  return imageNode;
}

