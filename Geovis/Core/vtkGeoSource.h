/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoSource.h

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
// .NAME vtkGeoSource - A multi-resolution geographic data source
//
// .SECTION Description
// vtkGeoSource is an abstract superclass for all multi-resolution data sources
// shown in a geographic view like vtkGeoView or vtkGeoView2D. vtkGeoSource
// subclasses need to implement the FetchRoot() method, which fills a
// vtkGeoTreeNode with the low-res data at the root, and FetchChild(), which
// produces a refinement of a parent node. Other geovis classes such as
// vtkGeoTerrain, vtkGeoTerrain2D, and vtkGeoAlignedImageSource use a
// vtkGeoSource subclass to build their geometry or image caches which are
// stored in trees. The source itself does not maintain the tree, but
// simply provides a mechanism for generating refined tree nodes.
//
// Sources are multi-threaded. Each source may have one or more worker threads
// associated with it, which this superclass manages. It is essential that the
// FetchChild() method is thread-safe, since it may be called from multiple
// workers simultaneously.

#ifndef __vtkGeoSource_h
#define __vtkGeoSource_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkAbstractTransform;
class vtkCollection;
class vtkConditionVariable;
class vtkGeoTreeNode;
class vtkMultiThreader;
class vtkMutexLock;

class VTKGEOVISCORE_EXPORT vtkGeoSource : public vtkObject
{
public:
  vtkTypeMacro(vtkGeoSource,vtkObject);

  vtkGeoSource();
  ~vtkGeoSource();

  // Description:
  // Blocking access methods to be implemented in subclasses.
  virtual bool FetchRoot(vtkGeoTreeNode* root) = 0;
  virtual bool FetchChild(vtkGeoTreeNode* node, int index, vtkGeoTreeNode* child) = 0;

  // Description:
  // Non-blocking methods for to use from the main application.
  // After calling RequestChildren() for a certain node,
  // GetRequestedNodes() will after a certain period of time return a
  // non-null pointer to a collection of four vtkGeoTreeNode objects,
  // which are the four children of the requested node.
  // The collection is reference counted, so you need to eventually
  // call Delete() on the returned collection pointer (if it is non-null).
  virtual void RequestChildren(vtkGeoTreeNode* node);
  virtual vtkCollection* GetRequestedNodes(vtkGeoTreeNode* node);

  // Description:
  // Spawn worker threads.
  void Initialize(int numThreads = 1);

  // Description:
  // Shut down the source. This terminates the thread and releases memory.
  void ShutDown();

  void WorkerThread();

  // Description:
  // Return the projection transformation used by this source.
  virtual vtkAbstractTransform* GetTransform() { return NULL; }

protected:

  vtkCollection* InputSet;
  vtkCollection* ProcessingSet;

  // Description:
  // Locks the set for reading or writing
  vtkMutexLock* InputSetLock;
  vtkMutexLock* ProcessingSetLock;
  vtkMutexLock* OutputSetLock;

  vtkMutexLock* Lock;

  vtkConditionVariable* Condition;

  vtkMultiThreader* Threader;
  bool StopThread;
  bool Initialized;

  //BTX
  class implementation;
  implementation* Implementation;
  //ETX

private:
  vtkGeoSource(const vtkGeoSource&); // Not implemented
  void operator=(const vtkGeoSource&); // Not implemented
};

#endif // __vtkGeoSource_h
