/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCapsuleSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.

  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Program: Bender
  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

/**
 * @class   vtkCapsuleSource
 * @brief   Generate a capsule centered at the origin
 *
 * vtkCapsuleSource creates a capsule (represented by polygons) of specified
 * radius centered at the origin. The resolution (polygonal discretization) in
 * both the latitude (phi) and longitude (theta) directions can be specified as
 * well as the length of the capsule cylinder (CylinderLength). By default, the
 * surface tessellation of the sphere uses triangles; however you can set
 * LatLongTessellation to produce a tessellation using quadrilaterals (except
 * at the poles of the capsule).
 */

#ifndef vtkCapsuleSource_h
#define vtkCapsuleSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkSphereSource.h" // For VTK_MAX_SPHERE_RESOLUTION

class VTKFILTERSSOURCES_EXPORT vtkCapsuleSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCapsuleSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a capsule with radius 0.5 and resolution 8 in both the Phi and Theta directions and a
   * cylinder of length 1.0.
   */
  static vtkCapsuleSource* New();

  //@{
  /**
   * Set/get the radius of the capsule. The initial value is 0.5.
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set/get the center of the capsule. The initial value is (0.0, 0.0, 0.0).
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set/get the length of the cylinder. The initial value is 1.0.
   */
  vtkSetClampMacro(CylinderLength, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(CylinderLength, double);
  //@}

  //@{
  /**
   * Set/get the number of points in the longitude direction for the spheres. The initial value
   * is 8.
   */
  vtkSetClampMacro(ThetaResolution, int, 8, VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(ThetaResolution, int);
  //@}

  //@{
  /**
   * Set/get the number of points in the latitude direction for the spheres. The initial value is 8.
   */
  vtkSetClampMacro(PhiResolution, int, 8, VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(PhiResolution, int);
  //@}

  //@{
  /**
   * Cause the spheres to be tessellated with edges along the latitude and longitude lines. If off,
   * triangles are generated at non-polar regions, which results in edges that are not parallel to
   * latitude and longitude lines. If on, quadrilaterals are generated everywhere except at the
   * poles. This can be useful for generating wireframe spheres with natural latitude and longitude
   * lines.
   */
  vtkSetMacro(LatLongTessellation, int);
  vtkGetMacro(LatLongTessellation, int);
  vtkBooleanMacro(LatLongTessellation, int);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  vtkCapsuleSource(int res = 8);
  ~vtkCapsuleSource() override {}

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double Radius;
  double Center[3];
  int ThetaResolution;
  int PhiResolution;
  int LatLongTessellation;
  int FillPoles;
  double CylinderLength;
  int OutputPointsPrecision;

private:
  vtkCapsuleSource(const vtkCapsuleSource&) = delete;
  void operator=(const vtkCapsuleSource&) = delete;
};

#endif
