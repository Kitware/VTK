/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkXYPlotRepresentation.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Written by Philippe Pebay, Kitware SAS 2012

#include "vtkXYPlotRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"
#include "vtkXYPlotActor.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"

vtkStandardNewMacro(vtkXYPlotRepresentation);

//-----------------------------------------------------------------------------
vtkXYPlotRepresentation::vtkXYPlotRepresentation()
{
//    this->PositionCoordinate->SetValue( 0.0, 0.0 );
//    this->Position2Coordinate->SetValue( 0.7, 0.65 );

  this->XYPlotActor = NULL;
  vtkXYPlotActor *actor = vtkXYPlotActor::New();
  this->SetXYPlotActor( actor );
  actor->Delete();

  this->ShowBorder = vtkBorderRepresentation::BORDER_ACTIVE;
  this->BWActor->VisibilityOff();
}

//-----------------------------------------------------------------------------
vtkXYPlotRepresentation::~vtkXYPlotRepresentation()
{
  this->SetXYPlotActor( NULL );
}

//-----------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetXYPlotActor( vtkXYPlotActor* actor )
{
  if ( this->XYPlotActor != actor )
  {
    vtkSmartPointer<vtkXYPlotActor> oldActor = this->XYPlotActor;
    vtkSetObjectBodyMacro( XYPlotActor, vtkXYPlotActor, actor );
  }
}

//-----------------------------------------------------------------------------
void vtkXYPlotRepresentation::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "XYPlotActor: " << this->XYPlotActor << endl;
}

//-----------------------------------------------------------------------------
void vtkXYPlotRepresentation::BuildRepresentation()
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetPosition( this->GetPosition() );
    this->XYPlotActor->SetPosition2( this->GetPosition2() );
  }

  this->Superclass::BuildRepresentation();
}

//-----------------------------------------------------------------------------
void vtkXYPlotRepresentation::WidgetInteraction( double eventPos[2])
{
  // Let superclass move things around.
  this->Superclass::WidgetInteraction( eventPos );
}

//-----------------------------------------------------------------------------
int vtkXYPlotRepresentation::GetVisibility()
{
  return this->XYPlotActor->GetVisibility();
}

//-----------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetVisibility( int vis )
{
  this->XYPlotActor->SetVisibility( vis );
}

//-----------------------------------------------------------------------------
void vtkXYPlotRepresentation::GetActors2D( vtkPropCollection* collection )
{
  if ( this->XYPlotActor )
  {
    collection->AddItem( this->XYPlotActor );
  }
  this->Superclass::GetActors2D( collection );
}

//-----------------------------------------------------------------------------
void vtkXYPlotRepresentation::ReleaseGraphicsResources( vtkWindow* w )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->ReleaseGraphicsResources( w );
  }
  this->Superclass::ReleaseGraphicsResources( w );
}

//-------------------------------------------------------------------------
int vtkXYPlotRepresentation::RenderOverlay( vtkViewport* w )
{
  int count = this->Superclass::RenderOverlay( w );
  if ( this->XYPlotActor )
  {
    count += this->XYPlotActor->RenderOverlay( w );
  }
  return count;
}

//-------------------------------------------------------------------------
int vtkXYPlotRepresentation::RenderOpaqueGeometry( vtkViewport* w )
{
  int count = this->Superclass::RenderOpaqueGeometry( w );
  if ( this->XYPlotActor )
  {
    count += this->XYPlotActor->RenderOpaqueGeometry( w );
  }
  return count;
}

//-------------------------------------------------------------------------
int vtkXYPlotRepresentation::RenderTranslucentPolygonalGeometry( vtkViewport* w )
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry( w );
  if ( this->XYPlotActor )
  {
    count += this->XYPlotActor->RenderTranslucentPolygonalGeometry( w );
  }
  return count;
}

//-------------------------------------------------------------------------
int vtkXYPlotRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  if ( this->XYPlotActor )
  {
    result |= this->XYPlotActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetBorder( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetBorder( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitle ( const char* title )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitle( title );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetXTitle ( const char* title )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetXTitle( title );
  }
}

//-------------------------------------------------------------------------------
char* vtkXYPlotRepresentation::GetXTitle ()
{
  if ( this->XYPlotActor )
  {
    return this->XYPlotActor->GetXTitle();
  }
  return 0;
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetXRange ( double xmin, double xmax )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetXRange( xmin, xmax );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetYTitle ( const char* title )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetYTitle( title );
  }
}

