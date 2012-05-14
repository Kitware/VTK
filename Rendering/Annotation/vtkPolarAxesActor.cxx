/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolarAxesActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPolarAxesActor.h"

#include "vtkArcSource.h"
#include "vtkAxisActor.h"
#include "vtkAxisFollower.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkFollower.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

#include <vtksys/ios/sstream>

#define VTK_POLAR_AXES_ACTOR_RTOL ( 1. - 10. * VTK_DBL_EPSILON )

vtkStandardNewMacro(vtkPolarAxesActor);
vtkCxxSetObjectMacro(vtkPolarAxesActor, Camera,vtkCamera);
vtkCxxSetObjectMacro(vtkPolarAxesActor,PolarAxisLabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkPolarAxesActor,PolarAxisTitleTextProperty,vtkTextProperty);

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os,indent );

  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", "
     << this->Bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", "
     << this->Bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", "
     << this->Bounds[5] << ")\n";

  os << indent << "ScreenSize: " << this->ScreenSize << "\n";

  os << indent << "Number Of Radial Axes: " << this->NumberOfRadialAxes << endl;
  os << indent << "Number Of Polar Axis Ticks: " << this->NumberOfPolarAxisTicks << endl;
  os << indent << "Auto Subdivide Polar Axis: "
     << ( this->AutoSubdividePolarAxis ? "On\n" : "Off\n" );

  os << indent << "Pole: ("
     << this->Pole[0] << ", "
     << this->Pole[1] << ", "
     << this->Pole[2] << ")\n";

  os << indent << "Maximum Radius: " << this->MaximumRadius << endl;
  os << indent << "Auto-Scale Radius: " << this->AutoScaleRadius << endl;
  os << indent << "Minimum Angle: " << this->MinimumAngle << endl;
  os << indent << "Maximum Angle: " << this->MaximumAngle << endl;
  os << indent << "Smallest Visible Polar Angle: " << this->SmallestVisiblePolarAngle << endl;
  os << indent << "Radial Units (degrees): "
     << ( this->RadialUnits ? "On\n" : "Off\n" ) << endl;

  if ( this->Camera )
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf( os,indent.GetNextIndent() );
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }

  os << indent << "EnableDistanceLOD: "
     << ( this->EnableDistanceLOD ? "On" : "Off" ) << endl;
  os << indent << "DistanceLODThreshold: "   << this->DistanceLODThreshold    << "\n";

  os << indent << "EnableViewAngleLOD: "
     << ( this->EnableViewAngleLOD ? "On" : "Off" ) << endl;
  os << indent << "ViewAngleLODThreshold: "   << this->ViewAngleLODThreshold    << "\n";

  os << indent << "Polar Axis Title: " << this->PolarAxisTitle << "\n";
  os << indent << "Polar Label Format: " << this->PolarLabelFormat << "\n";
  os << indent << "PolarAxisLabelTextProperty: " << this->PolarAxisLabelTextProperty << endl;
  os << indent << "PolarAxisTitleTextProperty: " << this->PolarAxisTitleTextProperty << endl;
  os << indent << "Polar Axis Visibility: "
     << ( this->PolarAxisVisibility ? "On\n" : "Off\n" );
  os << indent << "Polar Title Visibility: "
     << ( this->PolarTitleVisibility ? "On" : "Off" ) << endl;
  os << indent << "Polar Label Visibility: "
     << ( this->PolarLabelVisibility ? "On" : "Off" ) << endl;
  os << indent << "Polar Tick Visibility: "
     << ( this->PolarTickVisibility ? "On" : "Off" ) << endl;

  os << indent << "Radial Axes Visibility: "
     << ( this->RadialAxesVisibility ? "On\n" : "Off\n" );
  os << indent << "Radial Title Visibility: "
     << ( this->RadialTitleVisibility ? "On" : "Off" ) << endl;

  os << indent << "Polar Arcs Visibility: "
     << ( this->PolarArcsVisibility ? "On" : "Off" ) << endl;
}

