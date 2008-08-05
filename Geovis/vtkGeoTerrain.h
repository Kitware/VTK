/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrain.h

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
// .NAME vtkGeoTerrain - 
// .SECTION Description Stores surface of the earth selected from the 
// multi-resolution available from the vtkGeoTerrainCache.
// I would like this class to be as simple a posible.  It still has
// a pointer to a cache and an update method, but the method is just 
// forwarded to the cache.  We could get rid of these too, but I think it 
// is akward for the representation to point to a cache.

// .SECTION See Also
// vtkGeoBackgroundImageRepresentation.

#ifndef __vtkGeoTerrain_h
#define __vtkGeoTerrain_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // for SP
#include "vtkGeoTerrainNode.h" // for SP
#include "vtkGeoTerrainSource.h" // for SP
#include "vtkMultiThreader.h" // for SP
#include <vtkstd/vector> // vector

class vtkGeoTerrainNode;
class vtkGeoCamera;
class vtkCamera;


class VTK_GEOVIS_EXPORT vtkGeoTerrain : public vtkObject
{
public:
  static vtkGeoTerrain *New();
  vtkTypeRevisionMacro(vtkGeoTerrain, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build the best terrain for the last request given 
  // patches currently available in the cache.
  // It returns true if the terrain changed at all. 
  bool Update(vtkGeoCamera* camera);
  
  // Description:
  // These method is used by the cache to create the terrain model.
  void StartEdit();
  void AddNode(vtkGeoTerrainNode* node);
  void FinishEdit();

  // Description:
  // Get the nodes to buld the assembly.
  // These nodes are not affected by StartEdit or AddNodes 
  // until FinishedEdit is called..
  int GetNumberOfNodes();
  vtkGeoTerrainNode* GetNode(int idx);

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
  vtkGeoTerrain();
  ~vtkGeoTerrain();

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
  // This is the list of terrain nodes that is used as the official
  // terrain model.
  vtkstd::vector<vtkSmartPointer<vtkGeoTerrainNode> > Nodes;
  // This is a temporary list of nodes used when modifying the Node list.
  // It is like a double buffer.  It gets swapped with "Nodes"
  // when update is finished.
  vtkstd::vector<vtkSmartPointer<vtkGeoTerrainNode> > NewNodes;

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
  vtkGeoTerrain(const vtkGeoTerrain&);  // Not implemented.
  void operator=(const vtkGeoTerrain&);  // Not implemented.
};

#endif
