/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLeaderActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLeaderActor2D.h"

#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkLeaderActor2D);

vtkCxxSetObjectMacro(vtkLeaderActor2D,LabelTextProperty,vtkTextProperty);


//----------------------------------------------------------------------------
// Instantiate this object.
vtkLeaderActor2D::vtkLeaderActor2D()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.0, 0.0);

  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.75, 0.75);
  this->Position2Coordinate->SetReferenceCoordinate(NULL);

  this->Radius = 0.0;
  this->Length = 0.0;
  this->Angle = 0.0;

  this->Label = NULL;
  this->LabelFactor = 1.0;
  this->AutoLabel = 0;
  this->LabelFormat = new char[8];
  sprintf(this->LabelFormat,"%s","%-#6.3g");

  this->ArrowPlacement = vtkLeaderActor2D::VTK_ARROW_BOTH;
  this->ArrowStyle = vtkLeaderActor2D::VTK_ARROW_FILLED;
  this->ArrowLength = 0.04;
  this->ArrowWidth = 0.02;
  this->MinimumArrowSize = 2;
  this->MaximumArrowSize = 25;

  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(1);
  this->LabelTextProperty->SetFontFamilyToArial();
  this->LabelTextProperty->SetJustificationToCentered();
  this->LabelTextProperty->SetVerticalJustificationToCentered();

  this->LabelMapper = vtkTextMapper::New();
  this->LabelActor = vtkActor2D::New();
  this->LabelActor->SetMapper(this->LabelMapper);

  // Points 0-3 are the side1 of the arrow; Points 4-7 are the side2 of the arrow.
  this->LeaderPoints = vtkPoints::New();

  this->LeaderLines = vtkCellArray::New();
  this->LeaderLines->EstimateSize(1,2);

  this->LeaderArrows = vtkCellArray::New();
  this->LeaderArrows->EstimateSize(2,3);

  this->Leader = vtkPolyData::New();
  this->Leader->SetPoints(this->LeaderPoints);
  this->Leader->SetLines(this->LeaderLines);
  this->Leader->SetPolys(this->LeaderArrows);

  this->LeaderMapper = vtkPolyDataMapper2D::New();
  this->LeaderMapper->SetInputData(this->Leader);
  this->LeaderActor = vtkActor2D::New();
  this->LeaderActor->SetMapper(this->LeaderMapper);

  this->LastPosition[0] = this->LastPosition[1] = 0;
  this->LastPosition2[0] = this->LastPosition2[1] = 0;
  this->LastSize[0] = this->LastSize[1] = 0;
}

