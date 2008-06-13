/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageCache.h

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

// .NAME vtkGeoAlignedImageCache - 
// .SECTION vtkGeoAlignedImageCache holds image nodes that can be used for
// building a vtkGeoAlignedImage.  This object will be controlling
// a separate thread for loading, generating or receiving images.

// .SECTION See Also
// vtkGeoAlignedImage
   
#ifndef __vtkGeoAlignedImageCache_h
#define __vtkGeoAlignedImageCache_h

#include "vtkSmartPointer.h" // for SP
#include "vtkGeoAlignedImageSource.h" // for SP
#include "vtkGeoImageNode.h" // for SP
#include "vtkGeoTerrain.h" // for SP
#include "vtkMultiThreader.h" // for SP
#include "vtkObject.h"

class vtkGeoTerrainNode;

class VTK_GEOVIS_EXPORT vtkGeoAlignedImageCache : public vtkObject
{
public:
  static vtkGeoAlignedImageCache *New();
  vtkTypeRevisionMacro(vtkGeoAlignedImageCache, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetSource(vtkGeoAlignedImageSource* source);
  
  // Description:
  // Returns the best image we have for a specific terrain node.
  vtkGeoImageNode* GetBestImageNode(vtkGeoTerrainNode* newTerrainNode);

  // Description:
  // This is public so that the multi threader can call this method.
  void ThreadStart();

protected:
  vtkGeoAlignedImageCache();
  ~vtkGeoAlignedImageCache();

//BTX
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

//BTX
  vtkSmartPointer<vtkMultiThreader> Threader;
  // The thread needs the terrain.
  vtkSmartPointer<vtkGeoTerrain> Terrain;

  // Socket would be better ...
  vtkSmartPointer<vtkMutexLock> WaitForRequestMutex1;
  vtkSmartPointer<vtkMutexLock> WaitForRequestMutex2;
  vtkSmartPointer<vtkMutexLock> WaitForRequestMutex3;
  // The TreeMutex is used to block the background request thread.
  // The TreeLock variable is used to control the main thread.
  vtkSmartPointer<vtkMutexLock> TreeMutex;
//ETX

  unsigned char TreeLock;

  int ThreadId;
  
  
private:
  vtkGeoAlignedImageCache(const vtkGeoAlignedImageCache&);  // Not implemented.
  void operator=(const vtkGeoAlignedImageCache&);  // Not implemented.
};

#endif

