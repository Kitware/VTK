/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisActor2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Kitware & RPI/SCOREC who supported the development
             of this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkAxisActor2D.h"
#include "vtkObjectFactory.h"


//--------------------------------------------------------------------------
vtkAxisActor2D* vtkAxisActor2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAxisActor2D");
  if(ret)
    {
      return (vtkAxisActor2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAxisActor2D;
}

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
  this->FontFactor = 1.0;
  this->LabelFactor = 0.75;
  this->TickLength = 5;
  this->TickOffset = 2;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;

  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->LabelFormat = new char[8]; 
  sprintf(this->LabelFormat,"%s","%-#6.3g");

  this->TitleMapper = vtkTextMapper::New();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  
  // to avoid deleting/rebuilding create once up front
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
  this->LastSize[0] = this->LastSize[1] = -1;
  this->LastTitleFontSize = 0;
  this->LastLabelFontSize = 0;
}

vtkAxisActor2D::~vtkAxisActor2D()
{
  this->Point1Coordinate->Delete();
  this->Point1Coordinate = NULL;
  
  this->Point2Coordinate->Delete();
  this->Point2Coordinate = NULL;
  
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
}

// Build the axis, ticks, title, and labels and render.
//
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
	  renderedSomething += this->LabelActors[i]->RenderOpaqueGeometry(viewport);
	}
    }

  return renderedSomething;
}

// Render the axis, ticks, title, and labels.
//
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

void vtkAxisActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Number Of Labels: " << this->NumberOfLabels << "\n";
  os << indent << "Number Of Labels Built: " 
     << this->NumberOfLabelsBuilt << "\n";
  os << indent << "Range: (" << this->Range[0] 
     << ", " << this->Range[1] << ")\n";

  os << indent << "Font Family: ";
  if ( this->FontFamily == VTK_ARIAL )
    {
      os << "Arial\n";
    }
  else if ( this->FontFamily == VTK_COURIER )
    {
      os << "Courier\n";
    }
  else
    {
      os << "Times\n";
    }

  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
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
  
  os << indent << "Point1 Coordinate: " << this->Point1Coordinate << "\n";
  this->Point1Coordinate->PrintSelf(os, indent.GetNextIndent());
  
  os << indent << "Point2 Coordinate: " << this->Point2Coordinate << "\n";
  this->Point2Coordinate->PrintSelf(os, indent.GetNextIndent());
}

