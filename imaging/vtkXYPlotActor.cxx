/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXYPlotActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Kitware & RPI/SCOREC who supported the development
             of this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkXYPlotActor.h"
#include "vtkDataSetCollection.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkXYPlotActor* vtkXYPlotActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXYPlotActor");
  if(ret)
    {
    return (vtkXYPlotActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXYPlotActor;
}




// Instantiate object
vtkXYPlotActor::vtkXYPlotActor()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.25,0.25);
  
  this->Position2Coordinate = vtkCoordinate::New();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.5, 0.5);
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);
  
  this->InputList = vtkDataSetCollection::New();

  this->Title = NULL;
  this->XTitle = new char[7];
  sprintf(this->XTitle,"%s","X Axis");
  this->YTitle = new char[7];
  sprintf(this->YTitle,"%s","Y Axis");

  this->XValues = VTK_XYPLOT_INDEX;

  this->NumberOfXLabels = 5;
  this->NumberOfYLabels = 5;

  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->LabelFormat = new char[8]; 
  sprintf(this->LabelFormat,"%s","%-#6.3g");
  
  this->XRange[0] = 0.0;
  this->XRange[1] = 0.0;
  this->YRange[0] = 0.0;
  this->YRange[1] = 0.0;

  this->Border = 5;
  this->PlotLines = 1;
  this->PlotPoints = 0;

  this->TitleMapper = vtkTextMapper::New();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  
  this->XAxis = vtkAxisActor2D::New();
  this->XAxis->GetPoint1Coordinate()->SetCoordinateSystemToViewport();
  this->XAxis->GetPoint2Coordinate()->SetCoordinateSystemToViewport();
  this->XAxis->SetProperty(this->GetProperty());
  this->YAxis = vtkAxisActor2D::New();
  this->YAxis->GetPoint1Coordinate()->SetCoordinateSystemToViewport();
  this->YAxis->GetPoint2Coordinate()->SetCoordinateSystemToViewport();
  this->YAxis->SetProperty(this->GetProperty());
  
  this->PlotData = vtkPolyData::New();
  this->PlotMapper = vtkPolyDataMapper2D::New();
  this->PlotMapper->SetInput(this->PlotData);
  this->PlotActor = vtkActor2D::New();
  this->PlotActor->SetMapper(this->PlotMapper);
}

vtkXYPlotActor::~vtkXYPlotActor()
{
  this->Position2Coordinate->Delete();
  this->Position2Coordinate = NULL;
  
  this->InputList->Delete();
  this->InputList = NULL;

  this->TitleMapper->Delete();
  this->TitleMapper = NULL;
  this->TitleActor->Delete();
  this->TitleActor = NULL;

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }
  
  if (this->XTitle)
    {
    delete [] this->XTitle;
    this->XTitle = NULL;
    }
  
  if (this->YTitle)
    {
    delete [] this->YTitle;
    this->YTitle = NULL;
    }
  
  if (this->LabelFormat) 
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }

  this->XAxis->Delete();
  this->YAxis->Delete();
  
  this->PlotData->Delete();
  this->PlotMapper->Delete();
  this->PlotActor->Delete();
}

// Add a dataset to the list of data to plot.
void vtkXYPlotActor::AddInput(vtkDataSet *ds)
{
  if ( ! this->InputList->IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList->AddItem(ds);
    }
}

// Remove a dataset from the list of data to plot.
void vtkXYPlotActor::RemoveInput(vtkDataSet *ds)
{
  if ( this->InputList->IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList->RemoveItem(ds);
    }
}

void vtkXYPlotActor::SetWidth(float w)
{
  float *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(w,pos[1]);
}

void vtkXYPlotActor::SetHeight(float w)
{
  float *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(pos[0],w);
}
    
float vtkXYPlotActor::GetWidth()
{
  return this->Position2Coordinate->GetValue()[0];
}
float vtkXYPlotActor::GetHeight()
{
  return this->Position2Coordinate->GetValue()[1];
}

