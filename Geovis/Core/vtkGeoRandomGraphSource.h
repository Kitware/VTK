/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoRandomGraphSource.h

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
 * @class   vtkGeoRandomGraphSource
 * @brief   A geospatial graph with random edges
 *
 *
 * Generates a graph with a specified number of vertices, with the density of
 * edges specified by either an exact number of edges or the probability of
 * an edge.  You may additionally specify whether to begin with a random
 * tree (which enforces graph connectivity).
 *
 * The filter also adds random vertex attributes called latitude and longitude.
 * The latitude is distributed uniformly from -90 to 90, while longitude is
 * distributed uniformly from -180 to 180.
 *
 * @sa
 * vtkRandomGraphSource
*/

#ifndef vtkGeoRandomGraphSource_h
#define vtkGeoRandomGraphSource_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkRandomGraphSource.h"

class vtkGraph;

class VTKGEOVISCORE_EXPORT vtkGeoRandomGraphSource : public vtkRandomGraphSource
{
public:
  static vtkGeoRandomGraphSource* New();
  vtkTypeMacro(vtkGeoRandomGraphSource,vtkRandomGraphSource);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkGeoRandomGraphSource();
  ~vtkGeoRandomGraphSource() VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkGeoRandomGraphSource(const vtkGeoRandomGraphSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoRandomGraphSource&) VTK_DELETE_FUNCTION;
};

#endif