void vtkAxisActor2D::BuildAxis(vtkViewport *viewport)
{
  int i, *x, needToBuildFont;
  vtkIdType ptIds[2];
  float p1[3], p2[3], offset, maxWidth=0.0, maxHeight=0.0;
  int numLabels;
  float outRange[2], interval, deltaX, deltaY, xTick[3];
  float theta, val;
  int *size, fontSize[2], stringWidth, stringHeight;
  char string[512];

  if ( this->GetMTime() < this->BuildTime &&
       viewport->GetMTime() < this->BuildTime )
    {
      return; //already built
    }

  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() < this->BuildTime &&
       viewport->GetMTime() > this->BuildTime )
    { //viewport change may not require rebuild
      int *lastPoint1=this->Point1Coordinate->GetComputedViewportValue(viewport);
      int *lastPoint2=this->Point2Coordinate->GetComputedViewportValue(viewport);
      if ( lastPoint1[0] == this->LastPoint1[0] &&
	   lastPoint1[1] == this->LastPoint1[1] &&
	   lastPoint2[0] == this->LastPoint2[0] &&
	   lastPoint2[1] == this->LastPoint2[1] )
	{
	  return;
	}
    }

  vtkDebugMacro(<<"Rebuilding axis");

  // Initialize and get important info

  this->Axis->Initialize();
  this->AxisActor->SetProperty(this->GetProperty());
  this->TitleActor->SetProperty(this->GetProperty());

  // Compute the location of tick marks and labels
  if ( this->AdjustLabels )
    {
      this->ComputeRange(this->Range, outRange, this->NumberOfLabels, 
			 numLabels, interval);
    }
  else //compute our own ugly spacing
    {
      numLabels = this->NumberOfLabels;
      interval = (this->Range[1] - this->Range[0]) / (numLabels-1);
      outRange[0] = this->Range[0]; outRange[1] = this->Range[1];
    }
  this->NumberOfLabelsBuilt = numLabels;

  // Generate the axis and tick marks.
  // We'll do our computation in viewport coordinates. First determine the
  // location of the endpoints.
  x = this->Point1Coordinate->GetComputedViewportValue(viewport);
  p1[0] = (float)x[0]; p1[1] = (float)x[1]; p1[2] = 0.0;
  this->LastPoint1[0] = x[0]; this->LastPoint1[1] = x[1];
  x = this->Point2Coordinate->GetComputedViewportValue(viewport);
  p2[0] = (float)x[0]; p2[1] = (float)x[1]; p2[2] = 0.0;
  this->LastPoint2[0] = x[0]; this->LastPoint2[1] = x[1];
  size = viewport->GetSize();

  // See whether fonts have to be rebuilt
  if ( this->LastSize[0] != size[0] || this->LastSize[1] != size[1] )
    {
      needToBuildFont = 1;
      this->LastSize[0] = size[0];
      this->LastSize[1] = size[1];
    }
  else
    {
      needToBuildFont = 0;
    }

  vtkPoints *pts = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  this->Axis->SetPoints(pts);
  this->Axis->SetLines(lines);
  pts->Delete();
  lines->Delete();

  // generate point along axis (as well as tick points)
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

  ptIds[0] = pts->InsertNextPoint(p1); //first axis point
  xTick[0] = p1[0] + this->TickLength*sin(theta);
  xTick[1] = p1[1] - this->TickLength*cos(theta);
  xTick[2] = 0.0;

  pts->InsertNextPoint(xTick);
  for ( i=1; i < (numLabels-1); i++)
    {
      xTick[0] = p1[0] + i*(p2[0] - p1[0]) / (numLabels-1);
      xTick[1] = p1[1] + i*(p2[1] - p1[1]) / (numLabels-1);
      pts->InsertNextPoint(xTick);
      xTick[0] = xTick[0] + this->TickLength*sin(theta);
      xTick[1] = xTick[1] - this->TickLength*cos(theta);
      pts->InsertNextPoint(xTick);
    }

  ptIds[1] = pts->InsertNextPoint(p2); //last axis point
  xTick[0] = p2[0] + this->TickLength*sin(theta);
  xTick[1] = p2[1] - this->TickLength*cos(theta);
  pts->InsertNextPoint(xTick);

  if ( this->AxisVisibility )
    {
      lines->InsertNextCell(2, ptIds);
    }

  if ( this->TickVisibility ) //create points and lines
    {
      for (i=0; i < numLabels; i++)
	{
	  ptIds[0] = 2*i;
	  ptIds[1] = 2*i + 1;
	  lines->InsertNextCell(2, ptIds);
	}
    }

  // Build the labels
  if ( this->LabelVisibility )
    {
      for ( i=0; i < numLabels; i++)
	{
	  val = outRange[0] + i*interval;
	  sprintf(string, this->LabelFormat, val);
	  this->LabelMappers[i]->SetInput(string);
	  this->LabelMappers[i]->SetBold(this->Bold);
	  this->LabelMappers[i]->SetItalic(this->Italic);
	  this->LabelMappers[i]->SetShadow(this->Shadow);
	  this->LabelMappers[i]->SetFontFamily(this->FontFamily);
	  if ( needToBuildFont )
	    {
	      this->LastLabelFontSize = this->SetFontSize(viewport, 
							  this->LabelMappers[i], size,
							  this->FontFactor*this->LabelFactor,
							  stringWidth, stringHeight);
	    }
	  else
	    {
	      this->LabelMappers[i]->SetFontSize(this->LastLabelFontSize);
	    }
	  this->LabelMappers[i]->GetSize(viewport, fontSize);
	  maxWidth = (fontSize[0] > maxWidth ? fontSize[0] : maxWidth);
	  maxHeight = (fontSize[1] > maxHeight ? fontSize[1] : maxHeight);
	  this->LabelActors[i]->SetProperty(this->GetProperty());
	  pts->GetPoint(2*i+1, xTick);
	  this->SetOffsetPosition(xTick, theta, maxWidth, maxHeight, 
				  this->TickOffset, this->LabelActors[i]);
	}
    }// if labels visible

  // Now build the title
  if ( this->Title != NULL && this->Title[0] != 0 && this->TitleVisibility )
    {
      this->TitleMapper->SetInput(this->Title);
      this->TitleMapper->SetBold(this->Bold);
      this->TitleMapper->SetItalic(this->Italic);
      this->TitleMapper->SetShadow(this->Shadow);
      this->TitleMapper->SetFontFamily(this->FontFamily);
      if ( needToBuildFont )
	{
	  this->LastTitleFontSize = this->SetFontSize(viewport, this->TitleMapper, 
						      size, this->FontFactor,
						      stringWidth, stringHeight);
	}
      else
	{
	  this->TitleMapper->SetFontSize(this->LastTitleFontSize);
	}
      this->TitleMapper->GetSize(viewport, fontSize);
      xTick[0] = p1[0] + (p2[0] - p1[0]) / 2.0;
      xTick[1] = p1[1] + (p2[1] - p1[1]) / 2.0;
      xTick[0] = xTick[0] + (this->TickLength+this->TickOffset)*sin(theta);
      xTick[1] = xTick[1] - (this->TickLength+this->TickOffset)*cos(theta);
    
      offset = 0.0;
      if ( this->LabelVisibility)
	{
	  offset = this->ComputeStringOffset(maxWidth, maxHeight, theta);
	}

      this->SetOffsetPosition(xTick, theta, fontSize[0], fontSize[1], 
			      offset, this->TitleActor);
    } //if title visible

  this->BuildTime.Modified();
}

