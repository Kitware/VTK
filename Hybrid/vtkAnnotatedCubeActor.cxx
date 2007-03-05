/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnnotatedCubeActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnnotatedCubeActor.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCubeSource.h"
#include "vtkFeatureEdges.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkVectorText.h"

vtkCxxRevisionMacro(vtkAnnotatedCubeActor, "1.4");
vtkStandardNewMacro(vtkAnnotatedCubeActor);

vtkAnnotatedCubeActor::vtkAnnotatedCubeActor()
{
  this->Cube      = 1;
  this->TextEdges = 1;
  this->FaceText  = 1;
  this->FaceTextScale  = 0.5;
  this->XPlusFaceText  = NULL;
  this->XMinusFaceText = NULL;
  this->YPlusFaceText  = NULL;
  this->YMinusFaceText = NULL;
  this->ZPlusFaceText  = NULL;
  this->ZMinusFaceText = NULL;

  this->CubeSource = vtkCubeSource::New();
  this->CubeSource->SetBounds(-0.5, 0.5, -0.5, 0.5, -0.5, 0.5);
  this->CubeSource->SetCenter(0, 0, 0);

  vtkPolyDataMapper *cubeMapper = vtkPolyDataMapper::New();
  this->CubeActor = vtkActor::New();
  cubeMapper->SetInput( this->CubeSource->GetOutput() );
  this->CubeActor->SetMapper( cubeMapper );
  cubeMapper->Delete();

  vtkProperty* prop = this->CubeActor->GetProperty();
  prop->SetRepresentationToSurface();
  prop->SetColor(1, 1, 1);
  prop->SetLineWidth(1);

  this->SetXPlusFaceText ( "A" );
  this->SetXMinusFaceText( "P" );
  this->SetYPlusFaceText ( "L" );
  this->SetYMinusFaceText( "R" );
  this->SetZPlusFaceText ( "S" );
  this->SetZMinusFaceText( "I" );

  this->XPlusFaceVectorText  = vtkVectorText::New();
  this->XMinusFaceVectorText = vtkVectorText::New();
  this->YPlusFaceVectorText  = vtkVectorText::New();
  this->YMinusFaceVectorText = vtkVectorText::New();
  this->ZPlusFaceVectorText  = vtkVectorText::New();
  this->ZMinusFaceVectorText = vtkVectorText::New();

  vtkPolyDataMapper *xplusMapper  = vtkPolyDataMapper::New();
  vtkPolyDataMapper *xminusMapper = vtkPolyDataMapper::New();
  vtkPolyDataMapper *yplusMapper  = vtkPolyDataMapper::New();
  vtkPolyDataMapper *yminusMapper = vtkPolyDataMapper::New();
  vtkPolyDataMapper *zplusMapper  = vtkPolyDataMapper::New();
  vtkPolyDataMapper *zminusMapper = vtkPolyDataMapper::New();

  xplusMapper->SetInput ( this->XPlusFaceVectorText->GetOutput() );
  xminusMapper->SetInput( this->XMinusFaceVectorText->GetOutput() );
  yplusMapper->SetInput ( this->YPlusFaceVectorText->GetOutput() );
  yminusMapper->SetInput( this->YMinusFaceVectorText->GetOutput() );
  zplusMapper->SetInput ( this->ZPlusFaceVectorText->GetOutput() );
  zminusMapper->SetInput( this->ZMinusFaceVectorText->GetOutput() );

  this->XPlusFaceActor  = vtkActor::New();
  this->XMinusFaceActor = vtkActor::New();
  this->YPlusFaceActor  = vtkActor::New();
  this->YMinusFaceActor = vtkActor::New();
  this->ZPlusFaceActor  = vtkActor::New();
  this->ZMinusFaceActor = vtkActor::New();

  this->XPlusFaceActor-> SetMapper( xplusMapper );
  this->XMinusFaceActor->SetMapper( xminusMapper );
  this->YPlusFaceActor-> SetMapper( yplusMapper );
  this->YMinusFaceActor->SetMapper( yminusMapper );
  this->ZPlusFaceActor-> SetMapper( zplusMapper );
  this->ZMinusFaceActor->SetMapper( zminusMapper );

  xplusMapper->Delete();
  xminusMapper->Delete();
  yplusMapper->Delete();
  yminusMapper->Delete();
  zplusMapper->Delete();
  zminusMapper->Delete();

  prop = this->XPlusFaceActor->GetProperty();
  prop->SetColor(1, 1, 1);
  prop->SetDiffuse(0);
  prop->SetAmbient(1);
  prop->BackfaceCullingOn();
  this->XMinusFaceActor->GetProperty()->DeepCopy( prop );
  this->YPlusFaceActor-> GetProperty()->DeepCopy( prop );
  this->YMinusFaceActor->GetProperty()->DeepCopy( prop );
  this->ZPlusFaceActor-> GetProperty()->DeepCopy( prop );
  this->ZMinusFaceActor->GetProperty()->DeepCopy( prop );

  this->AppendTextEdges = vtkAppendPolyData::New();
  this->AppendTextEdges->UserManagedInputsOn();
  this->AppendTextEdges->SetNumberOfInputs(6);

  for (int i = 0; i < 6; i++)
    {
    vtkPolyData *edges = vtkPolyData::New();
    this->AppendTextEdges->SetInputByNumber(i,edges);
    edges->Delete();
    }

  this->ExtractTextEdges = vtkFeatureEdges::New();
  this->ExtractTextEdges->BoundaryEdgesOn();
  this->ExtractTextEdges->ColoringOff();
  this->ExtractTextEdges->SetInput( this->AppendTextEdges->GetOutput() );

  vtkPolyDataMapper* edgesMapper = vtkPolyDataMapper::New();
  edgesMapper->SetInput( this->ExtractTextEdges->GetOutput() );

  this->TextEdgesActor = vtkActor::New();
  this->TextEdgesActor->SetMapper( edgesMapper );
  edgesMapper->Delete();

  prop = this->TextEdgesActor->GetProperty();
  prop->SetRepresentationToWireframe();
  prop->SetColor(1,0.5,0);
  prop->SetDiffuse(0);
  prop->SetAmbient(1);
  prop->SetLineWidth(1);

  this->TransformFilter = vtkTransformFilter::New();
  this->Transform = vtkTransform::New();
  this->TransformFilter->SetTransform( this->Transform );

  this->XFaceTextRotation = 0.0;
  this->YFaceTextRotation = 0.0;
  this->ZFaceTextRotation = 0.0;

  this->UpdateProps();
}

