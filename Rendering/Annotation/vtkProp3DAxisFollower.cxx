/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DAxisFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProp3DAxisFollower.h"

#include "vtkAxisActor.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkViewport.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include <math.h>

vtkStandardNewMacro(vtkProp3DAxisFollower);

// List of vectors per axis (depending on which one needs to be
// followed.
// Order here is X, Y, and Z.
// Set of two axis aligned vectors that would define the Y vector.
// Order is MINMIN, MINMAX, MAXMAX, MAXMIN
namespace
{
  const double AxisAlignedY[3][4][2][3] =
  {
    { {{0.0,  1.0, 0.0}, {0.0, 0.0,  1.0}},
      {{0.0,  1.0, 0.0}, {0.0, 0.0, -1.0}},
      {{0.0, -1.0, 0.0}, {0.0, 0.0, -1.0}},
      {{0.0, -1.0, 0.0}, {0.0, 0.0,  1.0}}
    },
    {
      {{ 1.0, 0.0, 0.0}, {0.0, 0.0,  1.0}},
      {{ 1.0, 0.0, 0.0}, {0.0, 0.0, -1.0}},
      {{-1.0, 0.0, 0.0}, {0.0, 0.0, -1.0}},
      {{-1.0, 0.0, 0.0}, {0.0, 0.0,  1.0}}
    },
    {
      {{ 1.0, 0.0, 0.0},  {0.0,  1.0, 0.0}},
      {{ 1.0, 0.0, 0.0},  {0.0, -1.0, 0.0}},
      {{-1.0, 0.0, 0.0},  {0.0, -1.0, 0.0}},
      {{-1.0, 0.0, 0.0},  {0.0,  1.0, 0.0}}
    }
  };
}
//----------------------------------------------------------------------
// Creates a follower with no camera set
vtkProp3DAxisFollower::vtkProp3DAxisFollower()
{
  this->AutoCenter                = 1;

  this->EnableDistanceLOD         = 0;
  this->DistanceLODThreshold      = 0.80;

  this->EnableViewAngleLOD        = 1;
  this->ViewAngleLODThreshold     = 0.34;

  this->ScreenOffset              = 10.0;

  this->Axis                      = NULL;
  this->Viewport                  = NULL;

  this->TextUpsideDown            = -1;
  this->VisibleAtCurrentViewAngle = -1;
}

//----------------------------------------------------------------------
vtkProp3DAxisFollower::~vtkProp3DAxisFollower()
{
}

//----------------------------------------------------------------------
void vtkProp3DAxisFollower::SetAxis(vtkAxisActor *axis)
{
  if(!axis)
    {
    vtkErrorMacro("Invalid or NULL axis\n");
    return;
    }

  if(this->Axis != axis)
    {
    // \NOTE: Don't increment the ref count of axis as it could lead to
    // circular references.
    this->Axis = axis;
    this->Modified();
    }
}

//----------------------------------------------------------------------
vtkAxisActor* vtkProp3DAxisFollower::GetAxis()
{
  return this->Axis.GetPointer();
}


//----------------------------------------------------------------------
void vtkProp3DAxisFollower::SetViewport(vtkViewport* vp)
{
  if(this->Viewport != vp)
    {
    // \NOTE: Don't increment the ref count of vtkViewport as it could lead to
    // circular references.
    this->Viewport = vp;
    this->Modified();
    }
}

//----------------------------------------------------------------------
vtkViewport* vtkProp3DAxisFollower::GetViewport()
{
  return this->Viewport.GetPointer();
}

