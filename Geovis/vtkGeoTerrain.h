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
// vtkGeoTerrainCache vtkGeoBackgroundImageRepresentation.

#ifndef __vtkGeoTerrain_h
#define __vtkGeoTerrain_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // for SP
#include <vtkstd/vector> // vector

class vtkGeoTerrainNode;
class vtkGeoTerrainCache;
class vtkGeoCamera;

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
  // This terrain object gets its terrain patches from this cache.
  // The user needs to set this.
  void SetCache(vtkGeoTerrainCache* cache);
  vtkGeoTerrainCache* GetCache();
  
protected:
  vtkGeoTerrain();
  ~vtkGeoTerrain();

//BTX
  // This is the list of terrain nodes that is used as the official
  // terrain model.
  vtkstd::vector<vtkSmartPointer<vtkGeoTerrainNode> > Nodes;
  // This is a temporary list of nodes used when modifying the Node list.
  // It is like a double buffer.  It gets swapped with "Nodes"
  // when update is finished.
  vtkstd::vector<vtkSmartPointer<vtkGeoTerrainNode> > NewNodes;
  vtkSmartPointer<vtkGeoTerrainCache> Cache;
//ETX

private:
  vtkGeoTerrain(const vtkGeoTerrain&);  // Not implemented.
  void operator=(const vtkGeoTerrain&);  // Not implemented.
};

#endif
