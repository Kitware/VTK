/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImage.cxx

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
#include "vtkProp3DCollection.h"
#include "vtkAssembly.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkGeoAlignedImage.h"
#include "vtkGeoImageNode.h"
#include "vtkGeoPatch.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoTerrain.h"


vtkCxxRevisionMacro(vtkGeoAlignedImage, "1.3");
vtkStandardNewMacro(vtkGeoAlignedImage);

//----------------------------------------------------------------------------
vtkGeoAlignedImage::vtkGeoAlignedImage() 
{
  this->Cache = vtkSmartPointer<vtkGeoAlignedImageCache>::New();
}

//-----------------------------------------------------------------------------
vtkGeoAlignedImage::~vtkGeoAlignedImage() 
{
}

//-----------------------------------------------------------------------------
// This is to clean up actors, mappers, textures and other rendering object
// before the renderer and render window destruct.  It allows all graphics
// resources to be released cleanly.  Without this, the application 
// may crash on exit.
void vtkGeoAlignedImage::ExitCleanup()
{
  this->DeletePatches();
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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
void vtkGeoAlignedImage::SetCache(vtkGeoAlignedImageCache* cache)
{
  this->Cache = cache;
}

//-----------------------------------------------------------------------------
vtkGeoAlignedImageCache* vtkGeoAlignedImage::GetCache()
{
  return this->Cache;
}

//  this->Nodes.clear();
//  this->Nodes.push_back(node);
//  this->Nodes.size();

//-----------------------------------------------------------------------------
// This creates a new list of patches by assigning images to each terrain
// node.  It returns true if the model changes.
vtkGeoPatch* vtkGeoAlignedImage::GetPatch(int idx)
{
  if (idx >= 0 && idx < (int)(this->Patches.size()))
    {
    return this->Patches[idx];
    }
  return 0;
}

//-----------------------------------------------------------------------------
// This creates a new list of patches by assigning images to each terrain
// node.  It returns true if the model changes.
bool vtkGeoAlignedImage::Update(vtkGeoTerrain* terrain)
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
      newImageNode = this->Cache->GetBestImageNode(newTerrainNode);
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
        newImageNode = this->Cache->GetBestImageNode(newTerrainNode);
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
          newImageNode = this->Cache->GetBestImageNode(newTerrainNode);
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
        newImageNode = this->Cache->GetBestImageNode(newTerrainNode);
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
vtkGeoPatch* vtkGeoAlignedImage::GetNewPatchFromHeap()
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
void vtkGeoAlignedImage::ReturnPatchToHeap(vtkGeoPatch* patch)
{
  // There should be an initialize method.
  patch->SetImageNode(0);
  patch->SetTerrainNode(0);
  this->PatchHeap.push(patch);
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImage::DeletePatches()
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


//-----------------------------------------------------------------------------
// Add the actors that render the terrain image pairs to the assembly.
// We need a node by node indication that the node has changed and we need to 
// reuse actors, and models so we do not generate new texture coordinates 
// unless we have to.
void vtkGeoAlignedImage::UpdateAssembly(vtkAssembly* assembly)
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