//----------------------------------------------------------------------------
void vtkProp3DAxisFollower::CalculateOrthogonalVectors(double rX[3], double rY[3],
  double rZ[3], vtkAxisActor *axis, double *dop, vtkViewport *viewport)
{
  if (!rX || !rY || !rZ)
    {
    vtkErrorMacro("Invalid or NULL direction vectors\n");
    return;
    }

  if (!axis)
    {
    vtkErrorMacro("Invalid or NULL axis\n");
    return;
    }

  if (!dop)
    {
    vtkErrorMacro("Invalid or NULL direction of projection vector\n");
    return;
    }

  if (!viewport)
    {
    vtkErrorMacro("Invalid or NULL renderer\n");
    return;
    }

  vtkMatrix4x4* cameraMatrix = this->Camera->GetViewTransformMatrix();

  vtkCoordinate *c1Axis =  axis->GetPoint1Coordinate();
  vtkCoordinate *c2Axis =  axis->GetPoint2Coordinate();
  double *axisPt1 = c1Axis->GetComputedWorldValue(viewport);
  double *axisPt2 = c2Axis->GetComputedWorldValue(viewport);

  rX[0] = axisPt2[0] - axisPt1[0];
  rX[1] = axisPt2[1] - axisPt1[1];
  rX[2] = axisPt2[2] - axisPt1[2];
  vtkMath::Normalize(rX);

  if (rX[0] != dop[0] || rX[1] != dop[1] || rX[2] != dop[2])
    {
    // Get Y
    vtkMath::Cross(rX, dop, rY);
    vtkMath::Normalize(rY);

    // Get Z
    vtkMath::Cross(rX, rY, rZ);
    vtkMath::Normalize(rZ);
    }
  else
    {
    vtkMath::Perpendiculars(rX, rY, rZ, 0.);
    }
  double a[3], b[3];

  // Need homogeneous points.
  double homoPt1[4] = {axisPt1[0], axisPt1[1], axisPt1[2], 1.0};
  double homoPt2[4] = {axisPt2[0], axisPt2[1], axisPt2[2], 1.0};

  double *viewCoordinatePt1 = cameraMatrix->MultiplyDoublePoint(homoPt1);
  a[0] = viewCoordinatePt1[0];
  a[1] = viewCoordinatePt1[1];
  a[2] = viewCoordinatePt1[2];

  double *viewCoordinatePt2 = cameraMatrix->MultiplyDoublePoint(homoPt2);
  b[0] = viewCoordinatePt2[0];
  b[1] = viewCoordinatePt2[1];
  b[2] = viewCoordinatePt2[2];

  // If the text is upside down, we make a 180 rotation to keep it readable.
  if(this->IsTextUpsideDown(a, b))
    {
    this->TextUpsideDown = 1;
    rX[0] = -rX[0];
    rX[1] = -rX[1];
    rX[2] = -rX[2];
    rZ[0] = -rZ[0];
    rZ[1] = -rZ[1];
    rZ[2] = -rZ[2];
    }
  else
    {
    this->TextUpsideDown = 0;
    }
}

//----------------------------------------------------------------------------
double vtkProp3DAxisFollower::AutoScale(vtkViewport *viewport, vtkCamera *camera,
                                        double screenSize, double position[3])
{
  double newScale = 0.0;

  if(!viewport)
    {
    std::cerr << "Invalid or NULL viewport \n";
    return newScale;
    }

  if(!camera)
    {
    std::cerr << "Invalid or NULL camera \n";
    return newScale;
    }

  if(!position)
    {
    std::cerr << "Invalid or NULL position \n";
    return newScale;
    }

  double factor = 1;
  if (viewport->GetSize()[1] > 0)
    {
    factor = 2.0 * screenSize
      * tan(vtkMath::RadiansFromDegrees(camera->GetViewAngle()/2.0))
      / viewport->GetSize()[1];
    }

    double dist = sqrt(
          vtkMath::Distance2BetweenPoints(position,
                                          camera->GetPosition()));
    newScale = factor * dist;

    return newScale;
}

//----------------------------------------------------------------------------
void vtkProp3DAxisFollower::ComputeMatrix()
{
  if(!this->Axis)
    {
    vtkErrorMacro("ERROR: Invalid axis\n");
    return;
    }

  if (this->EnableDistanceLOD && !this->TestDistanceVisibility())
    {
    this->SetVisibility(0);
    return;
    }

  // check whether or not need to rebuild the matrix
  if ( this->GetMTime() > this->MatrixMTime ||
       (this->Camera && this->Camera->GetMTime() > this->MatrixMTime) )
    {
    this->GetOrientation();
    this->Transform->Push();
    this->Transform->Identity();
    this->Transform->PostMultiply();
    this->Transform->GetMatrix(this->Matrix);

    double pivotPoint[3] =
    {
      this->Origin[0],
      this->Origin[1],
      this->Origin[2]
    };

    if(this->AutoCenter)
      {
      // Don't apply the user matrix when retrieving the center.
      this->Device->SetUserMatrix(NULL);

      double* center = this->Device->GetCenter();
      pivotPoint[0] = center[0];
      pivotPoint[1] = center[1];
      pivotPoint[2] = center[2];
      }

    // Move pivot point to origin
    this->Transform->Translate(-pivotPoint[0],
                               -pivotPoint[1],
                               -pivotPoint[2]);
    // Scale
    this->Transform->Scale(this->Scale[0],
                           this->Scale[1],
                           this->Scale[2]);

    // Rotate
    this->Transform->RotateY(this->Orientation[1]);
    this->Transform->RotateX(this->Orientation[0]);
    this->Transform->RotateZ(this->Orientation[2]);

    double translation[3] = {0.0, 0.0, 0.0};
    if (this->Axis)
      {
      vtkMatrix4x4 *matrix = this->InternalMatrix;
      matrix->Identity();
      double rX[3], rY[3], rZ[3];

      this->ComputeRotationAndTranlation(this->Viewport, translation, rX, rY, rZ, this->Axis);

      vtkMath::Normalize(rX);
      vtkMath::Normalize(rY);
      vtkMath::Normalize(rZ);

      matrix->Element[0][0] = rX[0];
      matrix->Element[1][0] = rX[1];
      matrix->Element[2][0] = rX[2];
      matrix->Element[0][1] = rY[0];
      matrix->Element[1][1] = rY[1];
      matrix->Element[2][1] = rY[2];
      matrix->Element[0][2] = rZ[0];
      matrix->Element[1][2] = rZ[1];
      matrix->Element[2][2] = rZ[2];

      this->Transform->Concatenate(matrix);
      }

    this->Transform->Translate(this->Origin[0] + this->Position[0] + translation[0],
                               this->Origin[1] + this->Position[1] + translation[1],
                               this->Origin[2] + this->Position[2] + translation[2]);

    // Apply user defined matrix last if there is one
    if (this->UserMatrix)
      {
      this->Transform->Concatenate(this->UserMatrix);
      }

    this->Transform->PreMultiply();
    this->Transform->GetMatrix(this->Matrix);
    this->MatrixMTime.Modified();
    this->Transform->Pop();
    }

  this->SetVisibility(this->VisibleAtCurrentViewAngle);
}

