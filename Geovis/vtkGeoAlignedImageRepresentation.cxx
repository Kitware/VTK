/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageRepresentation.cxx

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

#include "vtkGeoAlignedImageRepresentation.h"

#include "vtkActor.h"
#include "vtkAssembly.h"
#include "vtkGeoImageNode.h"
#include "vtkGeoPatch.h"
#include "vtkGeoTerrain.h"
#include "vtkGeoTerrainNode.h"
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3DCollection.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkTimerLog.h"
#include "vtkView.h"


vtkCxxRevisionMacro(vtkGeoAlignedImageRepresentation, "1.4");
vtkStandardNewMacro(vtkGeoAlignedImageRepresentation);

//-----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkGeoAlignedImageRepresentationThreadStart( void *arg )
{
//  int threadId = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
//  int threadCount = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;
  
  vtkGeoAlignedImageRepresentation* self;
  self = (vtkGeoAlignedImageRepresentation*)
    (((vtkMultiThreader::ThreadInfo *)(arg))->UserData);

 self->ThreadStart();
  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
vtkGeoAlignedImageRepresentation::vtkGeoAlignedImageRepresentation() 
{
  this->Actor = vtkSmartPointer<vtkAssembly>::New();
  this->Terrain = NULL;

  // Turn off selectability.
  this->SelectableOff();

  this->Threader = vtkSmartPointer<vtkMultiThreader>::New();
  this->TreeLock = 0;
}

//-----------------------------------------------------------------------------
vtkGeoAlignedImageRepresentation::~vtkGeoAlignedImageRepresentation() 
{  
}

//-----------------------------------------------------------------------------
// This is to clean up actors, mappers, textures and other rendering object
// before the renderer and render window destruct.  It allows all graphics
// resources to be released cleanly.  Without this, the application 
// may crash on exit.
void vtkGeoAlignedImageRepresentation::ExitCleanup()
{
  this->Actor->GetParts()->RemoveAllItems();
  this->DeletePatches();
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Actor: " << this->Actor << endl;
  os << indent << "Terain: " << this->Terrain << endl;

  vtkGeoPatch *patch;
  int num, ii;
  num = static_cast<int>(this->Patches.size());
  for (ii = 0; ii < num; ++ii)
    {
    patch = this->Patches[ii];
    vtkGeoTerrainNode* node = patch->GetTerrainNode();
    if (node)
      {
      os << indent << patch << " level " << node->GetLevel() 
         << ", id = " << node->GetId() << endl;
      }
     else
      {
      os << "Missing node\n";
      }
    }
  os << "\n\n" << num << endl;
}

//-----------------------------------------------------------------------------
// This constructs the best model possible given the data currently available.
// The request will be a separate non blocking call.
bool vtkGeoAlignedImageRepresentation::Update(vtkGeoCamera* cam)
{
  if (!cam)
    {
    return false;
    }
  bool changedFlag = 0;
  // If the terrain does not update, the image can still change to pick
  // tiles that better match the terrain.
  if (this->Terrain->Update(cam))
    {
    changedFlag = 1;
    }
  if (this->UpdateImage(&(*this->Terrain)))
    {
    changedFlag = 1;
    }
  if (changedFlag)
    {
    // Now add the elements to the assembly.
    this->Actor->GetParts()->RemoveAllItems();
    this->UpdateAssembly(this->Actor);
    }

  return changedFlag;
}

//-----------------------------------------------------------------------------
// This creates a new list of patches by assigning images to each terrain
// node.  It returns true if the model changes.
bool vtkGeoAlignedImageRepresentation::UpdateImage(vtkGeoTerrain* terrain)
{
  int numNewNodes;
  int numOldNodes;
  int newIdx, oldIdx;
  vtkGeoPatch* newPatch;
  vtkGeoPatch* oldPatch = 0;
  vtkGeoTerrainNode* oldTerrainNode = 0;
  vtkGeoImageNode*   oldImageNode = 0;
  
  vtkGeoTerrainNode* newTerrainNode;
  vtkGeoImageNode*   newImageNode;

  bool changedFlag = false;
  
  // Create a new list of image nodes and copy/refine old list to the new list.
  vtkstd::vector<vtkGeoPatch* > newPatches;
  
  // Loop through the new terrain nodes.
  numNewNodes = terrain->GetNumberOfNodes();
  numOldNodes = static_cast<int>(this->Patches.size());
  oldIdx = 0;
  newIdx = 0;
  while (newIdx < numNewNodes)
    {
    newTerrainNode = terrain->GetNode(newIdx);
    oldPatch = this->GetPatch(oldIdx);
    if (oldPatch == 0)
      { // This should only happen when we first execute.
      // Create a new node and find an image for it.
      changedFlag = true;
      newPatch = this->GetNewPatchFromHeap();
      newPatch->SetTerrainNode(newTerrainNode);
      newImageNode = this->GetBestImageNode(newTerrainNode);
      newPatch->SetImageNode(newImageNode);
      newPatches.push_back(newPatch);
      if (newPatch->GetTerrainNode() == 0) {int* p = 0; *p = 0;}
      ++newIdx;
      }
    else if (oldPatch->GetTerrainNode() == newTerrainNode)
      { // We could just check to see if the levels are the same.
      // This would make the three cases more balanced.
      // Terrain for this node did not change.
      // Check if the image is the best available.
      // Reuse  the patch object.
      newPatch = oldPatch;
      // Check if we have a better image for this node.
      oldImageNode = newImageNode = newPatch->GetImageNode();
      // Image tile level will always be >= to terrain node level.
      // The image can be larger than the terrain, but not smaller.
      // If the levels are the same, then we already have the
      // best image available.
      if (newImageNode->GetLevel() < newTerrainNode->GetLevel())
        { // We have a lower res image.  Try for a better image.
        newImageNode = this->GetBestImageNode(newTerrainNode);
        }
      if (newImageNode != oldImageNode)
        {
        changedFlag = true;
        newPatch->SetImageNode(newImageNode);
        }
      newPatches.push_back(newPatch);
      if (newPatch->GetTerrainNode() == 0) {int* p = 0; *p = 0;}
      // We reused the patch.  No need to return a patch to the heap.
      // We are done with this nodes, move to the next.
      ++oldIdx;
      ++newIdx;
      }
    else
      {
      changedFlag = true;
      // The terrain node list has changed.
      // We have to sync up the two lists again.
      oldTerrainNode = oldPatch->GetTerrainNode();
      if (newTerrainNode->GetLevel() > oldTerrainNode->GetLevel())
        {
        // The old node has been refined. There are multiple nodes
        // in the new list which correspond to this one old node.
        // Forward through all decendants of the old node.
        while (newIdx < numNewNodes && 
               (newTerrainNode = terrain->GetNode(newIdx)) &&
               (newTerrainNode->IsDescendantOf(oldTerrainNode)))
          {
          newImageNode = this->GetBestImageNode(newTerrainNode);
          newPatch = this->GetNewPatchFromHeap();
          newPatch->SetImageNode(newImageNode);
          newPatch->SetTerrainNode(newTerrainNode);
          newPatches.push_back(newPatch);
          if (newPatch->GetTerrainNode() == 0) {int* p = 0; *p = 0;}
          ++newIdx;
          }
        // Return the old patch to the heap.
        this->ReturnPatchToHeap(oldPatch);
        ++oldIdx;
        }
      else if (newTerrainNode->GetLevel() < oldTerrainNode->GetLevel())
        {
        newImageNode = this->GetBestImageNode(newTerrainNode);
        newPatch = this->GetNewPatchFromHeap();
        newPatch->SetImageNode(newImageNode);
        newPatch->SetTerrainNode(newTerrainNode);
        newPatches.push_back(newPatch);
        if (newPatch->GetTerrainNode() == 0) {int* p = 0; *p = 0;}
        // Nodes have been merged.  Multiple nodes in the old list
        // correspond to a single node in the new list.
        // Forward through all decendants of the new node.
        while (oldIdx < numOldNodes &&
               (oldPatch = this->GetPatch(oldIdx)) &&
               (oldTerrainNode = oldPatch->GetTerrainNode()) && 
               (oldTerrainNode->IsDescendantOf(newTerrainNode)))
          {
          // Return the old patch to the heap.
          this->ReturnPatchToHeap(oldPatch);
          ++oldIdx;
          }
        ++newIdx;
        }
      }
    }

  if (changedFlag)
    {
    this->Patches = newPatches;
    //this->PrintList();
    }
    
  return changedFlag;
}

//-----------------------------------------------------------------------------
// Add the actors that render the terrain image pairs to the assembly.
// We need a node by node indication that the node has changed and we need to 
// reuse actors, and models so we do not generate new texture coordinates 
// unless we have to.
void vtkGeoAlignedImageRepresentation::UpdateAssembly(vtkAssembly* assembly)
{
  int idx;
  int numPatches;
  vtkGeoPatch* patch;
  
  // I assume this is only called when the update modified the terrain or image.
  assembly->GetParts()->RemoveAllItems();
  numPatches = static_cast<int>(this->Patches.size());
  for (idx = 0; idx < numPatches; ++idx)
    {
    patch = this->Patches[idx];
    patch->Update();
    assembly->AddPart(patch->GetActor());
    }
}

//-----------------------------------------------------------------------------
vtkGeoPatch* vtkGeoAlignedImageRepresentation::GetNewPatchFromHeap()
{
  vtkGeoPatch *patch;
  if (this->PatchHeap.size() > 0)
    {
    patch = this->PatchHeap.top();
    this->PatchHeap.pop();
    }
  else
    {
    patch = new vtkGeoPatch;
    }
  return patch;
}

//-----------------------------------------------------------------------------
// Starting the representation API
bool vtkGeoAlignedImageRepresentation::AddToView(vtkView* view)
{
  vtkRenderView* gv = vtkRenderView::SafeDownCast(view);
  if (!gv)
    {
    return false;
    }
  gv->GetRenderer()->AddActor(this->Actor);
  return true;
}

//-----------------------------------------------------------------------------
// Starting the representation API
bool vtkGeoAlignedImageRepresentation::RemoveFromView(vtkView* view)
{
  vtkRenderView* gv = vtkRenderView::SafeDownCast(view);
  if (!gv)
    {
    return false;
    }
  gv->GetRenderer()->RemoveActor(this->Actor);
  return true;
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::ThreadStart()
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
void vtkGeoAlignedImageRepresentation::RequestTerminate()
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
bool vtkGeoAlignedImageRepresentation::GetReadLock()
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
void vtkGeoAlignedImageRepresentation::ReleaseReadLock()
{
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::GetWriteLock()
{
  this->TreeMutex->Lock();
  this->TreeLock = 1;
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::ReleaseWriteLock()
{
  this->TreeMutex->Lock();
  this->TreeLock = 0;
  this->TreeMutex->Unlock();
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::SetSource(vtkGeoAlignedImageSource* source)
{
  // For now just grab the whole tree on intialization.
  // API for requesting tiles comes later.
  
  this->WesternHemisphere = source->WesternHemisphere;
  this->EasternHemisphere = source->EasternHemisphere;
  this->Source = source;
}

//-----------------------------------------------------------------------------
vtkGeoImageNode* vtkGeoAlignedImageRepresentation::GetBestImageNode(
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

//-----------------------------------------------------------------------------
// This creates a new list of patches by assigning images to each terrain
// node.  It returns true if the model changes.
vtkGeoPatch* vtkGeoAlignedImageRepresentation::GetPatch(int idx)
{
  if (idx >= 0 && idx < (int)(this->Patches.size()))
    {
    return this->Patches[idx];
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::ReturnPatchToHeap(vtkGeoPatch* patch)
{
  // There should be an initialize method.
  patch->SetImageNode(0);
  patch->SetTerrainNode(0);
  this->PatchHeap.push(patch);
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::DeletePatches()
{
  int num, ii;
  vtkGeoPatch* patch;
  
  while(this->PatchHeap.size() > 0)
    {
    patch = this->PatchHeap.top();
    this->PatchHeap.pop();
    delete patch;
    }
  num = static_cast<int>(this->Patches.size());
  for (ii = 0; ii < num; ++ii)
    {
    patch = this->Patches[ii];
    delete patch;
    }
}
  

