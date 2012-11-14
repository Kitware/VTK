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

#include "vtkAxisFollower.h"

#include "vtkAxisActor.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include <math.h>

vtkStandardNewMacro(vtkAxisFollower);

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
vtkAxisFollower::vtkAxisFollower() : vtkFollower()
{
  this->AutoCenter                = 1;

  this->EnableDistanceLOD         = 0;
  this->DistanceLODThreshold      = 0.80;

  this->EnableViewAngleLOD        = 1;
  this->ViewAngleLODThreshold     = 0.34;

  this->ScreenOffset              = 10.0;

  this->Axis                      = NULL;

  this->TextUpsideDown          = -1;
  this->VisibleAtCurrentViewAngle = -1;

  this->InternalMatrix = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------
vtkAxisFollower::~vtkAxisFollower()
{
  this->InternalMatrix->Delete();
}

//----------------------------------------------------------------------
void vtkAxisFollower::SetAxis(vtkAxisActor *axis)
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
vtkAxisActor* vtkAxisFollower::GetAxis()
{
  return this->Axis.GetPointer();
}

//----------------------------------------------------------------------------
void vtkAxisFollower::CalculateOrthogonalVectors(double rX[3], double rY[3],
  double rZ[3], vtkAxisActor *axis, double *dop, vtkRenderer *ren)
{
  if(!rX || !rY || !rZ)
    {
    vtkErrorMacro("Invalid or NULL direction vectors\n");
    return;
    }

  if(!axis)
    {
    vtkErrorMacro("Invalid or NULL axis\n");
    return;
    }

  if(!dop)
    {
    vtkErrorMacro("Invalid or NULL direction of projection vector\n");
    return;
    }

  if(!ren)
    {
    vtkErrorMacro("Invalid or NULL renderer\n");
    return;
    }

  vtkMatrix4x4* cameraMatrix = this->Camera->GetViewTransformMatrix();

  vtkCoordinate *c1Axis =  axis->GetPoint1Coordinate();
  vtkCoordinate *c2Axis =  axis->GetPoint2Coordinate();
  double *axisPt1 = c1Axis->GetComputedWorldValue(ren);
  double *axisPt2 = c2Axis->GetComputedWorldValue(ren);

  rX[0] = axisPt2[0] - axisPt1[0];
  rX[1] = axisPt2[1] - axisPt1[1];
  rX[2] = axisPt2[2] - axisPt1[2];
  vtkMath::Normalize(rX);

  // Get Y
  vtkMath::Cross(rX, dop, rY);
  vtkMath::Normalize(rY);

  // Get Z
  vtkMath::Cross(rX, rY, rZ);
  vtkMath::Normalize(rZ);

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
double vtkAxisFollower::AutoScale(vtkViewport *viewport, vtkCamera *camera,
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
void vtkAxisFollower::ComputeTransformMatrix(vtkRenderer *ren)
{
  if(!this->Axis)
    {
    vtkErrorMacro("ERROR: Invalid axis\n");
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

    double pivotPoint[3] =
    {
      this->Origin[0],
      this->Origin[1],
      this->Origin[2]
    };

    if(this->AutoCenter)
      {
      this->GetMapper()->GetCenter(pivotPoint);
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

      this->ComputeRotationAndTranlation(ren, translation, rX, rY, rZ, this->Axis);

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
}

//-----------------------------------------------------------------------------
void vtkAxisFollower::ComputeRotationAndTranlation(vtkRenderer *ren, double translation[3],
                                                   double rX[3], double rY[3], double rZ[3],
                                                   vtkAxisActor *axis)
{
  double autoScaleFactor =
    this->AutoScale(ren, this->Camera, this->ScreenOffset, this->Position);

  double dop[3];
  this->Camera->GetDirectionOfProjection(dop);
  vtkMath::Normalize(dop);

  this->CalculateOrthogonalVectors(rX, rY, rZ, axis, dop, ren);

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
void vtkAxisFollower::ComputerAutoCenterTranslation(
  const double& vtkNotUsed(autoScaleFactor), double translation[3])
{
  if(!translation)
    {
    vtkErrorMacro("ERROR: Invalid or NULL translation\n");
    return;
    }

  double *bounds = this->GetMapper()->GetBounds();

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
int vtkAxisFollower::TestDistanceVisibility()
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
void vtkAxisFollower::ExecuteViewAngleVisibility(double normal[3])
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
void vtkAxisFollower::PrintSelf(ostream& os, vtkIndent indent)
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
int vtkAxisFollower::RenderOpaqueGeometry(vtkViewport *vp)
{
  if ( ! this->Mapper )
    {
    return 0;
    }

  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  if (this->GetIsOpaque())
    {
    vtkRenderer *ren = static_cast<vtkRenderer *>(vp);
    this->Render(ren);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkAxisFollower::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
{
  if ( ! this->Mapper )
    {
    return 0;
    }

  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  if (!this->GetIsOpaque())
    {
    vtkRenderer *ren = static_cast<vtkRenderer *>(vp);
    this->Render(ren);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkAxisFollower::Render(vtkRenderer *ren)
{
  if(this->EnableDistanceLOD && !this->TestDistanceVisibility())
    {
    this->SetVisibility(0);
    return;
    }

  this->Property->Render(this, ren);

  this->Device->SetProperty (this->Property);
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
    }

  /* render the texture */
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }

  // make sure the device has the same matrix
  this->ComputeTransformMatrix(ren);
  this->Device->SetUserMatrix(this->Matrix);

  if(this->VisibleAtCurrentViewAngle)
    {
    this->Device->Render(ren,this->Mapper);
    }
  else
    {
    this->SetVisibility(this->VisibleAtCurrentViewAngle);
    }
}

//----------------------------------------------------------------------
void vtkAxisFollower::ShallowCopy(vtkProp *prop)
{
  vtkAxisFollower *f = vtkAxisFollower::SafeDownCast(prop);
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

bool vtkAxisFollower::IsTextUpsideDown( double* a, double* b )
{
  double angle = vtkMath::RadiansFromDegrees(this->Orientation[2]);
  return (b[0] - a[0]) * cos(angle) - (b[1] - a[1]) * sin(angle) < 0;
}
