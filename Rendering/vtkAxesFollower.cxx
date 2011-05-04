/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxesFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxesFollower.h"

#include "vtkAxisActor.h"
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

vtkStandardNewMacro(vtkAxesFollower);

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
vtkAxesFollower::vtkAxesFollower() : vtkFollower()
{
  this->FollowAxes = -1;
  this->AutoCenter =  1;
  this->EnableLOD  =  0;
  this->LODFactor  =  0.80;

  this->ScreenOffset = 10.0;

  this->XAxis = NULL;
  this->YAxis = NULL;
  this->ZAxis = NULL;

  this->AxisPointingLeft = -1;

  this->InternalMatrix = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------
vtkAxesFollower::~vtkAxesFollower()
{
  this->InternalMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkAxesFollower::CalculateOrthogonalVectors(double *Rx, double *Ry, double *Rz,
                                                 vtkAxisActor *axis1, vtkAxisActor *axis2,
                                                 vtkRenderer *ren)
{
  vtkMatrix4x4* cameraMatrix = this->Camera->GetViewTransformMatrix();

  vtkCoordinate *c1Axis1 =  axis1->GetPoint1Coordinate();
  vtkCoordinate *c2Axis1 =  axis1->GetPoint2Coordinate();
  double *axis1Pt1 = c1Axis1->GetComputedWorldValue(ren);
  double *axis1Pt2 = c2Axis1->GetComputedWorldValue(ren);

  Rx[0] = axis1Pt2[0] - axis1Pt1[0];
  Rx[1] = axis1Pt2[1] - axis1Pt1[1];
  Rx[2] = axis1Pt2[2] - axis1Pt1[2];

  vtkCoordinate *c1Axis2 =  axis2->GetPoint1Coordinate();
  vtkCoordinate *c2Axis2 =  axis2->GetPoint2Coordinate();

  double *axis2Pt1 = c1Axis2->GetComputedWorldValue(ren);
  double *axis2Pt2 = c2Axis2->GetComputedWorldValue(ren);

  Ry[0] = axis2Pt2[0] - axis2Pt1[0];
  Ry[1] = axis2Pt2[1] - axis2Pt1[1];
  Ry[2] = axis2Pt2[2] - axis2Pt1[2];

  vtkMath::Normalize(Rx);
  vtkMath::Normalize(Ry);

  vtkMath::Cross(Rx, Ry, Rz);
  vtkMath::Normalize(Rz);

  double a[3], b[3];
  double *tranformedPt1 = cameraMatrix->MultiplyDoublePoint(axis1Pt1);
  a[0] = tranformedPt1[0];
  a[1] = tranformedPt1[1];
  a[2] = tranformedPt1[2];

  double *tranformedPt2 = cameraMatrix->MultiplyDoublePoint(axis1Pt2);
  b[0] = tranformedPt2[0];
  b[1] = tranformedPt2[1];
  b[2] = tranformedPt2[2];

  // If the axis second point pointing towards left we would like to have a 180
  // rotation around the vertical axis so that text would still be readable and drawn
  // left to right.
  if(b[0] < a[0])
    {
    this->AxisPointingLeft = 1;
    Rx[0] = -Rx[0];
    Rx[1] = -Rx[1];
    Rx[2] = -Rx[2];
    Rz[0] = -Rz[0];
    Rz[1] = -Rz[1];
    Rz[2] = -Rz[2];
    }
  else
    {
    this->AxisPointingLeft = 0;
    }
}

//----------------------------------------------------------------------------
double vtkAxesFollower::AutoScale(vtkViewport *viewport, vtkCamera *camera,
                                  double screenOffset, double position[])
{
  if(!viewport || !camera || !position)
    {
    vtkErrorMacro("Invalid or NULL arguments \n");
    return 0;
    }

  double factor = 1;
  if (viewport->GetSize()[1] > 0)
    {
    factor = 2.0 * screenOffset
      * tan(vtkMath::RadiansFromDegrees(camera->GetViewAngle()/2.0))
      / viewport->GetSize()[1];
    }

    double dist = sqrt(
          vtkMath::Distance2BetweenPoints(position,
                                          camera->GetPosition()));
    double newScale = factor * dist;

    return newScale;
}

//----------------------------------------------------------------------------
void vtkAxesFollower::ComputeTransformMatrix(vtkRenderer *ren)
{
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

    // Move to pivto point.
    this->Transform->Translate(-pivotPoint[0],
                               -pivotPoint[1],
                               -pivotPoint[2]);
    // scale
    this->Transform->Scale(this->Scale[0],
                           this->Scale[1],
                           this->Scale[2]);

    // rotate
    this->Transform->RotateY(this->Orientation[1]);
    this->Transform->RotateX(this->Orientation[0]);
    this->Transform->RotateZ(this->Orientation[2]);

    double translation[3] = {0.0, 0.0, 0.0};
    if (this->XAxis && this->YAxis && this->ZAxis)
      {
      vtkMatrix4x4 *matrix = this->InternalMatrix;
      matrix->Identity();
      double Rx[3], Ry[3], Rz[3];

      // Follow X axis.
      if(this->FollowAxes == VTK_AXIS_TYPE_X)
        {
        this->ComputeRotationAndTranlation(ren, translation, Rx, Ry, Rz, this->XAxis,
                                           this->YAxis, this->ZAxis);
        }
      else if(this->FollowAxes == VTK_AXIS_TYPE_Y)
        {
        this->ComputeRotationAndTranlation(ren, translation, Rx, Ry, Rz, this->YAxis,
                                           this->XAxis, this->ZAxis);
        }
      else if(this->FollowAxes == VTK_AXIS_TYPE_Z)
        {
        this->ComputeRotationAndTranlation(ren, translation, Rx, Ry, Rz, this->ZAxis,
                                           this->XAxis, this->YAxis);
        }
      else
        {
        // Do nothing.
        return;
        }

      vtkMath::Normalize(Rx);
      vtkMath::Normalize(Ry);
      vtkMath::Normalize(Rz);

      matrix->Element[0][0] = Rx[0];
      matrix->Element[1][0] = Rx[1];
      matrix->Element[2][0] = Rx[2];
      matrix->Element[0][1] = Ry[0];
      matrix->Element[1][1] = Ry[1];
      matrix->Element[2][1] = Ry[2];
      matrix->Element[0][2] = Rz[0];
      matrix->Element[1][2] = Rz[1];
      matrix->Element[2][2] = Rz[2];

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
void vtkAxesFollower::ComputeRotationAndTranlation(vtkRenderer *ren, double translation[],
                                                   double Rx[], double Ry[], double Rz[],
                                                   vtkAxisActor *xAxis,
                                                   vtkAxisActor *orthoAxis1,
                                                   vtkAxisActor *orthoAxis2)
{
  double Ry1[3], Rz1[3];
  double Ry2[3], Rz2[3];

  double autoScaleFactor = this->AutoScale(ren, this->Camera, this->ScreenOffset, this->Position);

  this->CalculateOrthogonalVectors(Rx,  Ry1, Rz1,  xAxis, orthoAxis1, ren);
  this->CalculateOrthogonalVectors(Rx,  Ry2, Rz2, xAxis, orthoAxis2, ren);

  double dop[3];
  this->Camera->GetDirectionOfProjection(dop);
  vtkMath::Normalize(dop);

  double val1 = vtkMath::Dot(Rz1, dop);
  double val2 = vtkMath::Dot(Rz2, dop);

  double origRy[3] = {0.0, 0.0, 0.0};

  if(fabs(val1) > fabs(val2))
    {
    translation[0] =  -Ry1[0] * autoScaleFactor;
    translation[1] =  -Ry1[1] * autoScaleFactor;
    translation[2] =  -Ry1[2] * autoScaleFactor;

    origRy[0] = Ry1[0];
    origRy[1] = Ry1[1];
    origRy[2] = Ry1[2];

    Ry[0] = Ry1[0];
    Ry[1] = Ry1[1];
    Ry[2] = Ry1[2];

    Rz[0] = Rz1[0];
    Rz[1] = Rz1[1];
    Rz[2] = Rz1[2];

    // NOTE: Basically the idea here is that val1 will be positive
    // only when we have projection direction aligned with our z directon
    // and when that happens it means that our Y is inverted.
    // Similar operation we have to do when val2 > 0.
    if(val1 > 0)
      {
      Ry[0] = -Ry[0];
      Ry[1] = -Ry[1];
      Ry[2] = -Ry[2];
      }
    }
  else
    {
    translation[0] =  -Ry2[0] * autoScaleFactor;
    translation[1] =  -Ry2[1] * autoScaleFactor;
    translation[2] =  -Ry2[2] * autoScaleFactor;

    origRy[0] = Ry2[0];
    origRy[1] = Ry2[1];
    origRy[2] = Ry2[2];

    Ry[0] = Ry2[0];
    Ry[1] = Ry2[1];
    Ry[2] = Ry2[2];

    Rz[0] = Rz2[0];
    Rz[1] = Rz2[1];
    Rz[2] = Rz2[2];

    if(val2 > 0)
      {
      Ry[0] = -Ry[0];
      Ry[1] = -Ry[1];
      Ry[2] = -Ry[2];
      }
    }

  // Since we already stored all the possible Y axis that would go toward the geometry we need to
  // compare if the Y axis defined by the our math aligns with these. If not then we got inverted Y
  // and hence need to invert the translation too.
  int axisPosition = -1;
  if(this->FollowAxes == VTK_AXIS_TYPE_X)
    {
    axisPosition = this->XAxis->GetAxisPosition();
    }
  else if(this->FollowAxes == VTK_AXIS_TYPE_Y)
    {
    axisPosition = this->YAxis->GetAxisPosition();
    }
  else if(this->FollowAxes == VTK_AXIS_TYPE_Z)
    {
    axisPosition = this->ZAxis->GetAxisPosition();
    }
  else
    {
    // Do nothing.
    }

    if(!(vtkMath::Dot(AxisAlignedY[this->FollowAxes][axisPosition][0], origRy) > 0) &&
       !(vtkMath::Dot(AxisAlignedY[this->FollowAxes][axisPosition][1], origRy) > 0) )
    {
    translation[0] = -translation[0];
    translation[1] = -translation[1];
    translation[2] = -translation[2];
    }
}

//----------------------------------------------------------------------
void vtkAxesFollower::ComputerAutoCenterTranslation(
  const double& vtkNotUsed(autoScaleFactor), double translation[3])
{
  if(!translation)
    {
    vtkErrorMacro("ERROR: Invalid or NULL translation\n");
    return;
    }

  double *bounds    = this->GetMapper()->GetBounds();

  // Offset by half of width.
  double halfWidth  = (bounds[1] - bounds[0]) * 0.5 * this->Scale[0];

  if(this->AxisPointingLeft == 1)
    {
    halfWidth  = -halfWidth;
    }

  if(this->FollowAxes == VTK_AXIS_TYPE_X)
    {
    translation[0] = translation[0] - halfWidth;
    }
  else if(this->FollowAxes == VTK_AXIS_TYPE_Y)
    {
    translation[1] = translation[1] - halfWidth;
    }
  else if(this->FollowAxes == VTK_AXIS_TYPE_Z)
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
int vtkAxesFollower::EvaluateVisibility()
{
  if(!this->Camera->GetParallelProjection())
    {
    double cameraClippingRange[2];

    this->Camera->GetClippingRange(cameraClippingRange);

    // We are considering the far clip plane for evaluation. In certain
    // odd conditions it might not work.
    const double maxVisibleDistanceFromCamera = this->LODFactor * (cameraClippingRange[1]);

    double dist = sqrt(vtkMath::Distance2BetweenPoints(this->Camera->GetPosition(),
                                                       this->Position));

    if(dist > maxVisibleDistanceFromCamera)
      {
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
void vtkAxesFollower::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FollowAxes: ("  << this->FollowAxes   << ")\n";
  os << indent << "AutoCenter: ("  << this->AutoCenter   << ")\n";
  os << indent << "EnableLOD: ("   << this->EnableLOD    << ")\n";
  os << indent << "LODFactor: ("   << this->LODFactor    << ")\n";
  os << indent << "ScreenOffset: ("<< this->ScreenOffset << ")\n";

  if ( this->XAxis )
    {
    os << indent << "XAxis: (" << this->XAxis << ")\n";
    }
  else
    {
    os << indent << "XAxis: (none)\n";
    }

  if ( this->YAxis )
    {
    os << indent << "YAxis: (" << this->YAxis << ")\n";
    }
  else
    {
    os << indent << "YAxis: (none)\n";
    }

  if ( this->ZAxis )
    {
    os << indent << "ZAxis: (" << this->ZAxis << ")\n";
    }
  else
    {
    os << indent << "ZAxis: (none)\n";
    }

  if ( this->Camera )
    {
    os << indent << "Camera: (" << this->Camera << ")\n";
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }

  if ( this->Device )
    {
    os << indent << "Device: (" << this->Device << ")\n";
    }
  else
    {
    os << indent << "Device: (none)\n";
    }
}

//----------------------------------------------------------------------
int vtkAxesFollower::RenderOpaqueGeometry(vtkViewport *vp)
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
int vtkAxesFollower::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
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
void vtkAxesFollower::ReleaseGraphicsResources(vtkWindow *w)
{
  this->Device->ReleaseGraphicsResources(w);
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkAxesFollower::HasTranslucentPolygonalGeometry()
{
  if ( ! this->Mapper )
    {
    return 0;
    }
  // make sure we have a property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  // is this actor opaque ?
  return !this->GetIsOpaque();
}

//-----------------------------------------------------------------------------
// This causes the actor to be rendered. It, in turn, will render the actor's
// property and then mapper.
void vtkAxesFollower::Render(vtkRenderer *ren)
{
  if(this->EnableLOD && !this->EvaluateVisibility())
    {
    this->SetVisibility(this->EvaluateVisibility());
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

  this->Device->Render(ren,this->Mapper);
}

//----------------------------------------------------------------------
void vtkAxesFollower::ShallowCopy(vtkProp *prop)
{
  vtkAxesFollower *f = vtkAxesFollower::SafeDownCast(prop);
  if ( f != NULL )
    {
    this->SetFollowAxes(f->GetFollowAxes());
    this->SetAutoCenter(f->GetAutoCenter());
    this->SetEnableLOD(f->GetEnableLOD());
    this->SetLODFactor(f->GetLODFactor());
    this->SetScreenOffset(f->GetScreenOffset());
    this->SetAxes(f->XAxis, f->YAxis, f->ZAxis);
    }

  // Now do superclass
  this->vtkActor::ShallowCopy(prop);
}