//-------------------------------------------------------------------------------
char* vtkXYPlotRepresentation::GetYTitle ()
{
  if ( this->XYPlotActor )
  {
    return this->XYPlotActor->GetXTitle();
  }
  return 0;
}


//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetYRange ( double ymin, double ymax )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetYRange( ymin, ymax );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetYTitlePosition ( int pos )
{
  if ( this->XYPlotActor )
  {
    switch( pos )
    {
      case 0:
        this->XYPlotActor->SetYTitlePositionToTop();
        break;
      case 1:
        this->XYPlotActor->SetYTitlePositionToHCenter();
        break;
      case 2:
        this->XYPlotActor->SetYTitlePositionToVCenter();
        break;

    }
  }
}

//-------------------------------------------------------------------------------
int vtkXYPlotRepresentation::GetYTitlePosition () const
{
  if ( this->XYPlotActor )
  {
    return this->XYPlotActor->GetYTitlePosition();
  }
  return 0;
}


//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetXAxisColor ( double r, double g, double b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetXAxisColor( r, g, b );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetYAxisColor ( double r, double g, double b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetYAxisColor( r, g, b );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetXValues ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetXValues( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetLegend ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetLegend( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetLegendBorder ( int b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetLegendBorder( b );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetLegendBox ( int b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetLegendBox( b );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetLegendBoxColor ( double r, double g, double b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetLegendBoxColor( r, g, b );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetLegendPosition ( double x, double y )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetLegendPosition( x, y );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetLegendPosition2 ( double x, double y )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetLegendPosition2( x, y );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetLineWidth ( double w )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetLineWidth( w );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetPlotColor ( int i, int r, int g, int b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetPlotColor( i, r/255.0, g/255.0, b/255.0 );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetPlotLines ( int i )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetPlotLines( i );
  }
}


//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetPlotPoints ( int i )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetPlotPoints( i );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetPlotLabel ( int i, const char* label )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetPlotLabel( i, label );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetPlotGlyphType( int curve, int glyph )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetPlotGlyphType( curve, glyph );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetGlyphSize( double x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetGlyphSize( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::AddUserCurvesPoint ( double c, double x, double y )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->AddUserCurvesPoint( c, x, y );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::RemoveAllActiveCurves ()
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->RemoveAllActiveCurves();
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitleColor ( double r, double g, double b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitleColor( r, g, b );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitleFontFamily ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitleFontFamily( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitleBold ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitleBold( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitleItalic ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitleItalic( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitleShadow ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitleShadow( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitleFontSize ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitleFontSize( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitleJustification ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitleJustification( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitleVerticalJustification ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitleVerticalJustification( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAdjustTitlePosition ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAdjustTitlePosition( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetTitlePosition ( double x, double y )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetTitlePosition( x, y );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisTitleColor ( double r, double g, double b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisTitleColor( r, g, b );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisTitleFontFamily ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisTitleFontFamily( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisTitleBold ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisTitleBold( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisTitleItalic ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisTitleItalic( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisTitleShadow ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisTitleShadow( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisTitleFontSize ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisTitleFontSize( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisTitleJustification ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisTitleJustification( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisTitleVerticalJustification ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisTitleVerticalJustification( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisLabelColor ( double r, double g, double b )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisLabelColor( r, g, b );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisLabelFontFamily ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisLabelFontFamily( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisLabelBold ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisLabelBold( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisLabelItalic ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisLabelItalic( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisLabelShadow ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisLabelShadow( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisLabelFontSize ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisLabelFontSize( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisLabelJustification ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisLabelJustification( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetAxisLabelVerticalJustification ( int x )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetAxisLabelVerticalJustification( x );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetXLabelFormat( const char* arg )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetXLabelFormat( arg );
  }
}

//-------------------------------------------------------------------------------
void vtkXYPlotRepresentation::SetYLabelFormat( const char* arg )
{
  if ( this->XYPlotActor )
  {
    this->XYPlotActor->SetYLabelFormat( arg );
  }
}