// Plot scalar data for each input dataset.
int vtkXYPlotActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething=0;

  // Make sure input is up to date.
  if ( this->InputList->GetNumberOfItems() < 1 )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  renderedSomething += this->XAxis->RenderOverlay(viewport);
  renderedSomething += this->YAxis->RenderOverlay(viewport);
  renderedSomething += this->PlotActor->RenderOverlay(viewport);
  if ( this->Title != NULL )
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }

  return renderedSomething;
}

// Plot scalar data for each input dataset.
int vtkXYPlotActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  unsigned long mtime, dsMtime;
  vtkDataSet *ds;
  int numDS, renderedSomething=0;

  // Initialize
  vtkDebugMacro(<<"Plotting data");

  // Make sure input is up to date.
  if ( (numDS=this->InputList->GetNumberOfItems()) < 1 )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  for (mtime=0, this->InputList->InitTraversal(); 
  (ds = this->InputList->GetNextItem()); )
    {
    ds->Update();
    dsMtime = ds->GetMTime();
    if ( dsMtime > mtime )
      {
      mtime = dsMtime;
      }
    }

  // Check modified time to see whether we have to rebuild.
  if ( mtime > this->BuildTime || 
       viewport->GetMTime() > this->BuildTime ||
       this->GetMTime() > this->BuildTime )
    {
    float range[2], xRange[2], yRange[2], interval, *lengths=NULL;
    int pos[2], pos2[2], numTicks, *size=viewport->GetSize();
    int stringWidth, stringHeight;

    vtkDebugMacro(<<"Rebuilding plot");

    this->PlaceAxes(viewport, size, pos, pos2);
    
    // manage title
    if ( this->Title != NULL )
      {
      this->TitleMapper->SetInput(this->Title);
      this->TitleMapper->SetBold(this->Bold);
      this->TitleMapper->SetItalic(this->Italic);
      this->TitleMapper->SetShadow(this->Shadow);
      this->TitleMapper->SetFontFamily(this->FontFamily);
      vtkAxisActor2D::SetFontSize(viewport, this->TitleMapper, size, 1.0,
                                  stringWidth, stringHeight);
      this->TitleActor->GetPositionCoordinate()->SetValue(
        pos[0]+0.5*(pos2[0]-pos[0])-stringWidth/2.0,pos2[1]-stringHeight/2.0);
      this->TitleActor->SetProperty(this->GetProperty());
      }

    // setup x-axis
    lengths = new float[numDS];
    this->ComputeXRange(range, lengths);
    if ( this->XRange[0] < this->XRange[1] )
      {
      range[0] = this->XRange[0];
      range[1] = this->XRange[1];
      }

    vtkAxisActor2D::ComputeRange(range, xRange, this->NumberOfXLabels,
                                 numTicks, interval);
    this->XComputedRange[0] = xRange[0];
    this->XComputedRange[1] = xRange[1];
    
    this->XAxis->SetRange(range);
    this->XAxis->SetTitle(this->XTitle);
    this->XAxis->SetNumberOfLabels(this->NumberOfXLabels);
    this->XAxis->SetBold(this->Bold);
    this->XAxis->SetItalic(this->Italic);
    this->XAxis->SetShadow(this->Shadow);
    this->XAxis->SetFontFamily(this->FontFamily);
    this->XAxis->SetLabelFormat(this->LabelFormat);
    this->XAxis->SetProperty(this->GetProperty());
    
    // setup y-axis
    if ( this->YRange[0] >= this->YRange[1] )
      {
      this->ComputeYRange(range);
      }
    else
      {
      range[0] = this->YRange[0];
      range[1] = this->YRange[1];
      }

    vtkAxisActor2D::ComputeRange(range, yRange, this->NumberOfXLabels,
                                 numTicks, interval);
    this->YComputedRange[0] = yRange[0];
    this->YComputedRange[1] = yRange[1];
    
    this->YAxis->SetRange(range[1], range[0]); //get ticks on "correct" side
    this->YAxis->SetTitle(this->YTitle);
    this->YAxis->SetNumberOfLabels(this->NumberOfYLabels);
    this->YAxis->SetBold(this->Bold);
    this->YAxis->SetItalic(this->Italic);
    this->YAxis->SetShadow(this->Shadow);
    this->YAxis->SetFontFamily(this->FontFamily);
    this->YAxis->SetLabelFormat(this->LabelFormat);
    
    // Okay, now create the plot data
    this->CreatePlotData(pos, pos2, xRange, yRange, lengths);
    delete [] lengths;
    
    this->BuildTime.Modified();
    }//if need to rebuild the plot
  
  renderedSomething += this->XAxis->RenderOpaqueGeometry(viewport);
  renderedSomething += this->YAxis->RenderOpaqueGeometry(viewport);
  renderedSomething += this->PlotActor->RenderOpaqueGeometry(viewport);
  if ( this->Title != NULL )
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}

