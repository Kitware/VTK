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
/**
 * @class   vtkGeoArcs
 * @brief   layout graph edges on a globe as arcs.
 *
 *
 *
*/

#ifndef vtkGeoAdaptiveArcs_h
#define vtkGeoAdaptiveArcs_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkDoubleArray;
class vtkPolyData;
class vtkRenderer;

class VTKGEOVISCORE_EXPORT vtkGeoAdaptiveArcs : public vtkPolyDataAlgorithm
{
public:
  static vtkGeoAdaptiveArcs *New();

  vtkTypeMacro(vtkGeoAdaptiveArcs,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The base radius used to determine the earth's surface.
   * Default is the earth's radius in meters.
   * TODO: Change this to take in a vtkGeoTerrain to get altitude.
   */
  vtkSetMacro(GlobeRadius, double);
  vtkGetMacro(GlobeRadius, double);
  //@}

  //@{
  /**
   * The maximum number of pixels between points on the arcs.
   * If two adjacent points are farther than the threshold,
   * the line segment will be subdivided such that each point
   * is separated by at most the threshold.
   */
  vtkSetMacro(MaximumPixelSeparation, double);
  vtkGetMacro(MaximumPixelSeparation, double);
  //@}

  //@{
  /**
   * The minimum number of pixels between points on the arcs.
   * Points closer than the threshold will be skipped until
   * a point farther than the minimum threshold is reached.
   */
  vtkSetMacro(MinimumPixelSeparation, double);
  vtkGetMacro(MinimumPixelSeparation, double);
  //@}

  //@{
  /**
   * The renderer used to estimate the number of pixels between
   * points.
   */
  virtual void SetRenderer(vtkRenderer *ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  //@}

  /**
   * Return the modified time of this object.
   */
  vtkMTimeType GetMTime();

protected:
  vtkGeoAdaptiveArcs();
  ~vtkGeoAdaptiveArcs();

  /**
   * Convert the vtkGraph into vtkPolyData.
   */
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkRenderer *Renderer;
  double GlobeRadius;
  double MaximumPixelSeparation;
  double MinimumPixelSeparation;
  vtkMTimeType LastInputMTime;
  vtkPolyData* LastInput;
  vtkDoubleArray* InputLatitude;
  vtkDoubleArray* InputLongitude;

private:
  vtkGeoAdaptiveArcs(const vtkGeoAdaptiveArcs&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoAdaptiveArcs&) VTK_DELETE_FUNCTION;
};

#endif