//-----------------------------------------------------------------------------
void vtkProp3DAxisFollower
::ComputeRotationAndTranlation(vtkViewport* viewport, double translation[3],
                               double rX[3], double rY[3], double rZ[3],
                               vtkAxisActor *axis)
{
  double autoScaleFactor =
    this->AutoScale(viewport, this->Camera, this->ScreenOffset, this->Position);

  double dop[3];
  this->Camera->GetDirectionOfProjection(dop);
  vtkMath::Normalize(dop);

  this->CalculateOrthogonalVectors(rX, rY, rZ, axis, dop, this->Viewport);

  double dotVal = vtkMath::Dot(rZ, dop);

  double origRy[3] = {0.0, 0.0, 0.0};

  origRy[0] = rY[0];
  origRy[1] = rY[1];
  origRy[2] = rY[2];

  // NOTE: Basically the idea here is that dotVal will be positive
  // only when we have projection direction aligned with our z directon
  // and when that happens it means that our Y is inverted.
  if(dotVal > 0)
    {
    rY[0] = -rY[0];
    rY[1] = -rY[1];
    rY[2] = -rY[2];
    }

  // Check visibility at current view angle.
  if(this->EnableViewAngleLOD)
    {
    this->ExecuteViewAngleVisibility(rZ);
    }

  // Since we already stored all the possible Y axes that are geometry aligned,
  // we compare our vertical vector with these vectors and if it aligns then we
  // translate in opposite direction.
  int axisPosition = this->Axis->GetAxisPosition();

  double dotVal1 = vtkMath::Dot(AxisAlignedY[this->Axis->GetAxisType()][axisPosition][0], origRy) ;
  double dotVal2 = vtkMath::Dot(AxisAlignedY[this->Axis->GetAxisType()][axisPosition][1], origRy) ;

  if(fabs(dotVal1) > fabs(dotVal2))
    {
    int sign = (dotVal1 > 0 ? -1 : 1);

    translation[0] =  origRy[0] * autoScaleFactor * sign;
    translation[1] =  origRy[1] * autoScaleFactor * sign;
    translation[2] =  origRy[2] * autoScaleFactor * sign;
    }
  else
    {
    int sign = (dotVal2 > 0 ? -1 : 1);

    translation[0] =  origRy[0] * autoScaleFactor * sign;
    translation[1] =  origRy[1] * autoScaleFactor * sign;
    translation[2] =  origRy[2] * autoScaleFactor * sign;
    }
}

//----------------------------------------------------------------------
void vtkProp3DAxisFollower::ComputerAutoCenterTranslation(
  const double& vtkNotUsed(autoScaleFactor), double translation[3])
{
  if(!translation)
    {
    vtkErrorMacro("ERROR: Invalid or NULL translation\n");
    return;
    }

  double *bounds = this->GetProp3D()->GetBounds();

  // Offset by half of width.
  double halfWidth  = (bounds[1] - bounds[0]) * 0.5 * this->Scale[0];

  if(this->TextUpsideDown == 1)
    {
    halfWidth  = -halfWidth;
    }

  if(this->Axis->GetAxisType() == VTK_AXIS_TYPE_X)
    {
    translation[0] = translation[0] - halfWidth;
    }
  else if(this->Axis->GetAxisType() == VTK_AXIS_TYPE_Y)
    {
    translation[1] = translation[1] - halfWidth;
    }
  else if(this->Axis->GetAxisType() == VTK_AXIS_TYPE_Z)
    {
    translation[2] = translation[2] - halfWidth;
    }
  else
    {
    // Do nothing.
    }

  return;
}

