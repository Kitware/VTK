/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisActor2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAxisActor2D.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

vtkCxxRevisionMacro(vtkAxisActor2D, "1.28");
vtkStandardNewMacro(vtkAxisActor2D);

vtkCxxSetObjectMacro(vtkAxisActor2D,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkAxisActor2D,TitleTextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Instantiate this object.
vtkAxisActor2D::vtkAxisActor2D()
{
  this->Point1Coordinate = vtkCoordinate::New();
  this->Point1Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Point1Coordinate->SetValue(0.0, 0.0);

  this->Point2Coordinate = vtkCoordinate::New();
  this->Point2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Point2Coordinate->SetValue(0.75, 0.0);
  
  this->NumberOfLabels = 5;

  this->Title = NULL;

  this->AdjustLabels = 1;

  this->TickLength = 5;
  this->TickOffset = 2;

  this->Range[0] = 0.0;
  this->Range[1] = 1.0;

  this->FontFactor = 1.0;
  this->LabelFactor = 0.75;

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
  
  this->LastPoint1[0] = this->LastPoint1[1] = 0;
  this->LastPoint2[0] = this->LastPoint2[1] = 0;

  this->LastSize[0] = this->LastSize[1] = 0;

  this->LastTitleFontSize = 0;
  this->LastLabelFontSize = 0;
}

//----------------------------------------------------------------------------
vtkAxisActor2D::~vtkAxisActor2D()
{
  this->Point1Coordinate->Delete();
  this->Point2Coordinate->Delete();
  
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

  if (this->Point1Coordinate)
    {
    os << indent << "Point1 Coordinate:\n";
    this->Point1Coordinate->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Point1 Coordinate: (none)\n";
    }

  if (this->Point2Coordinate)
    {
    os << indent << "Point2 Coordinate:\n";
    this->Point2Coordinate->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Point2 Coordinate: (none)\n";
    }

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
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
}

//----------------------------------------------------------------------------
#define VTK_AA2D_DEBUG 0

void vtkAxisActor2D::BuildAxis(vtkViewport *viewport)
{
  int i, *x, viewportSizeHasChanged;
  vtkIdType ptIds[2];
  float p1[3], p2[3], offset, maxWidth=0.0, maxHeight=0.0;
  int numLabels;
  float outRange[2], interval, deltaX, deltaY, xTick[3];
  float theta, val;
  int *size, stringSize[2];
  char string[512];

  if (viewport->GetMTime() > this->BuildTime || 
      (viewport->GetVTKWindow() && 
       viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
    {
    // Check to see whether we have to rebuild everything
    // Viewport change may not require rebuild
    int *lastPoint1 = 
      this->Point1Coordinate->GetComputedViewportValue(viewport);
    int *lastPoint2 =
      this->Point2Coordinate->GetComputedViewportValue(viewport);
    if (lastPoint1[0] != this->LastPoint1[0] ||
        lastPoint1[1] != this->LastPoint1[1] ||
        lastPoint2[0] != this->LastPoint2[0] ||
        lastPoint2[1] != this->LastPoint2[1] )
      {
      this->Modified();
      }
    }
  
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

  if (this->GetMTime() < this->BuildTime &&
      (!this->LabelVisibility || 
       this->LabelTextProperty->GetMTime() < this->BuildTime) &&
      (!this->TitleVisibility || 
       this->TitleTextProperty->GetMTime() < this->BuildTime))
    {
    return;
    }
  
#if VTK_AA2D_DEBUG
  printf ("vtkAxisActor2D::BuildAxis: Rebuilding axis\n");
#endif

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

  x = this->Point1Coordinate->GetComputedViewportValue(viewport);
  p1[0] = (float)x[0]; 
  p1[1] = (float)x[1]; 
  p1[2] = 0.0;
  this->LastPoint1[0] = x[0]; 
  this->LastPoint1[1] = x[1];

  x = this->Point2Coordinate->GetComputedViewportValue(viewport);
  p2[0] = (float)x[0]; 
  p2[1] = (float)x[1]; 
  p2[2] = 0.0;
  this->LastPoint2[0] = x[0]; 
  this->LastPoint2[1] = x[1];

  size = viewport->GetSize();

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

  // First axis point

  ptIds[0] = pts->InsertNextPoint(p1); 
  xTick[0] = p1[0] + this->TickLength*sin(theta);
  xTick[1] = p1[1] - this->TickLength*cos(theta);
  xTick[2] = 0.0;

  pts->InsertNextPoint(xTick);
  for (i = 1; i < this->AdjustedNumberOfLabels - 1; i++)
    {
    xTick[0] = p1[0] + i * (p2[0] - p1[0]) / (this->AdjustedNumberOfLabels - 1);
    xTick[1] = p1[1] + i * (p2[1] - p1[1]) / (this->AdjustedNumberOfLabels - 1);
    pts->InsertNextPoint(xTick);
    xTick[0] = xTick[0] + this->TickLength * sin(theta);
    xTick[1] = xTick[1] - this->TickLength * cos(theta);
    pts->InsertNextPoint(xTick);
    }

  // Last axis point

  ptIds[1] = pts->InsertNextPoint(p2); 
  xTick[0] = p2[0] + this->TickLength*sin(theta);
  xTick[1] = p2[1] - this->TickLength*cos(theta);
  pts->InsertNextPoint(xTick);

  if (this->AxisVisibility)
    {
    lines->InsertNextCell(2, ptIds);
    }

  // Create points and lines

  if (this->TickVisibility) 
    {
    for (i = 0; i < this->AdjustedNumberOfLabels; i++)
      {
      ptIds[0] = 2*i;
      ptIds[1] = 2*i + 1;
      lines->InsertNextCell(2, ptIds);
      }
    }

  // See whether fonts have to be rebuilt (font size depends on viewport size)

  if (this->LastSize[0] != size[0] || this->LastSize[1] != size[1])
    {
    viewportSizeHasChanged = 1;
    this->LastSize[0] = size[0];
    this->LastSize[1] = size[1];
    }
  else
    {
    viewportSizeHasChanged = 0;
    }

  // Build the labels

  if (this->LabelVisibility)
    {

#if VTK_AA2D_DEBUG
    printf ("vtkAxisActor2D::BuildAxis: Rebuilding axis => labels\n");
#endif

    // Update the mappers/actors

    for (i = 0; i < this->AdjustedNumberOfLabels; i++)
      {
      val = this->AdjustedRange[0] + (float)i * interval;
      sprintf(string, this->LabelFormat, val);
      this->LabelMappers[i]->SetInput(string);

      this->LabelActors[i]->SetProperty(this->GetProperty());
      
      if (this->LabelTextProperty->GetMTime() > this->BuildTime)
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

    // Resize the mappers if needed

    if (viewportSizeHasChanged ||
        this->LabelTextProperty->GetMTime() > this->BuildTime)
      {
      this->SetMultipleFontSize(viewport, 
                                this->LabelMappers, 
                                this->AdjustedNumberOfLabels,
                                size,
                                this->FontFactor * this->LabelFactor,
                                stringSize);
      
      maxWidth = (stringSize[0] > maxWidth ? stringSize[0] : maxWidth);
      maxHeight = (stringSize[1] > maxHeight ? stringSize[1] : maxHeight);
      }
    
    // Position the mappers

    for (i = 0; i < this->AdjustedNumberOfLabels; i++)
      {
      pts->GetPoint(2 * i + 1, xTick);
      this->SetOffsetPosition(xTick, 
                              theta, 
                              static_cast<int>(maxWidth), 
                              static_cast<int>(maxHeight), 
                              this->TickOffset, 
                              this->LabelActors[i]);
      }
    } // If labels visible

  // Now build the title

  if (this->Title != NULL && this->Title[0] != 0 && this->TitleVisibility)
    {
#if VTK_AA2D_DEBUG
    printf ("vtkAxisActor2D::BuildAxis: Rebuilding axis => title\n");
#endif

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

    if (viewportSizeHasChanged ||
        this->TitleTextProperty->GetMTime() > this->BuildTime)
      {
      this->SetFontSize(viewport, 
                        this->TitleMapper,
                        size,
                        this->FontFactor,
                        stringSize);
      }
    else
      {
      this->TitleMapper->GetSize(viewport, stringSize);
      }

    xTick[0] = p1[0] + (p2[0] - p1[0]) / 2.0;
    xTick[1] = p1[1] + (p2[1] - p1[1]) / 2.0;
    xTick[0] = xTick[0] + (this->TickLength + this->TickOffset) * sin(theta);
    xTick[1] = xTick[1] - (this->TickLength + this->TickOffset) * cos(theta);
    
    offset = 0.0;
    if (this->LabelVisibility)
      {
      offset = this->ComputeStringOffset(maxWidth, maxHeight, theta);
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
#define VTK_AA2D_FACTOR 0.015

int vtkAxisActor2D::SetFontSize(vtkViewport *viewport, 
                                vtkTextMapper *textMapper, 
                                int *targetSize,
                                float factor, 
                                int *stringSize)
{
  int fontSize, targetWidth, targetHeight;

  // Find the best size for the font
  // WARNING: check that the above values are in sync with the above
  // similar function.

  targetWidth = targetSize [0] > targetSize[1] ? targetSize[0] : targetSize[1];

  targetHeight = (int)(VTK_AA2D_FACTOR * factor * targetSize[0] + 
                       VTK_AA2D_FACTOR * factor * targetSize[1]);

  fontSize = textMapper->SetConstrainedFontSize(viewport, 
                                                targetWidth, targetHeight);

  textMapper->GetSize(viewport, stringSize);

  return fontSize;
}

//----------------------------------------------------------------------------
int vtkAxisActor2D::SetMultipleFontSize(vtkViewport *viewport, 
                                        vtkTextMapper **textMappers, 
                                        int nbOfMappers, 
                                        int *targetSize,
                                        float factor, 
                                        int *stringSize)
{
  int fontSize, targetWidth, targetHeight;

  // Find the best size for the font
  // WARNING: check that the below values are in sync with the above
  // similar function.

  targetWidth = targetSize [0] > targetSize[1] ? targetSize[0] : targetSize[1];

  targetHeight = (int)(VTK_AA2D_FACTOR * factor * targetSize[0] + 
                       VTK_AA2D_FACTOR * factor * targetSize[1]);

  fontSize = 
    vtkTextMapper::SetMultipleConstrainedFontSize(viewport, 
                                                  targetWidth, targetHeight,
                                                  textMappers,
                                                  nbOfMappers,
                                                  stringSize);

  return fontSize;
}
#undef VTK_AA2D_FACTOR

//----------------------------------------------------------------------------
void vtkAxisActor2D::UpdateAdjustedRange()
{
  if (this->GetMTime() <= this->AdjustedRangeBuildTime)
    {
    return;
    }
  
  if ( this->AdjustLabels )
    {
    float interval;
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

//----------------------------------------------------------------------------
#define VTK_NUM_DIVS 11
void vtkAxisActor2D::ComputeRange(float inRange[2], 
                                  float outRange[2],
                                  int inNumTicks, 
                                  int &numTicks, 
                                  float &interval)
{
  static float divs[VTK_NUM_DIVS] = {10.0, 8.0, 5.0, 4.0, 2.0, 1.0,
                                     0.5, 0.25, 0.20, 0.125, 0.10};
  float range = fabs(inRange[1]-inRange[0]), sRange[2];
  float logFactor = (float)pow((double)(10.0), (double)(floor(log10(range))));
  float lastInterval;
  int j;

  // Handle the range
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
    float perturb = 100.;
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
    //recompute range
    range = fabs(sRange[1]-sRange[0]);
    //recompute logfactor
    logFactor = (float)pow((double)(10.0), (double)(floor(log10(range))));
    //recompute inRange so things get flipped correctly at the end
    inRange[0] = sRange[0];
    inRange[1] = sRange[1];
    }

  // let's figure out the interval. We'll loop until we find something with the
  // right number of tick marks.
  lastInterval = logFactor * divs[0];
  for ( j=1; j < VTK_NUM_DIVS; j++ )
    {
    interval = logFactor * divs[j];
    if ( ((inNumTicks-1)*interval) < range )
      {
      break;
      }
    lastInterval = interval;
    }
  interval = lastInterval;

  numTicks = (int) ceil(range/interval) + 1; //trim number of extra ticks

  // Get the start value of the range
  for ( j=0; j < VTK_NUM_DIVS; j++ )
    {
    outRange[0] = ((int)floor(sRange[0]/(logFactor*divs[j]))) * logFactor*divs[j];
    if ( (outRange[0] + (numTicks-1)*interval) >= sRange[1] )
      {
      break;
      }
    }

  // Get the end value of the range
  outRange[1] = outRange[0] + (numTicks-1)*interval;

  // Adust if necessary
  if ( inRange[0] > inRange[1] )
    {
    sRange[0] = outRange[1];
    outRange[1] = outRange[0];
    outRange[0] = sRange[0];
    interval = -interval;
    }

}
#undef VTK_NUM_DIVS

//----------------------------------------------------------------------------
// Position text with respect to a point (xTick) where the angle of the line
// from the point to the center of the text is given by theta. The offset
// is the spacing between ticks and labels.
void vtkAxisActor2D::SetOffsetPosition(float xTick[3], float theta, 
                                       int stringWidth, int stringHeight, 
                                       int offset, vtkActor2D *actor)
{
  float x, y, center[2];
  int pos[2];
   
  x = stringWidth/2.0 + offset;
  y = stringHeight/2.0 + offset;

  center[0] = xTick[0] + x*sin(theta);
  center[1] = xTick[1] - y*cos(theta);
    
  pos[0] = (int)(center[0] - stringWidth/2.0);
  pos[1] = (int)(center[1] - stringHeight/2.0);
  
  actor->SetPosition(pos[0], pos[1]);
}

//----------------------------------------------------------------------------
float vtkAxisActor2D::ComputeStringOffset(float width, float height,
                                          float theta)
{
  float f1 = height*cos(theta);
  float f2 = width*sin(theta);
  return (1.2 * sqrt(f1*f1 + f2*f2));
}

//----------------------------------------------------------------------------
void vtkAxisActor2D::ShallowCopy(vtkProp *prop)
{
  vtkAxisActor2D *a = vtkAxisActor2D::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetPoint1(a->GetPoint1());
    this->SetPoint2(a->GetPoint2());
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
    
//----------------------------------------------------------------------------
// Backward compatibility calls

void vtkAxisActor2D::SetFontFamily(int val) 
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetFontFamily(val); 
    }
  if (this->TitleTextProperty)
    {
    this->TitleTextProperty->SetFontFamily(val); 
    }
}

int vtkAxisActor2D::GetFontFamily()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetFontFamily(); 
    }
  else
    {
    return 0;
    }
}

void vtkAxisActor2D::SetBold(int val)
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetBold(val); 
    }
  if (this->TitleTextProperty)
    {
    this->TitleTextProperty->SetBold(val); 
    }
}

int vtkAxisActor2D::GetBold()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetBold(); 
    }
  else
    {
    return 0;
    }
}

void vtkAxisActor2D::SetItalic(int val)
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetItalic(val); 
    }
  if (this->TitleTextProperty)
    {
    this->TitleTextProperty->SetItalic(val); 
    }
}

int vtkAxisActor2D::GetItalic()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetItalic(); 
    }
  else
    {
    return 0;
    }
}

void vtkAxisActor2D::SetShadow(int val)
{ 
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->SetShadow(val); 
    }
  if (this->TitleTextProperty)
    {
    this->TitleTextProperty->SetShadow(val); 
    }
}

int vtkAxisActor2D::GetShadow()
{ 
  if (this->LabelTextProperty)
    {
    return this->LabelTextProperty->GetShadow(); 
    }
  else
    {
    return 0;
    }
}