//-----------------------------------------------------------------------------
vtkPolarAxesActor::vtkPolarAxesActor() : vtkActor()
{
  // Default bounds
  this->Bounds[0] = -1.0; this->Bounds[1] = 1.0;
  this->Bounds[2] = -1.0; this->Bounds[3] = 1.0;
  this->Bounds[4] = -1.0; this->Bounds[5] = 1.0;

  // Default pole coordinates
  this->Pole[0] = 0.;
  this->Pole[1] = 0.;
  this->Pole[2] = 0.;

  // Default number of radial axes
  this->NumberOfRadialAxes = VTK_DEFAULT_NUMBER_OF_RADIAL_AXES;

  // Invalid default number of polar arcs, and auto-calculate by default
  this->NumberOfPolarAxisTicks = -1;
  this->AutoSubdividePolarAxis = true;

  // Default maximum polar radius
  this->MaximumRadius = 1.;

  // Do not auto-scale radius by default
  this->AutoScaleRadius = false;

  // Default minimum polar angle
  this->MinimumAngle = 0.;

  // Default maximum polar angle
  this->MaximumAngle = 90.;

  // Default smallest radial angle distinguishable from polar axis
  this->SmallestVisiblePolarAngle = .5;

  // By default show angle units (degrees)
  this->RadialUnits = true;

  this->Camera = NULL;

  // Default text screen size
  this->ScreenSize = 10.0;

  // Screen offset for labels
  // Pivot point at center of the geometry hence this->ScreenSize * 0.5
  this->LabelScreenOffset = 15.0 + this->ScreenSize * 0.5;

  // Text properties of polar axis title and labels, with default color white
  // Properties of the radial axes, with default color black
  this->PolarAxisProperty = vtkProperty::New();
  this->PolarAxisProperty->SetColor( 0., 0., 0. );
  this->PolarAxisTitleTextProperty = vtkTextProperty::New();
  this->PolarAxisTitleTextProperty->SetColor( 1., 1. ,1. );
  this->PolarAxisTitleTextProperty->SetFontFamilyToArial();
  this->PolarAxisLabelTextProperty = vtkTextProperty::New();
  this->PolarAxisLabelTextProperty->SetColor( 1., 1. ,1. );
  this->PolarAxisLabelTextProperty->SetFontFamilyToArial();

  // Create and set polar axis of type X
  this->PolarAxis = vtkAxisActor::New();
  this->PolarAxis->SetAxisTypeToX();
  this->PolarAxis->SetAxisPositionToMinMax();
  this->PolarAxis->SetCalculateTitleOffset( 0 );
  this->PolarAxis->SetCalculateLabelOffset( 0 );

  // Default distance LOD settings
  this->EnableDistanceLOD = 1;
  this->DistanceLODThreshold = .7;

  // Default view angle LOD settings
  this->EnableViewAngleLOD = 1;
  this->ViewAngleLODThreshold = .3;

  // Properties of the radial axes, with default color black
  this->RadialAxesProperty = vtkProperty::New();
  this->RadialAxesProperty->SetColor( 0., 0., 0. );

  // Create and set radial axes of type X
  this->CreateRadialAxes();

  // Create and set polar arcs and ancillary objects, with default color white
  this->PolarArcs = vtkPolyData::New();
  this->PolarArcsMapper = vtkPolyDataMapper::New();
  this->PolarArcsMapper->SetInputData( this->PolarArcs );
  this->PolarArcsActor = vtkActor::New();
  this->PolarArcsActor->SetMapper( this->PolarArcsMapper );
  this->PolarArcsActor->GetProperty()->SetColor( 1., 1., 1. );

  // Default title for polar axis (sometimes also called "Radius")
  this->PolarAxisTitle = new char[16];
  sprintf(this->PolarAxisTitle, "%s", "Radial Distance");
  this->PolarLabelFormat = new char[8];
  sprintf( this->PolarLabelFormat, "%s", "%-#6.3g" );

  // By default all polar axis features are visible
  this->PolarAxisVisibility = 1;
  this->PolarTitleVisibility = 1;
  this->PolarLabelVisibility = 1;
  this->PolarTickVisibility = 1;

  // By default all radial axes features are visible
  this->RadialAxesVisibility = 1;
  this->RadialTitleVisibility = 1;

  // By default polar arcs are visible
  this->PolarArcsVisibility = 1;

  // Default title scale
  this->TitleScale = -1.;

  // Default label scale
  this->LabelScale = -1.;

  this->RenderCount = 0;

  this->RenderSomething = 0;
}

