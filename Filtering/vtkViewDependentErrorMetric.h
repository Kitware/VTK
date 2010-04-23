/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewDependentErrorMetric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkViewDependentErrorMetric - Objects that compute a
// screen-based error during cell tessellation.
//
// .SECTION Description
// It is a concrete error metric, based on a geometric criterium in 
// the screen space: the variation of the projected edge from a projected 
// straight line
//
// .SECTION See Also
// vtkGenericCellTessellator vtkGenericSubdivisionErrorMetric

#ifndef __vtkViewDependentErrorMetric_h
#define __vtkViewDependentErrorMetric_h

#include "vtkGenericSubdivisionErrorMetric.h"

class vtkViewport;
class vtkCoordinate;

class VTK_FILTERING_EXPORT vtkViewDependentErrorMetric : public vtkGenericSubdivisionErrorMetric
{
public:
  // Description:
  // Construct the error metric with a default squared screen-based geometric
  // accuracy measured in pixels equal to 0.25 (0.5^2).
  static vtkViewDependentErrorMetric *New();
  
  // Description:
  // Standard VTK type and error macros.
  vtkTypeMacro(vtkViewDependentErrorMetric,vtkGenericSubdivisionErrorMetric);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Return the squared screen-based geometric accurary measured in pixels.
  // An accuracy less or equal to 0.25 (0.5^2) ensures that the screen-space
  // interpolation of a mid-point matchs exactly with the projection of the
  // mid-point (a value less than 1 but greater than 0.25 is not enough,
  // because of 8-neighbors). Maybe it is useful for lower accuracy in case of
  // anti-aliasing?
  // \post positive_result: result>0
  vtkGetMacro(PixelTolerance, double);

  // Description:
  // Set the squared screen-based geometric accuracy measured in pixels.
  // Subdivision will be required if the square distance between the projection
  // of the real point and the straight line passing through the projection
  // of the vertices of the edge is greater than `value'.
  // For instance, 0.25 will give better result than 1.
  // \pre positive_value: value>0
  void SetPixelTolerance(double value);

  // Description:
  // Set/Get the renderer with `renderer' on which the error metric 
  // is based. The error metric use the active camera of the renderer.
  vtkGetObjectMacro(Viewport,vtkViewport);
  void SetViewport(vtkViewport *viewport);
  
  // Description:
  // Does the edge need to be subdivided according to the distance between
  // the line passing through its endpoints in screen space and the projection
  // of its mid point?
  // The edge is defined by its `leftPoint' and its `rightPoint'.
  // `leftPoint', `midPoint' and `rightPoint' have to be initialized before
  // calling RequiresEdgeSubdivision().
  // Their format is global coordinates, parametric coordinates and
  // point centered attributes: xyx rst abc de...
  // `alpha' is the normalized abscissa of the midpoint along the edge.
  // (close to 0 means close to the left point, close to 1 means close to the
  // right point)
  // \pre leftPoint_exists: leftPoint!=0
  // \pre midPoint_exists: midPoint!=0
  // \pre rightPoint_exists: rightPoint!=0
  // \pre clamped_alpha: alpha>0 && alpha<1
  // \pre valid_size: sizeof(leftPoint)=sizeof(midPoint)=sizeof(rightPoint)
  //          =GetAttributeCollection()->GetNumberOfPointCenteredComponents()+6
  int RequiresEdgeSubdivision(double *leftPoint, double *midPoint, double *rightPoint,
                              double alpha);
  
  // Description:
  // Return the error at the mid-point. The type of error depends on the state
  // of the concrete error metric. For instance, it can return an absolute
  // or relative error metric.
  // See RequiresEdgeSubdivision() for a description of the arguments.
  // \pre leftPoint_exists: leftPoint!=0
  // \pre midPoint_exists: midPoint!=0
  // \pre rightPoint_exists: rightPoint!=0
  // \pre clamped_alpha: alpha>0 && alpha<1
  // \pre valid_size: sizeof(leftPoint)=sizeof(midPoint)=sizeof(rightPoint)
  //          =GetAttributeCollection()->GetNumberOfPointCenteredComponents()+6
  // \post positive_result: result>=0
  double GetError(double *leftPoint, double *midPoint,
                  double *rightPoint, double alpha);

protected:
  vtkViewDependentErrorMetric();
  ~vtkViewDependentErrorMetric();
  
  // Description:
  // Square distance between a straight line (defined by points x and y)
  // and a point z. Property: if x and y are equal, the line is a point and
  // the result is the square distance between points x and z.
  double Distance2LinePoint(double x[2],
                            double y[2],
                            double z[2]);
  
  double PixelTolerance;
  vtkViewport *Viewport;
  // used to get display coordinates from world coordinates
  vtkCoordinate *Coordinate; 
  
private:
  vtkViewDependentErrorMetric(const vtkViewDependentErrorMetric&);  // Not implemented.
  void operator=(const vtkViewDependentErrorMetric&);  // Not implemented.
};

#endif
