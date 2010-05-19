/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxesActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAxesActor.h"

#include "vtkActor.h"
#include "vtkCaptionActor2D.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkLineSource.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkAxesActor);

vtkCxxSetObjectMacro( vtkAxesActor, UserDefinedTip, vtkPolyData );
vtkCxxSetObjectMacro( vtkAxesActor, UserDefinedShaft, vtkPolyData );

//----------------------------------------------------------------------------
vtkAxesActor::vtkAxesActor()
{
  this->AxisLabels = 1;

  this->XAxisLabelText = NULL;
  this->YAxisLabelText = NULL;
  this->ZAxisLabelText = NULL;

  this->SetXAxisLabelText("X");
  this->SetYAxisLabelText("Y");
  this->SetZAxisLabelText("Z");

  this->XAxisShaft = vtkActor::New();
  this->XAxisShaft->GetProperty()->SetColor(1, 0, 0);
  this->YAxisShaft = vtkActor::New();
  this->YAxisShaft->GetProperty()->SetColor(0, 1, 0);
  this->ZAxisShaft = vtkActor::New();
  this->ZAxisShaft->GetProperty()->SetColor(0, 0, 1);

  this->XAxisTip = vtkActor::New();
  this->XAxisTip->GetProperty()->SetColor(1, 0, 0);
  this->YAxisTip = vtkActor::New();
  this->YAxisTip->GetProperty()->SetColor(0, 1, 0);
  this->ZAxisTip = vtkActor::New();
  this->ZAxisTip->GetProperty()->SetColor(0, 0, 1);

  this->CylinderSource = vtkCylinderSource::New();
  this->CylinderSource->SetHeight(1.0);

  this->LineSource = vtkLineSource::New();
  this->LineSource->SetPoint1( 0.0, 0.0, 0.0 );
  this->LineSource->SetPoint2( 0.0, 1.0, 0.0 );

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetDirection( 0, 1, 0 );
  this->ConeSource->SetHeight( 1.0 );

  this->SphereSource = vtkSphereSource::New();

  vtkPolyDataMapper *shaftMapper = vtkPolyDataMapper::New();

  this->XAxisShaft->SetMapper( shaftMapper );
  this->YAxisShaft->SetMapper( shaftMapper );
  this->ZAxisShaft->SetMapper( shaftMapper );

  shaftMapper->Delete();

  vtkPolyDataMapper *tipMapper = vtkPolyDataMapper::New();

  this->XAxisTip->SetMapper( tipMapper );
  this->YAxisTip->SetMapper( tipMapper );
  this->ZAxisTip->SetMapper( tipMapper );

  tipMapper->Delete();

  this->TotalLength[0] = 1.0;
  this->TotalLength[1] = 1.0;
  this->TotalLength[2] = 1.0;

  this->NormalizedShaftLength[0] = 0.8;
  this->NormalizedShaftLength[1] = 0.8;
  this->NormalizedShaftLength[2] = 0.8;

  this->NormalizedTipLength[0] = 0.2;
  this->NormalizedTipLength[1] = 0.2;
  this->NormalizedTipLength[2] = 0.2;

  this->NormalizedLabelPosition[0] = 1.0;
  this->NormalizedLabelPosition[1] = 1.0;
  this->NormalizedLabelPosition[2] = 1.0;

  this->ConeResolution = 16;
  this->SphereResolution = 16;
  this->CylinderResolution = 16;

  this->ConeRadius = 0.4;
  this->SphereRadius = 0.5;
  this->CylinderRadius = 0.05;

  this->ShaftType = vtkAxesActor::LINE_SHAFT;
  this->TipType   = vtkAxesActor::CONE_TIP;

  this->UserDefinedTip = NULL;
  this->UserDefinedShaft = NULL;

  this->XAxisLabel = vtkCaptionActor2D::New();
  this->YAxisLabel = vtkCaptionActor2D::New();
  this->ZAxisLabel = vtkCaptionActor2D::New();

  this->XAxisLabel->ThreeDimensionalLeaderOff();
  this->XAxisLabel->LeaderOff();
  this->XAxisLabel->BorderOff();
  this->XAxisLabel->SetPosition(0, 0);

  this->YAxisLabel->ThreeDimensionalLeaderOff();
  this->YAxisLabel->LeaderOff();
  this->YAxisLabel->BorderOff();
  this->YAxisLabel->SetPosition(0, 0);

  this->ZAxisLabel->ThreeDimensionalLeaderOff();
  this->ZAxisLabel->LeaderOff();
  this->ZAxisLabel->BorderOff();
  this->ZAxisLabel->SetPosition(0, 0);

  this->UpdateProps();
}

