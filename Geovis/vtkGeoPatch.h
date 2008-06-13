/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoPatch.h

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
// .NAME vtkGeoPatch - Every thing to render one patch of the terrain.
// .SECTION Description   We might hide this object from the API
// because it really is only used by the vtkGeoBackgroundImageRepresentation.
// Wee needed a way to track when individual patches are modified
// so we can reuse old objects as much as possible.  This also
// provides a conveniant paring of terrain and image nodes.

// .SECTION See Also vtkGeoBackgroundImageRepresentation
   
#ifndef __vtkGeoPatch_h
#define __vtkGeoPatch_h

#include "vtkSmartPointer.h" // for SP
#include "vtkGeoTerrainNode.h" // for SP
#include "vtkGeoImageNode.h" // for SP
#include "vtkActor.h" // for SP
#include "vtkPolyDataMapper.h" // for SP
#include "vtkTexture.h" // for SP
#include "vtkGeoComputeTextureCoordinates.h" // for SP


class VTK_GEOVIS_EXPORT vtkGeoPatch
{
public:
  vtkGeoPatch();
  ~vtkGeoPatch();

  void SetTerrainNode(vtkGeoTerrainNode* node);
  vtkGeoTerrainNode* GetTerrainNode() {return this->TerrainNode;}
  
  void SetImageNode(vtkGeoImageNode* node);
  vtkGeoImageNode* GetImageNode() {return this->ImageNode;}

  vtkActor* GetActor();
  
  void Update();
  
protected:
//BTX
  vtkSmartPointer<vtkGeoTerrainNode>    TerrainNode;
  vtkSmartPointer<vtkGeoImageNode>      ImageNode;
  // This filter takes the terrain model as input
  // and generates a custom texture coordiante array for
  // the image.
  vtkSmartPointer<vtkGeoComputeTextureCoordinates> Filter;
  // We may not need to keep a reference to the mapper.
  vtkSmartPointer<vtkPolyDataMapper> Mapper;
  vtkSmartPointer<vtkActor>          Actor;
  vtkSmartPointer<vtkTexture>        Texture;
//ETX
  
  bool Valid;
  
private:
  vtkGeoPatch(const vtkGeoPatch&);  // Not implemented.
  void operator=(const vtkGeoPatch&);  // Not implemented.
};

#endif
