/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoEdgeStrategy.h

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
// .NAME vtkGeoEdgeStrategy - Layout graph edges on a globe as arcs.
//
// .SECTION Description
// vtkGeoEdgeStrategy produces arcs for each edge in the input graph.
// This is useful for viewing lines on a sphere (e.g. the earth).
// The arcs may "jump" above the sphere's surface using ExplodeFactor.

#ifndef __vtkGeoEdgeStrategy_h
#define __vtkGeoEdgeStrategy_h

#include "vtkEdgeLayoutStrategy.h"

class VTK_INFOVIS_EXPORT vtkGeoEdgeStrategy : public vtkEdgeLayoutStrategy
{
public:
  static vtkGeoEdgeStrategy *New();
  vtkTypeMacro(vtkGeoEdgeStrategy,vtkEdgeLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The base radius used to determine the earth's surface.
  // Default is the earth's radius in meters.
  // TODO: Change this to take in a vtkGeoTerrain to get altitude.
  vtkSetMacro(GlobeRadius, double);
  vtkGetMacro(GlobeRadius, double);
  
  // Description:
  // Factor on which to "explode" the arcs away from the surface.
  // A value of 0.0 keeps the values on the surface.
  // Values larger than 0.0 push the arcs away from the surface by a distance
  // proportional to the distance between the points.
  // The default is 0.2.
  vtkSetMacro(ExplodeFactor, double);
  vtkGetMacro(ExplodeFactor, double);
  
  // Description:
  // The number of subdivisions in the arc.
  // The default is 20.
  vtkSetMacro(NumberOfSubdivisions, int);
  vtkGetMacro(NumberOfSubdivisions, int);

  // Description:
  // Perform the layout.
  virtual void Layout();
  
protected:
  vtkGeoEdgeStrategy();
  ~vtkGeoEdgeStrategy() {}

  double GlobeRadius;
  double ExplodeFactor;
  int NumberOfSubdivisions;
  
private:
  vtkGeoEdgeStrategy(const vtkGeoEdgeStrategy&);  // Not implemented.
  void operator=(const vtkGeoEdgeStrategy&);  // Not implemented.
};

#endif
