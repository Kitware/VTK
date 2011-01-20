/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAxisActor2D.h"

#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkAxisActor2D);

vtkCxxSetObjectMacro(vtkAxisActor2D,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkAxisActor2D,TitleTextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Instantiate this object.
vtkAxisActor2D::vtkAxisActor2D()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.0, 0.0);

  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.75, 0.0);
  this->Position2Coordinate->SetReferenceCoordinate(NULL);

  this->NumberOfLabels = 5;

  this->Title = NULL;

  this->TitlePosition = 0.5;

  this->AdjustLabels = 1;

  this->TickLength = 5;
  this->MinorTickLength = 3;
  this->TickOffset = 2;
  this->NumberOfMinorTicks = 0;

  this->Range[0] = 0.0;
  this->Range[1] = 1.0;

  this->FontFactor = 1.0;
  this->LabelFactor = 0.75;

  this->SizeFontRelativeToAxis = 0;

  this->RulerMode = 0;
  this->RulerDistance = 1.0;

  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(1);
  this->LabelTextProperty->SetFontFamilyToArial();

  this->TitleTextProperty = vtkTextProperty::New();
  this->TitleTextProperty->ShallowCopy(this->LabelTextProperty);

  this->LabelFormat = new char[8];
  sprintf(this->LabelFormat,"%s","%-#6.3g");

  this->TitleMapper = vtkTextMapper::New();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);

  // To avoid deleting/rebuilding create once up front
  this->NumberOfLabelsBuilt = 0;
  this->LabelMappers = new vtkTextMapper * [VTK_MAX_LABELS];
  this->LabelActors = new vtkActor2D * [VTK_MAX_LABELS];
  for ( int i=0; i < VTK_MAX_LABELS; i++)
    {
    this->LabelMappers[i] = vtkTextMapper::New();
    this->LabelActors[i] = vtkActor2D::New();
    this->LabelActors[i]->SetMapper(this->LabelMappers[i]);
    }

  this->Axis = vtkPolyData::New();
  this->AxisMapper = vtkPolyDataMapper2D::New();
  this->AxisMapper->SetInput(this->Axis);
  this->AxisActor = vtkActor2D::New();
  this->AxisActor->SetMapper(this->AxisMapper);

  this->AxisVisibility = 1;
  this->TickVisibility = 1;
  this->LabelVisibility = 1;
  this->TitleVisibility = 1;

  this->LastPosition[0] = this->LastPosition[1] = 0;
  this->LastPosition2[0] = this->LastPosition2[1] = 0;

  this->LastSize[0] = this->LastSize[1] = 0;
  this->LastMaxLabelSize[0] = this->LastMaxLabelSize[1] = 0;
}

//----------------------------------------------------------------------------
vtkAxisActor2D::~vtkAxisActor2D()
{
  if (this->LabelFormat)
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }

  this->TitleMapper->Delete();
  this->TitleActor->Delete();

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }

  if (this->LabelMappers != NULL )
    {
    for (int i=0; i < VTK_MAX_LABELS; i++)
      {
      this->LabelMappers[i]->Delete();
      this->LabelActors[i]->Delete();
      }
    delete [] this->LabelMappers;
    delete [] this->LabelActors;
    }

  this->Axis->Delete();
  this->AxisMapper->Delete();
  this->AxisActor->Delete();

  this->SetLabelTextProperty(NULL);
  this->SetTitleTextProperty(NULL);
}

//----------------------------------------------------------------------------
// Build the axis, ticks, title, and labels and render.

int vtkAxisActor2D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int i, renderedSomething=0;

  this->BuildAxis(viewport);

  // Everything is built, just have to render
  if ( this->Title != NULL && this->Title[0] != 0 && this->TitleVisibility )
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }

  if ( this->AxisVisibility || this->TickVisibility )
    {
    renderedSomething += this->AxisActor->RenderOpaqueGeometry(viewport);
    }

  if ( this->LabelVisibility )
    {
    for (i=0; i<this->NumberOfLabelsBuilt; i++)
      {
      renderedSomething +=
        this->LabelActors[i]->RenderOpaqueGeometry(viewport);
      }
    }

  return renderedSomething;
}