//-----------------------------------------------------------------------------
vtkPolarAxesActor::~vtkPolarAxesActor()
{
  this->SetCamera( NULL );

  if ( this->PolarAxisProperty )
    {
    this->PolarAxisProperty->Delete();
    }

  if ( this->RadialAxesProperty )
    {
    this->RadialAxesProperty->Delete();
    }

  if ( this->PolarLabelFormat )
    {
    delete [] this->PolarLabelFormat;
    this->PolarLabelFormat = NULL;
    }

  if ( this->PolarAxisTitle )
    {
    delete [] this->PolarAxisTitle;
    this->PolarAxisTitle = NULL;
    }

  if ( this->PolarAxisTitleTextProperty )
    {
    this->PolarAxisTitleTextProperty->Delete();
    this->PolarAxisTitleTextProperty = NULL;
    }

  if ( this->PolarAxisLabelTextProperty )
    {
    this->PolarAxisLabelTextProperty->Delete();
    this->PolarAxisLabelTextProperty = NULL;
    }

  if ( this->PolarAxis )
    {
    this->PolarAxis->Delete();
    this->PolarAxis = NULL;
    }

  if ( this->RadialAxes )
    {
    for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
      {
      if ( this->RadialAxes[i] )
        {
        this->RadialAxes[i]->Delete();
        this->RadialAxes[i] = NULL;
        }
      }
    delete [] this->RadialAxes;
    this->RadialAxes = NULL;
    }

  if (this->PolarArcs)
    {
    this->PolarArcs->Delete();
    this->PolarArcs = NULL;
    }
  if (this->PolarArcsMapper)
    {
    this->PolarArcsMapper->Delete();
    this->PolarArcsMapper = NULL;
    }
  if (this->PolarArcsActor)
    {
    this->PolarArcsActor->Delete();
    this->PolarArcsActor = NULL;
    }

}

