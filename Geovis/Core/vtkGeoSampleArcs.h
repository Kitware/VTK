/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoSampleArcs.h

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
// .NAME vtkGeoSampleArcs - Samples geospatial lines at regular intervals.
//
// .SECTION Description
// vtkGeoSampleArcs refines lines in the input polygonal data
// so that the distance between adjacent points is no more than a threshold
// distance. Points are interpolated along the surface of the globe.
// This is useful in order to keep lines such as political boundaries from
// intersecting the globe and becoming invisible.

#ifndef __vtkGeoSampleArcs_h
#define __vtkGeoSampleArcs_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GEOVIS_EXPORT vtkGeoSampleArcs : public vtkPolyDataAlgorithm 
{
public:
  static vtkGeoSampleArcs *New();

  vtkTypeMacro(vtkGeoSampleArcs,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The base radius used to determine the earth's surface.
  // Default is the earth's radius in meters.
  // TODO: Change this to take in a vtkGeoTerrain to get altitude.
  vtkSetMacro(GlobeRadius, double);
  vtkGetMacro(GlobeRadius, double);

  // Description:
  // The maximum distance, in meters, between adjacent points.
  vtkSetMacro(MaximumDistanceMeters, double);
  vtkGetMacro(MaximumDistanceMeters, double);

  //BTX
  enum
    {
    RECTANGULAR,
    SPHERICAL
    };
  //ETX

  // Description:
  // The input coordinate system.
  // RECTANGULAR is x,y,z meters relative the the earth center.
  // SPHERICAL is longitude,latitude,altitude.
  vtkSetMacro(InputCoordinateSystem, int);
  vtkGetMacro(InputCoordinateSystem, int);
  virtual void SetInputCoordinateSystemToRectangular()
    { this->SetInputCoordinateSystem(RECTANGULAR); }
  virtual void SetInputCoordinateSystemToSpherical()
    { this->SetInputCoordinateSystem(SPHERICAL); }

  // Description:
  // The desired output coordinate system.
  // RECTANGULAR is x,y,z meters relative the the earth center.
  // SPHERICAL is longitude,latitude,altitude.
  vtkSetMacro(OutputCoordinateSystem, int);
  vtkGetMacro(OutputCoordinateSystem, int);
  virtual void SetOutputCoordinateSystemToRectangular()
    { this->SetOutputCoordinateSystem(RECTANGULAR); }
  virtual void SetOutputCoordinateSystemToSpherical()
    { this->SetOutputCoordinateSystem(SPHERICAL); }
  
protected:
  vtkGeoSampleArcs();
  ~vtkGeoSampleArcs();

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  double GlobeRadius;
  double MaximumDistanceMeters;
  int InputCoordinateSystem;
  int OutputCoordinateSystem;
  
private:
  vtkGeoSampleArcs(const vtkGeoSampleArcs&);  // Not implemented.
  void operator=(const vtkGeoSampleArcs&);  // Not implemented.
};

#endif
