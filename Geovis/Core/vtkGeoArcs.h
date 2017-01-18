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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkGeoArcs
 * @brief   Layout graph edges on a globe as arcs.
 *
 *
 * vtkGeoArcs produces arcs for each line in the input polydata. This is useful
 * for viewing lines on a sphere (e.g. the earth). The arcs may "jump" above
 * the sphere's surface using ExplodeFactor.
*/

#ifndef vtkGeoArcs_h
#define vtkGeoArcs_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKGEOVISCORE_EXPORT vtkGeoArcs : public vtkPolyDataAlgorithm
{
public:
  static vtkGeoArcs *New();

  vtkTypeMacro(vtkGeoArcs,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The base radius used to determine the earth's surface.
   * Default is the earth's radius in meters.
   */
  vtkSetMacro(GlobeRadius, double);
  vtkGetMacro(GlobeRadius, double);
  //@}

  //@{
  /**
   * Factor on which to "explode" the arcs away from the surface.
   * A value of 0.0 keeps the values on the surface.
   * Values larger than 0.0 push the arcs away from the surface by a distance
   * proportional to the distance between the points.
   * The default is 0.2.
   */
  vtkSetMacro(ExplodeFactor, double);
  vtkGetMacro(ExplodeFactor, double);
  //@}

  //@{
  /**
   * The number of subdivisions in the arc.
   * The default is 20.
   */
  vtkSetMacro(NumberOfSubdivisions, int);
  vtkGetMacro(NumberOfSubdivisions, int);
  //@}

protected:
  vtkGeoArcs();
  ~vtkGeoArcs() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  double GlobeRadius;
  double ExplodeFactor;
  int NumberOfSubdivisions;

private:
  vtkGeoArcs(const vtkGeoArcs&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoArcs&) VTK_DELETE_FUNCTION;
};

#endif
