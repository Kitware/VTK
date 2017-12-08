/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSphereSource
 * @brief   create a polygonal sphere centered at the origin
 *
 * vtkSphereSource creates a sphere (represented by polygons) of specified
 * radius centered at the origin. The resolution (polygonal discretization)
 * in both the latitude (phi) and longitude (theta) directions can be
 * specified. It also is possible to create partial spheres by specifying
 * maximum phi and theta angles. By default, the surface tessellation of
 * the sphere uses triangles; however you can set LatLongTessellation to
 * produce a tessellation using quadrilaterals.
 * @warning
 * Resolution means the number of latitude or longitude lines for a complete
 * sphere. If you create partial spheres the number of latitude/longitude
 * lines may be off by one.
*/

#ifndef vtkSphereSource_h
#define vtkSphereSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

class VTKFILTERSSOURCES_EXPORT vtkSphereSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSphereSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct sphere with radius=0.5 and default resolution 8 in both Phi
   * and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
   */
  static vtkSphereSource *New();

  //@{
  /**
   * Set radius of sphere. Default is .5.
   */
  vtkSetClampMacro(Radius,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Set the center of the sphere. Default is 0,0,0.
   */
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);
  //@}

  //@{
  /**
   * Set the number of points in the longitude direction (ranging from
   * StartTheta to EndTheta).
   */
  vtkSetClampMacro(ThetaResolution,int,3,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(ThetaResolution,int);
  //@}

  //@{
  /**
   * Set the number of points in the latitude direction (ranging
   * from StartPhi to EndPhi).
   */
  vtkSetClampMacro(PhiResolution,int,3,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(PhiResolution,int);
  //@}

  //@{
  /**
   * Set the starting longitude angle. By default StartTheta=0 degrees.
   */
  vtkSetClampMacro(StartTheta,double,0.0,360.0);
  vtkGetMacro(StartTheta,double);
  //@}

  //@{
  /**
   * Set the ending longitude angle. By default EndTheta=360 degrees.
   */
  vtkSetClampMacro(EndTheta,double,0.0,360.0);
  vtkGetMacro(EndTheta,double);
  //@}

  //@{
  /**
   * Set the starting latitude angle (0 is at north pole). By default
   * StartPhi=0 degrees.
   */
  vtkSetClampMacro(StartPhi,double,0.0,360.0);
  vtkGetMacro(StartPhi,double);
  //@}

  //@{
  /**
   * Set the ending latitude angle. By default EndPhi=180 degrees.
   */
  vtkSetClampMacro(EndPhi,double,0.0,360.0);
  vtkGetMacro(EndPhi,double);
  //@}

  //@{
  /**
   * Cause the sphere to be tessellated with edges along the latitude
   * and longitude lines. If off, triangles are generated at non-polar
   * regions, which results in edges that are not parallel to latitude and
   * longitude lines. If on, quadrilaterals are generated everywhere
   * except at the poles. This can be useful for generating a wireframe
   * sphere with natural latitude and longitude lines.
   */
  vtkSetMacro(LatLongTessellation,vtkTypeBool);
  vtkGetMacro(LatLongTessellation,vtkTypeBool);
  vtkBooleanMacro(LatLongTessellation,vtkTypeBool);
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
  vtkSphereSource(int res=8);
  ~vtkSphereSource() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  double Radius;
  double Center[3];
  int ThetaResolution;
  int PhiResolution;
  double StartTheta;
  double EndTheta;
  double StartPhi;
  double EndPhi;
  vtkTypeBool LatLongTessellation;
  int OutputPointsPrecision;

private:
  vtkSphereSource(const vtkSphereSource&) = delete;
  void operator=(const vtkSphereSource&) = delete;
};

#endif
