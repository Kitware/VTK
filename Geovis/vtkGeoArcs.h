/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoArcs.h

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
// .NAME vtkGeoArcs - layout graph edges on a globe as arcs.
//
// .SECTION Description

// .SECTION Thanks

#ifndef __vtkGeoArcs_h
#define __vtkGeoArcs_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GEOVIS_EXPORT vtkGeoArcs : public vtkPolyDataAlgorithm 
{
public:
  static vtkGeoArcs *New();

  vtkTypeRevisionMacro(vtkGeoArcs,vtkPolyDataAlgorithm);
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
  
protected:
  vtkGeoArcs();
  ~vtkGeoArcs() {}

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  double GlobeRadius;
  double ExplodeFactor;
  int NumberOfSubdivisions;
  
private:
  vtkGeoArcs(const vtkGeoArcs&);  // Not implemented.
  void operator=(const vtkGeoArcs&);  // Not implemented.
};

#endif