#define VTK_NUM_DIVS 11
void vtkAxisActor2D::ComputeRange(float inRange[2], float outRange[2],
                                  int inNumTicks, int &numTicks, 
                                  float &interval)
{
  static float divs[VTK_NUM_DIVS] = {10.0, 8.0, 5.0, 4.0, 2.0, 1.0,
				     0.5, 0.25, 0.20, 0.125, 0.10};
  float range = fabs(inRange[1]-inRange[0]), sRange[2];
  float logFactor = (float)pow(10.0, floor(log10(range)));
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
      logFactor = (float)pow(10.0, floor(log10(range)));
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

int vtkAxisActor2D::SetFontSize(vtkViewport *viewport, 
				vtkTextMapper *textMapper, int *size,
				float factor, int &stringWidth, int &stringHeight)
{
  int tempi[2], fontSize, target, length=(size[0]>size[1]?size[0]:size[1]);

  // find the best size for the font
  target = (int)(0.015*factor*size[0] + 0.015*factor*size[1]);

  fontSize = target;
  textMapper->SetFontSize(fontSize);
  textMapper->GetSize(viewport,tempi);
  if (tempi[0] <= 0 || tempi[1] <= 0 )
    {
      stringHeight = 0;
      stringWidth = 0;
      return 0;
    }
  
  while (tempi[1] < target && fontSize < 100)
    {
      fontSize++;
      textMapper->SetFontSize(fontSize);
      textMapper->GetSize(viewport,tempi);
    }
  while ((tempi[1] > target || tempi[0] > length) && fontSize > 0)
    {
      fontSize--;
      textMapper->SetFontSize(fontSize);
      textMapper->GetSize(viewport,tempi);
    }
  stringWidth = tempi[0];
  stringHeight = tempi[1];
  
  return fontSize;
}

// Posiion text with respect to a point (xTick) where the angle of the line
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

float vtkAxisActor2D::ComputeStringOffset(float width, float height,
                                          float theta)
{
  float f1 = height*cos(theta);
  float f2 = width*sin(theta);
  return (1.2 * sqrt(f1*f1 + f2*f2));
}

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
      this->SetBold(a->GetBold());
      this->SetItalic(a->GetItalic());
      this->SetShadow(a->GetShadow());
      this->SetFontFamily(a->GetFontFamily());
      this->SetTickLength(a->GetTickLength());
      this->SetTickOffset(a->GetTickOffset());
      this->SetAxisVisibility(a->GetAxisVisibility());
      this->SetTickVisibility(a->GetTickVisibility());
      this->SetLabelVisibility(a->GetLabelVisibility());
      this->SetTitleVisibility(a->GetTitleVisibility());
      this->SetFontFactor(a->GetFontFactor());
      this->SetLabelFactor(a->GetLabelFactor());
    }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}
      