//----------------------------------------------------------------------------
vtkLeaderActor2D::~vtkLeaderActor2D()
{
  this->LabelMapper->Delete();
  this->LabelActor->Delete();

  delete [] this->Label;
  this->Label = NULL;

  delete [] this->LabelFormat;
  this->LabelFormat = NULL;

  this->LeaderPoints->Delete();
  this->LeaderLines->Delete();
  this->LeaderArrows->Delete();
  this->Leader->Delete();
  this->LeaderMapper->Delete();
  this->LeaderActor->Delete();

  this->SetLabelTextProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkLeaderActor2D::BuildLeader(vtkViewport *viewport)
{
  // Check to see whether we need to rebuild-----------------------------
  int positionsHaveChanged = 0;
  if (viewport->GetMTime() > this->BuildTime ||
      (viewport->GetVTKWindow() && viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
    {
    int *lastPosition = this->PositionCoordinate->GetComputedViewportValue(viewport);
    int *lastPosition2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    if (lastPosition[0] != this->LastPosition[0] ||
        lastPosition[1] != this->LastPosition[1] ||
        lastPosition2[0] != this->LastPosition2[0] ||
        lastPosition2[1] != this->LastPosition2[1] )
      {
      positionsHaveChanged = 1;
      }
    }

  int *size = viewport->GetSize();
  int viewportSizeHasChanged=0;
  // See whether fonts have to be rebuilt (font size depends on viewport size)
  if (this->LastSize[0] != size[0] || this->LastSize[1] != size[1])
    {
    viewportSizeHasChanged = 1;
    this->LastSize[0] = size[0];
    this->LastSize[1] = size[1];
    }

  if (!positionsHaveChanged && !viewportSizeHasChanged &&
      this->GetMTime() < this->BuildTime &&
      this->LabelTextProperty->GetMTime() < this->BuildTime )
    {
    return;
    }

  // Okay, we have some work to do. We build the leader in three parts:
  // 1) the line connecting the two points, 2) the text label, and 3) the
  // arrow head(s) if any.
  vtkDebugMacro(<<"Rebuilding leader"); //---------------------------------

  // Initialize the data
  this->LeaderPoints->Initialize();
  this->LeaderLines->Initialize();
  this->LeaderArrows->Initialize();
  this->LeaderActor->SetProperty(this->GetProperty());
  this->LabelMapper->SetTextProperty(this->LabelTextProperty);

  // The easiest part is determining the two end points of the line.
  double p1[3], p2[3], ray[3];
  int *x = this->PositionCoordinate->GetComputedViewportValue(viewport);
  p1[0] = static_cast<double>(x[0]);
  p1[1] = static_cast<double>(x[1]);
  p1[2] = 0.0;
  this->LastPosition[0] = x[0];
  this->LastPosition[1] = x[1];

  x = this->Position2Coordinate->GetComputedViewportValue(viewport);
  p2[0] = static_cast<double>(x[0]);
  p2[1] = static_cast<double>(x[1]);
  p2[2] = 0.0;
  this->LastPosition2[0] = x[0];
  this->LastPosition2[1] = x[1];

  ray[0] = p2[0] - p1[0];
  ray[1] = p2[1] - p1[1];
  ray[2] = 0.0;
  double rayLength = vtkMath::Norm(ray);
  if ( rayLength <= 0.0 )
    {
    return;
    }

  double theta, theta2;
  if (ray[0] == 0. && ray[1] == 0.)
    {
    theta = 0.;
    }
  else
    {
    theta = atan2(ray[1], ray[0]);
    }
  theta2 = theta + vtkMath::Pi();

  // If there is a suitable radius then a curved leader must be created.
  // Remember the radius is expresses as a factor times the distance between (p1,p2).
  if ( fabs(this->Radius) > 0.5 )
    {
    this->BuildCurvedLeader(p1,p2,ray,rayLength,theta,viewport,viewportSizeHasChanged);
    return;
    }

  // Okay, we can continue building the straight leader--------------------------
  this->LeaderPoints->SetNumberOfPoints(8);
  this->LeaderPoints->SetPoint(0,p1);
  this->LeaderPoints->SetPoint(4,p2);
  this->LeaderPoints->Modified();

  // Build the labels
  int i, clippedLeader=0;
  double xL[3], xR[3], c1[3], c2[3];
  double *x1 = this->PositionCoordinate->GetComputedWorldValue(viewport);
  double *x2 = this->Position2Coordinate->GetComputedWorldValue(viewport);
  this->Length = sqrt(vtkMath::Distance2BetweenPoints(x1,x2));

  if ( this->AutoLabel || (this->Label != NULL && this->Label[0] != 0) )
    {
    int stringSize[2];
    if ( this->AutoLabel )
      {
      char string[512];
      sprintf(string, this->LabelFormat, this->Length);
      this->LabelMapper->SetInput(string);
      }
    else
      {
      this->LabelMapper->SetInput(this->Label);
      }

    if (this->LabelTextProperty->GetMTime() > this->BuildTime)
      {
      this->LabelMapper->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
      }

    if (viewportSizeHasChanged || this->LabelTextProperty->GetMTime() > this->BuildTime)
      {
      this->SetFontSize(viewport, this->LabelMapper, size, this->LabelFactor, stringSize);
      }
    else
      {
      this->LabelMapper->GetSize(viewport, stringSize);
      }
    for (i=0; i<3; i++)
      {
      xL[i] = p1[i] + 0.5*ray[i];
      }

    // Now clip the leader with the label box
    if ( (clippedLeader=this->ClipLeader(xL,stringSize,p1,ray,c1,c2)) )
      {
      this->LabelActor->SetPosition(xL[0],xL[1]);
      this->LeaderPoints->SetPoint(3,c1);
      this->LeaderPoints->SetPoint(7,c2);
      }
    else //we cannot fit the text in the leader, it has to be placed next to the leader
      {
      double w = static_cast<double>(stringSize[0])/2.0;
      double h = static_cast<double>(stringSize[1])/2.0;
      double r = sqrt(h*h+w*w);
      xL[0] = xL[0] + r*sin(theta);
      xL[1] = xL[1] - r*cos(theta);
      this->LabelActor->SetPosition(xL[0],xL[1]);
      }
    } // If label visible

  if ( !clippedLeader )
    { //we just draw a single line across 'cause there is no label in the leader
    this->LeaderLines->InsertNextCell(2);
    this->LeaderLines->InsertCellPoint(0);
    this->LeaderLines->InsertCellPoint(4);
    }
  else
    { //draw two lines separated by label
    this->LeaderLines->InsertNextCell(2);
    this->LeaderLines->InsertCellPoint(0);
    this->LeaderLines->InsertCellPoint(3);
    this->LeaderLines->InsertNextCell(2);
    this->LeaderLines->InsertCellPoint(4);
    this->LeaderLines->InsertCellPoint(7);
    }

  // Build the arrows---------------------------------------
  if ( this->ArrowPlacement == vtkLeaderActor2D::VTK_ARROW_NONE )
    {
    ; //do nothin
    }
  else //we are creating arrows
    {
    this->Leader->Modified();
    // Convert width and length to viewport (pixel) coordinates
    double dist = sqrt(static_cast<double>(size[0]*size[0] + size[1]*size[1]));
    double width = this->ArrowWidth * dist / 2.0;
    double length = this->ArrowLength * dist;
    if ( length < width && length < this->MinimumArrowSize )
      {
      width = this->MinimumArrowSize*width/length;
      length = this->MinimumArrowSize;
      }
    else if ( width < length && width < this->MinimumArrowSize )
      {
      length = this->MinimumArrowSize*length/width;
      width = this->MinimumArrowSize;
      }
    if ( length > width && length > this->MaximumArrowSize )
      {
      width = this->MaximumArrowSize*width/length;
      length = this->MaximumArrowSize;
      }
    else if ( width > length && width > this->MaximumArrowSize )
      {
      length = this->MaximumArrowSize*length/width;
      width = this->MaximumArrowSize;
      }

    // Find the position along the line for the arrows and create the additional points
    double a1[3], a2[3];
    for (i=0; i<3; i++)
      {
      a1[i] = p1[i] + (length/rayLength)*ray[i];
      a2[i] = p1[i] + (1.0-(length/rayLength))*ray[i];
      }
    if ( this->ArrowPlacement == vtkLeaderActor2D::VTK_ARROW_POINT1 ||
         this->ArrowPlacement == vtkLeaderActor2D::VTK_ARROW_BOTH )
      {
      xL[0] = a1[0] + width*sin(theta);
      xL[1] = a1[1] - width*cos(theta);
      xR[0] = a1[0] + width*sin(theta2);
      xR[1] = a1[1] - width*cos(theta2);
      xR[2] = xL[2] = 0.0;

      this->LeaderPoints->SetPoint(1,xL);
      this->LeaderPoints->SetPoint(2,xR);
      if ( this->ArrowStyle == vtkLeaderActor2D::VTK_ARROW_FILLED )
        {
        this->LeaderArrows->InsertNextCell(3);
        this->LeaderArrows->InsertCellPoint(0);
        this->LeaderArrows->InsertCellPoint(1);
        this->LeaderArrows->InsertCellPoint(2);
        }
      else if ( this->ArrowStyle == vtkLeaderActor2D::VTK_ARROW_OPEN )
        {
        this->LeaderLines->InsertNextCell(3);
        this->LeaderLines->InsertCellPoint(1);
        this->LeaderLines->InsertCellPoint(0);
        this->LeaderLines->InsertCellPoint(2);
        }
      else //if ( this->ArrowStyle == vtkLeaderActor2D::VTK_ARROW_HOLLOW )
        {
        this->LeaderLines->InsertNextCell(4);
        this->LeaderLines->InsertCellPoint(1);
        this->LeaderLines->InsertCellPoint(0);
        this->LeaderLines->InsertCellPoint(2);
        this->LeaderLines->InsertCellPoint(1);
        }
      }
    if ( this->ArrowPlacement == vtkLeaderActor2D::VTK_ARROW_POINT2 ||
         this->ArrowPlacement == vtkLeaderActor2D::VTK_ARROW_BOTH )
      {
      xL[0] = a2[0] + width*sin(theta);
      xL[1] = a2[1] - width*cos(theta);
      xR[0] = a2[0] + width*sin(theta2);
      xR[1] = a2[1] - width*cos(theta2);
      xR[2] = xL[2] = 0.0;

      this->LeaderPoints->SetPoint(5,xL);
      this->LeaderPoints->SetPoint(6,xR);
      if ( this->ArrowStyle == vtkLeaderActor2D::VTK_ARROW_FILLED )
        {
        this->LeaderArrows->InsertNextCell(3);
        this->LeaderArrows->InsertCellPoint(4);
        this->LeaderArrows->InsertCellPoint(5);
        this->LeaderArrows->InsertCellPoint(6);
        }
      else if ( this->ArrowStyle == vtkLeaderActor2D::VTK_ARROW_OPEN )
        {
        this->LeaderLines->InsertNextCell(3);
        this->LeaderLines->InsertCellPoint(5);
        this->LeaderLines->InsertCellPoint(4);
        this->LeaderLines->InsertCellPoint(6);
        }
      else //if ( this->ArrowStyle == vtkLeaderActor2D::VTK_ARROW_HOLLOW )
        {
        this->LeaderLines->InsertNextCell(4);
        this->LeaderLines->InsertCellPoint(5);
        this->LeaderLines->InsertCellPoint(4);
        this->LeaderLines->InsertCellPoint(6);
        this->LeaderLines->InsertCellPoint(5);
        }
      }
    } //creating arrows


  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
#define VTK_LA2D_FACTOR 0.015
int vtkLeaderActor2D::SetFontSize(vtkViewport *viewport, vtkTextMapper *textMapper,
                                  int *targetSize, double factor, int *stringSize)
{
  int fontSize, targetWidth, targetHeight;

  targetWidth = targetSize[0] > targetSize[1] ? targetSize[0] : targetSize[1];
  targetHeight = static_cast<int>(VTK_LA2D_FACTOR * factor * targetSize[0] +
                                  VTK_LA2D_FACTOR * factor * targetSize[1]);

  fontSize = textMapper->SetConstrainedFontSize(viewport, targetWidth, targetHeight);
  textMapper->GetSize(viewport, stringSize);

  return fontSize;
}
#undef VTK_LA2D_FACTOR


//----------------------------------------------------------------------------
int vtkLeaderActor2D::ClipLeader(double center[3], int box[2], double p1[3],
                                 double ray[3], double c1[3], double c2[3])
{
  // Separately compute the parametric coordintes due to x-line and y-line
  // intersections. Take the coordinate closest to the center of the line.
  double tx, ty, t;
  double x = center[0] + box[0];
  double y = center[1] + box[1];

  // x-line
  if ( ray[0] != 0.0 )
    {
    tx = (x - p1[0])/ray[0];
    }
  else
    {
    tx = VTK_FLOAT_MAX;
    }

  // y-line
  if ( ray[1] != 0.0 )
    {
    ty = (y - p1[1])/ray[1];
    }
  else
    {
    ty = VTK_FLOAT_MAX;
    }

  // Find the closest intersection point nearest the center of the box
  t = ( fabs(tx-0.5) < fabs(ty-0.5) ? tx : ty );
  if ( fabs(t-0.5) > 0.45 )
    {
    return 0; //wont fit along line
    }
  else
    {
    t = ( t>0.5 ? t : 1.0-t); //make sure t is to the right of the midpoint
    for (int i=0; i<3; i++)
      {
      c1[i] = p1[i] + (1.0-t)*ray[i];
      c2[i] = p1[i] +       t*ray[i];
      }
    return 1;
    }
}

//----------------------------------------------------------------------------
void vtkLeaderActor2D::BuildCurvedLeader(double p1[3], double p2[3], double ray[3],
                                         double rayLength, double theta,
                                         vtkViewport *viewport, int viewportChanged)
{
  // Determine where the center is
  double radius = fabs(this->Radius)*rayLength;
  double midPoint[3], center[3];
  int i;
  midPoint[0] = p1[0] + 0.5*ray[0];
  midPoint[1] = p1[1] + 0.5*ray[1];
  midPoint[2] = p1[2] + 0.5*ray[2];
  double d = sqrt(radius*radius - rayLength*rayLength/4.0);
  if ( this->Radius > 0 )
    {
    center[0] = midPoint[0] + d*sin(theta);
    center[1] = midPoint[1] - d*cos(theta);
    center[2] = 0.0;
    }
  else
    {
    center[0] = midPoint[0] - d*sin(theta);
    center[1] = midPoint[1] + d*cos(theta);
    center[2] = 0.0;
    }

  // Compute some angles; make sure they are <= 180 degrees
  double phi = atan2(rayLength/2.0,d);
  double theta1 = atan2(p1[1]-center[1],p1[0]-center[0]);
  double theta2 = atan2(p2[1]-center[1],p2[0]-center[0]);
  if ( (theta1 >= 0.0 && theta1 <= vtkMath::Pi() &&
        theta2 >= 0.0 && theta2 <= vtkMath::Pi()) ||
       (theta1 <= 0.0 && theta1 >= -vtkMath::Pi() &&
        theta2 <= 0.0 && theta2 >= -vtkMath::Pi()) )
    {
    ; //do nothin angles are fine
    }
  else if ( theta1 >= 0.0 && theta2 <= 0.0 )
    {
    if ( (theta1 - theta2) >= vtkMath::Pi() )
      {
      theta2 = theta2 + 2.0*vtkMath::Pi();
      }
    }
  else //if ( theta1 <= 0.0 && theta2 >= 0.0 )
    {
    if ( (theta2 - theta1) >= vtkMath::Pi() )
      {
      theta1 = theta1 + 2.0*vtkMath::Pi();
      }
    }

  // Build the polyline for the leader. Start by generating the points.
  double x[3]; x[2]=0.0;
  double length = radius*phi;
  int numDivs = static_cast<int>((length / 3.0) + 1); //every three pixels
  for (i=0; i<=numDivs; i++)
    {
    theta = theta1 + (static_cast<double>(i)/numDivs)*(theta2-theta1);
    x[0] = center[0] + radius*cos(theta);
    x[1] = center[1] + radius*sin(theta);
    this->LeaderPoints->InsertPoint(i,x);
    }

  // Now insert lines. Only those not clipped by the string are added.
  this->Angle = vtkMath::DegreesFromRadians( theta1 - theta2);
  if ( this->AutoLabel || (this->Label != NULL && this->Label[0] != 0) )
    {
    int stringSize[2];
    if ( this->AutoLabel )
      {
      char string[512];
      sprintf(string, this->LabelFormat, this->Angle);
      this->LabelMapper->SetInput(string);
      }
    else
      {
      this->LabelMapper->SetInput(this->Label);
      }
    if (this->LabelTextProperty->GetMTime() > this->BuildTime)
      {
      this->LabelMapper->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
      }

    if (viewportChanged || this->LabelTextProperty->GetMTime() > this->BuildTime)
      {
      int *size = viewport->GetSize();
      this->SetFontSize(viewport, this->LabelMapper, size, this->LabelFactor, stringSize);
      }
    else
      {
      this->LabelMapper->GetSize(viewport, stringSize);
      }

    double x1[3], c[3];
    theta = (theta1 + theta2)/2.0;
    c[0] = center[0] + radius*cos(theta);
    c[1] = center[1] + radius*sin(theta);
    c[2] = 0.0;
    this->LabelActor->SetPosition(c[0],c[1]);
    for (i=0; i<numDivs; i++)
      {
      this->LeaderPoints->GetPoint(i,x);
      this->LeaderPoints->GetPoint(i+1,x1);
      if ( ! this->InStringBox(c,stringSize,x) &&
           ! this->InStringBox(c,stringSize,x1)  )
        {
        this->LeaderLines->InsertNextCell(2);
        this->LeaderLines->InsertCellPoint(i);
        this->LeaderLines->InsertCellPoint(i+1);
        }
      }
    }
  else // no clipping against the string necessary
    {
    for (i=0; i<numDivs; i++)
      {
      this->LeaderLines->InsertNextCell(2);
      this->LeaderLines->InsertCellPoint(i);
      this->LeaderLines->InsertCellPoint(i+1);
      }
    }
}

int vtkLeaderActor2D::InStringBox(double center[3], int stringSize[2], double x[3])
{
  double minX = center[0] - static_cast<double>(stringSize[0])/2.0;
  double maxX = center[0] + static_cast<double>(stringSize[0])/2.0;
  double minY = center[1] - static_cast<double>(stringSize[1])/2.0;
  double maxY = center[1] + static_cast<double>(stringSize[1])/2.0;

  if ( minX <= x[0] && x[0] <= maxX && minY <= x[1] && x[1] <= maxY )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkLeaderActor2D::ReleaseGraphicsResources(vtkWindow *win)
{
  this->LabelActor->ReleaseGraphicsResources(win);
  this->LeaderActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
// Build the axis, ticks, title, and labels and render.

int vtkLeaderActor2D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething=0;
  this->BuildLeader(viewport);

  // Everything is built, just have to render
  if ( (this->Label != NULL && this->Label[0]) || \
       (this->AutoLabel && this->LabelMapper->GetInput() != NULL ) )
    {
    renderedSomething += this->LabelActor->RenderOpaqueGeometry(viewport);
    }
  renderedSomething += this->LeaderActor->RenderOpaqueGeometry(viewport);

  return renderedSomething;
}

//----------------------------------------------------------------------------
// Render the axis, ticks, title, and labels.

int vtkLeaderActor2D::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething=0;
  this->BuildLeader(viewport);

  // Everything is built, just have to render
  if ( (this->Label != NULL && this->Label[0]) || \
       (this->AutoLabel && this->LabelMapper->GetInput() != NULL ) )
    {
    renderedSomething += this->LabelActor->RenderOverlay(viewport);
    }
  renderedSomething += this->LeaderActor->RenderOverlay(viewport);

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkLeaderActor2D::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkLeaderActor2D::ShallowCopy(vtkProp *prop)
{
  vtkLeaderActor2D *a = vtkLeaderActor2D::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetLabel(a->GetLabel());
    this->SetLabelTextProperty(a->GetLabelTextProperty());
    this->SetLabelFactor(a->GetLabelFactor());
    this->SetArrowPlacement(a->GetArrowPlacement());
    this->SetArrowStyle(a->GetArrowStyle());
    this->SetArrowLength(a->GetArrowLength());
    this->SetArrowWidth(a->GetArrowWidth());
    this->SetMinimumArrowSize(a->GetMinimumArrowSize());
    this->SetMaximumArrowSize(a->GetMaximumArrowSize());
    }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}
//----------------------------------------------------------------------------
void vtkLeaderActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Label: " << (this->Label ? this->Label : "(none)") << "\n";
  if (this->LabelTextProperty)
    {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Label Text Property: (none)\n";
    }
  os << indent << "Label Factor: " << this->LabelFactor << "\n";
  os << indent << "Auto Label: " << (this->AutoLabel ? "On\n" : "Off\n");
  os << indent << "Label Format: " << this->LabelFormat << "\n";

  os << indent << "Arrow Style: ";
  if ( this->ArrowStyle == VTK_ARROW_FILLED )
    {
    os << "Filled\n";
    }
  else if ( this->ArrowStyle == VTK_ARROW_OPEN )
    {
    os << "Open\n";
    }
  else // if ( this->ArrowStyle == VTK_ARROW_HOLLOW )
    {
    os << "Hollow\n";
    }

  os << indent << "Arrow Length: " << this->ArrowLength << "\n";
  os << indent << "Arrow Width: " << this->ArrowWidth << "\n";
  os << indent << "Minimum Arrow Size: " << this->MinimumArrowSize << "\n";
  os << indent << "Maximum Arrow Size: " << this->MaximumArrowSize << "\n";

  os << indent << "Arrow Placement: ";
  if ( this->ArrowPlacement == VTK_ARROW_NONE )
    {
    os << "No Arrows\n";
    }
  else if ( this->ArrowPlacement == VTK_ARROW_POINT1 )
    {
    os << "Arrow on first point\n";
    }
  else if ( this->ArrowPlacement == VTK_ARROW_POINT2 )
    {
    os << "Arrow on second point\n";
    }
  else
    {
    os << "Arrow on both ends\n";
    }

  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "Length: " << this->Length << "\n";
}