//----------------------------------------------------------------------
int vtkProp3DAxisFollower::TestDistanceVisibility()
{
  if(!this->Camera->GetParallelProjection())
    {
    double cameraClippingRange[2];

    this->Camera->GetClippingRange(cameraClippingRange);

    // We are considering the far clip plane for evaluation. In certain
    // odd conditions it might not work.
    const double maxVisibleDistanceFromCamera = this->DistanceLODThreshold * (cameraClippingRange[1]);

    double dist = sqrt(vtkMath::Distance2BetweenPoints(this->Camera->GetPosition(),
                                                       this->Position));

    if(dist > maxVisibleDistanceFromCamera)
      {
      // Need to make sure we are not looking at a flat axis and therefore should enable it anyway
      if(this->Axis)
        {
        vtkBoundingBox bbox(this->Axis->GetBounds());
        return (bbox.GetDiagonalLength() > (cameraClippingRange[1] - cameraClippingRange[0])) ? 1 : 0;
        }
      return 0;
      }
    else
      {
      return 1;
      }
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------
void vtkProp3DAxisFollower::ExecuteViewAngleVisibility(double normal[3])
{
  if(!normal)
    {
    vtkErrorMacro("ERROR: Invalid or NULL normal\n");
    return;
    }

  double *cameraPos = this->Camera->GetPosition();
  double  dir[3] = {this->Position[0] - cameraPos[0],
                    this->Position[1] - cameraPos[1],
                    this->Position[2] - cameraPos[2]};
  vtkMath::Normalize(dir);
  double dotDir = vtkMath::Dot(dir, normal);
  if( fabs(dotDir) < this->ViewAngleLODThreshold )
    {
    this->VisibleAtCurrentViewAngle = 0;
    }
  else
    {
    this->VisibleAtCurrentViewAngle = 1;
    }
}

//----------------------------------------------------------------------
void vtkProp3DAxisFollower::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutoCenter: ("  << this->AutoCenter   << ")\n";
  os << indent << "EnableDistanceLOD: ("   << this->EnableDistanceLOD    << ")\n";
  os << indent << "DistanceLODThreshold: ("   << this->DistanceLODThreshold    << ")\n";
  os << indent << "EnableViewAngleLOD: ("   << this->EnableViewAngleLOD    << ")\n";
  os << indent << "ViewAngleLODThreshold: ("   << this->ViewAngleLODThreshold    << ")\n";
  os << indent << "ScreenOffset: ("<< this->ScreenOffset << ")\n";

  if ( this->Axis )
    {
    os << indent << "Axis: (" << this->Axis << ")\n";
    }
  else
    {
    os << indent << "Axis: (none)\n";
    }
}

//----------------------------------------------------------------------
void vtkProp3DAxisFollower::ShallowCopy(vtkProp *prop)
{
  vtkProp3DAxisFollower *f = vtkProp3DAxisFollower::SafeDownCast(prop);
  if ( f != NULL )
    {
    this->SetAutoCenter(f->GetAutoCenter());
    this->SetEnableDistanceLOD(f->GetEnableDistanceLOD());
    this->SetDistanceLODThreshold(f->GetDistanceLODThreshold());
    this->SetEnableViewAngleLOD(f->GetEnableViewAngleLOD());
    this->SetViewAngleLODThreshold(f->GetViewAngleLODThreshold());
    this->SetScreenOffset(f->GetScreenOffset());
    this->SetAxis(f->GetAxis());
    }

  // Now do superclass
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
bool vtkProp3DAxisFollower::IsTextUpsideDown( double* a, double* b )
{
  double angle = vtkMath::RadiansFromDegrees(this->Orientation[2]);
  return (b[0] - a[0]) * cos(angle) - (b[1] - a[1]) * sin(angle) < 0;
}

//----------------------------------------------------------------------
int vtkProp3DAxisFollower::RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->SetViewport(viewport);
  return this->Superclass::RenderOpaqueGeometry(viewport);
}

//----------------------------------------------------------------------
int vtkProp3DAxisFollower::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  this->SetViewport(viewport);
  return this->Superclass::RenderTranslucentPolygonalGeometry(viewport);
}

//----------------------------------------------------------------------
int vtkProp3DAxisFollower::RenderVolumetricGeometry(vtkViewport *viewport)
{
  this->SetViewport(viewport);
  return this->Superclass::RenderVolumetricGeometry(viewport);
}
