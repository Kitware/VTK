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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkGeoEdgeStrategy - layout graph edges on a globe as arcs.
//
// .SECTION Description

// .SECTION Thanks

#ifndef __vtkGeoEdgeStrategy_h
#define __vtkGeoEdgeStrategy_h

#include "vtkEdgeLayoutStrategy.h"

class VTK_GEOVIS_EXPORT vtkGeoEdgeStrategy : public vtkEdgeLayoutStrategy
{
public:
  static vtkGeoEdgeStrategy *New();

  vtkTypeRevisionMacro(vtkGeoEdgeStrategy,vtkEdgeLayoutStrategy);
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
