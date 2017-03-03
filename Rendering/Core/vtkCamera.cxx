/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCamera.h"

#include "vtkMath.h"
#include "vtkTimeStamp.h"
#include "vtkObjectFactory.h"
#include "vtkPerspectiveTransform.h"
#include "vtkTransform.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderer.h"

#include <cassert>
#include <cmath>

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkAbstractObjectFactoryNewMacro(vtkCamera)

vtkCxxSetObjectMacro(vtkCamera, EyeTransformMatrix, vtkMatrix4x4);
vtkCxxSetObjectMacro(vtkCamera, ModelTransformMatrix, vtkMatrix4x4);
vtkCxxSetObjectMacro(vtkCamera, ExplicitProjectionTransformMatrix, vtkMatrix4x4)

//-----------------------------------------------------------------------------
class vtkCameraCallbackCommand : public vtkCommand
{
public:
  static vtkCameraCallbackCommand *New()
    { return new vtkCameraCallbackCommand; }
  vtkCamera *Self;
  void Execute(vtkObject *, unsigned long, void *) VTK_OVERRIDE
  {
      if (this->Self)
      {
        this->Self->Modified();
        this->Self->ComputeViewTransform();
        this->Self->ComputeDistance();
        this->Self->ComputeCameraLightTransform();
      }
  }
protected:
  vtkCameraCallbackCommand() { this->Self = NULL; }
  ~vtkCameraCallbackCommand() VTK_OVERRIDE {}
};

//----------------------------------------------------------------------------
// Construct camera instance with its focal point at the origin,
// and position=(0,0,1). The view up is along the y-axis,
// view angle is 30 degrees, and the clipping range is (.1,1000).
vtkCamera::vtkCamera()
{
  this->FocalPoint[0] = 0.0;
  this->FocalPoint[1] = 0.0;
  this->FocalPoint[2] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 1.0;

  this->ViewUp[0] = 0.0;
  this->ViewUp[1] = 1.0;
  this->ViewUp[2] = 0.0;

  this->DirectionOfProjection[0] = 0.0;
  this->DirectionOfProjection[1] = 0.0;
  this->DirectionOfProjection[2] = 0.0;

  this->ViewAngle = 30.0;
  this->UseHorizontalViewAngle = 0;

  this->UseOffAxisProjection  = 0;

  this->ScreenBottomLeft[0] = -0.5;
  this->ScreenBottomLeft[1] = -0.5;
  this->ScreenBottomLeft[2] = -0.5;

  this->ScreenBottomRight[0] =  0.5;
  this->ScreenBottomRight[1] = -0.5;
  this->ScreenBottomRight[2] = -0.5;

  this->ScreenTopRight[0] =  0.5;
  this->ScreenTopRight[1] =  0.5;
  this->ScreenTopRight[2] = -0.5;

  this->EyeSeparation = 0.06;

  this->WorldToScreenMatrix = vtkMatrix4x4::New();
  this->WorldToScreenMatrix->Identity();

  this->EyeTransformMatrix = vtkMatrix4x4::New();
  this->EyeTransformMatrix->Identity();

  this->ModelTransformMatrix = vtkMatrix4x4::New();
  this->ModelTransformMatrix->Identity();

  this->ClippingRange[0] = 0.01;
  this->ClippingRange[1] = 1000.01;
  this->Thickness = 1000.0;

  this->ParallelProjection = 0;
  this->ParallelScale = 1.0;

  this->EyeAngle = 2.0;
  this->Stereo = 0;
  this->LeftEye = 1;

  this->WindowCenter[0] = 0.0;
  this->WindowCenter[1] = 0.0;

  this->ViewShear[0] = 0.0;
  this->ViewShear[1] = 0.0;
  this->ViewShear[2] = 1.0;

  this->FocalDisk = 1.0;

  this->Transform = vtkPerspectiveTransform::New();
  this->ViewTransform = vtkTransform::New();
  this->ProjectionTransform = vtkPerspectiveTransform::New();
  this->CameraLightTransform = vtkTransform::New();
  this->ModelViewTransform = vtkTransform::New();
  this->ExplicitProjectionTransformMatrix = NULL;
  this->UseExplicitProjectionTransformMatrix = false;
  this->UserTransform = NULL;
  this->UserViewTransform = NULL;
  this->UserViewTransformCallbackCommand = NULL;

  // initialize the ViewTransform
  this->ComputeViewTransform();
  this->ComputeDistance();
  this->ComputeCameraLightTransform();

  this->FreezeFocalPoint = false;
  this->UseScissor = false;
}

