/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrainCache.h

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
// .NAME vtkGeoTerrainCache - A quadtree of terrain pathces.
// .SECTION Description 
// This class contains all terrain nodes save on the client.  
// It doe not prune yet.  It does not have caching or prefetching strategies.
// This object containes most of the client API.  It takes requests from
// the client to update the terrain nodes based on a vtkGeoCamera.
// It has a method "Update", which builds the best terrain given available
// terrain nodes in the cache.
// This object implements the interface between the asynchonous 
// processes/threads that allow the client to remain responsive as the
// terrain is generated or received.

// .SECTION See Also
   
#ifndef __vtkGeoTerrainCache_h
#define __vtkGeoTerrainCache_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // for SP
#include "vtkGeoTerrainNode.h" // for SP
#include "vtkGeoTerrainSource.h" // for SP
#include "vtkMultiThreader.h" // for SP

class vtkCamera;
class vtkGeoCamera;
class vtkGeoTerrain;

class VTK_GEOVIS_EXPORT vtkGeoTerrainCache : public vtkObject
{
public:
  static vtkGeoTerrainCache *New();
  vtkTypeRevisionMacro(vtkGeoTerrainCache, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This supplies the terrain polydata patches.
  void SetTerrainSource(vtkGeoTerrainSource* source);
  vtkGeoTerrainSource* GetTerrainSource() {return this->TerrainSource;}
  
  // Description:
  // Returns true if the terrain changed.
  bool Update(vtkGeoTerrain* terrain, vtkGeoCamera* camera);
  
  // Description:
  // Asynchronous update of the terrain.  This returns immediately
  // The tree will reflect the request sometime in the future.
  void Request(vtkGeoCamera* camera);
  
  // Description:
  // Terminates the request process.
  void RequestTerminate();
  
  // Description:
  // This is public so that the multi threader can call this method.
  void ThreadStart();
  
protected:
  vtkGeoTerrainCache();
  ~vtkGeoTerrainCache();

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

  // Set the terrain to be the lowest resolution surface of the Earth.
  // The terrain will always cover the entire earth.
  void InitializeTerrain(vtkGeoTerrain* terrain);

  // Returns 0 if there should be no change, -1 if the node resolution is too
  // high, and +1 if the nodes resolution is too low.
  int EvaluateNode(vtkGeoTerrainNode* node, vtkGeoCamera* cam);

  // Returns VTK_ERROR if the children are not created.
  int RefineNode(vtkGeoTerrainNode* node);

  // This is run by the thread to create nodes if necessary.
  void Request(vtkGeoTerrainNode* node, vtkGeoCamera* cam);

  //BTX
  vtkSmartPointer<vtkGeoTerrainNode> EasternHemisphere;
  vtkSmartPointer<vtkGeoTerrainNode> WesternHemisphere;
  vtkSmartPointer<vtkGeoTerrainSource> TerrainSource;

  
  vtkSmartPointer<vtkMultiThreader> Threader;
  // The thread needs the camera.  This is just used to start the thread.
  vtkSmartPointer<vtkGeoCamera> Camera;

  // Socket would be better ...
  vtkSmartPointer<vtkMutexLock> WaitForRequestMutex1;
  // The TreeMutex is used to block the background request thread.
  // The TreeLock variable is used to control the main thread.
  vtkSmartPointer<vtkMutexLock> TreeMutex;
//ETX
  unsigned char TreeLock;

  int ThreadId;
  
private:
  vtkGeoTerrainCache(const vtkGeoTerrainCache&);  // Not implemented.
  void operator=(const vtkGeoTerrainCache&);  // Not implemented.
};

#endif