char *vtkXYPlotActor::GetXValuesAsString()
{
  if ( this->XValues == VTK_XYPLOT_INDEX ) 
    {
    return "Index";
    }
  else if ( this->XValues == VTK_XYPLOT_ARC_LENGTH ) 
    {
    return "ArcLength";
    }
  else 
    {
    return "NormalizedArcLength";
    }
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkXYPlotActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  this->XAxis->ReleaseGraphicsResources(win);
  this->YAxis->ReleaseGraphicsResources(win);
  this->PlotActor->ReleaseGraphicsResources(win);
}

void vtkXYPlotActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

  os << indent << "Position2 Coordinate: " 
     << this->Position2Coordinate << "\n";
  this->Position2Coordinate->PrintSelf(os, indent.GetNextIndent());
  
  os << indent << "Input DataSets:\n";
  this->InputList->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "X Title: " 
     << (this->XTitle ? this->XTitle : "(none)") << "\n";
  os << indent << "Y Title: " 
     << (this->YTitle ? this->YTitle : "(none)") << "\n";
 
  os << indent << "X Values: " << this->GetXValuesAsString() << endl;
  os << indent << "Plot points: " << (this->PlotPoints ? "On\n" : "Off\n");
  os << indent << "Plot lines: " << (this->PlotLines ? "On\n" : "Off\n");

  os << indent << "Number Of X Labels: " << this->NumberOfXLabels << "\n";
  os << indent << "Number Of Y Labels: " << this->NumberOfYLabels << "\n";

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
  os << indent << "Border: " << this->Border << "\n";
  
  os << indent << "X Range: ";
  if ( this->XRange[0] >= this->XRange[1] )
    {
    os << indent << "(Automatically Computed)\n";
    }
  else
    {
    os << "(" << this->XRange[0] << ", " << this->XRange[1] << ")\n";
    }

  os << indent << "Y Range: ";
  if ( this->XRange[0] >= this->YRange[1] )
    {
    os << indent << "(Automatically Computed)\n";
    }
  else
    {
    os << "(" << this->YRange[0] << ", " << this->YRange[1] << ")\n";
    }
}

void vtkXYPlotActor::ComputeXRange(float range[2], float *lengths)
{
  int maxNum, numPts, ptId, dsNum;
  float maxLength=0.0, xPrev[3], x[3];
  vtkDataSet *ds;

  for ( dsNum=0, maxNum=0, this->InputList->InitTraversal(); 
       (ds = this->InputList->GetNextItem()); dsNum++)
    {
    numPts = ds->GetNumberOfPoints();

    if ( this->XValues != VTK_XYPLOT_INDEX )
      {
      ds->GetPoint(0, xPrev);
      for ( lengths[dsNum]=0.0, ptId=1; ptId < numPts; ptId++ )
        {
        ds->GetPoint(ptId, x);
        lengths[dsNum] += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
        xPrev[0] = x[0]; xPrev[1] = x[1]; xPrev[2] = x[2];
        }
      if ( lengths[dsNum] > maxLength )
        {
        maxLength = lengths[dsNum];
        }
      }//if need to compute arc length
    
    else // if VTK_XYPLOT_INDEX
      {
      if ( numPts > maxNum )
        {
        maxNum = numPts;
        }
      }
    }//over all datasets

  // determine the range
  range[0] = 0.0;
  if ( this->XValues == VTK_XYPLOT_ARC_LENGTH )
    {
    range[1] = maxLength;
    }
  
  else if ( this->XValues == VTK_XYPLOT_NORMALIZED_ARC_LENGTH )
    {
    range[1] = 1.0;
    }
  
  else //if ( this->XValues == VTK_XYPLOT_INDEX )
    {
    range[1] = (float)(maxNum - 1);
    }
}

