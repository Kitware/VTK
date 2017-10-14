/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArcSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkArcSource
 * @brief   create a circular arc
 *
 *
 * vtkArcSource is a source object that creates an arc defined by two
 * endpoints and a center. The number of segments composing the polyline
 * is controlled by setting the object resolution.
 * Alternatively, one can use a better API (that does not allow for
 * inconsistent nor ambiguous inputs), using a starting point (polar vector,
 * measured from the arc's center), a normal to the plane of the arc,
 * and an angle defining the arc length.
 * Since the default API remains the original one, in order to use
 * the improved API, one must switch the UseNormalAndAngle flag to TRUE.
 *
 * The development of an improved, consistent API (based on point, normal,
 * and angle) was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France, and implemented
 * by Philippe Pebay, Kitware SAS 2012.
 *
 * @sa
 * vtkEllipseArcSource
*/

#ifndef vtkArcSource_h
#define vtkArcSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkArcSource : public vtkPolyDataAlgorithm
{
public:
  static vtkArcSource *New();
  vtkTypeMacro(vtkArcSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set position of the first end point.
   */
  vtkSetVector3Macro(Point1,double);
  vtkGetVectorMacro(Point1,double,3);
  //@}

  //@{
  /**
   * Set position of the other end point.
   */
  vtkSetVector3Macro(Point2,double);
  vtkGetVectorMacro(Point2,double,3);
  //@}

  //@{
  /**
   * Set position of the center of the circle that defines the arc.
   * Note: you can use the function vtkMath::Solve3PointCircle to
   * find the center from 3 points located on a circle.
   */
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);
  //@}

  //@{
  /**
   * Set the normal vector to the plane of the arc.
   * By default it points in the positive Z direction.
   * Note: This is only used when UseNormalAndAngle is ON.
   */
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);
  //@}

  //@{
  /**
   * Set polar vector (starting point of the arc).
   * By default it is the unit vector in the positive X direction.
   * Note: This is only used when UseNormalAndAngle is ON.
   */
  vtkSetVector3Macro(PolarVector,double);
  vtkGetVectorMacro(PolarVector,double,3);
  //@}

  //@{
  /**
   * Arc length (in degrees), beginning at the polar vector.
   * The direction is counterclockwise by default;
   * a negative value draws the arc in the clockwise direction.
   * Note: This is only used when UseNormalAndAngle is ON.
   */
  vtkSetClampMacro(Angle,double,-360.0,360.0);
  vtkGetMacro(Angle,double);
  //@}

  //@{
  /**
   * Define the number of segments of the polyline that draws the arc.
   * Note: if the resolution is set to 1 (the default value),
   * the arc is drawn as a straight line.
   */
  vtkSetClampMacro(Resolution,int,1,VTK_INT_MAX);
  vtkGetMacro(Resolution,int);
  //@}

  //@{
  /**
   * By default the arc spans the shortest angular sector point1 and point2.
   * By setting this to true, the longest angular sector is used instead
   * (i.e. the negative coterminal angle to the shortest one).
   * Note: This is only used when UseNormalAndAngle is OFF. False by default.
   */
  vtkSetMacro(Negative, bool);
  vtkGetMacro(Negative, bool);
  vtkBooleanMacro(Negative, bool);
  //@}

  //@{
  /**
   * Activate the API based on a normal vector, a starting point
   * (polar vector) and an angle defining the arc length.
   * The previous API (which remains the default) allows for inputs that are
   * inconsistent (when Point1 and Point2 are not equidistant from Center)
   * or ambiguous (when Point1, Point2, and Center are aligned).
   * Note: false by default.
   */
  vtkSetMacro(UseNormalAndAngle, bool);
  vtkGetMacro(UseNormalAndAngle, bool);
  vtkBooleanMacro(UseNormalAndAngle, bool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkArcSource(int res=1);
  ~vtkArcSource() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  double Point1[3];
  double Point2[3];
  double Center[3];
  double Normal[3];
  double PolarVector[3];
  double Angle;
  int Resolution;
  bool Negative;
  bool UseNormalAndAngle;
  int OutputPointsPrecision;

private:
  vtkArcSource(const vtkArcSource&) = delete;
  void operator=(const vtkArcSource&) = delete;
};

#endif
