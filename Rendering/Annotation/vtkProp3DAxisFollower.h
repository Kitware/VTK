/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DAxisFollower.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProp3DAxisFollower - a subclass of vtkProp3DFollower that ensures
// that data is always parallel to the axis defined by a vtkAxisActor.
// .SECTION Description
// vtkProp3DAxisFollower is a subclass of vtkProp3DFollower that always follows
// its specified axis. More specifically it will not change its position or
// scale, but it will continually update its orientation so that it is aligned
// with the axis and facing at angle to the camera to provide maximum visibilty.
// This is typically used for text labels for 3d plots.
// .SECTION see also
// vtkFollower vtkAxisFollower vtkProp3DFollower

#ifndef vtkProp3DAxisFollower_h
#define vtkProp3DAxisFollower_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkProp3DFollower.h"
#include "vtkWeakPointer.h" // For vtkWeakPointer

class vtkAxisActor;
class vtkViewport;

class VTKRENDERINGANNOTATION_EXPORT vtkProp3DAxisFollower
  : public vtkProp3DFollower
{
 public:
  // Description:
  // Creates a follower with no camera set.
  static vtkProp3DAxisFollower *New();

  // Description:
  // Standard VTK methods for type and printing.
  vtkTypeMacro(vtkProp3DAxisFollower,vtkProp3DFollower);
  void PrintSelf(ostream& os, vtkIndent indent);

 // Description:
 // Set axis that needs to be followed.
 virtual void SetAxis(vtkAxisActor*);
 virtual vtkAxisActor* GetAxis();

 // Description:
 // Set/Get state of auto center mode where additional
 // translation will be added to make sure the underlying
 // geometry has its pivot point at the center of its bounds.
 vtkSetMacro(AutoCenter, int);
 vtkGetMacro(AutoCenter, int);
 vtkBooleanMacro(AutoCenter, int);

 // Description:
 // Enable / disable use of distance based LOD. If enabled the actor
 // will not be visible at a certain distance from the camera.
 // Default is false.
 vtkSetMacro(EnableDistanceLOD, int);
 vtkGetMacro(EnableDistanceLOD, int);

 // Description:
 // Set distance LOD threshold (0.0 - 1.0).This determines at what fraction
 // of camera far clip range, actor is not visible.
 // Default is 0.80.
 vtkSetClampMacro(DistanceLODThreshold, double, 0.0, 1.0);
 vtkGetMacro(DistanceLODThreshold, double);

 // Description:
 // Enable / disable use of view angle based LOD. If enabled the actor
 // will not be visible at a certain view angle.
 // Default is true.
 vtkSetMacro(EnableViewAngleLOD, int);
 vtkGetMacro(EnableViewAngleLOD, int);

 // Description:
 // Set view angle LOD threshold (0.0 - 1.0).This determines at what view
 // angle to geometry will make the geometry not visibile.
 // Default is 0.34.
 vtkSetClampMacro(ViewAngleLODThreshold, double, 0.0, 1.0);
 vtkGetMacro(ViewAngleLODThreshold, double);

 // Description:
 // Set/Get the desired screen offset from the axis.
 vtkSetMacro(ScreenOffset, double);
 vtkGetMacro(ScreenOffset, double);

  // Description:
  // Generate the matrix based on ivars. This method overloads its superclasses
  // ComputeMatrix() method due to the special vtkProp3DAxisFollower matrix operations.
  virtual void ComputeMatrix();

  // Description:
  // Shallow copy of a follower. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

 // Description:
 // Calculate scale factor to maintain same size of a object
 // on the screen.
 static double AutoScale(vtkViewport *viewport, vtkCamera * camera,
                         double screenSize, double position[3]);

  // Description:
  // This causes the actor to be rendered. It in turn will render the actor's
  // property, texture map and then mapper. If a property hasn't been
  // assigned, then the actor will create one automatically.
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  virtual int RenderVolumetricGeometry(vtkViewport *viewport);

  virtual void SetViewport(vtkViewport* viewport);
  virtual vtkViewport* GetViewport();

protected:
  vtkProp3DAxisFollower();
  ~vtkProp3DAxisFollower();

 void CalculateOrthogonalVectors(double Rx[3], double Ry[3], double Rz[3],
                                 vtkAxisActor *axis1, double *dop,
                                 vtkViewport *ren);


 void ComputeRotationAndTranlation(vtkViewport *ren, double translation[3],
                                   double Rx[3], double Ry[3], double Rz[3],
                                   vtkAxisActor *axis);

 // \NOTE: Not used as of now.
 void ComputerAutoCenterTranslation(const double& autoScaleFactor,
                                    double translation[3]);


  int  TestDistanceVisibility();
  void ExecuteViewAngleVisibility(double normal[3]);

  bool IsTextUpsideDown(double* a, double* b);

  int          AutoCenter;

  int          EnableDistanceLOD;
  double       DistanceLODThreshold;

  int          EnableViewAngleLOD;
  double       ViewAngleLODThreshold;

  double       ScreenOffset;

  vtkWeakPointer<vtkAxisActor> Axis;
  vtkWeakPointer<vtkViewport> Viewport;
private:
  vtkProp3DAxisFollower(const vtkProp3DAxisFollower&);  // Not implemented.
  void operator=(const vtkProp3DAxisFollower&);  // Not implemented.

  int TextUpsideDown;
  int VisibleAtCurrentViewAngle;

};

#endif