void vtkXYPlotActor::ComputeYRange(float range[2])
{
  vtkDataSet *ds;
  vtkScalars *scalars;
  float sRange[2];

  range[0]=VTK_LARGE_FLOAT, range[1]=(-VTK_LARGE_FLOAT);

  for ( this->InputList->InitTraversal(); 
       (ds = this->InputList->GetNextItem()); )
    {
    scalars = ds->GetPointData()->GetScalars();
    if ( !scalars)
      {
      vtkErrorMacro(<<"No scalar data to plot!");
      continue;
      }
    
    scalars->GetRange(sRange);
    if ( sRange[0] < range[0] )
      {
      range[0] = sRange[0];
      }

    if ( sRange[1] > range[1] )
      {
      range[1] = sRange[1];
      }

    }//over all datasets
}

void vtkXYPlotActor::CreatePlotData(int *pos, int *pos2, float xRange[2], 
                                    float yRange[2], float *lengths)
{
  float xyz[3]; xyz[2] = 0.0;
  int id, numPts, numLinePts, ptId, dsNum;
  float length, x[3], xPrev[3];
  vtkScalars *scalars;
  vtkDataSet *ds;

  this->PlotActor->SetProperty(this->GetProperty());
  this->PlotData->Initialize();

  if (!this->PlotPoints && !this->PlotLines)
    {
    return;
    }

  vtkCellArray *lines=vtkCellArray::New();
  vtkPoints *pts=vtkPoints::New();

  lines->Allocate(10,10);
  pts->Allocate(10,10);
  this->PlotData->SetPoints(pts);

  // Should the lines be rendered?
  if (this->PlotLines)
    {
    this->PlotData->SetLines(lines);  
    }

  // Should the points be rendered?
  if (this->PlotPoints)
    {
    this->PlotData->SetVerts(lines);   // use the lines as verts
    }
  
  for ( dsNum=0, this->InputList->InitTraversal(); 
       (ds = this->InputList->GetNextItem()); dsNum++ )
    {
    numPts = ds->GetNumberOfPoints();
    scalars = ds->GetPointData()->GetScalars();
    if ( !scalars)
      {
      continue;
      }

    lines->InsertNextCell(0); //update the count later

    ds->GetPoint(0, xPrev);
    for ( numLinePts=0, length=0.0, ptId=0; ptId < numPts; ptId++ )
      {
      xyz[1] = scalars->GetScalar(ptId);
      ds->GetPoint(ptId, x);
      if ( this->XValues == VTK_XYPLOT_NORMALIZED_ARC_LENGTH )
        {
        length += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
        xyz[0] = length / lengths[dsNum];
        xPrev[0] = x[0]; xPrev[1] = x[1]; xPrev[2] = x[2];
        }

      else if ( this->XValues == VTK_XYPLOT_INDEX )
        {
        xyz[0] = (float)ptId;
        }

      else //if ( this->XValues == VTK_XYPLOT_ARC_LENGTH )
        {
        length += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
        xyz[0] = length;
        xPrev[0] = x[0]; xPrev[1] = x[1]; xPrev[2] = x[2];
	}


      // normalize and position
      if ( xyz[0] >= xRange[0] && xyz[0] <= xRange[1] &&
           xyz[1] >= yRange[0] && xyz[1] <= yRange[1] )
        {
        numLinePts++;
        xyz[0] = pos[0] + 
          (xyz[0]-xRange[0])/(xRange[1]-xRange[0])*(pos2[0]-pos[0]);
        xyz[1] = pos[1] + 
          (xyz[1]-yRange[0])/(yRange[1]-yRange[0])*(pos2[1]-pos[1]);
        id = pts->InsertNextPoint(xyz);
        lines->InsertCellPoint(id);
        }
      }//for all input points

    lines->UpdateCellCount(numLinePts);
    }//loop over all input data sets

  lines->Delete();
  pts->Delete();
}

