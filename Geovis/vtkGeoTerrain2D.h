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

#include "vtkObject.h"

class vtkAssembly;
class vtkCollection;
class vtkGeoImageRepresentation;
class vtkGeoSource;
class vtkGeoTerrainNode;
class vtkRenderer;

class VTK_GEOVIS_EXPORT vtkGeoTerrain2D : public vtkObject
{
public:
  static vtkGeoTerrain2D *New();
  vtkTypeRevisionMacro(vtkGeoTerrain2D,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The source used to obtain geometry patches.
  virtual vtkGeoSource* GetSource()
    { return this->GeoSource; }
  virtual void SetSource(vtkGeoSource* source);

  // Description:
  // Save the set of patches up to a given maximum depth.
  void SaveDatabase(const char* path, int depth);

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
  // Update the actors in an assembly used to render the globe.
  // ren is the current renderer, and imageReps holds the collection of
  // vtkGeoAlignedImageRepresentations that will be blended together to
  // form the image on the globe.
  void AddActors(
    vtkRenderer* ren,
    vtkAssembly* assembly,
    vtkCollection* imageReps);

protected:
  vtkGeoTerrain2D();
  ~vtkGeoTerrain2D();

  virtual void SetGeoSource(vtkGeoSource* source);
  vtkGeoSource* GeoSource;
  vtkGeoTerrainNode* Root;
  double LocationTolerance;
  double TextureTolerance;

  void Initialize();
  void PrintTree(ostream & os, vtkIndent indent, vtkGeoTerrainNode* node);

private:
  vtkGeoTerrain2D(const vtkGeoTerrain2D&); // Not implemented
  void operator=(const vtkGeoTerrain2D&); // Not implemented
};

