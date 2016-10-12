/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEllipseArcSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEllipseArcSource
 * @brief   create an elliptical arc
 *
 *
 * vtkEllipseArcSource is a source object that creates an elliptical arc
 * defined by a normal, a center and the major radius vector.
 * You can define an angle to draw only a section of the ellipse. The number of
 * segments composing the polyline is controlled by setting the object
 * resolution.
 *
 * @sa
 * vtkArcSource
*/

#ifndef vtkEllipseArcSource_h
#define vtkEllipseArcSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkEllipseArcSource : public vtkPolyDataAlgorithm
{
public:
  static vtkEllipseArcSource *New();
  vtkTypeMacro(vtkEllipseArcSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set position of the center of the ellipse that define the arc.
   * Default is 0, 0, 0.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set normal vector. Represents the plane in which the ellipse will be drawn.
   * Default 0, 0, 1.
   */
  vtkSetVector3Macro(Normal, double);
  vtkGetVectorMacro(Normal, double, 3);
  //@}

  //@{
  /**
   * Set Major Radius Vector. It defines the origin of polar angle and the major
   * radius size.
   * Default is 1, 0, 0.
   */
  vtkSetVector3Macro(MajorRadiusVector, double);
  vtkGetVectorMacro(MajorRadiusVector, double, 3);
  //@}

  //@{
  /**
   * Set the start angle. The angle where the plot begins.
   * Default is 0.
   */
  vtkSetClampMacro(StartAngle, double, -360.0, 360.0);
  vtkGetMacro(StartAngle, double);
  //@}

  //@{
  /**
   * Angular sector occupied by the arc, beginning at Start Angle
   * Default is 90.
   */
  vtkSetClampMacro(SegmentAngle, double, 0.0, 360.0);
  vtkGetMacro(SegmentAngle, double);
  //@}

  //@{
  /**
   * Divide line into resolution number of pieces.
   * Note: if Resolution is set to 1 the arc is a
   * straight line. Default is 100.
   */
  vtkSetClampMacro(Resolution, int, 1, VTK_INT_MAX);
  vtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point,
   * This is the default.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

  //@{
  /**
   * Set the ratio of the ellipse, i.e. the ratio b/a _ b: minor radius;
   * a: major radius
   * default is 1.
   */
  vtkSetClampMacro(Ratio, double, 0.001, 100.0);
  vtkGetMacro(Ratio, double);
  //@}

protected:
  vtkEllipseArcSource();
  ~vtkEllipseArcSource() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) VTK_OVERRIDE;

  double Center[3];
  double Normal[3];
  double MajorRadiusVector[3];
  double StartAngle;
  double SegmentAngle;
  int Resolution;
  double Ratio;
  int OutputPointsPrecision;

private:
  vtkEllipseArcSource(const vtkEllipseArcSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEllipseArcSource&) VTK_DELETE_FUNCTION;
};

#endif