//----------------------------------------------------------------------------
vtkAxesActor::~vtkAxesActor()
{
  this->CylinderSource->Delete();
  this->LineSource->Delete();
  this->ConeSource->Delete();
  this->SphereSource->Delete();

  this->XAxisShaft->Delete();
  this->YAxisShaft->Delete();
  this->ZAxisShaft->Delete();

  this->XAxisTip->Delete();
  this->YAxisTip->Delete();
  this->ZAxisTip->Delete();

  this->SetUserDefinedTip( NULL );
  this->SetUserDefinedShaft( NULL );

  this->SetXAxisLabelText( NULL );
  this->SetYAxisLabelText( NULL );
  this->SetZAxisLabelText( NULL );

  this->XAxisLabel->Delete();
  this->YAxisLabel->Delete();
  this->ZAxisLabel->Delete();
}

//----------------------------------------------------------------------------
// Shallow copy of an actor.
void vtkAxesActor::ShallowCopy(vtkProp *prop)
{
  vtkAxesActor *a = vtkAxesActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetAxisLabels( a->GetAxisLabels() );
    this->SetXAxisLabelText( a->GetXAxisLabelText() );
    this->SetYAxisLabelText( a->GetYAxisLabelText() );
    this->SetZAxisLabelText( a->GetZAxisLabelText() );
    this->SetTotalLength( a->GetTotalLength() );
    this->SetNormalizedShaftLength( a->GetNormalizedShaftLength() );
    this->SetNormalizedTipLength( a->GetNormalizedTipLength() );
    this->SetNormalizedLabelPosition( a->GetNormalizedLabelPosition() );
    this->SetConeResolution( a->GetConeResolution() );
    this->SetSphereResolution( a->GetSphereResolution() );
    this->SetCylinderResolution( a->GetCylinderResolution() );
    this->SetConeRadius( a->GetConeRadius() );
    this->SetSphereRadius( a->GetSphereRadius() );
    this->SetCylinderRadius( a->GetCylinderRadius() );
    this->SetTipType( a->GetTipType() );
    this->SetShaftType( a->GetShaftType() );
    this->SetUserDefinedTip( a->GetUserDefinedTip() );
    this->SetUserDefinedShaft( a->GetUserDefinedShaft() );
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkAxesActor::GetActors(vtkPropCollection *ac)
{
  ac->AddItem( this->XAxisShaft );
  ac->AddItem( this->YAxisShaft );
  ac->AddItem( this->ZAxisShaft );
  ac->AddItem( this->XAxisTip );
  ac->AddItem( this->YAxisTip );
  ac->AddItem( this->ZAxisTip );
}

//----------------------------------------------------------------------------
int vtkAxesActor::RenderOpaqueGeometry(vtkViewport *vp)
{
  int renderedSomething = 0;

  this->UpdateProps();

  renderedSomething += this->XAxisShaft->RenderOpaqueGeometry( vp );
  renderedSomething += this->YAxisShaft->RenderOpaqueGeometry( vp );
  renderedSomething += this->ZAxisShaft->RenderOpaqueGeometry( vp );

  renderedSomething += this->XAxisTip->RenderOpaqueGeometry( vp );
  renderedSomething += this->YAxisTip->RenderOpaqueGeometry( vp );
  renderedSomething += this->ZAxisTip->RenderOpaqueGeometry( vp );

  if ( this->AxisLabels )
    {
    renderedSomething += this->XAxisLabel->RenderOpaqueGeometry( vp );
    renderedSomething += this->YAxisLabel->RenderOpaqueGeometry( vp );
    renderedSomething += this->ZAxisLabel->RenderOpaqueGeometry( vp );
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkAxesActor::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
{
  int renderedSomething = 0;

  this->UpdateProps();

  renderedSomething += this->XAxisShaft->RenderTranslucentPolygonalGeometry( vp );
  renderedSomething += this->YAxisShaft->RenderTranslucentPolygonalGeometry( vp );
  renderedSomething += this->ZAxisShaft->RenderTranslucentPolygonalGeometry( vp );

  renderedSomething += this->XAxisTip->RenderTranslucentPolygonalGeometry( vp );
  renderedSomething += this->YAxisTip->RenderTranslucentPolygonalGeometry( vp );
  renderedSomething += this->ZAxisTip->RenderTranslucentPolygonalGeometry( vp );

  if ( this->AxisLabels )
    {
    renderedSomething += this->XAxisLabel->RenderTranslucentPolygonalGeometry( vp );
    renderedSomething += this->YAxisLabel->RenderTranslucentPolygonalGeometry( vp );
    renderedSomething += this->ZAxisLabel->RenderTranslucentPolygonalGeometry( vp );
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkAxesActor::HasTranslucentPolygonalGeometry()
{
  int result = 0;

  this->UpdateProps();

  result |= this->XAxisShaft->HasTranslucentPolygonalGeometry();
  result |= this->YAxisShaft->HasTranslucentPolygonalGeometry();
  result |= this->ZAxisShaft->HasTranslucentPolygonalGeometry();

  result |= this->XAxisTip->HasTranslucentPolygonalGeometry();
  result |= this->YAxisTip->HasTranslucentPolygonalGeometry();
  result |= this->ZAxisTip->HasTranslucentPolygonalGeometry();

  if ( this->AxisLabels )
    {
    result |= this->XAxisLabel->HasTranslucentPolygonalGeometry();
    result |= this->YAxisLabel->HasTranslucentPolygonalGeometry();
    result |= this->ZAxisLabel->HasTranslucentPolygonalGeometry();
    }
  return result;
}

//-----------------------------------------------------------------------------
int vtkAxesActor::RenderOverlay(vtkViewport *vp)
{
  int renderedSomething = 0;

  if ( !this->AxisLabels )
    {
    return renderedSomething;
    }

  this->UpdateProps();

  renderedSomething += this->XAxisLabel->RenderOverlay( vp );
  renderedSomething += this->YAxisLabel->RenderOverlay( vp );
  renderedSomething += this->ZAxisLabel->RenderOverlay( vp );

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

//----------------------------------------------------------------------------
void vtkAxesActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->XAxisShaft->ReleaseGraphicsResources( win );
  this->YAxisShaft->ReleaseGraphicsResources( win );
  this->ZAxisShaft->ReleaseGraphicsResources( win );

  this->XAxisTip->ReleaseGraphicsResources( win );
  this->YAxisTip->ReleaseGraphicsResources( win );
  this->ZAxisTip->ReleaseGraphicsResources( win );

  this->XAxisLabel->ReleaseGraphicsResources( win );
  this->YAxisLabel->ReleaseGraphicsResources( win );
  this->ZAxisLabel->ReleaseGraphicsResources( win );
}

//----------------------------------------------------------------------------
void vtkAxesActor::GetBounds(double bounds[6])
{
  double *bds = this->GetBounds();
  bounds[0] = bds[0];
  bounds[1] = bds[1];
  bounds[2] = bds[2];
  bounds[3] = bds[3];
  bounds[4] = bds[4];
  bounds[5] = bds[5];
}

//----------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkAxesActor::GetBounds()
{
  double bounds[6];
  int i;

  this->XAxisShaft->GetBounds(this->Bounds);

  this->YAxisShaft->GetBounds(bounds);
  for ( i = 0; i < 3; ++i )
    {
    this->Bounds[2*i+1] =
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->ZAxisShaft->GetBounds(bounds);
  for ( i = 0; i < 3; ++i )
    {
    this->Bounds[2*i+1] = 
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->XAxisTip->GetBounds(bounds);
  for ( i = 0; i < 3; ++i )
    {
    this->Bounds[2*i+1] = 
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->YAxisTip->GetBounds(bounds);
  for ( i = 0; i < 3; ++i )
    {
    this->Bounds[2*i+1] = 
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->ZAxisTip->GetBounds(bounds);
  for ( i = 0; i < 3; ++i )
    {
    this->Bounds[2*i+1] = 
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  double dbounds[6];
  (vtkPolyDataMapper::SafeDownCast(this->YAxisShaft->GetMapper()))->
    GetInput()->GetBounds( dbounds );

  for ( i = 0; i < 3; ++i )
    {
    this->Bounds[2*i+1] = 
      (dbounds[2*i+1]>this->Bounds[2*i+1])?(dbounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  // We want this actor to rotate / re-center about the origin, so give it
  // the bounds it would have if the axes were symmetric.
  for ( i = 0; i < 3; ++i )
    {
    this->Bounds[2*i] = -this->Bounds[2*i+1];
    }

  return this->Bounds;
}

//----------------------------------------------------------------------------
unsigned long int vtkAxesActor::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  return mTime;
}

//----------------------------------------------------------------------------
unsigned long int vtkAxesActor::GetRedrawMTime()
{
  unsigned long mTime = this->GetMTime();
  return mTime;
}

//----------------------------------------------------------------------------
void vtkAxesActor::SetTotalLength( double x, double y, double z )
{
  if ( this->TotalLength[0] != x ||
       this->TotalLength[1] != y ||
       this->TotalLength[2] != z )
    {
    this->TotalLength[0] = x;
    this->TotalLength[1] = y;
    this->TotalLength[2] = z;

    if ( x < 0.0 || y < 0.0 || z < 0.0 )
      {
      vtkGenericWarningMacro("One or more axes lengths are < 0 \
                        and may produce unexpected results.");
      }

    this->Modified();

    this->UpdateProps();
    }
}

//----------------------------------------------------------------------------
void vtkAxesActor::SetNormalizedShaftLength( double x, double y, double z )
{
  if ( this->NormalizedShaftLength[0] != x ||
       this->NormalizedShaftLength[1] != y ||
       this->NormalizedShaftLength[2] != z )
    {
    this->NormalizedShaftLength[0] = x;
    this->NormalizedShaftLength[1] = y;
    this->NormalizedShaftLength[2] = z;

    if ( x < 0.0 || x > 1.0 || y < 0.0 || y > 1.0 || z < 0.0 || z > 1.0 )
      {
      vtkGenericWarningMacro( "One or more normalized shaft lengths \
      are < 0 or > 1 and may produce unexpected results." );
      }

    this->Modified();

    this->UpdateProps();
    }
}

//----------------------------------------------------------------------------
void vtkAxesActor::SetNormalizedTipLength( double x, double y, double z )
{
  if ( this->NormalizedTipLength[0] != x ||
       this->NormalizedTipLength[1] != y ||
       this->NormalizedTipLength[2] != z )
    {
    this->NormalizedTipLength[0] = x;
    this->NormalizedTipLength[1] = y;
    this->NormalizedTipLength[2] = z;

    if ( x < 0.0 || x > 1.0 || y < 0.0 || y > 1.0 || z < 0.0 || z > 1.0 )
      {
      vtkGenericWarningMacro( "One or more normalized tip lengths \
      are < 0 or > 1 and may produce unexpected results." );
      }

    this->Modified();

    this->UpdateProps();
    }
}

//----------------------------------------------------------------------------
void vtkAxesActor::SetNormalizedLabelPosition( double x, double y, double z )
{
  if ( this->NormalizedLabelPosition[0] != x ||
       this->NormalizedLabelPosition[1] != y ||
       this->NormalizedLabelPosition[2] != z )
    {
    this->NormalizedLabelPosition[0] = x;
    this->NormalizedLabelPosition[1] = y;
    this->NormalizedLabelPosition[2] = z;

    if ( x < 0.0 || y < 0.0 || z < 0.0 )
      {
      vtkGenericWarningMacro( "One or more label positions are < 0 \
                        and may produce unexpected results." );
      }

    this->Modified();

    this->UpdateProps();
    }
}

//----------------------------------------------------------------------------
void vtkAxesActor::SetShaftType( int type )
{
  if ( this->ShaftType != type )
    {
    if (type < vtkAxesActor::CYLINDER_SHAFT || \
        type > vtkAxesActor::USER_DEFINED_SHAFT)
      {
      vtkErrorMacro( "Undefined axes shaft type." );
      return;
      }

    if ( type == vtkAxesActor::USER_DEFINED_SHAFT && \
         this->UserDefinedShaft == NULL)
      {
      vtkErrorMacro( "Set the user defined shaft before changing the type." );
      return;
      }

    this->ShaftType = type;

    this->Modified();

    this->UpdateProps();
    }
}

//----------------------------------------------------------------------------
void vtkAxesActor::SetTipType( int type )
{
  if ( this->TipType != type )
    {
    if (type < vtkAxesActor::CONE_TIP || \
        type > vtkAxesActor::USER_DEFINED_TIP)
      {
      vtkErrorMacro( "Undefined axes tip type." );
      return;
      }

    if ( type == vtkAxesActor::USER_DEFINED_TIP && \
         this->UserDefinedTip == NULL)
      {
      vtkErrorMacro( "Set the user defined tip before changing the type." );
      return;
      }

    this->TipType = type;

    this->Modified();

    this->UpdateProps();
    }
}

//----------------------------------------------------------------------------
void vtkAxesActor::UpdateProps()
{
  this->CylinderSource->SetRadius( this->CylinderRadius );
  this->CylinderSource->SetResolution( this->CylinderResolution );

  this->ConeSource->SetResolution( this->ConeResolution );
  this->ConeSource->SetRadius( this->ConeRadius );

  this->SphereSource->SetThetaResolution( this->SphereResolution );
  this->SphereSource->SetPhiResolution( this->SphereResolution );
  this->SphereSource->SetRadius( this->SphereRadius );

  switch ( this->ShaftType )
    {
    case vtkAxesActor::CYLINDER_SHAFT:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))->
        SetInputConnection( this->CylinderSource->GetOutputPort() );
      break;
    case vtkAxesActor::LINE_SHAFT:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))->
        SetInputConnection( this->LineSource->GetOutputPort() );
      break;
    case vtkAxesActor::USER_DEFINED_SHAFT:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))->
        SetInputConnection( this->UserDefinedShaft->GetProducerPort() );
    }

  switch ( this->TipType )
    {
    case vtkAxesActor::CONE_TIP:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))->
        SetInputConnection( this->ConeSource->GetOutputPort() );
      break;
    case vtkAxesActor::SPHERE_TIP:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))->
        SetInputConnection( this->SphereSource->GetOutputPort() );
      break;
    case vtkAxesActor::USER_DEFINED_TIP:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))->
        SetInputConnection( this->UserDefinedTip->GetProducerPort() );
    }

  (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))->
    GetInput()->Update();
  (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))->
    GetInput()->Update();

  if ( this->GetUserTransform() )
    {
    this->XAxisShaft->SetUserTransform( NULL );
    this->YAxisShaft->SetUserTransform( NULL );
    this->ZAxisShaft->SetUserTransform( NULL );
    this->XAxisTip->SetUserTransform( NULL );
    this->YAxisTip->SetUserTransform( NULL );
    this->ZAxisTip->SetUserTransform( NULL );
    }

  double scale[3];
  double bounds[6];

  (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))->
    GetInput()->GetBounds( bounds );

  // The shaft and tip geometry are both initially along direction 0 1 0
  // in the case of cylinder, line, and cone.  Build up the axis from
  // constituent elements defined in their default positions.

  int i;
  for ( i = 0; i < 3; ++i )
    {
    scale[i] =
      this->NormalizedShaftLength[i]*this->TotalLength[i] /
      (bounds[3] - bounds[2]);
    }

  vtkTransform *xTransform = vtkTransform::New();
  vtkTransform *yTransform = vtkTransform::New();
  vtkTransform *zTransform = vtkTransform::New();

  xTransform->RotateZ( -90 );
  zTransform->RotateX( 90 );

  xTransform->Scale( scale[0], scale[0], scale[0] );
  yTransform->Scale( scale[1], scale[1], scale[1] );
  zTransform->Scale( scale[2], scale[2], scale[2] );

  xTransform->Translate( -(bounds[0]+bounds[1])/2,
                         -bounds[2],
                         -(bounds[4]+bounds[5])/2 );
  yTransform->Translate( -(bounds[0]+bounds[1])/2,
                         -bounds[2],
                         -(bounds[4]+bounds[5])/2 );
  zTransform->Translate( -(bounds[0]+bounds[1])/2,
                         -bounds[2],
                         -(bounds[4]+bounds[5])/2 );

  this->XAxisShaft->SetScale( xTransform->GetScale() );
  this->XAxisShaft->SetPosition( xTransform->GetPosition() );
  this->XAxisShaft->SetOrientation( xTransform->GetOrientation() );

  this->YAxisShaft->SetScale( yTransform->GetScale() );
  this->YAxisShaft->SetPosition( yTransform->GetPosition() );
  this->YAxisShaft->SetOrientation( yTransform->GetOrientation() );

  this->ZAxisShaft->SetScale( zTransform->GetScale() );
  this->ZAxisShaft->SetPosition( zTransform->GetPosition() );
  this->ZAxisShaft->SetOrientation( zTransform->GetOrientation() );

  (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))->
    GetInput()->GetBounds( bounds );

  xTransform->Identity();
  yTransform->Identity();
  zTransform->Identity();

  xTransform->RotateZ( -90 );
  zTransform->RotateX( 90 );

  xTransform->Scale( this->TotalLength[0], this->TotalLength[0], this->TotalLength[0] );
  yTransform->Scale( this->TotalLength[1], this->TotalLength[1], this->TotalLength[1] );
  zTransform->Scale( this->TotalLength[2], this->TotalLength[2], this->TotalLength[2] );

  xTransform->Translate( 0, (1.0 - this->NormalizedTipLength[0]), 0 );
  yTransform->Translate( 0, (1.0 - this->NormalizedTipLength[1]), 0 );
  zTransform->Translate( 0, (1.0 - this->NormalizedTipLength[2]), 0 );

  xTransform->Scale( this->NormalizedTipLength[0],
                     this->NormalizedTipLength[0],
                     this->NormalizedTipLength[0] );

  yTransform->Scale( this->NormalizedTipLength[1],
                     this->NormalizedTipLength[1],
                     this->NormalizedTipLength[1] );

  zTransform->Scale( this->NormalizedTipLength[2],
                     this->NormalizedTipLength[2],
                     this->NormalizedTipLength[2] );

  xTransform->Translate( -(bounds[0]+bounds[1])/2,
                         -bounds[2],
                         -(bounds[4]+bounds[5])/2 );
  yTransform->Translate( -(bounds[0]+bounds[1])/2,
                         -bounds[2],
                         -(bounds[4]+bounds[5])/2 );
  zTransform->Translate( -(bounds[0]+bounds[1])/2,
                         -bounds[2],
                         -(bounds[4]+bounds[5])/2 );

  this->XAxisTip->SetScale( xTransform->GetScale() );
  this->XAxisTip->SetPosition( xTransform->GetPosition() );
  this->XAxisTip->SetOrientation( xTransform->GetOrientation() );

  this->YAxisTip->SetScale( yTransform->GetScale() );
  this->YAxisTip->SetPosition( yTransform->GetPosition() );
  this->YAxisTip->SetOrientation( yTransform->GetOrientation() );

  this->ZAxisTip->SetScale( zTransform->GetScale() );
  this->ZAxisTip->SetPosition( zTransform->GetPosition() );
  this->ZAxisTip->SetOrientation( zTransform->GetOrientation() );

  xTransform->Delete();
  yTransform->Delete();
  zTransform->Delete();

  this->XAxisLabel->SetCaption( this->XAxisLabelText );
  this->YAxisLabel->SetCaption( this->YAxisLabelText );
  this->ZAxisLabel->SetCaption( this->ZAxisLabelText );

  this->XAxisShaft->GetBounds(bounds);
  double offset = this->NormalizedLabelPosition[0]*(bounds[1]-bounds[0]);
  this->XAxisLabel->SetAttachmentPoint( bounds[0] + offset,
                                 bounds[2] - (bounds[3]-bounds[2])*2.0,
                                 bounds[5] + (bounds[5]-bounds[4])/2.0);

  this->YAxisShaft->GetBounds(bounds);
  offset = this->NormalizedLabelPosition[1]*(bounds[3]-bounds[2]);
  this->YAxisLabel->SetAttachmentPoint( (bounds[0]+bounds[1])/2,
                                 bounds[2] + offset,
                                 bounds[5] + (bounds[5]-bounds[4])/2.0 );

  this->ZAxisShaft->GetBounds(bounds);
  offset = this->NormalizedLabelPosition[2]*(bounds[5]-bounds[4]);
  this->ZAxisLabel->SetAttachmentPoint( bounds[0],
                                 bounds[2] - (bounds[3]-bounds[2])*2.0,
                                 bounds[4] + offset );

  vtkLinearTransform* transform = this->GetUserTransform();
  if ( transform )
    {
    this->XAxisShaft->SetUserTransform( transform );
    this->YAxisShaft->SetUserTransform( transform );
    this->ZAxisShaft->SetUserTransform( transform );

    this->XAxisTip->SetUserTransform( transform );
    this->YAxisTip->SetUserTransform( transform );
    this->ZAxisTip->SetUserTransform( transform );

    double newpos[3];
    double* pos = this->XAxisLabel->GetAttachmentPoint();
    transform->TransformPoint( pos, newpos );
    this->XAxisLabel->SetAttachmentPoint( newpos );

    pos = this->YAxisLabel->GetAttachmentPoint();
    transform->TransformPoint( pos, newpos );
    this->YAxisLabel->SetAttachmentPoint( newpos );

    pos = this->ZAxisLabel->GetAttachmentPoint();
    transform->TransformPoint( pos, newpos );
    this->ZAxisLabel->SetAttachmentPoint( newpos );
    }
}

