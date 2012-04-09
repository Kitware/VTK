/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkConvexHull2D.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

// .NAME vtkConvexHull2D - Produce filled convex hulls around a set of points.
//
// .SECTION Description
// Produces a vtkPolyData comprised of a filled polygon of the convex hull
// of the input points. You may alternatively choose to output a bounding
// rectangle. Static methods are provided that calculate a (counter-clockwise)
// hull based on a set of input points.
//
// To help maintain the property of <i>guaranteed visibility</i> hulls may be
// artificially scaled by setting MinHullSizeInWorld. This is particularly
// helpful in the case that there are only one or two points as it avoids
// producing a degenerate polygon. This setting is also available as an
// argument to the static methods.
//
// Setting a vtkRenderer on the filter enables the possibility to set
// MinHullSizeInDisplay to the desired number of display pixels to cover in
// each of the x- and y-dimensions.
//
// Setting OutlineOn() additionally produces an outline of the hull on output
// port 1.
//
// .SECTION Note
// This filter operates in the x,y-plane and as such works best with an
// interactor style that does not permit camera rotation such as
// vtkInteractorStyleRubberBand2D.
//
// .SECTION Thanks
// Thanks to Colin Myers, University of Leeds for providing this implementation.

#ifndef __vtkConvexHull2D_h
#define __vtkConvexHull2D_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h" // needed for ivars

class vtkCoordinate;
class vtkPoints;
class vtkPolygon;
class vtkPolyLine;
class vtkRenderer;
class vtkTransform;
class vtkTransformPolyDataFilter;

class VTK_INFOVIS_EXPORT vtkConvexHull2D: public vtkPolyDataAlgorithm
{
public:
  static vtkConvexHull2D *New();
  vtkTypeMacro(vtkConvexHull2D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Scale the hull by the amount specified. Defaults to 1.0.
  vtkGetMacro(ScaleFactor, double);
  vtkSetMacro(ScaleFactor, double);

  // Description:
  // Produce an outline (polyline) of the hull on output port 1.
  vtkGetMacro(Outline, bool);
  vtkSetMacro(Outline, bool);
  vtkBooleanMacro(Outline, bool);

  enum HullShapes {
    BoundingRectangle = 0,
    ConvexHull
  };

  // Description:
  // Set the shape of the hull to BoundingRectangle or ConvexHull.
  vtkGetMacro(HullShape, int);
  vtkSetClampMacro(HullShape, int, 0, 1);

  // Description:
  // Set the minimum x,y-dimensions of each hull in world coordinates. Defaults
  // to 1.0. Set to 0.0 to disable.
  vtkSetClampMacro(MinHullSizeInWorld, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MinHullSizeInWorld, double);


  // Description:
  // Set the minimum x,y-dimensions of each hull in pixels. You must also set a
  // vtkRenderer. Defaults to 1. Set to 0 to disable.
  vtkSetClampMacro(MinHullSizeInDisplay, int, 0, VTK_INT_MAX);
  vtkGetMacro(MinHullSizeInDisplay, int);

  // Description:
  // Renderer needed for MinHullSizeInDisplay calculation. Not reference counted.
  void SetRenderer(vtkRenderer* renderer);
  vtkRenderer* GetRenderer();

  // Description:
  // The modified time of this filter.
  virtual unsigned long GetMTime();

  // Description:
  // Convenience methods to calculate a convex hull from a set of vtkPointS.
  static void CalculateBoundingRectangle(vtkPoints* inPoints,
    vtkPoints* outPoints, double minimumHullSize=1.0);
  static void CalculateConvexHull(vtkPoints* inPoints, vtkPoints* outPoints,
    double minimumHullSize=1.0);

protected:
  vtkConvexHull2D();
  ~vtkConvexHull2D();

  // Description:
  // This is called by the superclass. This is the method you should override.
  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);

private:
  vtkConvexHull2D(const vtkConvexHull2D&); // Not implemented.
  void operator=(const vtkConvexHull2D&); // Not implemented.

  void ResizeHullToMinimumInDisplay(vtkPolyData* hullPolyData);

  double ScaleFactor;
  bool Outline;
  int HullShape;
  int MinHullSizeInDisplay;
  double MinHullSizeInWorld;
  vtkRenderer* Renderer;

  vtkSmartPointer<vtkCoordinate> Coordinate;
  vtkSmartPointer<vtkTransform> Transform;
  vtkSmartPointer<vtkTransform> OutputTransform;
  vtkSmartPointer<vtkTransformPolyDataFilter> OutputTransformFilter;
  vtkSmartPointer<vtkPolyLine> OutlineSource;
  vtkSmartPointer<vtkPolygon> HullSource;
};

#endif // __vtkConvexHull2D_h