//----------------------------------------------------------------------------
// Render the axis, ticks, title, and labels.

int vtkAxisActor2D::RenderOverlay(vtkViewport *viewport)
{
  int i, renderedSomething=0;

  // Everything is built, just have to render
  if ( this->Title != NULL && this->Title[0] != 0 && this->TitleVisibility )
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }

  if ( this->AxisVisibility || this->TickVisibility )
    {
    renderedSomething += this->AxisActor->RenderOverlay(viewport);
    }

  if ( this->LabelVisibility )
    {
    for (i=0; i<this->NumberOfLabelsBuilt; i++)
      {
      renderedSomething += this->LabelActors[i]->RenderOverlay(viewport);
      }
    }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkAxisActor2D::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkAxisActor2D::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  for (int i=0; i < VTK_MAX_LABELS; i++)
    {
    this->LabelActors[i]->ReleaseGraphicsResources(win);
    }
  this->AxisActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkAxisActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->TitleTextProperty)
    {
    os << indent << "Title Text Property:\n";
    this->TitleTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Title Text Property: (none)\n";
    }

  if (this->LabelTextProperty)
    {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Label Text Property: (none)\n";
    }

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Ruler Mode: "
     << (this->RulerMode ? "On" : "Off") <<"\n";
  os << indent << "Ruler Distance: " << this->GetRulerDistance() <<"\n";
  os << indent << "Number Of Labels: " << this->NumberOfLabels << "\n";
  os << indent << "Number Of Labels Built: "
     << this->NumberOfLabelsBuilt << "\n";
  os << indent << "Range: (" << this->Range[0]
     << ", " << this->Range[1] << ")\n";

  os << indent << "Label Format: " << this->LabelFormat << "\n";
  os << indent << "Font Factor: " << this->FontFactor << "\n";
  os << indent << "Label Factor: " << this->LabelFactor << "\n";
  os << indent << "Tick Length: " << this->TickLength << "\n";
  os << indent << "Tick Offset: " << this->TickOffset << "\n";

  os << indent << "Adjust Labels: "
     << (this->AdjustLabels ? "On\n" : "Off\n");

  os << indent << "Axis Visibility: "
     << (this->AxisVisibility ? "On\n" : "Off\n");

  os << indent << "Tick Visibility: "
     << (this->TickVisibility ? "On\n" : "Off\n");

  os << indent << "Label Visibility: "
     << (this->LabelVisibility ? "On\n" : "Off\n");

  os << indent << "Title Visibility: "
     << (this->TitleVisibility ? "On\n" : "Off\n");

  os << indent << "MinorTickLength: " << this->MinorTickLength << endl;
  os << indent << "NumberOfMinorTicks: " << this->NumberOfMinorTicks
     << endl;
  os << indent << "TitlePosition: " << this->TitlePosition << endl;

  os << indent << "Size Font Relative To Axis: "
     << (this->SizeFontRelativeToAxis ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void vtkAxisActor2D::BuildAxis(vtkViewport *viewport)
{
  int i, *x, viewportSizeHasChanged, positionsHaveChanged;
  vtkIdType ptIds[2];
  double p1[3], p2[3], offset;
  double interval, deltaX, deltaY;
  double xTick[3];
  double theta, val;
  int *size, stringSize[2];
  char string[512];

  if (this->TitleVisibility && !this->TitleTextProperty)
    {
    vtkErrorMacro(<<"Need title text property to render axis actor");
    return;
    }

  if (this->LabelVisibility && !this->LabelTextProperty)
    {
    vtkErrorMacro(<<"Need label text property to render axis actor");
    return;
    }

  // Check to see whether we have to rebuild everything
  // Viewport change may not require rebuild
  positionsHaveChanged = 0;
  int *lastPosition =
    this->PositionCoordinate->GetComputedViewportValue(viewport);
  int *lastPosition2 =
    this->Position2Coordinate->GetComputedViewportValue(viewport);
  if (lastPosition[0] != this->LastPosition[0] ||
      lastPosition[1] != this->LastPosition[1] ||
      lastPosition2[0] != this->LastPosition2[0] ||
      lastPosition2[1] != this->LastPosition2[1] )
    {
    positionsHaveChanged = 1;
    }

  // See whether fonts have to be rebuilt (font size depends on viewport size)
  viewportSizeHasChanged = 0;
  size = viewport->GetSize();
  if (this->LastSize[0] != size[0] || this->LastSize[1] != size[1])
    {
    viewportSizeHasChanged = 1;
    this->LastSize[0] = size[0];
    this->LastSize[1] = size[1];
    }

  if ( ! viewport->GetVTKWindow() ||
       (!positionsHaveChanged && !viewportSizeHasChanged &&
        viewport->GetMTime() < this->BuildTime &&
        viewport->GetVTKWindow()->GetMTime() < this->BuildTime &&
        this->GetMTime() < this->BuildTime &&
        (!this->LabelVisibility ||
         this->LabelTextProperty->GetMTime() < this->BuildTime) &&
        (!this->TitleVisibility ||
         this->TitleTextProperty->GetMTime() < this->BuildTime)) )
    {
    return;
    }

  vtkDebugMacro(<<"Rebuilding axis");

  // Initialize and get important info
  this->Axis->Initialize();
  this->AxisActor->SetProperty(this->GetProperty());
  this->TitleActor->SetProperty(this->GetProperty());

  // Compute the location of tick marks and labels

  this->UpdateAdjustedRange();

  interval = (this->AdjustedRange[1] - this->AdjustedRange[0]) / (this->AdjustedNumberOfLabels - 1);

  this->NumberOfLabelsBuilt = this->AdjustedNumberOfLabels;

  // Generate the axis and tick marks.
  // We'll do our computation in viewport coordinates. First determine the
  // location of the endpoints.
  x = this->PositionCoordinate->GetComputedViewportValue(viewport);
  p1[0] = x[0];
  p1[1] = x[1];
  p1[2] = 0.0;
  this->LastPosition[0] = x[0];
  this->LastPosition[1] = x[1];

  x = this->Position2Coordinate->GetComputedViewportValue(viewport);
  p2[0] = x[0];
  p2[1] = x[1];
  p2[2] = 0.0;
  this->LastPosition2[0] = x[0];
  this->LastPosition2[1] = x[1];

  double *xp1, *xp2, len=0.0;
  if ( this->SizeFontRelativeToAxis )
    {
    xp1 = this->PositionCoordinate->GetComputedDoubleDisplayValue(viewport);
    xp2 = this->Position2Coordinate->GetComputedDoubleViewportValue(viewport);
    len = sqrt((xp2[0]-xp1[0])*(xp2[0]-xp1[0]) + (xp2[1]-xp1[1])*(xp2[1]-xp1[1]));
    }

  vtkPoints *pts = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  this->Axis->SetPoints(pts);
  this->Axis->SetLines(lines);
  pts->Delete();
  lines->Delete();

  // Generate point along axis (as well as tick points)
  deltaX = p2[0] - p1[0];
  deltaY = p2[1] - p1[1];

  if (deltaX == 0. && deltaY == 0.)
    {
    theta = 0.;
    }
  else
    {
    theta = atan2(deltaY, deltaX);
    }

  // First axis point, where first tick is located
  ptIds[0] = pts->InsertNextPoint(p1);
  xTick[0] = p1[0] + this->TickLength*sin(theta);
  xTick[1] = p1[1] - this->TickLength*cos(theta);
  xTick[2] = 0.0;
  pts->InsertNextPoint(xTick);

  // Set up creation of ticks
  double p21[3], length;
  p21[0] = p2[0] - p1[0];
  p21[1] = p2[1] - p1[1];
  p21[2] = p2[2] - p1[2];
  length = vtkMath::Normalize(p21);

  int numTicks;
  double distance;
  if ( this->RulerMode )
    {
    double wp1[3], wp2[3], wp21[3], wLength, wDistance;
    this->PositionCoordinate->GetValue(wp1);
    this->Position2Coordinate->GetValue(wp2);
    wp21[0] = wp2[0] - wp1[0];
    wp21[1] = wp2[1] - wp1[1];
    wp21[2] = wp2[2] - wp1[2];
    wLength = vtkMath::Norm(wp21);
    wDistance = this->RulerDistance / (this->NumberOfMinorTicks+1);
    numTicks = (wDistance <= 0.0 ? 0 : static_cast<int>(wLength / wDistance)+1);
    wDistance *= numTicks;
    distance = (length / (numTicks-1)) * (wDistance/wLength);
    }
  else
    {
    numTicks = (this->AdjustedNumberOfLabels-1) *
      (this->NumberOfMinorTicks+1) + 1;
    distance = length / (numTicks-1);
    }

  for (i = 1; i < numTicks - 1; i++)
    {
    int tickLength = 0;
    if ( i % (this->NumberOfMinorTicks+1) == 0 )
      {
      tickLength = this->TickLength;
      }
    else
      {
      tickLength = this->MinorTickLength;
      }
    xTick[0] = p1[0] + i * p21[0] * distance;
    xTick[1] = p1[1] + i * p21[1] * distance;
    pts->InsertNextPoint(xTick);
    xTick[0] = xTick[0] + tickLength * sin(theta);
    xTick[1] = xTick[1] - tickLength * cos(theta);
    pts->InsertNextPoint(xTick);
    }

  // Last axis point
  ptIds[1] = pts->InsertNextPoint(p2);
  xTick[0] = p2[0] + this->TickLength*sin(theta);
  xTick[1] = p2[1] - this->TickLength*cos(theta);
  pts->InsertNextPoint(xTick);

  // Add the axis if requested
  if (this->AxisVisibility)
    {
    lines->InsertNextCell(2, ptIds);
    }

  // Create lines representing the tick marks
  if (this->TickVisibility)
    {
    for (i = 0; i < numTicks; i++)
      {
      ptIds[0] = 2*i;
      ptIds[1] = 2*i + 1;
      lines->InsertNextCell(2, ptIds);
      }
    }

  // Build the labels
  if (this->LabelVisibility)
    {
    // Update the labels text. Do it only if the range has been adjusted,
    // i.e. if we think that new labels must be created.
    // WARNING: if LabelFormat has changed, they should be recreated too
    // but at this point the check on LabelFormat is "included" in
    // UpdateAdjustedRange(), which is the function that update
    // AdjustedRangeBuildTime or not.
    unsigned long labeltime = this->AdjustedRangeBuildTime;
    if (this->AdjustedRangeBuildTime > this->BuildTime)
      {
      for (i = 0; i < this->AdjustedNumberOfLabels; i++)
        {
        val = this->AdjustedRange[0] + i * interval;
        sprintf(string, this->LabelFormat, val);
        this->LabelMappers[i]->SetInput(string);

        // Check if the label text has changed

        if (this->LabelMappers[i]->GetMTime() > labeltime)
          {
          labeltime = this->LabelMappers[i]->GetMTime();
          }
        }
      }

    // Copy prop and text prop eventually
    for (i = 0; i < this->AdjustedNumberOfLabels; i++)
        {
        this->LabelActors[i]->SetProperty(this->GetProperty());
        if (this->LabelTextProperty->GetMTime() > this->BuildTime ||
            this->AdjustedRangeBuildTime > this->BuildTime)
          {
          // Shallow copy here so that the size of the label prop is not
          // affected by the automatic adjustment of its text mapper's
          // size (i.e. its mapper's text property is identical except
          // for the font size which will be modified later). This
          // allows text actors to share the same text property, and in
          // that case specifically allows the title and label text prop
          // to be the same.
          this->LabelMappers[i]->GetTextProperty()->ShallowCopy(
            this->LabelTextProperty);
          }
        }

    // Resize the mappers if needed (i.e. viewport has changed, than
    // font size should be changed, or label text property has changed,
    // or some of the labels have changed (got bigger for example)
    if (positionsHaveChanged || viewportSizeHasChanged ||
        this->LabelTextProperty->GetMTime() > this->BuildTime ||
        labeltime > this->BuildTime)
      {
      if ( ! this->SizeFontRelativeToAxis )
        {
        vtkTextMapper::SetMultipleRelativeFontSize(viewport,
                                                   this->LabelMappers,
                                                   this->AdjustedNumberOfLabels,
                                                   size,
                                                   this->LastMaxLabelSize,
                                                   0.015*this->FontFactor*this->LabelFactor);
        }
      else
        {
         int minFontSize=1000, fontSize, minLabel=0;
         for (i = 0; i < this->AdjustedNumberOfLabels; i++)
          {
          fontSize = this->LabelMappers[i]->
            SetConstrainedFontSize(viewport,
                                   static_cast<int>((1.0/this->AdjustedNumberOfLabels)*len),
                                   static_cast<int>(0.2*len) );
          if ( fontSize < minFontSize )
            {
            minFontSize = fontSize;
            minLabel = i;
            }
          }
         for (i=0; i<this->AdjustedNumberOfLabels; i++)
           {
           this->LabelMappers[i]->GetTextProperty()->SetFontSize(minFontSize);
           }
         this->LabelMappers[minLabel]->GetSize(viewport,this->LastMaxLabelSize);
        }
      }

    // Position the mappers
    for (i = 0; i < this->AdjustedNumberOfLabels; i++)
      {
      pts->GetPoint((this->NumberOfMinorTicks+1) * 2 * i + 1, xTick);
      this->LabelMappers[i]->GetSize(viewport, stringSize);
      this->SetOffsetPosition(xTick,
                              theta,
                              this->LastMaxLabelSize[0],
                              this->LastMaxLabelSize[1],
                              this->TickOffset,
                              this->LabelActors[i]);
      }
    } // If labels visible

  // Now build the title
  if (this->Title != NULL && this->Title[0] != 0 && this->TitleVisibility)
    {
    this->TitleMapper->SetInput(this->Title);

    if (this->TitleTextProperty->GetMTime() > this->BuildTime)
      {
      // Shallow copy here so that the size of the title prop is not
      // affected by the automatic adjustment of its text mapper's
      // size (i.e. its mapper's text property is identical except for
      // the font size which will be modified later). This allows text
      // actors to share the same text property, and in that case
      // specifically allows the title and label text prop to be the same.
      this->TitleMapper->GetTextProperty()->ShallowCopy(
        this->TitleTextProperty);
      }

    if (positionsHaveChanged || viewportSizeHasChanged ||
        this->TitleTextProperty->GetMTime() > this->BuildTime)
      {
      if ( ! this->SizeFontRelativeToAxis )
        {
        vtkTextMapper::SetRelativeFontSize(this->TitleMapper, viewport, size, stringSize, 0.015*this->FontFactor);
        }
      else
        {
        this->TitleMapper->SetConstrainedFontSize(viewport,
                                                  static_cast<int>(0.33*len),
                                                  static_cast<int>(0.2*len) );
        this->TitleMapper->GetSize(viewport, stringSize);
        }
      }
    else
      {
      this->TitleMapper->GetSize(viewport, stringSize);
      }

    xTick[0] = p1[0] + (p2[0] - p1[0]) * this->TitlePosition;
    xTick[1] = p1[1] + (p2[1] - p1[1]) * this->TitlePosition;
    xTick[0] = xTick[0] + (this->TickLength + this->TickOffset) * sin(theta);
    xTick[1] = xTick[1] - (this->TickLength + this->TickOffset) * cos(theta);

    offset = 0.0;
    if (this->LabelVisibility)
      {
      offset = this->ComputeStringOffset(this->LastMaxLabelSize[0],
                                         this->LastMaxLabelSize[1],
                                         theta);
      }

    this->SetOffsetPosition(xTick,
                            theta,
                            stringSize[0],
                            stringSize[1],
                            static_cast<int>(offset),
                            this->TitleActor);
    } // If title visible

  this->BuildTime.Modified();
}


//----------------------------------------------------------------------------
void vtkAxisActor2D::UpdateAdjustedRange()
{
  // Try not to update/adjust the range to often, do not update it
  // if the object has not been modified.
  // Nevertheless, try the following optimization: there is no need to
  // update the range if the position coordinate of this actor have
  // changed. But since vtkActor2D::GetMTime() includes the check for
  // both Position and Position2 coordinates, we will have to bypass
  // it.

  if (this->vtkActor2D::Superclass::GetMTime() <= this->AdjustedRangeBuildTime)
    {
    return;
    }

  if ( this->AdjustLabels )
    {
    double interval;
    this->ComputeRange(this->Range,
                       this->AdjustedRange,
                       this->NumberOfLabels,
                       this->AdjustedNumberOfLabels,
                       interval);
    }
  else
    {
    this->AdjustedNumberOfLabels = this->NumberOfLabels;
    this->AdjustedRange[0] = this->Range[0];
    this->AdjustedRange[1] = this->Range[1];
    }
  this->AdjustedRangeBuildTime.Modified();
}

// this is a helper function that computes some useful functions
// for an axis. It returns the number of ticks
int vtkAxisActor2DComputeTicks(double sRange[2], double &interval,
                               double &root)
{
  // first we try assuming the first value is reasonable
  int numTicks;
  double range    = fabs(sRange[1]-sRange[0]);
  int rootPower   = static_cast<int>(floor(log10(range)-1));
  root     = pow(10.0,rootPower);
  // val will be between 10 and 100 inclusive of 10 but not 100
  double val      = range/root;
  // first we check for an exact match
  for (numTicks = 5; numTicks < 9; ++numTicks)
    {
    if (fabs(val/(numTicks-1.0) - floor(val/(numTicks-1.0))) < .0001)
      {
      interval = val*root/(numTicks-1.0);
      return numTicks;
      }
    }

  // if there isn't an exact match find a reasonable value
  int newIntScale = 10;
  if (val > 10)
    {
    newIntScale = 12;
    }
  if (val > 12)
    {
    newIntScale = 15;
    }
  if (val > 15)
    {
    newIntScale = 18;
    }
  if (val > 18)
    {
    newIntScale = 20;
    }
  if (val > 20)
    {
    newIntScale = 25;
    }
  if (val > 25)
    {
    newIntScale = 30;
    }
  if (val > 30)
    {
    newIntScale = 40;
    }
  if (val > 40)
    {
    newIntScale = 50;
    }
  if (val > 50)
    {
    newIntScale = 60;
    }
  if (val > 60)
    {
    newIntScale = 70;
    }
  if (val > 70)
    {
    newIntScale = 80;
    }
  if (val > 80)
    {
    newIntScale = 90;
    }
  if (val > 90)
    {
    newIntScale = 100;
    }

  // how many ticks should we have
  switch (newIntScale)
    {
    case 12:
    case 20:
    case 40:
    case 80:
      numTicks = 5;
      break;
    case 18:
    case 30:
    case 60:
    case 90:
      numTicks = 7;
      break;
    case 10:
    case 15:
    case 25:
    case 50:
    case 100:
      numTicks = 6;
      break;
    case 70:
      numTicks = 8;
      break;
    }

  interval = newIntScale*root/(numTicks-1.0);
  return numTicks;
}

//----------------------------------------------------------------------------
//this method takes an initial range and an initial number of ticks and then
//computes a final range and number of ticks so that two properties are
//satisfied. First the final range includes at least the initial range, and
//second the final range divided by the number of ticks (minus one) will be a
//reasonable interval
void vtkAxisActor2D::ComputeRange(double inRange[2],
                                  double outRange[2],
                                  int vtkNotUsed(inNumTicks),
                                  int &numTicks,
                                  double &interval)
{
  // Handle the range
  double sRange[2];
  if ( inRange[0] < inRange[1] )
    {
    sRange[0] = inRange[0];
    sRange[1] = inRange[1];
    }
  else if ( inRange[0] > inRange[1] )
    {
    sRange[1] = inRange[0];
    sRange[0] = inRange[1];
    }
  else // they're equal, so perturb them by 1 percent
    {
    double perturb = 100.;
    if (inRange[0] == 0.0)
      { // if they are both zero, then just perturb about zero
      sRange[0] = -1/perturb;
      sRange[1] = 1/perturb;
      }
    else
      {
      sRange[0] = inRange[0] - inRange[0]/perturb;
      sRange[1] = inRange[0] + inRange[0]/perturb;
      }
    }

  double root;
  numTicks = vtkAxisActor2DComputeTicks(sRange, interval, root);

  // is the starting point reasonable?
  if (fabs(sRange[0]/root - floor(sRange[0]/root)) < 0.01)
    {
    outRange[0] = sRange[0];
    outRange[1] = outRange[0] + (numTicks-1.0)*interval;
    }
  else
    {
    // OK the starting point is not a good number, so we must widen the range
    // First see if the current range will handle moving the start point
    outRange[0] = floor(sRange[0]/root)*root;
    if (outRange[0]+(numTicks-1.0)*interval <= sRange[1])
      {
      outRange[1] = outRange[0] + (numTicks-1.0)*interval;
      }
    else
      {
      // Finally in this case we must switch to a larger range to
      // have reasonable starting and ending values
      sRange[0] = outRange[0];
      numTicks = vtkAxisActor2DComputeTicks(sRange, interval, root);
      outRange[1] = outRange[0] + (numTicks-1.0)*interval;
      }
    }

  // Adust if necessary
  if ( inRange[0] > inRange[1] )
    {
    sRange[0] = outRange[1];
    outRange[1] = outRange[0];
    outRange[0] = sRange[0];
    interval = -interval;
    }

}

//----------------------------------------------------------------------------
// Position text with respect to a point (xTick) where the angle of the line
// from the point to the center of the text is given by theta. The offset
// is the spacing between ticks and labels.
void vtkAxisActor2D::SetOffsetPosition(double xTick[3], double theta,
                                       int stringWidth, int stringHeight,
                                       int offset, vtkActor2D *actor)
{
  double x, y, center[2];
  int pos[2];

  x = stringWidth/2.0 + offset;
  y = stringHeight/2.0 + offset;

  center[0] = xTick[0] + x*sin(theta);
  center[1] = xTick[1] - y*cos(theta);

  pos[0] = static_cast<int>(center[0] - stringWidth/2.0);
  pos[1] = static_cast<int>(center[1] - stringHeight/2.0);

  actor->SetPosition(pos[0], pos[1]);
}

//----------------------------------------------------------------------------
double vtkAxisActor2D::ComputeStringOffset(double width, double height,
                                           double theta)
{
  double f1 = height*cos(theta);
  double f2 = width*sin(theta);
  return (1.2 * sqrt(f1*f1 + f2*f2));
}

//----------------------------------------------------------------------------
void vtkAxisActor2D::ShallowCopy(vtkProp *prop)
{
  vtkAxisActor2D *a = vtkAxisActor2D::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetRange(a->GetRange());
    this->SetNumberOfLabels(a->GetNumberOfLabels());
    this->SetLabelFormat(a->GetLabelFormat());
    this->SetAdjustLabels(a->GetAdjustLabels());
    this->SetTitle(a->GetTitle());
    this->SetTickLength(a->GetTickLength());
    this->SetTickOffset(a->GetTickOffset());
    this->SetAxisVisibility(a->GetAxisVisibility());
    this->SetTickVisibility(a->GetTickVisibility());
    this->SetLabelVisibility(a->GetLabelVisibility());
    this->SetTitleVisibility(a->GetTitleVisibility());
    this->SetFontFactor(a->GetFontFactor());
    this->SetLabelFactor(a->GetLabelFactor());
    this->SetLabelTextProperty(a->GetLabelTextProperty());
    this->SetTitleTextProperty(a->GetTitleTextProperty());
    }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}
