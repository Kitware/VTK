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
/**
 * @class   vtkGeoGlobeSource
 * @brief   Spherical globe source.
 *
 *
 * vtkGeoGlobeSource is a 3D vtkGeoSource suitable for use in vtkGeoTerrain.
 * It uses the vtkGlobeSource filter to produce terrain patches.
*/

#ifndef vtkGeoGlobeSource_h
#define vtkGeoGlobeSource_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkGeoSource.h"

class vtkGeoTerrainNode;

class VTKGEOVISCORE_EXPORT vtkGeoGlobeSource : public vtkGeoSource
{
public:
  static vtkGeoGlobeSource *New();
  vtkTypeMacro(vtkGeoGlobeSource,vtkGeoSource);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Fetches a low-resolution sphere for the entire globe.
   */
  bool FetchRoot(vtkGeoTreeNode* root) VTK_OVERRIDE;

  /**
   * Fetches a refined geometry patch, a section of a sphere.
   */
  bool FetchChild(vtkGeoTreeNode* node, int index, vtkGeoTreeNode* child) VTK_OVERRIDE;

protected:
  vtkGeoGlobeSource();
  ~vtkGeoGlobeSource() VTK_OVERRIDE;

private:
  vtkGeoGlobeSource(const vtkGeoGlobeSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoGlobeSource&) VTK_DELETE_FUNCTION;
};

#endif