// Position the axes taking into account the expected padding due to labels
// and titles. We want the result to fit in the box specified. This method
// knows something about how the vtkAxisActor2D functions, so it may have 
// to change if that class changes dramatically.
//
void vtkXYPlotActor::PlaceAxes(vtkViewport *viewport, int *size,
                               int pos[2], int pos2[2])
{
  int titleWidth, titleHeight, labelWidth, labelHeight;
  float labelFactor=this->XAxis->GetLabelFactor();
  float tickOffset=this->XAxis->GetTickOffset();
  float tickLength=this->XAxis->GetTickLength();
  char string[512];

  // Create a dummy text mapper for getting font sizes
  vtkTextMapper *textMapper = vtkTextMapper::New();
  textMapper->SetItalic(this->Italic);
  textMapper->SetBold(this->Bold);
  textMapper->SetShadow(this->Shadow);
  textMapper->SetFontFamily(this->FontFamily);

  // Get the location of the corners of the box
  int *p1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
  int *p2 = this->Position2Coordinate->GetComputedViewportValue(viewport);

  // Estimate the padding around the x and y axes
  textMapper->SetInput(this->YTitle);
  vtkAxisActor2D::SetFontSize(viewport, textMapper, size, 1.0,
                              titleWidth, titleHeight);
  sprintf(string, this->LabelFormat, 0.0);
  textMapper->SetInput(string);
  vtkAxisActor2D::SetFontSize(viewport, textMapper, size, labelFactor,
                              labelWidth, labelHeight);
  
  // Okay, estimate the size
  pos[0] = (int)(p1[0] + titleWidth + tickOffset + tickLength + labelWidth +
		 this->Border);
  pos2[0] = (int)(p2[0] - labelWidth/2 - tickOffset - this->Border);
  pos[1] = (int)(p1[1] + titleHeight + tickOffset + tickLength + labelHeight +
		 this->Border);
  pos2[1] = (int)(p2[1] - labelHeight/2 - tickOffset - this->Border);

  // Now specify the location of the axes
  this->XAxis->GetPoint1Coordinate()->SetValue(pos[0], pos[1]);
  this->XAxis->GetPoint2Coordinate()->SetValue(pos2[0], pos[1]);
  this->YAxis->GetPoint1Coordinate()->SetValue(pos[0], pos2[1]);
  this->YAxis->GetPoint2Coordinate()->SetValue(pos[0], pos[1]);
  
  textMapper->Delete();
}

void vtkXYPlotActor::ViewportToPlotCoordinate(vtkViewport *viewport, float &u, float &v)
{
  int *p0, *p1, *p2;

  // XAxis, YAxis are in viewport coordinates already
  p0 = this->XAxis->GetPoint1Coordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPoint2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPoint1Coordinate()->GetComputedViewportValue(viewport);

  u = ((u - p0[0]) / (float)(p1[0] - p0[0]))
    *(this->XComputedRange[1] - this->XComputedRange[0])
    + this->XComputedRange[0];
  v = ((v - p0[1]) / (float)(p2[1] - p0[1]))
    *(this->YComputedRange[1] - this->YComputedRange[0])
    + this->YComputedRange[0];
}

void vtkXYPlotActor::PlotToViewportCoordinate(vtkViewport *viewport,
					      float &u, float &v)
{
  int *p0, *p1, *p2;

  // XAxis, YAxis are in viewport coordinates already
  p0 = this->XAxis->GetPoint1Coordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPoint2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPoint1Coordinate()->GetComputedViewportValue(viewport);

  u = (((u - this->XComputedRange[0])
	/ (this->XComputedRange[1] - this->XComputedRange[0]))
       * (float)(p1[0] - p0[0])) + p0[0];
  v = (((v - this->YComputedRange[0])
	/ (this->YComputedRange[1] - this->YComputedRange[0]))
       * (float)(p2[1] - p0[1])) + p0[1];
}

int vtkXYPlotActor::IsInPlot(vtkViewport *viewport, float u, float v)
{
  int *p0, *p1, *p2;

  // Bounds of the plot are based on the axes...
  p0 = this->XAxis->GetPoint1Coordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPoint2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPoint1Coordinate()->GetComputedViewportValue(viewport);
  
  if (u >= p0[0] && u <= p1[0] && v >= p0[1] && v <= p2[1])
    {
    return 1;
    }

  return 0;
}
