/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoGlobeSource.h

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
// .NAME vtkGeoGlobeSource - Spherical globe source.
//
// .SECTION Description
// vtkGeoGlobeSource is a 3D vtkGeoSource suitable for use in vtkGeoTerrain.
// It uses the vtkGlobeSource filter to produce terrain patches.

#include "vtkGeoSource.h"

class vtkGeoTerrainNode;

class VTK_GEOVIS_EXPORT vtkGeoGlobeSource : public vtkGeoSource
{
public:
  static vtkGeoGlobeSource *New();
  vtkTypeMacro(vtkGeoGlobeSource,vtkGeoSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Fetches a low-resolution sphere for the entire globe.
  virtual bool FetchRoot(vtkGeoTreeNode* root);

  // Description:
  // Fetches a refined geometry patch, a section of a sphere.
  virtual bool FetchChild(vtkGeoTreeNode* node, int index, vtkGeoTreeNode* child);

protected:
  vtkGeoGlobeSource();
  ~vtkGeoGlobeSource();

private:
  vtkGeoGlobeSource(const vtkGeoGlobeSource&); // Not implemented
  void operator=(const vtkGeoGlobeSource&); // Not implemented
};

