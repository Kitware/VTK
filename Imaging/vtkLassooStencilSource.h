/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLassooStencilSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLassooStencilSource - Create a stencil from a contour
// .SECTION Description
// vtkLassooStencilSource will create an image stencil from a
// set of points that define a contour.  Its output can be
// used with vtkImageStecil or other vtk classes that apply
// a stencil to an image.
// .SECTION See Also
// vtkROIStencilSource vtkPolyDataToImageStencil
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef __vtkLassooStencilSource_h
#define __vtkLassooStencilSource_h


#include "vtkImageStencilSource.h"

class vtkImageData;
class vtkPoints;
class vtkSpline;
class vtkLSSPointMap;

class VTK_IMAGING_EXPORT vtkLassooStencilSource : public vtkImageStencilSource
{
public:
  static vtkLassooStencilSource *New();
  vtkTypeMacro(vtkLassooStencilSource, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  enum {
    POLYGON = 0,
    SPLINE = 1,
  };
//ETX

  // Description:
  // The shape to use, default is "Polygon".  The spline is a
  // cardinal spline.  Bezier splines are not yet supported.
  vtkGetMacro(Shape, int);
  vtkSetClampMacro(Shape, int, POLYGON, SPLINE);
  void SetShapeToPolygon() { this->SetShape(POLYGON); };
  void SetShapeToSpline() { this->SetShape(SPLINE); };
  virtual const char *GetShapeAsString();

  // Description:
  // The points that make up the lassoo.  The loop does not
  // have to be closed, the last point will automatically be
  // connected to the first point by a straight line segment.
  virtual void SetPoints(vtkPoints *points);
  vtkGetObjectMacro(Points, vtkPoints);

  // Description:
  // The points for a particular slice.  This will override the
  // points that were set by calling SetPoints() for the slice.
  // To clear the setting, call SetSlicePoints(slice, NULL).
  virtual void SetSlicePoints(int i, vtkPoints *points);
  virtual vtkPoints *GetSlicePoints(int i);

  // Description:
  // Remove points from all slices.
  virtual void RemoveAllSlicePoints();

  // Description:
  // Set a vtkImageData that has the Spacing, Origin, and
  // WholeExtent that will be used for the stencil.  This
  // input should be set to the image that you wish to
  // apply the stencil to.  If you use this method, then
  // any values set with the SetOutputSpacing, SetOutputOrigin,
  // and SetOutputWholeExtent methods will be ignored.
  virtual void SetInformationInput(vtkImageData*);
  vtkGetObjectMacro(InformationInput, vtkImageData);

  // Description:
  // Set the Origin to be used for the stencil.  It should be
  // set to the Origin of the image you intend to apply the
  // stencil to. The default value is (0,0,0).
  vtkSetVector3Macro(OutputOrigin, double);
  vtkGetVector3Macro(OutputOrigin, double);

  // Description:
  // Set the Spacing to be used for the stencil. It should be
  // set to the Spacing of the image you intend to apply the
  // stencil to. The default value is (1,1,1)
  vtkSetVector3Macro(OutputSpacing, double);
  vtkGetVector3Macro(OutputSpacing, double);

  // Description:
  // Set the whole extent for the stencil (anything outside
  // this extent will be considered to be "outside" the stencil).
  // If this is not set, then the stencil will always use
  // the requested UpdateExtent as the stencil extent.
  vtkSetVector6Macro(OutputWholeExtent, int);
  vtkGetVector6Macro(OutputWholeExtent, int);

  // Description:
  // Overload GetMTime() to include the timestamp on the points.
  unsigned long GetMTime();

protected:
  vtkLassooStencilSource();
  ~vtkLassooStencilSource();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);

  int Shape;
  vtkPoints *Points;
  vtkSpline *SplineX;
  vtkSpline *SplineY;
  vtkLSSPointMap *PointMap;

  vtkImageData *InformationInput;

  int OutputWholeExtent[6];
  double OutputOrigin[3];
  double OutputSpacing[3];

private:
  vtkLassooStencilSource(const vtkLassooStencilSource&);  // Not implemented.
  void operator=(const vtkLassooStencilSource&);  // Not implemented.
};

#endif