vtkAnnotatedCubeActor::~vtkAnnotatedCubeActor()
{
  this->CubeSource->Delete();
  this->CubeActor->Delete();

  this->SetXPlusFaceText ( NULL );
  this->SetXMinusFaceText( NULL );
  this->SetYPlusFaceText ( NULL );
  this->SetYMinusFaceText( NULL );
  this->SetZPlusFaceText ( NULL );
  this->SetZMinusFaceText( NULL );

  this->XPlusFaceVectorText->Delete();
  this->XMinusFaceVectorText->Delete();
  this->YPlusFaceVectorText->Delete();
  this->YMinusFaceVectorText->Delete();
  this->ZPlusFaceVectorText->Delete();
  this->ZMinusFaceVectorText->Delete();

  this->XPlusFaceActor->Delete();
  this->XMinusFaceActor->Delete();
  this->YPlusFaceActor->Delete();
  this->YMinusFaceActor->Delete();
  this->ZPlusFaceActor->Delete();
  this->ZMinusFaceActor->Delete();

  this->AppendTextEdges->Delete();
  this->ExtractTextEdges->Delete();
  this->TextEdgesActor->Delete();

  this->TransformFilter->Delete();
  this->Transform->Delete();
}

// Shallow copy of an actor.
void vtkAnnotatedCubeActor::ShallowCopy(vtkProp *prop)
{
  vtkAnnotatedCubeActor *a = vtkAnnotatedCubeActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetXPlusFaceText( a->GetXPlusFaceText() );
    this->SetXMinusFaceText( a->GetXMinusFaceText() );
    this->SetYPlusFaceText( a->GetYPlusFaceText() );
    this->SetYMinusFaceText( a->GetYMinusFaceText() );
    this->SetZPlusFaceText( a->GetZPlusFaceText() );
    this->SetZMinusFaceText( a->GetZMinusFaceText() );
    this->SetFaceTextScale( a->GetFaceTextScale() );
    this->SetTextEdges( a->GetTextEdges() );
    this->SetCube( a->GetCube() );
    this->SetFaceText( a->GetFaceText() );
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

void vtkAnnotatedCubeActor::GetActors(vtkPropCollection *ac)
{
  ac->AddItem( this->CubeActor );
  ac->AddItem( this->XPlusFaceActor );
  ac->AddItem( this->XMinusFaceActor );
  ac->AddItem( this->YPlusFaceActor );
  ac->AddItem( this->YMinusFaceActor );
  ac->AddItem( this->ZPlusFaceActor );
  ac->AddItem( this->ZMinusFaceActor );
  ac->AddItem( this->TextEdgesActor );
}

int vtkAnnotatedCubeActor::RenderOpaqueGeometry(vtkViewport *vp)
{
  this->UpdateProps();
  int renderedSomething = 0;

  if ( this->Cube )
    {
    renderedSomething += this->CubeActor->RenderOpaqueGeometry( vp );
    }
  if ( this->FaceText )
    {
    renderedSomething += this->XPlusFaceActor->RenderOpaqueGeometry( vp );
    renderedSomething += this->XMinusFaceActor->RenderOpaqueGeometry( vp );
    renderedSomething += this->YPlusFaceActor->RenderOpaqueGeometry( vp );
    renderedSomething += this->YMinusFaceActor->RenderOpaqueGeometry( vp );
    renderedSomething += this->ZPlusFaceActor->RenderOpaqueGeometry( vp );
    renderedSomething += this->ZMinusFaceActor->RenderOpaqueGeometry( vp );
    }
  if ( this->TextEdges )
    {
    renderedSomething += this->TextEdgesActor->RenderOpaqueGeometry( vp );
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkAnnotatedCubeActor::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
{
  this->UpdateProps();
  int renderedSomething = 0;

  if ( this->Cube )
    {
    renderedSomething += this->CubeActor->RenderTranslucentPolygonalGeometry( vp );
    }
  if ( this->FaceText )
    {
    renderedSomething += this->XPlusFaceActor->RenderTranslucentPolygonalGeometry( vp );
    renderedSomething += this->XMinusFaceActor->RenderTranslucentPolygonalGeometry( vp );
    renderedSomething += this->YPlusFaceActor->RenderTranslucentPolygonalGeometry( vp );
    renderedSomething += this->YMinusFaceActor->RenderTranslucentPolygonalGeometry( vp );
    renderedSomething += this->ZPlusFaceActor->RenderTranslucentPolygonalGeometry( vp );
    renderedSomething += this->ZMinusFaceActor->RenderTranslucentPolygonalGeometry( vp );
    }
  if ( this->TextEdges )
    {
    renderedSomething += this->TextEdgesActor->RenderTranslucentPolygonalGeometry( vp );
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkAnnotatedCubeActor::HasTranslucentPolygonalGeometry()
{
  this->UpdateProps();
  int result=0;

  if ( this->Cube )
    {
    result |= this->CubeActor->HasTranslucentPolygonalGeometry();
    }
  if ( this->FaceText )
    {
    result |= this->XPlusFaceActor->HasTranslucentPolygonalGeometry();
    result |= this->XMinusFaceActor->HasTranslucentPolygonalGeometry();
    result |= this->YPlusFaceActor->HasTranslucentPolygonalGeometry();
    result |= this->YMinusFaceActor->HasTranslucentPolygonalGeometry();
    result |= this->ZPlusFaceActor->HasTranslucentPolygonalGeometry();
    result |= this->ZMinusFaceActor->HasTranslucentPolygonalGeometry();
    }
  if ( this->TextEdges )
    {
    result |= this->TextEdgesActor->HasTranslucentPolygonalGeometry();
    }
  return result;
}

//-----------------------------------------------------------------------------
void vtkAnnotatedCubeActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->CubeActor->ReleaseGraphicsResources( win );
  this->XPlusFaceActor->ReleaseGraphicsResources( win );
  this->XMinusFaceActor->ReleaseGraphicsResources( win );
  this->YPlusFaceActor->ReleaseGraphicsResources( win );
  this->YMinusFaceActor->ReleaseGraphicsResources( win );
  this->ZPlusFaceActor->ReleaseGraphicsResources( win );
  this->ZMinusFaceActor->ReleaseGraphicsResources( win );
  this->TextEdgesActor->ReleaseGraphicsResources( win );
}

void vtkAnnotatedCubeActor::GetBounds(double bounds[6])
{
  double *bds = this->GetBounds();
  bounds[0] = bds[0];
  bounds[1] = bds[1];
  bounds[2] = bds[2];
  bounds[3] = bds[3];
  bounds[4] = bds[4];
  bounds[5] = bds[5];
}

// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkAnnotatedCubeActor::GetBounds()
{
  double bounds[6];
  int i;

  this->CubeActor->GetBounds(this->Bounds);

  this->XPlusFaceActor->GetBounds(bounds);
  for (i=0; i<3; i++)
    {
    this->Bounds[2*i+1] =
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->XMinusFaceActor->GetBounds(bounds);
  for (i=0; i<3; i++)
    {
    this->Bounds[2*i+1] =
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->YPlusFaceActor->GetBounds(bounds);
  for (i=0; i<3; i++)
    {
    this->Bounds[2*i+1] =
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->YMinusFaceActor->GetBounds(bounds);
  for (i=0; i<3; i++)
    {
    this->Bounds[2*i+1] =
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->ZPlusFaceActor->GetBounds(bounds);
  for (i=0; i<3; i++)
    {
    this->Bounds[2*i+1] =
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  this->ZMinusFaceActor->GetBounds(bounds);
  for (i=0; i<3; i++)
    {
    this->Bounds[2*i+1] =
      (bounds[2*i+1]>this->Bounds[2*i+1])?(bounds[2*i+1]):(this->Bounds[2*i+1]);
    }

  // We want this actor to rotate / re-center about the origin, so give it
  // the bounds it would have if everything were symmetric.
  for (i = 0; i < 3; i++)
    {
    this->Bounds[2*i] = -this->Bounds[2*i+1];
    }

  return this->Bounds;
}

unsigned long int vtkAnnotatedCubeActor::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  return mTime;
}

unsigned long int vtkAnnotatedCubeActor::GetRedrawMTime()
{
  unsigned long mTime = this->GetMTime();
  return mTime;
}

vtkProperty *vtkAnnotatedCubeActor::GetXPlusFaceProperty()
{
  return this->XPlusFaceActor->GetProperty();
}

vtkProperty *vtkAnnotatedCubeActor::GetXMinusFaceProperty()
{
  return this->XMinusFaceActor->GetProperty();
}

vtkProperty *vtkAnnotatedCubeActor::GetYPlusFaceProperty()
{
  return this->YPlusFaceActor->GetProperty();
}

vtkProperty *vtkAnnotatedCubeActor::GetYMinusFaceProperty()
{
  return this->YMinusFaceActor->GetProperty();
}

vtkProperty *vtkAnnotatedCubeActor::GetZPlusFaceProperty()
{
  return this->ZPlusFaceActor->GetProperty();
}

vtkProperty *vtkAnnotatedCubeActor::GetZMinusFaceProperty()
{
  return this->ZMinusFaceActor->GetProperty();
}               

vtkProperty *vtkAnnotatedCubeActor::GetCubeProperty()
{
  return this->CubeActor->GetProperty();
}

vtkProperty *vtkAnnotatedCubeActor::GetTextEdgesProperty()
{
  return this->TextEdgesActor->GetProperty();
}

void vtkAnnotatedCubeActor::SetFaceTextScale(double scale)
{
  if(this->FaceTextScale == scale)
    {
    return;
    }
  this->FaceTextScale = scale;
  this->UpdateProps();
}

void vtkAnnotatedCubeActor::UpdateProps()
{
  this->XPlusFaceVectorText-> SetText( this->XPlusFaceText );
  this->XMinusFaceVectorText->SetText( this->XMinusFaceText );
  this->YPlusFaceVectorText-> SetText( this->YPlusFaceText );
  this->YMinusFaceVectorText->SetText( this->YMinusFaceText );
  this->ZPlusFaceVectorText-> SetText( this->ZPlusFaceText );
  this->ZMinusFaceVectorText->SetText( this->ZMinusFaceText );

  vtkProperty* prop = this->CubeActor->GetProperty();

  // Place the text slightly offset from the cube face to prevent
  // rendering problems when the cube is in surface render mode.
  double offset = (prop->GetRepresentation() == VTK_SURFACE)? (0.501) : (0.5);

  this->XPlusFaceVectorText->Update();
  double* bounds = this->XPlusFaceVectorText->GetOutput()->GetBounds();
  double cu = -this->FaceTextScale*fabs(0.5*(bounds[0] + bounds[1]));
  double cv = -this->FaceTextScale*fabs(0.5*(bounds[2] + bounds[3]));

  this->XPlusFaceActor->SetScale( this->FaceTextScale );
  this->XPlusFaceActor->SetPosition( offset, cu, cv );
  this->XPlusFaceActor->SetOrientation( 90 , 0, 90 );

  this->XMinusFaceVectorText->Update();
  bounds = this->XMinusFaceVectorText->GetOutput()->GetBounds();
  cu = this->FaceTextScale*fabs(0.5*(bounds[0] + bounds[1]));
  cv = -this->FaceTextScale*fabs(0.5*(bounds[2] + bounds[3]));

  this->XMinusFaceActor->SetScale( this->FaceTextScale );
  this->XMinusFaceActor->SetPosition( -offset, cu, cv );
  this->XMinusFaceActor->SetOrientation( 90 , 0, -90 );

  if ( this->XFaceTextRotation != 0.0 )
    {
    vtkTransform* transform = vtkTransform::New();
    transform->Identity();
    transform->RotateX( this->XFaceTextRotation );
    this->XPlusFaceActor->SetUserTransform( transform );
    this->XMinusFaceActor->SetUserTransform( transform );
    transform->Delete();
    }

  this->YPlusFaceVectorText->Update();
  bounds = this->YPlusFaceVectorText->GetOutput()->GetBounds();
  cu = this->FaceTextScale*0.5*(bounds[0] + bounds[1]);
  cv = -this->FaceTextScale*0.5*(bounds[2] + bounds[3]);

  this->YPlusFaceActor->SetScale( this->FaceTextScale );
  this->YPlusFaceActor->SetPosition( cu, offset, cv );
  this->YPlusFaceActor->SetOrientation( 90, 0, 180 );

  this->YMinusFaceVectorText->Update();
  bounds = this->YMinusFaceVectorText->GetOutput()->GetBounds();
  cu = -this->FaceTextScale*0.5*(bounds[0] + bounds[1]);
  cv = -this->FaceTextScale*0.5*(bounds[2] + bounds[3]);

  this->YMinusFaceActor->SetScale( this->FaceTextScale );
  this->YMinusFaceActor->SetPosition( cu, -offset, cv );
  this->YMinusFaceActor->SetOrientation( 90, 0, 0 );

  if ( this->YFaceTextRotation != 0.0 )
    {
    vtkTransform* transform = vtkTransform::New();
    transform->Identity();
    transform->RotateY( this->YFaceTextRotation );
    this->YPlusFaceActor->SetUserTransform( transform );
    this->YMinusFaceActor->SetUserTransform( transform );
    transform->Delete();
    }

  this->ZPlusFaceVectorText->Update();
  bounds = this->ZPlusFaceVectorText->GetOutput()->GetBounds();
  cu = this->FaceTextScale*0.5*(bounds[0] + bounds[1]);
  cv = -this->FaceTextScale*0.5*(bounds[2] + bounds[3]);

  this->ZPlusFaceActor->SetScale( this->FaceTextScale );
  this->ZPlusFaceActor->SetPosition( cv, cu, offset );
  this->ZPlusFaceActor->SetOrientation( 0, 0, -90 );

  this->ZMinusFaceVectorText->Update();
  bounds = this->ZMinusFaceVectorText->GetOutput()->GetBounds();
  cu = -this->FaceTextScale*0.5*(bounds[0] + bounds[1]);
  cv = -this->FaceTextScale*0.5*(bounds[2] + bounds[3]);

  this->ZMinusFaceActor->SetScale( this->FaceTextScale );
  this->ZMinusFaceActor->SetPosition( cv, cu, -offset );
  this->ZMinusFaceActor->SetOrientation( 180, 0, 90 );

  if ( this->ZFaceTextRotation != 0.0 )
    {
    vtkTransform* transform = vtkTransform::New();
    transform->Identity();
    transform->RotateZ( this->ZFaceTextRotation );
    this->ZPlusFaceActor->SetUserTransform( transform );
    this->ZMinusFaceActor->SetUserTransform( transform );
    transform->Delete();
    }

  this->XPlusFaceActor->ComputeMatrix();
  this->TransformFilter->SetInput( this->XPlusFaceVectorText->GetOutput() );
  this->Transform->SetMatrix( this->XPlusFaceActor->GetMatrix() );
  this->TransformFilter->Update();
  vtkPolyData* edges = this->AppendTextEdges->GetInput( 0 );
  edges->CopyStructure( this->TransformFilter->GetOutput() );

  this->XMinusFaceActor->ComputeMatrix();
  this->TransformFilter->SetInput( this->XMinusFaceVectorText->GetOutput() );
  this->Transform->SetMatrix( this->XMinusFaceActor->GetMatrix() );
  this->TransformFilter->Update();
  edges = this->AppendTextEdges->GetInput( 1 );
  edges->CopyStructure( this->TransformFilter->GetOutput() );

  this->YPlusFaceActor->ComputeMatrix();
  this->TransformFilter->SetInput( this->YPlusFaceVectorText->GetOutput() );
  this->Transform->SetMatrix( this->YPlusFaceActor->GetMatrix() );
  this->TransformFilter->Update();
  edges = this->AppendTextEdges->GetInput( 2 );
  edges->CopyStructure( this->TransformFilter->GetOutput() );

  this->YMinusFaceActor->ComputeMatrix();
  this->TransformFilter->SetInput( this->YMinusFaceVectorText->GetOutput() );
  this->Transform->SetMatrix( this->YMinusFaceActor->GetMatrix() );
  this->TransformFilter->Update();
  edges = this->AppendTextEdges->GetInput( 3 );
  edges->CopyStructure( this->TransformFilter->GetOutput() );

  this->ZPlusFaceActor->ComputeMatrix();
  this->TransformFilter->SetInput( this->ZPlusFaceVectorText->GetOutput() );
  this->Transform->SetMatrix( this->ZPlusFaceActor->GetMatrix() );
  this->TransformFilter->Update();
  edges = this->AppendTextEdges->GetInput( 4 );
  edges->CopyStructure(this->TransformFilter->GetOutput());

  this->ZMinusFaceActor->ComputeMatrix();
  this->TransformFilter->SetInput( this->ZMinusFaceVectorText->GetOutput() );
  this->Transform->SetMatrix( this->ZMinusFaceActor->GetMatrix() );
  this->TransformFilter->Update();
  edges = this->AppendTextEdges->GetInput( 5 );
  edges->CopyStructure( this->TransformFilter->GetOutput() );
}

void vtkAnnotatedCubeActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "XPlusFaceText: " << (this->XPlusFaceText ?
                                         this->XPlusFaceText : "(none)")
     << endl;

  os << indent << "XMinusFaceText: " << (this->XMinusFaceText ?
                                         this->XMinusFaceText : "(none)")
     << endl;

  os << indent << "YPlusFaceText: " << (this->YPlusFaceText ?
                                         this->YPlusFaceText : "(none)")
     << endl;

  os << indent << "YMinusFaceText: " << (this->YMinusFaceText ?
                                         this->YMinusFaceText : "(none)")
     << endl;

  os << indent << "ZPlusFaceText: " << (this->ZPlusFaceText ?
                                         this->ZPlusFaceText : "(none)")
     << endl;

  os << indent << "ZMinusFaceText: " << (this->ZMinusFaceText ?
                                         this->ZMinusFaceText : "(none)")
     << endl;

  os << indent << "FaceTextScale: " << this->FaceTextScale << endl;

  os << indent << "TextEdges: " << (this->TextEdges ? "On\n" : "Off\n");

  os << indent << "FaceText: " << (this->FaceText ? "On\n" : "Off\n");

  os << indent << "Cube: " << (this->Cube ? "On\n" : "Off\n");

  os << indent << "XFaceTextRotation: " << this->XFaceTextRotation << endl;

  os << indent << "YFaceTextRotation: " << this->YFaceTextRotation << endl;

  os << indent << "ZFaceTextRotation: " << this->ZFaceTextRotation << endl;
}

