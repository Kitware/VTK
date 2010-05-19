/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAdaptiveArcs.h

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
// .NAME vtkGeoArcs - layout graph edges on a globe as arcs.
//
// .SECTION Description

// .SECTION Thanks

#ifndef __vtkGeoAdaptiveArcs_h
#define __vtkGeoAdaptiveArcs_h

#include "vtkPolyDataAlgorithm.h"

class vtkDoubleArray;
class vtkPolyData;
class vtkRenderer;

class VTK_GEOVIS_EXPORT vtkGeoAdaptiveArcs : public vtkPolyDataAlgorithm 
{
public:
  static vtkGeoAdaptiveArcs *New();

  vtkTypeMacro(vtkGeoAdaptiveArcs,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The base radius used to determine the earth's surface.
  // Default is the earth's radius in meters.
  // TODO: Change this to take in a vtkGeoTerrain to get altitude.
  vtkSetMacro(GlobeRadius, double);
  vtkGetMacro(GlobeRadius, double);
  
  // Description:
  // The maximum number of pixels between points on the arcs.
  // If two adjacent points are farther than the threshold,
  // the line segment will be subdivided such that each point
  // is separated by at most the threshold.
  vtkSetMacro(MaximumPixelSeparation, double);
  vtkGetMacro(MaximumPixelSeparation, double);
  
  // Description:
  // The minimum number of pixels between points on the arcs.
  // Points closer than the threshold will be skipped until
  // a point farther than the minimum threshold is reached.
  vtkSetMacro(MinimumPixelSeparation, double);
  vtkGetMacro(MinimumPixelSeparation, double);
  
  // Description:
  // The renderer used to estimate the number of pixels between
  // points.
  virtual void SetRenderer(vtkRenderer *ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  
  // Description:
  // Return the modified time of this object.
  virtual unsigned long GetMTime();
  
protected:
  vtkGeoAdaptiveArcs();
  ~vtkGeoAdaptiveArcs();

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  vtkRenderer *Renderer;
  double GlobeRadius;
  double MaximumPixelSeparation;
  double MinimumPixelSeparation;
  unsigned long LastInputMTime;
  vtkPolyData* LastInput;
  vtkDoubleArray* InputLatitude;
  vtkDoubleArray* InputLongitude;
  
private:
  vtkGeoAdaptiveArcs(const vtkGeoAdaptiveArcs&);  // Not implemented.
  void operator=(const vtkGeoAdaptiveArcs&);  // Not implemented.
};

#endif
