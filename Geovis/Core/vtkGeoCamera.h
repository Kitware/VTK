/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoCamera.h

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
 * @class   vtkGeoCamera
 * @brief   Geo interface to a camera.
 *
 *
 * I wanted to hide the normal vtkCamera API
 * so I did not make this a subclass.  The camera is a helper object.
 * You can get a pointer to the camera, but it should be treated like
 * a const.
 *
 * View up of the camera is restricted so there is no roll relative
 * to the earth.  I am going to keep view up of the camera orthogonalized to
 * avoid the singularity that exists when the camera is pointing straight down.
 * In this case, view up is the same as heading.
 *
 * The state of the view is specified by the vector:
 * (Longitude,Latitude,Distance,Heading,Tilt).
 *   Longitude in degrees: (-180->180)
 *     Relative to absolute coordinates.
 *   Latitude in degrees: (-90->90)
 *     Relative to Longitude.
 *   Distance in Meters
 *     Relative to Longitude and Latitude.
 *     above sea level   ???? should we make this from center of earth ????
 *                       ???? what about equatorial bulge ????
 *   Heading in degrees:  (-180->180)
 *     Relative to Logitude and Latitude.
 *     0 is north.
 *     90 is east.       ???? what is the standard ????
 *     180 is south.
 *     -90 is west.
 *   Tilt in degrees: (0->90)
 *     Relative to Longitude, Latitude, Distance and Heading.
 *
 *
 * Transformation:
 *   Post concatenate.
 *   All rotations use right hand rule and are around (0,0,0) (earth center).
 *   (0,0,0,0,0) is this rectilinear point (0, EarthRadius, 0)
 *               pointing (0,0,1), view up (0,1,0).
 *
 *   Rotate Tilt       around x axis,
 *   Rotate Heading    around -y axis Center,
 *   Translate EarthRadius in y direction.
 *   Rotate Latitude   around x axis by Latitude,
 *   Rotate Longitude  around z axis (earth axis),
 *
 *
 * @sa
 * vtkGeoInteractorStyle vtkCamera
*/

#ifndef vtkGeoCamera_h
#define vtkGeoCamera_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for SP

class vtkCamera;
class vtkGeoTerrainNode;
class vtkTransform;

class VTKGEOVISCORE_EXPORT vtkGeoCamera : public vtkObject
{
public:
  static vtkGeoCamera *New();
  vtkTypeMacro(vtkGeoCamera, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the world position without the origin shift.
   */
  vtkGetVector3Macro(Position, double);
  //@}

  //@{
  /**
   * Longitude is in degrees: (-180->180)
   * Relative to absolute coordinates.
   * Rotate Longitude  around z axis (earth axis),
   */
  void SetLongitude(double longitude);
  vtkGetMacro(Longitude,double);
  //@}

  //@{
  /**
   * Latitude is in degrees: (-90->90)
   * Relative to Longitude.
   * Rotate Latitude   around x axis by Latitude,
   */
  void SetLatitude(double latitude);
  vtkGetMacro(Latitude,double);
  //@}

  //@{
  /**
   * Distance is in Meters
   * Relative to Longitude and Latitude.
   * above sea level   ???? should we make this from center of earth ????
   * ???? what about equatorial bulge ????
   */
  void SetDistance(double Distance);
  vtkGetMacro(Distance,double);
  //@}

  //@{
  /**
   * Heading is in degrees:  (-180->180)
   * Relative to Logitude and Latitude.
   * 0 is north.
   * 90 is east.       ???? what is the standard ????
   * 180 is south.
   * -90 is west.
   * Rotate Heading    around -y axis Center,
   */
  void SetHeading(double heading);
  vtkGetMacro(Heading,double);
  //@}

  //@{
  /**
   * Tilt is also know as pitch.
   * Tilt is in degrees: (0->90)
   * Relative to Longitude, Latitude, and Heading.
   * Rotate Tilt       around x axis,
   */
  void SetTilt(double tilt);
  vtkGetMacro(Tilt,double);
  //@}

  /**
   * This vtk camera is updated to match this geo cameras state.
   * It should be treated as a const and should not be modified.
   */
  vtkCamera* GetVTKCamera();

  /**
   * We precompute some values to speed up update of the terrain.
   * Unfortunately, they have to be manually/explicitly updated
   * when the camera or renderer size changes.
   */
  void InitializeNodeAnalysis(int rendererSize[2]);

  /**
   * This method estimates how much of the view is covered by the sphere.
   * Returns a value from 0 to 1.
   */
  double GetNodeCoverage(vtkGeoTerrainNode* node);

  //@{
  /**
   * Whether to lock the heading a particular value,
   * or to let the heading "roam free" when performing
   * latitude and longitude changes.
   */
  vtkGetMacro(LockHeading, bool);
  vtkSetMacro(LockHeading, bool);
  vtkBooleanMacro(LockHeading, bool);
  //@}

  //@{
  /**
   * This point is shifted to 0,0,0 to avoid openGL issues.
   */
  void SetOriginLatitude(double oLat);
  vtkGetMacro(OriginLatitude, double);
  void SetOriginLongitude(double oLat);
  vtkGetMacro(OriginLongitude, double);
  //@}

  //@{
  /**
   * Get the rectilinear cooridinate location of the origin.
   * This is used to shift the terrain points.
   */
  vtkGetVector3Macro(Origin, double);
  void SetOrigin( double ox, double oy, double oz ) {
    this->Origin[0] = ox; this->Origin[1] = oy; this->Origin[2] = oz;
    this->UpdateVTKCamera();
  }
  //@}

protected:
  vtkGeoCamera();
  ~vtkGeoCamera() VTK_OVERRIDE;

  void UpdateVTKCamera();
  void UpdateAngleRanges();

  vtkSmartPointer<vtkCamera> VTKCamera;
  vtkSmartPointer<vtkTransform> Transform;

  // This point is shifted to 0,0,0 to avoid openGL issues.
  double OriginLatitude;
  double OriginLongitude;
  double Origin[3];
  void ComputeRectilinearOrigin();

  double Longitude;
  double Latitude;
  double Distance;
  double Heading;
  double Tilt;
  bool   LockHeading;

  // Values precomputed to make updating terrain mode efficient.
  // The vislibility of many terrain nodes is analyzed every render.
  double ForwardNormal[3];
  double RightNormal[3];
  double UpNormal[3];
  double Aspect[2];

  // Frustum planes is better than other options for culling spheres.
  double LeftPlaneNormal[3];
  double RightPlaneNormal[3];
  double DownPlaneNormal[3];
  double UpPlaneNormal[3];

  double Position[3];

private:
  vtkGeoCamera(const vtkGeoCamera&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoCamera&) VTK_DELETE_FUNCTION;
};

#endif