//----------------------------------------------------------------------------
vtkProperty *vtkAxesActor::GetXAxisTipProperty()
{
  return this->XAxisTip->GetProperty();
}

//----------------------------------------------------------------------------
vtkProperty *vtkAxesActor::GetYAxisTipProperty()
{
  return this->YAxisTip->GetProperty();
}

//----------------------------------------------------------------------------
vtkProperty *vtkAxesActor::GetZAxisTipProperty()
{
  return this->ZAxisTip->GetProperty();
}

//----------------------------------------------------------------------------
vtkProperty *vtkAxesActor::GetXAxisShaftProperty()
{
  return this->XAxisShaft->GetProperty();
}

//----------------------------------------------------------------------------
vtkProperty *vtkAxesActor::GetYAxisShaftProperty()
{
  return this->YAxisShaft->GetProperty();
}

//----------------------------------------------------------------------------
vtkProperty *vtkAxesActor::GetZAxisShaftProperty()
{
  return this->ZAxisShaft->GetProperty();
}

//----------------------------------------------------------------------------
void vtkAxesActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "UserDefinedShaft: ";
  if (this->UserDefinedShaft)
    {
    os << this->UserDefinedShaft << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  
  os << indent << "UserDefinedTip: ";
  if (this->UserDefinedTip)
    {
    os << this->UserDefinedTip << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  
  os << indent << "XAxisLabelText: " << (this->XAxisLabelText ?
                                         this->XAxisLabelText : "(none)")
     << endl;
  os << indent << "YAxisLabelText: " << (this->YAxisLabelText ?
                                         this->YAxisLabelText : "(none)")
     << endl;
  os << indent << "ZAxisLabelText: " << (this->ZAxisLabelText ?
                                         this->ZAxisLabelText : "(none)")
     << endl;

  os << indent << "AxisLabels: " << (this->AxisLabels ? "On\n" : "Off\n");

  os << indent << "ShaftType: " << this->ShaftType << endl;
  os << indent << "TipType: " << this->TipType << endl;
  os << indent << "SphereRadius: " << this->SphereRadius << endl;
  os << indent << "SphereResolution: " << this->SphereResolution << endl;
  os << indent << "CylinderRadius: " << this->CylinderRadius << endl;
  os << indent << "CylinderResolution: " << this->CylinderResolution << endl;
  os << indent << "ConeRadius: " << this->ConeRadius << endl;
  os << indent << "ConeResolution: " << this->ConeResolution << endl;

  os << indent << "NormalizedShaftLength: "
     << this->NormalizedShaftLength[0] << ","
     << this->NormalizedShaftLength[1] << ","
     << this->NormalizedShaftLength[2] << endl;

  os << indent << "NormalizedTipLength: "
     << this->NormalizedTipLength[0] << ","
     << this->NormalizedTipLength[1] << ","
     << this->NormalizedTipLength[2] << endl;

  os << indent << "TotalLength: "
     << this->TotalLength[0] << ","
     << this->TotalLength[1] << ","
     << this->TotalLength[2] << endl;
     
  os << indent << "NormalizedLabelPosition: "
     << this->NormalizedLabelPosition[0] << ","
     << this->NormalizedLabelPosition[1] << ","
     << this->NormalizedLabelPosition[2] << endl;
}