//-----------------------------------------------------------------------------
int vtkPolarAxesActor::RenderOpaqueGeometry( vtkViewport *viewport )
{
  // Initialization
  int renderedSomething = 0;
  if ( !this->Camera )
    {
    vtkErrorMacro( <<"No camera!" );
    this->RenderSomething = 0;
    return renderedSomething;
    }

  this->BuildAxes( viewport );

  // Render the polar axis
  if ( this->PolarAxisVisibility )
    {
    renderedSomething += this->PolarAxis->RenderOpaqueGeometry( viewport );
    }

  // Render the radial axes
  if ( this->RadialAxesVisibility )
    {
    for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
      {
      renderedSomething += this->RadialAxes[i]->RenderOpaqueGeometry( viewport );
      }
    }

  // Render the polar arcs
  if ( this->PolarArcsVisibility )
    {
    renderedSomething += this->PolarArcsActor->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetScreenSize( double screenSize )
{
  this->ScreenSize = screenSize;
  // Considering pivot point at center of the geometry,
  // hence ( this->ScreenSize * 0.5 ).
  this->LabelScreenOffset = 15.0 + this->ScreenSize * 0.5;

  vtkAxisFollower** labelActors = this->PolarAxis->GetLabelActors();
  int numberOfLabels = this->PolarAxis->GetNumberOfLabelsBuilt();
  for( int i = 0; i < numberOfLabels; ++ i )
    {
    labelActors[i]->SetScreenOffset( this->LabelScreenOffset );
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::ReleaseGraphicsResources( vtkWindow *win )
{
  this->PolarAxis->ReleaseGraphicsResources(win);
  for ( int i = 0; i < this->NumberOfRadialAxes;  ++ i )
    {
    this->RadialAxes[i]->ReleaseGraphicsResources( win );
    }
  this->PolarArcsActor->ReleaseGraphicsResources( win );
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::CalculateBounds()
{
  // Fetch angles, at this point it is already known that angular sector <= 360.
  double minAngle = this->MinimumAngle;
  double maxAngle = this->MaximumAngle;

  // Ensure that angles are not both < -180 nor both > 180 degrees
  if ( maxAngle < -180. )
    {
    // Increment angles modulo 360 degrees
    minAngle += 360.;
    maxAngle += 360.;
    }
  else if ( minAngle > 180. )
    {
    // Decrement angles modulo 360 degrees
    minAngle -= 360.;
    maxAngle -= 360.;
    }

  // Prepare trigonometric quantities
  double thetaMin = vtkMath::RadiansFromDegrees( minAngle );
  double cosThetaMin = cos( thetaMin );
  double sinThetaMin = sin( thetaMin );
  double thetaMax = vtkMath::RadiansFromDegrees( maxAngle );
  double cosThetaMax = cos( thetaMax );
  double sinThetaMax = sin( thetaMax );

  // Calculate extremal cosines across angular sector
  double minCos;
  double maxCos;
  if ( minAngle * maxAngle < 0. )
    {
    // Angular sector contains null angle
    maxCos = 1.;
    if ( minAngle < 180. && maxAngle > 180. )
      {
      // Angular sector also contains flat angle
      minCos = -1.;
      }
    else
      {
      // Angular sector does not contain flat angle
      minCos = cosThetaMin < cosThetaMax ? cosThetaMin : cosThetaMax;
      }

    }
  else if ( minAngle < 180. && maxAngle > 180. )
    {
    // Angular sector does not contain flat angle (and not null angle)
    minCos = -1.;
    maxCos = cosThetaMax > cosThetaMin ? cosThetaMax : cosThetaMin;
    }
  else
    {
    // Angular sector does not contain flat nor null angle
    minCos = cosThetaMin < cosThetaMax ? cosThetaMin : cosThetaMax;
    maxCos = cosThetaMax > cosThetaMin ? cosThetaMax : cosThetaMin;
    }

  // Calculate extremal sines across angular sector
  double minSin;
  double maxSin;
  if ( minAngle < -90. && maxAngle > -90. )
    {
    // Angular sector contains negative right angle
    minSin = -1.;
    if ( minAngle < 90. && maxAngle > 90. )
      {
      // Angular sector also contains positive right angle
      maxSin = 1.;
      }
    else
      {
      // Angular sector contain does not contain positive right angle
      maxSin = sinThetaMax > sinThetaMin ? sinThetaMax : sinThetaMin;
      }
    }
  else if ( minAngle < 90. && maxAngle > 90. )
    {
    // Angular sector contains positive right angle (and not negative one)
    minSin = sinThetaMin < sinThetaMax ? sinThetaMin : sinThetaMax;
    maxSin = 1.;
    }
  else
    {
    // Angular sector contain does not contain either right angle
    minSin = sinThetaMin < sinThetaMax ? sinThetaMin : sinThetaMax;
    maxSin = sinThetaMax > sinThetaMin ? sinThetaMax : sinThetaMin;
    }

  // Now calculate bounds
  // xmin
  this->Bounds[0] = this->Pole[0] + this->MaximumRadius * minCos;
  // xmax
  this->Bounds[1] = this->Pole[0] + this->MaximumRadius * maxCos;
  // ymin
  this->Bounds[2] = this->Pole[1] + this->MaximumRadius * minSin;
  // ymax
  this->Bounds[3] = this->Pole[1] + this->MaximumRadius * maxSin;
  // zmin
  this->Bounds[4] = this->Pole[2];
  // zmax
  this->Bounds[5] = this->Pole[2];

  // Update modification time of bounds
  this->BoundsMTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::GetBounds( double bounds[6])
{
  for ( int i=0; i< 6; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::GetBounds( double& xmin, double& xmax,
                                   double& ymin, double& ymax,
                                   double& zmin, double& zmax )
{
  xmin = this->Bounds[0];
  xmax = this->Bounds[1];
  ymin = this->Bounds[2];
  ymax = this->Bounds[3];
  zmin = this->Bounds[4];
  zmax = this->Bounds[5];
}

//-----------------------------------------------------------------------------
double *vtkPolarAxesActor::GetBounds()
{
  return this->Bounds;
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::BuildAxes( vtkViewport *viewport )
{
  if ( ( this->GetMTime() < this->BuildTime.GetMTime() ) )
    {
    this->AutoScale( viewport );
    return;
    }

  if ( this->MaximumAngle < this->MinimumAngle )
    {
    // Incorrect angle input
    vtkWarningMacro( << "Cannot draw radial axes: "
                     << " minimum angle = "
                     << this->MinimumAngle
                     << " > maximum angle = "
                     << this->MaximumAngle
                     << ".");
    return;
    }

  if ( this->MaximumAngle - this->MinimumAngle > 360. )
    {
    // Incorrect angle input
    vtkWarningMacro( << "Cannot draw radial axes: "
                     << " angular sector = "
                     << this->MaximumAngle - this->MinimumAngle
                     << " > 360 deg." );
    return;
    }

  // Determine the bounds
  this->CalculateBounds();

  // Set polar axis endpoints
  vtkAxisActor* axis = this->PolarAxis;
  double ox = this->Pole[0] + this->MaximumRadius;
  axis->GetPoint1Coordinate()->SetValue( this->Pole[0], this->Pole[1], this->Pole[2] );
  axis->GetPoint2Coordinate()->SetValue( ox, this->Pole[1], this->Pole[2] );

  // Set common axis attributes
  this->SetCommonAxisAttributes( axis );

  // Set polar axis lines
  axis->SetAxisVisibility( this->PolarAxisVisibility );
  axis->SetAxisLinesProperty( this->PolarAxisProperty );

  // Set polar axis title
  axis->SetTitleVisibility( this->PolarTitleVisibility );
  axis->SetTitle( this->PolarAxisTitle );
  axis->SetTitleTextProperty( this->PolarAxisTitleTextProperty );

  // Set polar axis ticks (major only)
  axis->SetTickVisibility( this->PolarTickVisibility );
  axis->SetTickLocation( VTK_TICKS_BOTH );
  axis->SetMajorTickSize( .02 * this->MaximumRadius );

  // Set polar axis labels
  axis->SetLabelVisibility( this->PolarLabelVisibility );
  axis->SetLabelTextProperty( this->PolarAxisLabelTextProperty );

  // Build radial axes
  this->BuildRadialAxes();

  // Build polar axis ticks
  this->BuildPolarAxisTicks( this->Pole[0] );

  // Build polar axis labels
  this->BuildPolarAxisLabelsArcs();

  // Build polar axis
  this->PolarAxis->BuildAxis( viewport, true );

  // Scale appropriately
  this->AutoScale( viewport );

  this->RenderSomething = 1;
  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetCommonAxisAttributes( vtkAxisActor* axis )
{
  vtkProperty *prop = this->GetProperty();
  prop->SetAmbient( 1.0 );
  prop->SetDiffuse( 0.0 );
  axis->SetProperty( prop );

  // Common space and range attributes
  axis->SetCamera( this->Camera );
  axis->SetBounds( this->Bounds );
  axis->SetRange( 0., this->MaximumRadius );

  // No minor ticks for any kind of axes
  axis->SetMinorTicksVisible( 0 );
}

//-----------------------------------------------------------------------------
inline double vtkPolarAxesActor::FFix( double value )
{
  int ivalue = static_cast<int>( value );
  return ivalue;
}

//-----------------------------------------------------------------------------
inline double vtkPolarAxesActor::FSign( double value, double sign )
{
  value = fabs( value );
  if ( sign < 0.)
    {
    value *= -1.;
    }
  return value;
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::CreateRadialAxes()
{
  // Create requested number of radial axes
  this->RadialAxes = new vtkAxisActor*[this->NumberOfRadialAxes];
  for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
    {
    // Create axis of type X
    this->RadialAxes[i] = vtkAxisActor::New();
    vtkAxisActor* axis = this->RadialAxes[i];
    axis->SetAxisTypeToX();
    axis->SetAxisPositionToMinMax();
    axis->SetCalculateTitleOffset( 0 );
    axis->SetCalculateLabelOffset( 0 );
    }
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::BuildRadialAxes()
{
  // Base offset for radial axis title followers
  double offset = .67 * ( this->LabelScreenOffset + this->ScreenSize * 0.5 );

  // Create requested number of radial axes
  double dAlpha =
    ( this->MaximumAngle  - this->MinimumAngle ) / ( this->NumberOfRadialAxes - 1. );
  double alpha = this->MinimumAngle;
  for ( int i = 0; i < this->NumberOfRadialAxes;  ++ i, alpha += dAlpha )
    {
    // Calculate endpoint coordinates
    double alphaRad = vtkMath::RadiansFromDegrees( alpha );
    double x = this->Pole[0] + this->MaximumRadius * cos( alphaRad );
    double y = this->Pole[1] + this->MaximumRadius * sin( alphaRad );

    // Set radial axis endpoints
    vtkAxisActor* axis = this->RadialAxes[i];
    axis->GetPoint1Coordinate()->SetValue( this->Pole[0], this->Pole[1], this->Pole[2] );
    axis->GetPoint2Coordinate()->SetValue( x, y, this->Pole[2] );

    // Set common axis attributes
    this->SetCommonAxisAttributes( axis );

    // Set radial axis lines
    axis->SetAxisVisibility( this->RadialAxesVisibility );
    axis->SetAxisLinesProperty( this->RadialAxesProperty );

    // Set radial axis title with polar angle as title for non-polar axes
    if ( this->PolarAxisVisibility && fabs( alpha ) < 2. )
      {
      // Prevent conflict between radial and polar axes titles
      axis->SetTitleVisibility( false );

      if ( fabs( alpha ) < this->SmallestVisiblePolarAngle )
        {
        // Do not show radial axes too close to polar axis
        axis->SetAxisVisibility( false );
        }
      }
    else
      {
      // Use polar angle as a title for the radial axis
      axis->SetTitleVisibility( this->RadialTitleVisibility );
      axis->GetTitleTextProperty()->SetColor( this->RadialAxesProperty->GetColor() );
      vtksys_ios::ostringstream title;
      title << alpha
            << ( this->RadialUnits ? " deg" : "" );
      axis->SetTitle( title.str().c_str() );

      // Update axis title followers
      axis->GetTitleActor()->SetAxis( axis );
      axis->GetTitleActor()->SetScreenOffset( offset );
      axis->GetTitleActor()->SetEnableDistanceLOD( this->EnableDistanceLOD );
      axis->GetTitleActor()->SetDistanceLODThreshold( this->DistanceLODThreshold );
      axis->GetTitleActor()->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
      axis->GetTitleActor()->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );
      }

    // No labels nor ticks for radial axes
    axis->SetLabelVisibility( 0 );
    axis->SetTickVisibility( 0 );
    }
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::BuildPolarAxisTicks( double x0 )
{
  double delta;

  if ( this->AutoSubdividePolarAxis
       || this->NumberOfPolarAxisTicks < 0
       || this->NumberOfPolarAxisTicks > VTK_MAXIMUM_NUMBER_OF_POLAR_AXIS_TICKS )
    {
    // Programatically figure the number of divisions of the polar axis
    double pow10 = log10( this->MaximumRadius );

    // Build in numerical tolerance
    if ( pow10 != 0.)
      {
      double eps = 10.0e-10;
      pow10 = this->FSign( ( fabs( pow10 ) + eps ), pow10 );
      }

    // FFix will move in the wrong direction if pow10 is negative.
    if ( pow10 < 0.)
      {
      pow10 = pow10 - 1.;
      }

    // Find the number of integral points in the interval.
    delta = pow( 10., this->FFix( pow10 ) );
    double fnt = this->MaximumRadius / delta;
    fnt  = this->FFix( fnt );
    int numTicks = fnt <= 0.5 ?
      static_cast<int>( this->FFix( fnt ) ) : static_cast<int>( this->FFix( fnt ) + 1 );

    // If not enough tick points in this decade, scale down
    double div = 1.;
    if ( numTicks < 5 )
      {
      div = 2.;
      }
    if ( numTicks <= 2 )
      {
      div = 5.;
      }
    if ( div != 1.)
      {
      delta /= div;
      }

    // Finally calculate number of tick points
    this->NumberOfPolarAxisTicks = 0;
    while ( delta * this->NumberOfPolarAxisTicks <= this->MaximumRadius )
      {
      ++ this->NumberOfPolarAxisTicks;
      }
    }
  else // if ( this->AutoSubdividePolarAxis || this->NumberOfPolarAxisTicks ... )
    {
    // Use pre-set number of arcs when it is valid and no auto-subdivision was requested
    delta =  this->MaximumRadius / ( this->NumberOfPolarAxisTicks - 1 );
    }

  // Set major start and delta corresponding to range and coordinates
  this->PolarAxis->SetMajorRangeStart( 0. );
  this->PolarAxis->SetDeltaRangeMajor( delta );
  this->PolarAxis->SetMajorStart( VTK_AXIS_TYPE_X, x0 );

  // Build in numerical robustness to avoid truncation errors at endpoint
  delta *= VTK_POLAR_AXES_ACTOR_RTOL;
  this->PolarAxis->SetDeltaMajor( VTK_AXIS_TYPE_X, delta );
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::BuildPolarAxisLabelsArcs()
{
  // Prepare storage for polar axis labels
  vtkStringArray* labels = vtkStringArray::New();
  labels->SetNumberOfValues( this->NumberOfPolarAxisTicks );

  // Prepare trigonometric quantities
  double thetaPolar = vtkMath::RadiansFromDegrees( this->MinimumAngle );
  double cosThetaPolar = cos( thetaPolar );
  double sinThetaPolar = sin( thetaPolar );
  double angularSector = this->MaximumAngle - this->MinimumAngle;
  vtkIdType arcResolution
    = static_cast<vtkIdType>( angularSector * VTK_POLAR_ARC_RESOLUTION_PER_DEG );

  // Arc points
  vtkPoints* polarArcsPoints = vtkPoints::New();
  this->PolarArcs->SetPoints( polarArcsPoints );
  polarArcsPoints->Delete();

  // Arc lines
  vtkCellArray* polarArcsLines = vtkCellArray::New();
  this->PolarArcs->SetLines( polarArcsLines );
  polarArcsLines->Delete();

  // Retreave label features
  vtkAxisActor* axis = this->PolarAxis;
  double delta = axis->GetDeltaMajor( VTK_AXIS_TYPE_X );
  double value = axis->GetMajorRangeStart();

  // Now create labels and polar arcs
  const char *format = this->PolarLabelFormat;
  char label[64];
  vtkIdType pointIdOffset = 0;
  for ( int  i = 0; i < this->NumberOfPolarAxisTicks; ++ i )
    {
    // Store label
    sprintf( label, format, value );
    labels->SetValue( i, label );

    // Build polar arcs for non-zero values
    if ( value  > 0. )
      {
      // Compute polar vector for this tick mark
      double xPolar = value * cosThetaPolar;
      double yPolar = value * sinThetaPolar;

      // Create polar arc with corresponding to this tick mark
      vtkArcSource* arc = vtkArcSource::New();
      arc->UseNormalAndAngleOn(); // Use new arc source API
      arc->SetCenter( this->Pole );
      arc->SetNormal( 0., 0., 1. );
      arc->SetPolarVector( xPolar, yPolar, 0. );
      arc->SetAngle( angularSector );
      arc->SetResolution( arcResolution );
      arc->Update();

      // Append new polar arc to existing ones
      vtkPoints* arcPoints = arc->GetOutput()->GetPoints();
      vtkIdType nPoints = arcResolution + 1;
      vtkIdType* arcPointIds = new vtkIdType[nPoints];
      for ( vtkIdType j = 0; j < nPoints; ++ j )
        {
        polarArcsPoints->InsertNextPoint( arcPoints->GetPoint( j ) );
        arcPointIds[j] = pointIdOffset + j;
        }
      polarArcsLines->InsertNextCell( nPoints, arcPointIds );

      // Clean up
      arc->Delete();
      delete [] arcPointIds;

      // Update polyline cell offset
      pointIdOffset += nPoints;
      }

    // Move to next value
    value += delta;
    }

  // Store labels
  this->PolarAxis->SetLabels( labels );

  // Clean up
  labels->Delete();

  // Update axis title follower
  vtkAxisFollower* follower = axis->GetTitleActor();
  follower->SetAxis( axis );
  follower->SetScreenOffset( 2. * this->LabelScreenOffset + this->ScreenSize + 5 );
  follower->SetEnableDistanceLOD( this->EnableDistanceLOD );
  follower->SetDistanceLODThreshold( this->DistanceLODThreshold );
  follower->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
  follower->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );

  // Update axis label followers
  vtkAxisFollower** labelActors = axis->GetLabelActors();
  for( int i = 0; i < this->NumberOfPolarAxisTicks; ++ i )
    {
    labelActors[i]->SetAxis( axis );
    labelActors[i]->SetScreenOffset( this->LabelScreenOffset );
    labelActors[i]->SetEnableDistanceLOD( this->EnableDistanceLOD );
    labelActors[i]->SetDistanceLODThreshold( this->DistanceLODThreshold );
    labelActors[i]->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
    labelActors[i]->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );
    }
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::AutoScale( vtkViewport *viewport )
{

  // Scale polar axis title
  vtkAxisActor* axis = this->PolarAxis;
  double newTitleScale
    = vtkAxisFollower::AutoScale( viewport,
                                  this->Camera,
                                  this->ScreenSize,
                                  axis->GetTitleActor()->GetPosition() );
  axis->SetTitleScale( newTitleScale );

  // Scale polar axis labels
  vtkAxisFollower** labelActors = axis->GetLabelActors();
  for( int i = 0; i < axis->GetNumberOfLabelsBuilt(); ++ i )
    {
    double newLabelScale
      = vtkAxisFollower::AutoScale( viewport,
                                    this->Camera,
                                    this->ScreenSize,
                                    labelActors[i]->GetPosition() );
    labelActors[i]->SetScale( newLabelScale );
    }

  // Loop over radial axes
  for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
    {
    axis = this->RadialAxes[i];
    // Scale title
    newTitleScale
      = vtkAxisFollower::AutoScale( viewport,
                                    this->Camera,
                                    this->ScreenSize,
                                    axis->GetTitleActor()->GetPosition() );
    axis->SetTitleScale( newTitleScale );
    }
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetPole( double p[3] )
{
  this->Pole[0] = p[0];
  this->Pole[1] = p[1];
  this->Pole[2] = p[2];

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetPole( double x, double y, double z )
{
  this->Pole[0] = x;
  this->Pole[1] = y;
  this->Pole[2] = z;

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetNumberOfRadialAxes( vtkIdType n )
{
  // Limit number of redial axes
  if ( n > VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES )
    {
    n = VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES;
    }

  // If number of radial axes does not change, do nothing
  if ( this->NumberOfRadialAxes == n )
    {
    return;
    }

  // Delete existing radial axes
  if ( this->RadialAxes )
    {
    for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
      {
      if ( this->RadialAxes[i] )
        {
        this->RadialAxes[i]->Delete();
        this->RadialAxes[i] = NULL;
        }
      }
    delete [] this->RadialAxes;
    this->RadialAxes = NULL;
    }

  // Create and set n radial axes of type X
  this->NumberOfRadialAxes = n;
  this->CreateRadialAxes();

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetMaximumRadius( double r )
{
  this->MaximumRadius = r > 0. ? r : 0.;

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetMinimumAngle( double a )
{
  if ( a > 360. )
    {
    this->MinimumAngle = 360.;
    }
  else if ( a < -360. )
    {
    this->MinimumAngle = -360.;
    }
  else
    {
    this->MinimumAngle = a;
    }

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetMaximumAngle( double a )
{
  if ( a > 360. )
    {
    this->MaximumAngle = 360.;
    }
  else if ( a < -360. )
    {
    this->MaximumAngle = -360.;
    }
  else
    {
    this->MaximumAngle = a;
    }

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetPolarAxisProperty( vtkProperty *prop )
{
  this->PolarAxisProperty->DeepCopy( prop );
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPolarAxesActor::GetPolarAxisProperty()
{
  return this->PolarAxisProperty;
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetRadialAxesProperty( vtkProperty *prop )
{
  this->RadialAxesProperty->DeepCopy( prop );
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPolarAxesActor::GetRadialAxesProperty()
{
  return this->RadialAxesProperty;
}

//-----------------------------------------------------------------------------
void vtkPolarAxesActor::SetPolarArcsProperty( vtkProperty *prop )
{
  this->PolarArcsActor->SetProperty(prop);
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPolarAxesActor::GetPolarArcsProperty()
{
  return this->PolarArcsActor->GetProperty();
}
