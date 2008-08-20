/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageRepresentation.h

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

// .NAME vtkGeoAlignedImageRepresentation - Earth with a background image.
// .SECTION vtkGeoAlignedImageRepresentation renders the terrain with a 
// background image.  It interfaces with the vtkGeoTerrain 
// and vtkGeoImageCache to get the data it requires to make the model.
// This representation assumes that the terrain and image caches share the
// same tree structure.

// Eventually, socket activity will indicate that new data is available.
// For now, I am supplying a non blocking method that checks for new data.

// .SECTION See Also
   
#ifndef __vtkGeoAlignedImageRepresentation_h
#define __vtkGeoAlignedImageRepresentation_h

#include "vtkDataRepresentation.h"

#include "vtkSmartPointer.h" // for SP
#include "vtkAssembly.h" // for SP
#include "vtkGeoAlignedImageSource.h" // for SP
#include "vtkGeoImageNode.h" // for SP
#include "vtkGeoTerrain.h" // for SP
#include "vtkMultiThreader.h" // for SP

#include <vtkstd/vector> // vector
#include <vtkstd/stack> // stack

class vtkRenderer;
class vtkGeoCamera;
class vtkGeoTerrain;
class vtkAssembly;
class vtkGeoPatch;

class VTK_GEOVIS_EXPORT vtkGeoAlignedImageRepresentation : public vtkDataRepresentation
{
public:
  static vtkGeoAlignedImageRepresentation *New();
  vtkTypeRevisionMacro(vtkGeoAlignedImageRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This constructs the best model possible given the data currently available.
  // The request will be a separate non blocking call.
  bool Update(vtkGeoCamera* cam);
  
  // Description:
  // This actor contains the actor which will render the earth.
  vtkAssembly *GetActor() { return this->Actor; }
  
  // Description:
  // This is the terrain that has the polydata models.  It is set by the user
  // because multiple representations share the same terrain model.
  void SetTerrain(vtkGeoTerrain *terrain) 
    { this->Terrain = terrain;}
  vtkGeoTerrain* GetTerrain() { return this->Terrain;}

  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().
  virtual bool AddToView(vtkView* view);
  
  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().
  virtual bool RemoveFromView(vtkView* view);
  
  // Description:
  // This is to clean up actors, mappers, textures and other rendering object
  // before the renderer and render window destruct.  It allows all graphics
  // resources to be released cleanly.  Without this, the application 
  // may crash on exit.
  void ExitCleanup();

  void SetSource(vtkGeoAlignedImageSource* source);
  
  // Description:
  // Returns the best image we have for a specific terrain node.
  vtkGeoImageNode* GetBestImageNode(vtkGeoTerrainNode* newTerrainNode);

  // Description:
  // This is public so that the multi threader can call this method.
  void ThreadStart();

  // Description:
  // This builds the image from the latest request using the image patches
  // currently available.  It returns true if the model changes.
  bool UpdateImage(vtkGeoTerrain* terrain);

  // Description:
  // Add the actors that render the terrain image pairs to the assembly.
  void UpdateAssembly(vtkAssembly* assembly);

protected:
  vtkGeoAlignedImageRepresentation();
  ~vtkGeoAlignedImageRepresentation();

//BTX
  vtkSmartPointer<vtkAssembly> Actor;
  vtkSmartPointer<vtkGeoTerrain> Terrain;
  vtkSmartPointer<vtkGeoAlignedImageSource> Source;
  vtkSmartPointer<vtkGeoImageNode> WesternHemisphere;
  vtkSmartPointer<vtkGeoImageNode> EasternHemisphere;
//ETX

  // Description:
  // This stops the thread used to make the request.
  void RequestTerminate();

  // Description:
  // Non blocking call.  Returns true if the lock was obtained.
  // If the lock was obtained, then you need to release the lock.
  bool GetReadLock();
  void ReleaseReadLock();

  // Description:
  // This is used by the background thread.
  // It blocks to get write access to the tree.
  void GetWriteLock();
  void ReleaseWriteLock();

  // Returns 0 if index is out of range.
  vtkGeoPatch* GetPatch(int idx);

  vtkGeoPatch* GetNewPatchFromHeap();
  void ReturnPatchToHeap(vtkGeoPatch* patch);
  void DeletePatches();

//BTX
  vtkSmartPointer<vtkMultiThreader> Threader;

  // Socket would be better ...
  vtkSmartPointer<vtkMutexLock> WaitForRequestMutex1;
  vtkSmartPointer<vtkMutexLock> WaitForRequestMutex2;
  vtkSmartPointer<vtkMutexLock> WaitForRequestMutex3;
  // The TreeMutex is used to block the background request thread.
  // The TreeLock variable is used to control the main thread.
  vtkSmartPointer<vtkMutexLock> TreeMutex;

  vtkstd::vector<vtkGeoPatch* > Patches;
  vtkstd::stack<vtkGeoPatch* > PatchHeap;
//ETX

  unsigned char TreeLock;

  int ThreadId;

private:
  vtkGeoAlignedImageRepresentation(const vtkGeoAlignedImageRepresentation&);  // Not implemented.
  void operator=(const vtkGeoAlignedImageRepresentation&);  // Not implemented.
};

#endif