//----------------------------------------------------------------------------
vtkCamera::~vtkCamera()
{
  this->WorldToScreenMatrix->Delete();
  this->WorldToScreenMatrix = NULL;

  this->EyeTransformMatrix->Delete();
  this->EyeTransformMatrix = NULL;

  this->ModelTransformMatrix->Delete();
  this->ModelTransformMatrix = NULL;

  this->Transform->Delete();
  this->ViewTransform->Delete();
  this->ProjectionTransform->Delete();
  this->CameraLightTransform->Delete();
  this->ModelViewTransform->Delete();
  if (this->ExplicitProjectionTransformMatrix)
  {
    this->ExplicitProjectionTransformMatrix->UnRegister(this);
    this->ExplicitProjectionTransformMatrix = NULL;
  }
  if (this->UserTransform)
  {
    this->UserTransform->UnRegister(this);
    this->UserTransform = NULL;
  }
  if (this->UserViewTransform)
  {
    this->UserViewTransform->RemoveObserver(
      this->UserViewTransformCallbackCommand);
    this->UserViewTransform->UnRegister(this);
    this->UserViewTransform = NULL;
  }
  if (this->UserViewTransformCallbackCommand)
  {
    this->UserViewTransformCallbackCommand->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkCamera::SetScissorRect(vtkRecti scissorRect)
{
  this->ScissorRect = scissorRect;
}

//----------------------------------------------------------------------------
void vtkCamera::GetScissorRect(vtkRecti& scissorRect)
{
  scissorRect = this->ScissorRect;
}

//----------------------------------------------------------------------------
// The first set of methods deal exclusively with the ViewTransform, which
// is the only transform which is set up entirely in the camera.  The
// perspective transform must be set up by the Renderer because the
// Camera doesn't know the Renderer's aspect ratio.
//----------------------------------------------------------------------------
void vtkCamera::SetPosition(double x, double y, double z)
{
  if (x == this->Position[0] &&
      y == this->Position[1] &&
      z == this->Position[2])
  {
    return;
  }

  this->Position[0] = x;
  this->Position[1] = y;
  this->Position[2] = z;

  vtkDebugMacro(<< " Position set to ( " <<  this->Position[0] << ", "
                << this->Position[1] << ", " << this->Position[2] << ")");

  this->ComputeViewTransform();
  // recompute the focal distance
  this->ComputeDistance();
  this->ComputeCameraLightTransform();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCamera::SetUserTransform(vtkHomogeneousTransform *transform)
{
  if (transform == this->UserTransform)
  {
    return;
  }
  if (this->UserTransform)
  {
    this->UserTransform->Delete();
    this->UserTransform = NULL;
  }
  if (transform)
  {
    this->UserTransform = transform;
    this->UserTransform->Register(this);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCamera::SetUserViewTransform(vtkHomogeneousTransform *transform)
{
  if (transform == this->UserViewTransform)
  {
    return;
  }
  if (this->UserViewTransform)
  {
    this->UserViewTransform->RemoveObserver(
      this->UserViewTransformCallbackCommand);
    this->UserViewTransform->Delete();
    this->UserViewTransform = NULL;
  }
  if (transform)
  {
    this->UserViewTransform = transform;
    this->UserViewTransform->Register(this);
    if (!this->UserViewTransformCallbackCommand)
    {
      this->UserViewTransformCallbackCommand = vtkCameraCallbackCommand::New();
      this->UserViewTransformCallbackCommand->Self = this;
    }
    this->UserViewTransform->AddObserver(
      vtkCommand::ModifiedEvent,
      this->UserViewTransformCallbackCommand);
  }
  this->Modified();
  this->ComputeViewTransform();
  this->ComputeDistance();
  this->ComputeCameraLightTransform();
}

//----------------------------------------------------------------------------
void vtkCamera::SetFocalPoint(double x, double y, double z)
{
  if (x == this->FocalPoint[0] &&
      y == this->FocalPoint[1] &&
      z == this->FocalPoint[2])
  {
    return;
  }

  this->FocalPoint[0] = x;
  this->FocalPoint[1] = y;
  this->FocalPoint[2] = z;

  vtkDebugMacro(<< " FocalPoint set to ( " <<  this->FocalPoint[0] << ", " << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")");

  this->ComputeViewTransform();
  // recompute the focal distance
  this->ComputeDistance();
  this->ComputeCameraLightTransform();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCamera::SetViewUp(double x, double y, double z)
{
  // normalize ViewUp, but do _not_ orthogonalize it by default
  double norm = sqrt(x*x + y*y + z*z);

  if(norm != 0)
  {
    x /= norm;
    y /= norm;
    z /= norm;
  }
  else
  {
    x = 0;
    y = 1;
    z = 0;
  }

  if (x == this->ViewUp[0] &&
      y == this->ViewUp[1] &&
      z == this->ViewUp[2])
  {
    return;
  }

  this->ViewUp[0] = x;
  this->ViewUp[1] = y;
  this->ViewUp[2] = z;

  vtkDebugMacro(<< " ViewUp set to ( " <<  this->ViewUp[0] << ", "
    << this->ViewUp[1] << ", " << this->ViewUp[2] << ")");

  this->ComputeViewTransform();
  this->ComputeCameraLightTransform();
  this->Modified();
}

//----------------------------------------------------------------------------
// The ViewTransform depends on only three ivars:  the Position, the
// FocalPoint, and the ViewUp vector.  All the other methods are there
// simply for the sake of the users' convenience.
void vtkCamera::ComputeViewTransform()
{
  // main view through the camera
  this->Transform->Identity();
  if (this->UserViewTransform)
  {
    this->Transform->Concatenate(this->UserViewTransform);
  }
  this->Transform->SetupCamera(this->Position, this->FocalPoint, this->ViewUp);
  this->ViewTransform->SetMatrix(this->Transform->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkCamera::ComputeCameraLightTransform()
{
  vtkTransform *t;
  double d;

  // assumes a valid view transform and valid camera distance

  t = this->CameraLightTransform;
  t->Identity();
  t->SetMatrix(this->ViewTransform->GetMatrix());
  t->Inverse();

  d = this->Distance;
  t->Scale(d, d, d);
  t->Translate(0.0, 0.0, -1.0);
}

//----------------------------------------------------------------------------
void vtkCamera::ComputeWorldToScreenMatrix()
{
  // Avoid recalculating screen orientation if we don't need to.
  if(this->WorldToScreenMatrixMTime.GetMTime() < this->GetMTime())
  {
    double xAxis[3];
    double yAxis[3];
    double zAxis[3];

    for(int i = 0; i < 3; ++i)
    {
      xAxis[i] = this->ScreenBottomRight[i] - this->ScreenBottomLeft[i];
      yAxis[i] = this->ScreenTopRight[i]    - this->ScreenBottomRight[i];
    }

    vtkMath::Normalize(xAxis);
    vtkMath::Normalize(yAxis);
    vtkMath::Cross(xAxis, yAxis, zAxis);
    vtkMath::Normalize(zAxis);

    // Setting individual elements of the matrix.

    // Make it column major and then invert it to make sure the translation is correct
    // This is using column major (vectors are copied into the column)
    // \Note: while the initial element assignments are made in column-major
    // ordering, the matrix will be inverted, resulting in a row-major
    // matrix that provides the transformation from World to Screen space.
    this->WorldToScreenMatrix->SetElement(0, 0, xAxis[0]);
    this->WorldToScreenMatrix->SetElement(1, 0, xAxis[1]);
    this->WorldToScreenMatrix->SetElement(2, 0, xAxis[2]);

    this->WorldToScreenMatrix->SetElement(0, 1, yAxis[0]);
    this->WorldToScreenMatrix->SetElement(1, 1, yAxis[1]);
    this->WorldToScreenMatrix->SetElement(2, 1, yAxis[2]);

    this->WorldToScreenMatrix->SetElement(0, 2, zAxis[0]);
    this->WorldToScreenMatrix->SetElement(1, 2, zAxis[1]);
    this->WorldToScreenMatrix->SetElement(2, 2, zAxis[2]);

    this->WorldToScreenMatrix->SetElement(0, 3, this->ScreenBottomLeft[0]);
    this->WorldToScreenMatrix->SetElement(1, 3, this->ScreenBottomLeft[1]);
    this->WorldToScreenMatrix->SetElement(2, 3, this->ScreenBottomLeft[2]);

    this->WorldToScreenMatrix->SetElement(3, 3, 1.0);

    // The reason for doing this as an Invert as the goal here is to put
    // the translation through the rotation that we've just assigned ie.
    // the translation has to be put into screen space too.
    this->WorldToScreenMatrix->Invert();

    this->WorldToScreenMatrixMTime.Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCamera::ComputeOffAxisProjectionFrustum()
{
  this->ComputeWorldToScreenMatrix();

  // \NOTE: Varibles names reflect naming convention used in
  // "High Resolution Virtual Reality", in Proc.
  // SIGGRAPH '92, Computer Graphics, pages 195-202, 1992.

  // OffAxis calculations.


  // vtkMatrix::MultiplyPoint expect homogeneous coordinate.
  double E[4] = {0.0, 0.0, 0.0, 1.0};

  double L[4] = {this->ScreenBottomLeft[0], this->ScreenBottomLeft[1], this->ScreenBottomLeft[2], 1.0};
  double H[4] = {this->ScreenTopRight[0],   this->ScreenTopRight[1], this->ScreenTopRight[2], 1.0};

  double eyeSeparationCorrectionFactor = 10.0;
  double shiftDistance = this->EyeSeparation / (2.0 * eyeSeparationCorrectionFactor);
  if(this->Distance < 1.0)
  {
    shiftDistance *= this->Distance;
  }
  if(this->LeftEye)
  {
    E[0] -= shiftDistance;
  }
  else
  {
    E[0] += shiftDistance;
  }

  // First transform the eye to new position.
  this->EyeTransformMatrix->MultiplyPoint(E, E);

  // Now transform the eye and screen corner points into the screen
  // coordinate system.
  this->WorldToScreenMatrix->MultiplyPoint(E, E);
  this->WorldToScreenMatrix->MultiplyPoint(H, H);
  this->WorldToScreenMatrix->MultiplyPoint(L, L);

  double matrix[4][4];
  double width  = H[0] - L[0];
  double height = H[1] - L[1];

  // Back and front are not traditional near and far.
  // Front (aka near)
  double F = E[2] - (this->Distance + this->Thickness);//E[2] - 10000.0;//this->ClippingRange[1];
  // Back (aka far)
  double nearDistanceCorrectionFactor  = 1000.0;
  double B = E[2] - (this->Distance / nearDistanceCorrectionFactor);//E[2] - .1;//this->ClippingRange[0];

  double depth = B - F;
  matrix[0][0] =  ( 2*E[2] ) / width;
  matrix[1][0] =  0;
  matrix[2][0] =  0;
  matrix[3][0] =  0;

  matrix[0][1] =  0;
  matrix[1][1] =  ( 2*E[2] )/ height;
  matrix[2][1] =  0;
  matrix[3][1] =  0;

  matrix[0][2] = ( H[0]+L[0] - 2*E[0] )/width;
  matrix[1][2] = ( H[1]+L[1] - 2*E[1] )/height;
  matrix[2][2] = ( B+F-2*E[2] )/depth;
  matrix[3][2] = -1;

  matrix[0][3] = ( -E[2]*( H[0]+L[0] ) )/width;
  matrix[1][3] = ( -E[2]*( H[1]+L[1] ) )/height;
  matrix[2][3] = B-E[2]- ( B *( B+F - 2*E[2] )/depth );
  matrix[3][3] = E[2];

  for ( int i=0; i<4; i++ )
  {
    for ( int j=0; j<4; j++ )
    {
      this->ProjectionTransform->GetMatrix()->SetElement( i,j,  matrix[i][j] ) ;
    }
  }

  //  Now move the world into display space.
  vtkMatrix4x4::Multiply4x4(this->ProjectionTransform->GetMatrix(), this->WorldToScreenMatrix,
                            this->ProjectionTransform->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkCamera::ComputeModelViewMatrix()
{
  if(this->ModelViewTransform->GetMTime() < this->ModelTransformMatrix->GetMTime() ||
     this->ModelViewTransform->GetMTime() < this->ViewTransform->GetMTime())
  {
    vtkMatrix4x4::Multiply4x4(this->ViewTransform->GetMatrix(), this->ModelTransformMatrix,
                              this->ModelViewTransform->GetMatrix());
  }
}

//----------------------------------------------------------------------------
void vtkCamera::OrthogonalizeViewUp()
{
  // the orthogonalized ViewUp is just the second row of the view matrix
  vtkMatrix4x4 *matrix = this->ViewTransform->GetMatrix();
  this->ViewUp[0] = matrix->GetElement(1,0);
  this->ViewUp[1] = matrix->GetElement(1,1);
  this->ViewUp[2] = matrix->GetElement(1,2);

  this->Modified();
}

//----------------------------------------------------------------------------
// Set the distance of the focal point from the camera. The focal point is
// modified accordingly. This should be positive.
void vtkCamera::SetDistance(double d)
{
  if (this->Distance == d)
  {
    return;
  }

  this->Distance = d;

  // Distance should be greater than .0002
  if (this->Distance < 0.0002)
  {
    this->Distance = 0.0002;
    vtkDebugMacro(<< " Distance is set to minimum.");
  }

  // we want to keep the camera pointing in the same direction
  double *vec = this->DirectionOfProjection;

  // recalculate FocalPoint
  this->FocalPoint[0] = this->Position[0] + vec[0]*this->Distance;
  this->FocalPoint[1] = this->Position[1] + vec[1]*this->Distance;
  this->FocalPoint[2] = this->Position[2] + vec[2]*this->Distance;

  vtkDebugMacro(<< " Distance set to ( " <<  this->Distance << ")");

  this->ComputeViewTransform();
  this->ComputeCameraLightTransform();
  this->Modified();
}

//----------------------------------------------------------------------------
// This method must be called when the focal point or camera position changes
void vtkCamera::ComputeDistance()
{
  double dx = this->FocalPoint[0] - this->Position[0];
  double dy = this->FocalPoint[1] - this->Position[1];
  double dz = this->FocalPoint[2] - this->Position[2];

  this->Distance = sqrt(dx*dx + dy*dy + dz*dz);

  if (this->Distance < 1e-20)
  {
    this->Distance = 1e-20;
    vtkDebugMacro(<< " Distance is set to minimum.");

    double *vec = this->DirectionOfProjection;

    // recalculate FocalPoint
    this->FocalPoint[0] = this->Position[0] + vec[0]*this->Distance;
    this->FocalPoint[1] = this->Position[1] + vec[1]*this->Distance;
    this->FocalPoint[2] = this->Position[2] + vec[2]*this->Distance;
  }

  this->DirectionOfProjection[0] = dx/this->Distance;
  this->DirectionOfProjection[1] = dy/this->Distance;
  this->DirectionOfProjection[2] = dz/this->Distance;

  this->ComputeViewPlaneNormal();
}

//----------------------------------------------------------------------------
// Move the position of the camera along the view plane normal. Moving
// towards the focal point (e.g., > 1) is a dolly-in, moving away
// from the focal point (e.g., < 1) is a dolly-out.
void vtkCamera::Dolly(double amount)
{
  if (amount <= 0.0)
  {
    return;
  }

  // dolly moves the camera towards the focus
  double d = this->Distance/amount;

  this->SetPosition(this->FocalPoint[0] - d*this->DirectionOfProjection[0],
                    this->FocalPoint[1] - d*this->DirectionOfProjection[1],
                    this->FocalPoint[2] - d*this->DirectionOfProjection[2]);
}

//----------------------------------------------------------------------------
// Set the roll angle of the camera about the direction of projection
void vtkCamera::SetRoll(double roll)
{
  // roll is a rotation of camera view up about the direction of projection
  vtkDebugMacro(<< " Setting Roll to " << roll << "");

  // subtract the current roll
  roll -= this->GetRoll();

  if (fabs(roll) < 0.00001)
  {
    return;
  }

  this->Roll(roll);
}

//----------------------------------------------------------------------------
// Returns the roll of the camera.
double vtkCamera::GetRoll()
{
  double orientation[3];
  this->ViewTransform->GetOrientation(orientation);
  return orientation[2];
}

//----------------------------------------------------------------------------
// Rotate the camera around the view plane normal.
void vtkCamera::Roll(double angle)
{
  double newViewUp[3];
  this->Transform->Identity();

  // rotate ViewUp about the Direction of Projection
  this->Transform->RotateWXYZ(angle,this->DirectionOfProjection);

  // okay, okay, TransformPoint shouldn't be used on vectors -- but
  // the transform is rotation with no translation so this works fine.
  this->Transform->TransformPoint(this->ViewUp,newViewUp);
  this->SetViewUp(newViewUp);
}

//----------------------------------------------------------------------------
// Rotate the focal point about the view up vector centered at the camera's
// position.
void vtkCamera::Yaw(double angle)
{
  double newFocalPoint[3];
  double *pos = this->Position;
  this->Transform->Identity();

  // translate the camera to the origin,
  // rotate about axis,
  // translate back again
  this->Transform->Translate(+pos[0],+pos[1],+pos[2]);
  this->Transform->RotateWXYZ(angle,this->ViewUp);
  this->Transform->Translate(-pos[0],-pos[1],-pos[2]);

  // now transform focal point
  this->Transform->TransformPoint(this->FocalPoint,newFocalPoint);
  this->SetFocalPoint(newFocalPoint);
}

//----------------------------------------------------------------------------
// Rotate the focal point about the cross product of the view up vector
// and the negative of the , centered at the camera's position.
void vtkCamera::Pitch(double angle)
{
  double axis[3], newFocalPoint[3], savedViewUp[3];
  double *pos = this->Position;
  this->Transform->Identity();

  // the axis is the first row of the view transform matrix
  axis[0] = this->ViewTransform->GetMatrix()->GetElement(0,0);
  axis[1] = this->ViewTransform->GetMatrix()->GetElement(0,1);
  axis[2] = this->ViewTransform->GetMatrix()->GetElement(0,2);

  // temporarily set the view up with the transformation applied
  // to avoid bad cross product computations during SetFocalPoint call
  this->GetViewUp(savedViewUp);
  this->Transform->RotateWXYZ(angle, axis);
  this->Transform->TransformPoint(this->ViewUp, this->ViewUp);
  this->Transform->Identity();

  // translate the camera to the origin,
  // rotate about axis,
  // translate back again
  this->Transform->Translate(+pos[0],+pos[1],+pos[2]);
  this->Transform->RotateWXYZ(angle,axis);
  this->Transform->Translate(-pos[0],-pos[1],-pos[2]);

  // now transform focal point
  this->Transform->TransformPoint(this->FocalPoint,newFocalPoint);
  this->SetFocalPoint(newFocalPoint);

  // restore the previous ViewUp vector
  this->ViewUp[0] = savedViewUp[0];
  this->ViewUp[1] = savedViewUp[1];
  this->ViewUp[2] = savedViewUp[2];
}

//----------------------------------------------------------------------------
// Rotate the camera about the view up vector centered at the focal point.
void vtkCamera::Azimuth(double angle)
{
  double newPosition[3];
  double *fp = this->FocalPoint;
  this->Transform->Identity();

  // translate the focal point to the origin,
  // rotate about view up,
  // translate back again
  this->Transform->Translate(+fp[0],+fp[1],+fp[2]);
  this->Transform->RotateWXYZ(angle,this->ViewUp);
  this->Transform->Translate(-fp[0],-fp[1],-fp[2]);

  // apply the transform to the position
  this->Transform->TransformPoint(this->Position,newPosition);
  this->SetPosition(newPosition);
}

//----------------------------------------------------------------------------
// Rotate the camera about the cross product of the negative of the
// direction of projection and the view up vector centered on the focal point.
void vtkCamera::Elevation(double angle)
{
  double axis[3], newPosition[3], savedViewUp[3];
  double *fp = this->FocalPoint;
  this->Transform->Identity();

  // snatch the axis from the view transform matrix
  axis[0] = -this->ViewTransform->GetMatrix()->GetElement(0,0);
  axis[1] = -this->ViewTransform->GetMatrix()->GetElement(0,1);
  axis[2] = -this->ViewTransform->GetMatrix()->GetElement(0,2);

  // temporarily set the view up with the transformation applied
  // to avoid bad cross product computations during SetPosition call
  this->GetViewUp(savedViewUp);
  this->Transform->RotateWXYZ(angle, axis);
  this->Transform->TransformPoint(this->ViewUp, this->ViewUp);
  this->Transform->Identity();

    // translate the focal point to the origin,
  // rotate about axis,
  // translate back again
  this->Transform->Translate(+fp[0],+fp[1],+fp[2]);
  this->Transform->RotateWXYZ(angle,axis);
  this->Transform->Translate(-fp[0],-fp[1],-fp[2]);

  // now transform position
  this->Transform->TransformPoint(this->Position,newPosition);
  this->SetPosition(newPosition);

  // restore the previous ViewUp vector
  this->ViewUp[0] = savedViewUp[0];
  this->ViewUp[1] = savedViewUp[1];
  this->ViewUp[2] = savedViewUp[2];
}

//----------------------------------------------------------------------------
// Apply Transform to camera
void vtkCamera::ApplyTransform(vtkTransform *t)
{
  double posOld[4], posNew[4], fpOld[4], fpNew[4], vuOld[4], vuNew[4];

  this->GetPosition(posOld);
  this->GetFocalPoint(fpOld);
  this->GetViewUp(vuOld);

  posOld[3] = 1.0;
  fpOld[3] = 1.0;
  vuOld[3] = 1.0;

  vuOld[0] += posOld[0];
  vuOld[1] += posOld[1];
  vuOld[2] += posOld[2];

  t->MultiplyPoint(posOld, posNew);
  t->MultiplyPoint(fpOld, fpNew);
  t->MultiplyPoint(vuOld, vuNew);

  vuNew[0] -= posNew[0];
  vuNew[1] -= posNew[1];
  vuNew[2] -= posNew[2];

  this->SetPosition(posNew);
  this->SetFocalPoint(fpNew);
  this->SetViewUp(vuNew);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// The following methods set up the information that the Renderer needs
// to set up the perspective transform.  The transformation matrix is
// created using the GetPerspectiveTransformMatrix method.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkCamera::SetParallelProjection(int flag)
{
  if ( this->ParallelProjection != flag )
  {
    this->ParallelProjection = flag;
    this->Modified();
    this->ViewingRaysModified();
  }
}

//----------------------------------------------------------------------------
void vtkCamera::SetViewAngle(double angle)
{
  double min = 0.00000001;
  double max = 179.0;

  if ( this->ViewAngle != angle )
  {
    this->ViewAngle = (angle<min?min:(angle>max?max:angle));
    this->Modified();
    this->ViewingRaysModified();
  }
}

//----------------------------------------------------------------------------
void vtkCamera::SetUseHorizontalViewAngle(int flag)
{
  if (flag == this->UseHorizontalViewAngle)
  {
    return;
  }
  this->UseHorizontalViewAngle = flag;
  this->Modified();
  this->ViewingRaysModified();
}

//----------------------------------------------------------------------------
void vtkCamera::SetParallelScale(double scale)
{
  if ( this->ParallelScale != scale )
  {
    this->ParallelScale = scale;
    this->Modified();
    this->ViewingRaysModified();
  }
}

//----------------------------------------------------------------------------
// Change the ViewAngle (for perspective) or the ParallelScale (for parallel)
// so that more or less of a scene occupies the viewport.  A value > 1 is a
// zoom-in. A value < 1 is a zoom-out.
void vtkCamera::Zoom(double amount)
{
  if (amount <= 0.0)
  {
    return;
  }

  if (this->ParallelProjection)
  {
    this->SetParallelScale(this->ParallelScale/amount);
  }
  else
  {
    this->SetViewAngle(this->ViewAngle/amount);
  }
}

//----------------------------------------------------------------------------
void vtkCamera::SetClippingRange(double nearz, double farz)
{
  double thickness;

  // check the order
  if ( nearz > farz )
  {
    vtkDebugMacro(<< " Front and back clipping range reversed");
    double temp = nearz;
    nearz = farz;
    farz = temp;
  }

  thickness = farz - nearz;

  // thickness should be greater than 1e-20
  if (thickness < 1e-20)
  {
    thickness = 1e-20;
    vtkDebugMacro(<< " ClippingRange thickness is set to minimum.");

    // set back plane
    farz = nearz + thickness;
  }

  if (nearz == this->ClippingRange[0] &&
      farz == this->ClippingRange[1] &&
      this->Thickness == thickness)
  {
    return;
  }

  this->ClippingRange[0] = nearz;
  this->ClippingRange[1] = farz;
  this->Thickness = thickness;

  vtkDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0]
    << ", "  << this->ClippingRange[1] << ")");

  this->Modified();
}

//----------------------------------------------------------------------------
// Set the distance between clipping planes.
// This method adjusts the back clipping plane to the specified thickness
// behind the front clipping plane
void vtkCamera::SetThickness(double s)
{
  if (this->Thickness == s)
  {
    return;
  }

  this->Thickness = s;

  // thickness should be greater than 1e-20
  if (this->Thickness < 1e-20)
  {
    this->Thickness = 1e-20;
    vtkDebugMacro(<< " ClippingRange thickness is set to minimum.");
  }

  // set back plane
  this->ClippingRange[1] = this->ClippingRange[0] + this->Thickness;

  vtkDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0]
    << ", " << this->ClippingRange[1] << ")");

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCamera::SetWindowCenter(double x, double y)
{
  if (this->WindowCenter[0] != x || this->WindowCenter[1] != y)
  {
    this->Modified();
    this->ViewingRaysModified();
    this->WindowCenter[0] = x;
    this->WindowCenter[1] = y;
  }
}

//----------------------------------------------------------------------------
void vtkCamera::SetObliqueAngles(double alpha, double beta)
{
  alpha = vtkMath::RadiansFromDegrees( alpha );
  beta = vtkMath::RadiansFromDegrees( beta );

  double cotbeta = cos( beta ) / sin( beta );
  double dxdz = cos( alpha ) * cotbeta;
  double dydz = sin( alpha ) * cotbeta;

  this->SetViewShear( dxdz, dydz, 1.0 );
}

//----------------------------------------------------------------------------
// Set the shear transform of the viewing frustum.  Parameters are
// dx/dz, dy/dz, and center.  center is a factor that describes where
// to shear around. The distance dshear from the camera where
// no shear occurs is given by (dshear = center * FocalDistance).
//
void vtkCamera::SetViewShear(double dxdz, double dydz, double center)
{
  if(dxdz   != this->ViewShear[0] ||
     dydz   != this->ViewShear[1] ||
     center != this->ViewShear[2])
  {
    this->Modified();
    this->ViewingRaysModified();

    this->ViewShear[0] = dxdz;
    this->ViewShear[1] = dydz;
    this->ViewShear[2] = center;

    this->ComputeViewPlaneNormal();
  }
}
//----------------------------------------------------------------------------

void vtkCamera::SetViewShear(double d[3])
{
  this->SetViewShear(d[0], d[1], d[2]);
}

//----------------------------------------------------------------------------
// Compute the projection transform matrix. This is used in converting
// between view and world coordinates.
void vtkCamera::ComputeProjectionTransform(double aspect,
                                           double nearz, double farz)
{
  this->ProjectionTransform->Identity();

  // apply user defined transform last if there is one
  if ( this->UserTransform )
  {
    this->ProjectionTransform->Concatenate( this->UserTransform->GetMatrix() );
  }

  if (this->UseExplicitProjectionTransformMatrix)
  {
    assert(this->ExplicitProjectionTransformMatrix != NULL);
    this->ProjectionTransform->Concatenate(
          this->ExplicitProjectionTransformMatrix);
    return;
  }

  // adjust Z-buffer range
  this->ProjectionTransform->AdjustZBuffer( -1, +1, nearz, farz );

  if ( this->ParallelProjection )
  {
    // set up a rectangular parallelipiped

    double width = this->ParallelScale * aspect;
    double height = this->ParallelScale;

    double xmin = ( this->WindowCenter[0] - 1.0 ) * width;
    double xmax = ( this->WindowCenter[0] + 1.0 ) * width;
    double ymin = ( this->WindowCenter[1] - 1.0 ) * height;
    double ymax = ( this->WindowCenter[1] + 1.0 ) * height;

    this->ProjectionTransform->Ortho( xmin, xmax, ymin, ymax,
                                      this->ClippingRange[0],
                                      this->ClippingRange[1] );
  }
  else if(this->UseOffAxisProjection)
  {
    this->ComputeOffAxisProjectionFrustum();
  }
  else
  {
    // set up a perspective frustum

    double tmp = tan( vtkMath::RadiansFromDegrees( this->ViewAngle ) / 2. );
    double width;
    double height;
    if ( this->UseHorizontalViewAngle )
    {
      width = this->ClippingRange[0] * tmp;
      height = this->ClippingRange[0] * tmp / aspect;
    }
    else
    {
      width = this->ClippingRange[0] * tmp * aspect;
      height = this->ClippingRange[0] * tmp;
    }

    double xmin = ( this->WindowCenter[0] - 1.0 ) * width;
    double xmax = ( this->WindowCenter[0] + 1.0 ) * width;
    double ymin = ( this->WindowCenter[1] - 1.0 ) * height;
    double ymax = ( this->WindowCenter[1] + 1.0 ) * height;

    this->ProjectionTransform->Frustum( xmin, xmax, ymin, ymax,
                                        this->ClippingRange[0],
                                        this->ClippingRange[1] );
  }

  if ( this->Stereo && !this->UseOffAxisProjection)
  {
    // set up a shear for stereo views
    if ( this->LeftEye )
    {
      this->ProjectionTransform->Stereo( -this->EyeAngle/2,
                                          this->Distance );
    }
    else
    {
      this->ProjectionTransform->Stereo( +this->EyeAngle/2,
                                          this->Distance );
    }
  }

  if ( this->ViewShear[0] != 0.0 || this->ViewShear[1] != 0.0 )
  {
    this->ProjectionTransform->Shear( this->ViewShear[0],
                                      this->ViewShear[1],
                                      this->ViewShear[2] * this->Distance );
  }

}

//----------------------------------------------------------------------------
// Return the projection transform matrix. See ComputeProjectionTransform.
vtkMatrix4x4 *vtkCamera::GetProjectionTransformMatrix(vtkRenderer *ren)
{
  double aspect[2];
  int  lowerLeft[2];
  int usize, vsize;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft+1);

  // some renderer subclasses may have more complicated computations for the
  // aspect ratio. So take that into account by computing the difference
  // between our simple aspect ratio and what the actual renderer is reporting.
  ren->ComputeAspect();
  ren->GetAspect(aspect);
  double aspect2[2];
  ren->vtkViewport::ComputeAspect();
  ren->vtkViewport::GetAspect(aspect2);
  double aspectModification = aspect[0] * aspect2[1] / (aspect[1] * aspect2[0]);

  if (usize && vsize)
  {
    matrix->DeepCopy(this->GetProjectionTransformMatrix(
                       aspectModification * usize / vsize, -1, 1));
    matrix->Transpose();
  }

  return matrix;
}

//----------------------------------------------------------------------------
// Return the projection transform matrix. See ComputeProjectionTransform.
vtkMatrix4x4 *vtkCamera::GetProjectionTransformMatrix(double aspect,
                                                      double nearz,
                                                      double farz)
{
  this->ComputeProjectionTransform(aspect, nearz, farz);

  // return the transform
  return this->ProjectionTransform->GetMatrix();
}

//----------------------------------------------------------------------------
// Return the projection transform object. See ComputeProjectionTransform.
vtkPerspectiveTransform *vtkCamera::GetProjectionTransformObject(double aspect,
                                                                 double nearz,
                                                                 double farz)
{
  this->ComputeProjectionTransform(aspect, nearz, farz);

  // return the transform
  return this->ProjectionTransform;
}

//----------------------------------------------------------------------------
// Return the projection transform matrix. See ComputeProjectionTransform.
vtkMatrix4x4 *vtkCamera::GetCompositeProjectionTransformMatrix(double aspect,
                                                               double nearz,
                                                               double farz)
{
  // turn off stereo, the CompositeProjectionTransformMatrix is used for
  // picking, not for rendering.
  int stereo = this->Stereo;
  this->Stereo = 0;

  this->Transform->Identity();
  this->Transform->Concatenate(this->GetProjectionTransformMatrix(aspect,
                                                                  nearz,
                                                                  farz));
  this->Transform->Concatenate(this->GetViewTransformMatrix());

  this->Stereo = stereo;

  // return the transform
  return this->Transform->GetMatrix();
}

//----------------------------------------------------------------------------
// Return the attached light transform matrix.
vtkMatrix4x4 *vtkCamera::GetCameraLightTransformMatrix()
{
  // return the transform
  return this->CameraLightTransform->GetMatrix();
}


//----------------------------------------------------------------------------
void vtkCamera::ComputeViewPlaneNormal()
{
  if (this->ViewShear[0] != 0.0 || this->ViewShear[1] != 0.0)
  {
    // set the VPN in camera coordinates
    this->ViewPlaneNormal[0] = this->ViewShear[0];
    this->ViewPlaneNormal[1] = this->ViewShear[1];
    this->ViewPlaneNormal[2] = 1.0;
    // transform the VPN to world coordinates using inverse of view transform
    this->ViewTransform->GetLinearInverse()->TransformNormal(
                                              this->ViewPlaneNormal,
                                              this->ViewPlaneNormal);
  }
  else
  {
    // VPN is -DOP
    this->ViewPlaneNormal[0] = -this->DirectionOfProjection[0];
    this->ViewPlaneNormal[1] = -this->DirectionOfProjection[1];
    this->ViewPlaneNormal[2] = -this->DirectionOfProjection[2];
  }
}

//----------------------------------------------------------------------------
// Return the 6 planes (Ax + By + Cz + D = 0) that bound
// the view frustum.
void vtkCamera::GetFrustumPlanes(double aspect, double planes[24])
{
  int i;
  double f, normals[6][4], matrix[4][4];

  // set up the normals
  for (i = 0; i < 6; i++)
  {
    normals[i][0] = 0.0;
    normals[i][1] = 0.0;
    normals[i][2] = 0.0;
    normals[i][3] = 1.0;
    // if i is even set to -1, if odd set to +1
    normals[i][i/2] = 1 - (i%2)*2;
  }

  // get the composite perspective matrix
  vtkMatrix4x4::DeepCopy(
    *matrix,
    this->GetCompositeProjectionTransformMatrix(aspect,-1,+1));

  // transpose the matrix for use with normals
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  // transform the normals to world coordinates
  for (i = 0; i < 6; i++)
  {
    vtkMatrix4x4::MultiplyPoint(*matrix,normals[i],normals[i]);

    f = 1.0/sqrt(normals[i][0]*normals[i][0] +
                 normals[i][1]*normals[i][1] +
                 normals[i][2]*normals[i][2]);

    planes[4*i + 0] = normals[i][0]*f;
    planes[4*i + 1] = normals[i][1]*f;
    planes[4*i + 2] = normals[i][2]*f;
    planes[4*i + 3] = normals[i][3]*f;
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkCamera::GetViewingRaysMTime()
{
  return this->ViewingRaysMTime.GetMTime();
}

//----------------------------------------------------------------------------
void vtkCamera::ViewingRaysModified()
{
  this->ViewingRaysMTime.Modified();
}

// ----------------------------------------------------------------------------
// Description:
// Copy the properties of `source' into `this'.
// Copy pointers of matrices.
// \pre source_exists!=0
// \pre not_this: source!=this
void vtkCamera::ShallowCopy(vtkCamera *source)
{
  assert("pre: source_exists" && source!=0);
  assert("pre: not_this" && source!=this);

  this->PartialCopy(source);

  // Shallow copy of matrices:
  if(this->UserTransform!=0)
  {
    this->UserTransform->Delete();
  }
  this->UserTransform=source->UserTransform;
  if(this->UserTransform!=0)
  {
    this->UserTransform->Register(this);
  }
  if(this->UserViewTransform!=0)
  {
    this->UserViewTransform->Delete();
  }
  this->UserViewTransform=source->UserViewTransform;
  if(this->UserViewTransform!=0)
  {
    this->UserViewTransform->Register(this);
  }

  if(this->ViewTransform!=0)
  {
    this->ViewTransform->Delete();
  }
  this->ViewTransform=source->ViewTransform;
  if(this->ViewTransform!=0)
  {
    this->ViewTransform->Register(this);
  }

  if(this->ProjectionTransform!=0)
  {
    this->ProjectionTransform->Delete();
  }
  this->ProjectionTransform=source->ProjectionTransform;
  if(this->ProjectionTransform!=0)
  {
    this->ProjectionTransform->Register(this);
  }

  if(this->Transform!=0)
  {
    this->Transform->Delete();
  }
  this->Transform=source->Transform;
  if(this->Transform!=0)
  {
    this->Transform->Register(this);
  }

  if(this->CameraLightTransform!=0)
  {
    this->CameraLightTransform->Delete();
  }
  this->CameraLightTransform=source->CameraLightTransform;
  if(this->CameraLightTransform!=0)
  {
    this->CameraLightTransform->Register(this);
  }

  if (this->EyeTransformMatrix != NULL)
  {
    this->EyeTransformMatrix->Delete();
  }
  this->EyeTransformMatrix = source->EyeTransformMatrix;
  if (this->EyeTransformMatrix!=0)
  {
    this->EyeTransformMatrix->Register(this);
  }

  if (this->WorldToScreenMatrix != NULL)
  {
    this->WorldToScreenMatrix->Delete();
  }
  this->WorldToScreenMatrix = source->WorldToScreenMatrix;
  if (this->WorldToScreenMatrix!=0)
  {
    this->WorldToScreenMatrix->Register(this);
  }

  if (this->ModelTransformMatrix != NULL)
  {
    this->ModelTransformMatrix->Delete();
  }
  this->ModelTransformMatrix = source->ModelTransformMatrix;
  if (this->ModelTransformMatrix!=0)
  {
    this->ModelTransformMatrix->Register(this);
  }

  if (this->ModelViewTransform != NULL)
  {
    this->ModelViewTransform->Delete();
  }
  this->ModelViewTransform = source->ModelViewTransform;
  if (this->ModelViewTransform!=0)
  {
    this->ModelViewTransform->Register(this);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Copy the properties of `source' into `this'.
// Copy the contents of the matrices.
// \pre source_exists!=0
// \pre not_this: source!=this
void vtkCamera::DeepCopy(vtkCamera *source)
{
  assert("pre: source_exists" && source!=0);
  assert("pre: not_this" && source!=this);

  this->PartialCopy(source);

  // Deep copy the matrices:
  if(source->UserTransform==0)
  {
    if(this->UserTransform!=0)
    {
      this->UserTransform->UnRegister(this);
      this->UserTransform=0;
    }
  }
  else
  {
    if(this->UserTransform==0)
    {
      this->UserTransform=
        static_cast<vtkHomogeneousTransform *>(
          source->UserTransform->MakeTransform());
    }
    this->UserTransform->DeepCopy(source->UserTransform);
  }

  if(source->UserViewTransform==0)
  {
    if(this->UserViewTransform!=0)
    {
      this->UserViewTransform->UnRegister(this);
      this->UserViewTransform=0;
    }
  }
  else
  {
    if(this->UserViewTransform==0)
    {
      this->UserViewTransform=
        static_cast<vtkHomogeneousTransform *>(
          source->UserViewTransform->MakeTransform());
    }
    this->UserViewTransform->DeepCopy(source->UserViewTransform);
  }

  if(source->ViewTransform==0)
  {
    if(this->ViewTransform!=0)
    {
      this->ViewTransform->UnRegister(this);
      this->ViewTransform=0;
    }
  }
  else
  {
    if(this->ViewTransform==0)
    {
      this->ViewTransform=
        static_cast<vtkTransform *>(
          source->ViewTransform->MakeTransform());
    }
    this->ViewTransform->DeepCopy(source->ViewTransform);
  }

  if(source->ProjectionTransform==0)
  {
    if(this->ProjectionTransform!=0)
    {
      this->ProjectionTransform->UnRegister(this);
      this->ProjectionTransform=0;
    }
  }
  else
  {
    if(this->ProjectionTransform==0)
    {
      this->ProjectionTransform=
        static_cast<vtkPerspectiveTransform *>(
          source->ProjectionTransform->MakeTransform());
    }
    this->ProjectionTransform->DeepCopy(source->ProjectionTransform);
  }

  if(source->Transform==0)
  {
    if(this->Transform!=0)
    {
      this->Transform->UnRegister(this);
      this->Transform=0;
    }
  }
  else
  {
    if(this->Transform==0)
    {
      this->Transform=
        static_cast<vtkPerspectiveTransform *>(
          source->Transform->MakeTransform());
    }
    this->Transform->DeepCopy(source->Transform);
  }

  if(source->CameraLightTransform==0)
  {
    if(this->CameraLightTransform!=0)
    {
      this->CameraLightTransform->UnRegister(this);
      this->CameraLightTransform=0;
    }
  }
  else
  {
    if(this->CameraLightTransform==0)
    {
      this->CameraLightTransform=
        static_cast<vtkTransform *>(
          source->CameraLightTransform->MakeTransform());
    }
    this->CameraLightTransform->DeepCopy(source->CameraLightTransform);
  }

  if(source->ModelViewTransform==0)
  {
    if(this->ModelViewTransform!=0)
    {
      this->ModelViewTransform->UnRegister(this);
      this->ModelViewTransform=0;
    }
  }
  else
  {
    if(this->ModelViewTransform==0)
    {
      this->ModelViewTransform=
        static_cast<vtkTransform *>(
          source->ModelViewTransform->MakeTransform());
    }
    this->ModelViewTransform->DeepCopy(source->ModelViewTransform);
  }

  if(source->ModelTransformMatrix == 0)
  {
    if(this->ModelTransformMatrix != 0)
    {
      this->ModelTransformMatrix->UnRegister(this);
      this->ModelTransformMatrix = 0;
    }
  }
  else
  {
    if(this->ModelTransformMatrix==0)
    {
      this->ModelTransformMatrix=
        static_cast<vtkMatrix4x4 *>(
          source->ModelTransformMatrix->NewInstance());
    }
    this->ModelTransformMatrix->DeepCopy(source->ModelTransformMatrix);
  }

  if(source->EyeTransformMatrix == 0)
  {
    if(this->EyeTransformMatrix != 0)
    {
      this->EyeTransformMatrix->UnRegister(this);
      this->EyeTransformMatrix = 0;
    }
  }
  else
  {
    if(this->EyeTransformMatrix==0)
    {
      this->EyeTransformMatrix=
        static_cast<vtkMatrix4x4 *>(
          source->EyeTransformMatrix->NewInstance());
    }
    this->EyeTransformMatrix->DeepCopy(source->EyeTransformMatrix);
  }

  if(source->WorldToScreenMatrix == 0)
  {
    if(this->WorldToScreenMatrix != 0)
    {
      this->WorldToScreenMatrix->UnRegister(this);
      this->WorldToScreenMatrix = 0;
    }
  }
  else
  {
    if(this->WorldToScreenMatrix==0)
    {
      this->WorldToScreenMatrix=
        static_cast<vtkMatrix4x4 *>(
          source->WorldToScreenMatrix->NewInstance());
    }
    this->WorldToScreenMatrix->DeepCopy(source->WorldToScreenMatrix);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Copy the ivars. Do nothing for the matrices.
// Called by ShallowCopy() and DeepCopy()
// \pre source_exists!=0
// \pre not_this: source!=this
void vtkCamera::PartialCopy(vtkCamera *source)
{
  assert("pre: source_exists" && source!=0);
  assert("pre: not_this" && source!=this);

  int i;

  i=0;
  while(i<2)
  {
    this->WindowCenter[i]=source->WindowCenter[i];
    this->ObliqueAngles[i]=source->ObliqueAngles[i];
    this->ClippingRange[i]=source->ClippingRange[i];
    ++i;
  }
  i=0;
  while(i<3)
  {
    this->FocalPoint[i]=source->FocalPoint[i];
    this->Position[i]=source->Position[i];
    this->ViewUp[i]=source->ViewUp[i];
    this->DirectionOfProjection[i]=source->DirectionOfProjection[i];
    this->ViewPlaneNormal[i]=source->ViewPlaneNormal[i];
    this->ViewShear[i]=source->ViewShear[i];

    this->ScreenBottomLeft[i]=source->ScreenBottomLeft[i];
    this->ScreenBottomRight[i]=source->ScreenBottomRight[i];
    this->ScreenTopRight[i]=source->ScreenTopRight[i];
    ++i;
  }

  this->ViewAngle=source->ViewAngle;
  this->EyeAngle=source->EyeAngle;
  this->ParallelProjection=source->ParallelProjection;
  this->ParallelScale=source->ParallelScale;
  this->Stereo=source->Stereo;
  this->LeftEye=source->LeftEye;
  this->Thickness=source->Thickness;
  this->Distance=source->Distance;
  this->UseHorizontalViewAngle=source->UseHorizontalViewAngle;
  this->UseOffAxisProjection=source->UseOffAxisProjection;

  this->FocalDisk=source->FocalDisk;
  this->EyeSeparation=source->EyeSeparation;
  this->WorldToScreenMatrixMTime=source->WorldToScreenMatrixMTime;

  this->ViewingRaysMTime=source->ViewingRaysMTime;
}

//----------------------------------------------------------------------------
void vtkCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ClippingRange: (" << this->ClippingRange[0] << ", "
     << this->ClippingRange[1] << ")\n";
  os << indent << "DirectionOfProjection: (" << this->DirectionOfProjection[0]
     << ", " << this->DirectionOfProjection[1]
     << ", " << this->DirectionOfProjection[2] << ")\n";
  os << indent << "Distance: " << this->Distance << "\n";
  os << indent << "EyeAngle: " << this->EyeAngle << "\n";
  os << indent << "FocalDisk: " << this->FocalDisk << "\n";
  os << indent << "FocalPoint: (" << this->FocalPoint[0] << ", "
     << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")\n";
  os << indent << "ViewShear: (" << this->ViewShear[0]
     << ", " << this->ViewShear[1]
     << ", " << this->ViewShear[2] << ")\n";
  os << indent << "ParallelProjection: " <<
    (this->ParallelProjection ? "On\n" : "Off\n");
  os << indent << "ParallelScale: " << this->ParallelScale << "\n";
  os << indent << "Position: (" << this->Position[0] << ", "
     << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Stereo: " << (this->Stereo ? "On\n" : "Off\n");
  os << indent << "Left Eye: " << this->LeftEye << endl;
  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "ViewAngle: " << this->ViewAngle << "\n";
  os << indent << "UseHorizontalViewAngle: " << this->UseHorizontalViewAngle
     << "\n";
  os << indent << "UserTransform: ";
  if (this->UserTransform)
  {
    os << this->UserTransform << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  if (this->UserViewTransform)
  {
    os << this->UserViewTransform << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "FreezeFocalPoint: ";
  if( this->FreezeFocalPoint )
  {
    os << this->FreezeFocalPoint << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "ViewPlaneNormal: (" << this->ViewPlaneNormal[0]
     << ", " << this->ViewPlaneNormal[1]
     << ", " << this->ViewPlaneNormal[2] << ")\n";
  os << indent << "ViewUp: (" << this->ViewUp[0] << ", "
     << this->ViewUp[1] << ", " << this->ViewUp[2] << ")\n";
  os << indent << "WindowCenter: (" << this->WindowCenter[0] << ", "
     << this->WindowCenter[1] << ")\n";

  os << indent << "UseOffAxisProjection: (" << this->UseOffAxisProjection
     << ")\n";

  os << indent << "ScreenBottomLeft: (" << this->ScreenBottomLeft[0]
     << ", " << this->ScreenBottomLeft[1] << ", " << this->ScreenBottomLeft[2]
     << ")\n";

  os << indent << "ScreenBottomRight: (" << this->ScreenBottomRight[0]
     << ", " << this->ScreenBottomRight[1] << ", " << this->ScreenBottomRight[2]
     << ")\n";

  os << indent << "ScreenTopRight: (" << this->ScreenTopRight[0]
     << ", " << this->ScreenTopRight[1] << ", " << this->ScreenTopRight[2]
     << ")\n";

  os << indent << "EyeSeparation: (" << this->EyeSeparation
     << ")\n";

  os << indent << "WorldToScreenMatrix: (" << this->WorldToScreenMatrix << "\n";
  this->WorldToScreenMatrix->PrintSelf(os, indent.GetNextIndent());
  os << indent << ")\n";

  os << indent << "EyeTransformMatrix: (" << this->EyeTransformMatrix << "\n";
  this->EyeTransformMatrix->PrintSelf(os, indent.GetNextIndent());
  os << indent << ")\n";

  os << indent << "ModelTransformMatrix: (" << this->ModelTransformMatrix << "\n";
  this->ModelTransformMatrix->PrintSelf(os, indent.GetNextIndent());
  os << indent << ")\n";

  os << indent << "ProjectionTransform: (" << this->ProjectionTransform << "\n";
  this->ProjectionTransform->PrintSelf(os, indent.GetNextIndent());
  os << indent << ")\n";
}

//-----------------------------------------------------------------------------
void vtkCamera::SetEyePosition(double eyePosition[3])
{
  if(!eyePosition)
  {
    vtkErrorMacro(<< "ERROR: Invalid or NULL eye position\n");
    return;
  }

  this->EyeTransformMatrix->SetElement(0, 3, eyePosition[0]);
  this->EyeTransformMatrix->SetElement(1, 3, eyePosition[1]);
  this->EyeTransformMatrix->SetElement(2, 3, eyePosition[2]);

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCamera::GetEyePosition(double eyePosition[3])
{
  if(!eyePosition)
  {
    vtkErrorMacro(<< "ERROR: Invalid or NULL eye position\n");
    return;
  }

  eyePosition[0] = this->EyeTransformMatrix->GetElement(0, 3);
  eyePosition[1] = this->EyeTransformMatrix->GetElement(1, 3);
  eyePosition[2] = this->EyeTransformMatrix->GetElement(2, 3);
}

//-----------------------------------------------------------------------------
void vtkCamera::GetEyePlaneNormal(double normal[3])
{
  if(!normal)
  {
    vtkErrorMacro(<< "ERROR: Invalid or NULL normal\n");
    return;
  }

  // Homogeneous normal.
  double localNormal[4];

  // Get the normal from the screen orientation.
  localNormal[0] = this->WorldToScreenMatrix->GetElement(2, 0);
  localNormal[1] = this->WorldToScreenMatrix->GetElement(2, 1);
  localNormal[2] = this->WorldToScreenMatrix->GetElement(2, 2);
  localNormal[3] = 0.0;

  // Just to be sure.
  vtkMath::Normalize(localNormal);

  normal[0] = localNormal[0];
  normal[1] = localNormal[1];
  normal[2] = localNormal[2];
}

//-----------------------------------------------------------------------------
vtkMatrix4x4 *vtkCamera::GetModelViewTransformMatrix()
{
  this->ComputeModelViewMatrix();

  return this->ModelViewTransform->GetMatrix();
}

//-----------------------------------------------------------------------------
vtkTransform *vtkCamera::GetModelViewTransformObject()
{
  this->ComputeModelViewMatrix();

  return this->ModelViewTransform;
}

//-----------------------------------------------------------------------------
vtkMatrix4x4 *vtkCamera::GetViewTransformMatrix()
{
  return this->GetModelViewTransformMatrix();
}

//-----------------------------------------------------------------------------
vtkTransform *vtkCamera::GetViewTransformObject()
{
  return this->GetModelViewTransformObject();
}

//-----------------------------------------------------------------------------
double *vtkCamera::GetOrientation()
{ return this->ViewTransform->GetOrientation(); };

//-----------------------------------------------------------------------------
double *vtkCamera::GetOrientationWXYZ()
{ return this->ViewTransform->GetOrientationWXYZ(); };

// ----------------------------------------------------------------------------
void vtkCamera::SetEyeTransformMatrix(const double elements[16])
{
  this->EyeTransformMatrix->Element[0][0] = elements[0];
  this->EyeTransformMatrix->Element[0][1] = elements[1];
  this->EyeTransformMatrix->Element[0][2] = elements[2];
  this->EyeTransformMatrix->Element[0][3] = elements[3];

  this->EyeTransformMatrix->Element[1][0] = elements[4];
  this->EyeTransformMatrix->Element[1][1] = elements[5];
  this->EyeTransformMatrix->Element[1][2] = elements[6];
  this->EyeTransformMatrix->Element[1][3] = elements[7];

  this->EyeTransformMatrix->Element[2][0] = elements[8];
  this->EyeTransformMatrix->Element[2][1] = elements[9];
  this->EyeTransformMatrix->Element[2][2] = elements[10];
  this->EyeTransformMatrix->Element[2][3] = elements[11];

  this->EyeTransformMatrix->Element[3][0] = elements[12];
  this->EyeTransformMatrix->Element[3][1] = elements[13];
  this->EyeTransformMatrix->Element[3][2] = elements[14];
  this->EyeTransformMatrix->Element[3][3] = elements[15];
  this->Modified();
}

// ----------------------------------------------------------------------------
void vtkCamera::SetModelTransformMatrix(const double elements[16])
{
  this->ModelTransformMatrix->Element[0][0] = elements[0];
  this->ModelTransformMatrix->Element[0][1] = elements[1];
  this->ModelTransformMatrix->Element[0][2] = elements[2];
  this->ModelTransformMatrix->Element[0][3] = elements[3];

  this->ModelTransformMatrix->Element[1][0] = elements[4];
  this->ModelTransformMatrix->Element[1][1] = elements[5];
  this->ModelTransformMatrix->Element[1][2] = elements[6];
  this->ModelTransformMatrix->Element[1][3] = elements[7];

  this->ModelTransformMatrix->Element[2][0] = elements[8];
  this->ModelTransformMatrix->Element[2][1] = elements[9];
  this->ModelTransformMatrix->Element[2][2] = elements[10];
  this->ModelTransformMatrix->Element[2][3] = elements[11];

  this->ModelTransformMatrix->Element[3][0] = elements[12];
  this->ModelTransformMatrix->Element[3][1] = elements[13];
  this->ModelTransformMatrix->Element[3][2] = elements[14];
  this->ModelTransformMatrix->Element[3][3] = elements[15];
  this->Modified();
}
