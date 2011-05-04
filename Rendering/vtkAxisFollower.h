/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkAxisFollower_h
#define __vtkAxisFollower_h

#include "vtkFollower.h"

// Forward declarations.
class vtkAxisActor;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkAxisFollower : public vtkFollower
{
public:
 vtkTypeMacro(vtkAxisFollower,vtkFollower);
 virtual void PrintSelf(ostream& os, vtkIndent indent);

 // Description:
 // Creates a follower with no camera set
 static vtkAxisFollower *New();

//BTX
 // Description:
 // Set axis that needs to be followed.
 inline void SetFollowAxis(vtkAxisActor *axis)
   {
   if(!axis)
     {
     vtkErrorMacro("Axis is invalid or null\n");
     return;
     }
   this->Axis = axis;

   this->Modified();
   }

 // Get axis that is being followed.
 inline vtkAxisActor* GetFollowAxis()
   {
   return this->Axis;
   }
//ETX

 // Description:
 // Set/Get state of auto center mode where additional
 // translation will be added to make sure the underlying
 // geometry has its pivot point at the center of its bounds.
 vtkSetMacro(AutoCenter, int);
 vtkGetMacro(AutoCenter, int);
 vtkBooleanMacro(AutoCenter, int);

 // Description:
 // Enable / disable use of LOD. If enabled the actor
 // will not be visible at a certain distance from the
 // camera (default is 0.80 * clipRange[1])
 //
 vtkSetMacro(EnableLOD, int);
 vtkGetMacro(EnableLOD, int);

 // Description:
 // Set LOD factor (0.0 - 1.0), default is 0.80. This determines at what fraction
 // of camera far clip distance, we need to make this actor not visible.
 vtkSetClampMacro(LODFactor, double, 0.0, 1.0);
 vtkGetMacro(LODFactor, double);

 // Description:
 // Set/Get the desired screen offset from the axis.
 vtkSetMacro(ScreenOffset, double);
 vtkGetMacro(ScreenOffset, double);

 // Description:
 // This causes the actor to be rendered. It in turn will render the actor's
 // property, texture map and then mapper. If a property hasn't been
 // assigned, then the actor will create one automatically.
 virtual int RenderOpaqueGeometry(vtkViewport *viewport);
 virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
 virtual void Render(vtkRenderer *ren);

 // Description:
 // Does this prop have some translucent polygonal geometry?
 virtual int HasTranslucentPolygonalGeometry();

 // Description:
 // Release any graphics resources associated with this vtkProp3DFollower.
 virtual void ReleaseGraphicsResources(vtkWindow*);

 // Description:
 // Generate the matrix based on ivars. This method overloads its superclasses
 // ComputeMatrix() method due to the special vtkFollower matrix operations.
 virtual void ComputeTransformMatrix(vtkRenderer *ren);

 // Description:
 // Shallow copy of a follower. Overloads the virtual vtkProp method.
 void ShallowCopy(vtkProp *prop);

protected:
 vtkAxisFollower();
 ~vtkAxisFollower();

 void CalculateOrthogonalVectors(double *Rx, double *Ry, double *Rz,
                                 vtkAxisActor *axis1, double *dop,
                                 vtkRenderer *ren);

 double AutoScale(vtkViewport *viewport, vtkCamera * camera,
                  double screenOffset, double position[3]);

 void ComputeRotationAndTranlation(vtkRenderer *ren, double translation[3],
                                   double Rx[3], double Ry[3], double Rz[3],
                                   vtkAxisActor *axis);

 // \NOTE: Not used as of now.
 void ComputerAutoCenterTranslation(const double& autoScaleFactor,
                                    double translation[3]);


 int  EvaluateVisibility();

 int          AutoCenter;

 int          EnableLOD;

 double       LODFactor;

 double       ScreenOffset;

 vtkAxisActor *Axis;


private:

 int AxisPointingLeft;

 vtkAxisFollower(const vtkAxisFollower&);  // Not implemented.
 void operator =(const vtkAxisFollower&);  // Not implemented.

 // hide the two parameter Render() method from the user and the compiler.
 virtual void Render(vtkRenderer *, vtkMapper *) {};

 //Internal matrices to avoid New/Delete for performance reasons
 vtkMatrix4x4 *InternalMatrix;

};

#endif // __vtkAxisFollower_h
