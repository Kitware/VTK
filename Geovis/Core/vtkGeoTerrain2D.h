/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrain2D.h

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
// .NAME vtkGeoTerrain2D - A 2D terrain model for the globe.
//
// .SECTION Description
// vtkGeoTerrain2D contains a multi-resolution tree of geometry representing
// the globe. It uses a vtkGeoSource subclass to generate the terrain, such
// as vtkGeoProjectionSource. This source must be set before using the
// terrain in a vtkGeoView2D. The terrain also contains an AddActors()
// method which updates the set of actors representing the globe given the
// current camera position.

#ifndef vtkGeoTerrain2D_h
#define vtkGeoTerrain2D_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkGeoTerrain.h"

class vtkAbstractTransform;
class vtkAssembly;
class vtkCollection;
class vtkGeoImageRepresentation;
class vtkGeoSource;
class vtkGeoTerrainNode;
class vtkRenderer;

class VTKGEOVISCORE_EXPORT vtkGeoTerrain2D : public vtkGeoTerrain
{
public:
  static vtkGeoTerrain2D *New();
  vtkTypeMacro(vtkGeoTerrain2D,vtkGeoTerrain);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The maximum size of a single texel in pixels.
  // Images will be refined if a texel becomes larger than the tolerance.
  vtkSetMacro(TextureTolerance, double);
  vtkGetMacro(TextureTolerance, double);

  // Description:
  // The maximum allowed deviation of geometry in pixels.
  // Geometry will be refined if the deviation is larger than the tolerance.
  vtkSetMacro(LocationTolerance, double);
  vtkGetMacro(LocationTolerance, double);

  // Description:
  // Return the projection transformation used by this 2D terrain.
  virtual vtkAbstractTransform* GetTransform();

protected:
  vtkGeoTerrain2D();
  ~vtkGeoTerrain2D();

  double LocationTolerance;
  double TextureTolerance;

  // Description:
  // AddActors() calls this to setup parameters for evaluating nodes.
  virtual void InitializeNodeAnalysis(vtkRenderer* ren);

  // Description:
  // AddActors() calls this to determine if a node is in the current
  // viewport.
  virtual bool NodeInViewport(vtkGeoTerrainNode* node);

  // Description:
  // AddActors() calls to to evaluate whether a node should be
  // refined (1), coarsened (-1), or remain at the same level (0).
  virtual int EvaluateNode(vtkGeoTerrainNode* node);

  double CameraBounds[4];
  double PixelSize;

private:
  vtkGeoTerrain2D(const vtkGeoTerrain2D&); // Not implemented
  void operator=(const vtkGeoTerrain2D&); // Not implemented
};

#endif
