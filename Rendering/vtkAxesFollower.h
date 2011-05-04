
#ifndef __vtkAxesFollower_h
#define __vtkAxesFollower_h

#include "vtkFollower.h"

// Forward declarations.
class vtkAxisActor;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkAxesFollower : public vtkFollower
{
public:
 vtkTypeMacro(vtkAxesFollower,vtkFollower);
 virtual void PrintSelf(ostream& os, vtkIndent indent);

 // Description:
 // Creates a follower with no camera set
 static vtkAxesFollower *New();

//BTX
 // Description:
 // Set three orthogonal axes one of which needs to be followed.
 inline void SetAxes(vtkAxisActor *xAxis,
                     vtkAxisActor *yAxis,
                     vtkAxisActor *zAxis)
   {
   if(!xAxis || !yAxis || !zAxis)
     {
     vtkErrorMacro("One of the axis is invalid or null\n");
     return;
     }
   this->XAxis = xAxis;
   this->YAxis = yAxis;
   this->ZAxis = zAxis;

   this->Modified();
   }
//ETX

 // Description:
 // Set/Get the axis that needs to be followed.
 vtkSetClampMacro(FollowAxes, int, 0, 2);
 vtkGetMacro(FollowAxes, int);

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
 // camera (currently usind threhold distance = 0.80 * clipRange[1])
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
 vtkAxesFollower();
 ~vtkAxesFollower();

 void CalculateOrthogonalVectors(double *Rx, double *Ry, double *Rz,
                                 vtkAxisActor *axis1, vtkAxisActor *axis2,
                                 vtkRenderer *ren);

 double AutoScale(vtkViewport *viewport, vtkCamera * camera,
                  double screenOffset, double position[3]);

 void ComputeRotationAndTranlation(vtkRenderer *ren, double translation[3],
                                   double Rx[3], double Ry[3], double Rz[3],
                                   vtkAxisActor *xAxis, vtkAxisActor *orthoAxis1,
                                   vtkAxisActor *orthoAxis2);

 // \NOTE: Not used as of now.
 void ComputerAutoCenterTranslation(const double& autoScaleFactor,
                                    double translation[3]);


 int  EvaluateVisibility();

 int          FollowAxes;

 int          AutoCenter;

 int          EnableLOD;

 double       LODFactor;

 double       ScreenOffset;

 vtkAxisActor *XAxis;
 vtkAxisActor *YAxis;
 vtkAxisActor *ZAxis;

private:

 int AxisPointingLeft;

 vtkAxesFollower(const vtkAxesFollower&);  // Not implemented.
 void operator=(const vtkAxesFollower&);  // Not implemented.

 // hide the two parameter Render() method from the user and the compiler.
 virtual void Render(vtkRenderer *, vtkMapper *) {};

 //Internal matrices to avoid New/Delete for performance reasons
 vtkMatrix4x4 *InternalMatrix;

};

#endif // __vtkAxesFollower_h
