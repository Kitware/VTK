/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXYPlotActor.cxx
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
#include "vtkXYPlotActor.h"
#include "vtkDataSetCollection.h"
#include "vtkDataObjectCollection.h"
#include "vtkMath.h"
#include "vtkGlyph2D.h"
#include "vtkAppendPolyData.h"
#include "vtkGlyphSource2D.h"
#include "vtkLegendBoxActor.h"
#include "vtkPlanes.h"
#include "vtkPlane.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"

#define VTK_MAX_PLOTS 50

//--------------------------------------------------------------------------
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
  this->Position2Coordinate->SetValue(0.5, 0.5);

  this->InputList = vtkDataSetCollection::New();
  this->DataObjectInputList = vtkDataObjectCollection::New();

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
  this->Logx = 0;
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
  this->PlotCurveLines = 0;
  this->PlotCurvePoints = 0;
  this->ExchangeAxes = 0;
  this->ReverseXAxis = 0;
  this->ReverseYAxis = 0;

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
  
  this->NumberOfInputs = 0;
  this->PlotData = NULL;
  this->PlotGlyph = NULL;
  this->PlotAppend = NULL;
  this->PlotMapper = NULL;
  this->PlotActor = NULL;

  this->ViewportCoordinate[0] = 0.0;
  this->ViewportCoordinate[1] = 0.0;
  this->PlotCoordinate[0] = 0.0;
  this->PlotCoordinate[1] = 0.0;

  this->DataObjectPlotMode = VTK_XYPLOT_COLUMN;
  this->XComponent = vtkIntArray::New();
  this->XComponent->SetNumberOfValues(VTK_MAX_PLOTS);
  this->YComponent = vtkIntArray::New();
  this->YComponent->SetNumberOfValues(VTK_MAX_PLOTS);

  this->LinesOn = vtkIntArray::New();
  this->LinesOn->SetNumberOfValues(VTK_MAX_PLOTS);
  this->PointsOn = vtkIntArray::New();
  this->PointsOn->SetNumberOfValues(VTK_MAX_PLOTS);
  for (int i=0; i<VTK_MAX_PLOTS; i++)
    {
      this->XComponent->SetValue(i,0);
      this->YComponent->SetValue(i,0);
      this->LinesOn->SetValue(i,this->PlotLines);
      this->PointsOn->SetValue(i,this->PlotPoints);
    }

  this->Legend = 0;
  this->LegendPosition[0] = 0.85;
  this->LegendPosition[1] = 0.75;
  this->LegendPosition2[0] = 0.15;
  this->LegendPosition2[1] = 0.20;
  this->LegendActor = vtkLegendBoxActor::New();
  this->LegendActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->LegendActor->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->LegendActor->GetPosition2Coordinate()->SetReferenceCoordinate(NULL);
  this->LegendActor->BorderOff();
  this->LegendActor->SetNumberOfEntries(VTK_MAX_PLOTS); //initial allocation
  this->GlyphSource = vtkGlyphSource2D::New();
  this->GlyphSource->SetGlyphTypeToNone();
  this->GlyphSource->DashOn();
  this->GlyphSource->FilledOff();
  this->GlyphSize = 0.020;

  this->ClipPlanes = vtkPlanes::New();
  vtkPoints *pts = vtkPoints::New();
  pts->SetNumberOfPoints(4);
  this->ClipPlanes->SetPoints(pts);
  pts->Delete();
  vtkNormals *n = vtkNormals::New();
  n->SetNumberOfNormals(4);
  this->ClipPlanes->SetNormals(n);
  n->Delete();

  this->CachedSize[0] = 0;
  this->CachedSize[1] = 0;
}

vtkXYPlotActor::~vtkXYPlotActor()
{
  this->InputList->Delete();
  this->InputList = NULL;

  this->DataObjectInputList->Delete();
  this->DataObjectInputList = NULL;

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
  
  this->InitializeEntries();

  this->LegendActor->Delete();
  this->GlyphSource->Delete();
  this->ClipPlanes->Delete();
  
  this->XComponent->Delete();
  this->YComponent->Delete();

  this->LinesOn->Delete();
  this->PointsOn->Delete();
}

void vtkXYPlotActor::InitializeEntries()
{
  if ( this->NumberOfInputs > 0 )
    {
      for (int i=0; i<this->NumberOfInputs; i++)
	{
	  this->PlotData[i]->Delete();
	  this->PlotGlyph[i]->Delete();
	  this->PlotAppend[i]->Delete();
	  this->PlotMapper[i]->Delete();
	  this->PlotActor[i]->Delete();
	}//for all entries
      delete [] this->PlotData; this->PlotData = NULL;
      delete [] this->PlotGlyph; this->PlotGlyph = NULL;
      delete [] this->PlotAppend; this->PlotAppend = NULL;
      delete [] this->PlotMapper; this->PlotMapper = NULL;
      delete [] this->PlotActor; this->PlotActor = NULL;
      this->NumberOfInputs = 0;
    }//if entries have been defined
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

// Add a data object to the list of data to plot.
void vtkXYPlotActor::AddDataObjectInput(vtkDataObject *in)
{
  if ( ! this->DataObjectInputList->IsItemPresent(in) )
    {
      this->Modified();
      this->DataObjectInputList->AddItem(in);
    }
}

// Remove a data object from the list of data to plot.
void vtkXYPlotActor::RemoveDataObjectInput(vtkDataObject *in)
{
  if ( this->DataObjectInputList->IsItemPresent(in) )
    {
      this->Modified();
      this->DataObjectInputList->RemoveItem(in);
    }
}

// Plot scalar data for each input dataset.
int vtkXYPlotActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething=0;

  // Make sure input is up to date.
  if ( this->InputList->GetNumberOfItems() < 1 && 
       this->DataObjectInputList->GetNumberOfItems() < 1 )
    {
      vtkErrorMacro(<< "Nothing to plot!");
      return 0;
    }

  renderedSomething += this->XAxis->RenderOverlay(viewport);
  renderedSomething += this->YAxis->RenderOverlay(viewport);
  if ( this->Title != NULL )
    {
      renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }
  for (int i=0; i < this->NumberOfInputs; i++)
    {
      renderedSomething += this->PlotActor[i]->RenderOverlay(viewport);
    }
  if ( this->Legend )
    {
      renderedSomething += this->LegendActor->RenderOverlay(viewport);
    }

  return renderedSomething;
}

// Plot scalar data for each input dataset.
int vtkXYPlotActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  unsigned long mtime, dsMtime;
  vtkDataSet *ds;
  vtkDataObject *dobj;
  int numDS, numDO, renderedSomething=0;

  // Initialize
  // Make sure input is up to date.
  numDS = this->InputList->GetNumberOfItems();
  numDO = this->DataObjectInputList->GetNumberOfItems();
  if ( numDS > 0 )
    {
      vtkDebugMacro(<<"Plotting input data sets");
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
    }
  else if ( numDO > 0 )
    {
      vtkDebugMacro(<<"Plotting input data objects");
      for (mtime=0, this->DataObjectInputList->InitTraversal(); 
	   (dobj = this->DataObjectInputList->GetNextItem()); )
	{
	  dobj->Update();
	  dsMtime = dobj->GetMTime();
	  if ( dsMtime > mtime )
	    {
	      mtime = dsMtime;
	    }
	}
    }
  else
    {
      vtkErrorMacro(<< "Nothing to plot!");
      return 0;
    }

  // Check modified time to see whether we have to rebuild.
  int *size=viewport->GetSize();
  if ( mtime > this->BuildTime || 
       size[0] != this->CachedSize[0] || size[1] != this->CachedSize[1] ||
       this->GetMTime() > this->BuildTime )
    {
      float range[2], yrange[2], xRange[2], yRange[2], interval, *lengths=NULL;
      int pos[2], pos2[2], numTicks;
      int stringWidth, stringHeight;
      int num = ( numDS > 0 ? numDS : numDO );

      vtkDebugMacro(<<"Rebuilding plot");
      this->CachedSize[0] = size[0];
      this->CachedSize[1] = size[1];

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
	  this->TitleActor->GetPositionCoordinate()->SetValue(pos[0]+
							      0.5*(pos2[0]-pos[0])-
							      stringWidth/2.0,pos2[1]-
							      stringHeight/2.0);
	  this->TitleActor->SetProperty(this->GetProperty());
	}

      // manage legend
      vtkDebugMacro(<<"Rebuilding legend");
      if ( this->Legend )
	{
	  int legPos[2], legPos2[2];
	  int *p1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
	  int *p2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
	  legPos[0] = (int)(p1[0] + this->LegendPosition[0]*(p2[0]-p1[0]));
	  legPos2[0] = (int)(legPos[0] + this->LegendPosition2[0]*(p2[0]-p1[0]));
	  legPos[1] = (int)(p1[1] + this->LegendPosition[1]*(p2[1]-p1[1]));
	  legPos2[1] = (int)(legPos[1] + this->LegendPosition2[1]*(p2[1]-p1[1]));
      
	  this->LegendActor->GetPositionCoordinate()->SetValue(legPos[0], legPos[1]);
	  this->LegendActor->GetPosition2Coordinate()->SetValue(legPos2[0], legPos2[1]);
	  this->LegendActor->SetNumberOfEntries(num);
	  for (int i=0; i<num; i++)
	    {
	      if ( ! this->LegendActor->GetEntrySymbol(i) )
		{
		  this->LegendActor->SetEntrySymbol(i,this->GlyphSource->GetOutput());
		}
	      if ( ! this->LegendActor->GetEntryString(i) )
		{
		  static char legendString[12];
		  sprintf(legendString, "%s%d", "Curve ", i);
		  this->LegendActor->SetEntryString(i,legendString);
		}
	    }

	  this->LegendActor->SetPadding(2);
	  this->LegendActor->GetProperty()->DeepCopy(this->GetProperty());
	  this->LegendActor->ScalarVisibilityOff();
	}

      // setup x-axis
      vtkDebugMacro(<<"Rebuilding x-axis");

      this->XAxis->SetTitle(this->XTitle);
      this->XAxis->SetNumberOfLabels(this->NumberOfXLabels);
      this->XAxis->SetBold(this->Bold);
      this->XAxis->SetItalic(this->Italic);
      this->XAxis->SetShadow(this->Shadow);
      this->XAxis->SetFontFamily(this->FontFamily);
      this->XAxis->SetLabelFormat(this->LabelFormat);
      this->XAxis->SetProperty(this->GetProperty());

      lengths = new float[num];
      if ( numDS > 0 ) //plotting data sets
	{
	  this->ComputeXRange(range, lengths);
	}
      else
	{
	  this->ComputeDORange(range, yrange, lengths);
	}
      if ( this->XRange[0] < this->XRange[1] )
	{
	  range[0] = this->XRange[0];
	  range[1] = this->XRange[1];
	}

      vtkAxisActor2D::ComputeRange(range, xRange, this->NumberOfXLabels,
				   numTicks, interval);
    if ( !this->ExchangeAxes )
      {
	this->XComputedRange[0] = xRange[0];
	this->XComputedRange[1] = xRange[1];
	if ( this->ReverseXAxis )
	  {
	    this->XAxis->SetRange(range[1],range[0]);
	  }
	else
	  {
	    this->XAxis->SetRange(range[0],range[1]);
	  }
      }
    else
      {
	this->XComputedRange[1] = xRange[0];
	this->XComputedRange[0] = xRange[1];
	if ( this->ReverseYAxis )
	  {
	    this->XAxis->SetRange(range[0],range[1]);
	  }
	else
	  {
	    this->XAxis->SetRange(range[1],range[0]);
	  }
      }
    
      this->XAxis->Modified();

      // setup y-axis
      vtkDebugMacro(<<"Rebuilding y-axis");
      this->YAxis->SetTitle(this->YTitle);
      this->YAxis->SetNumberOfLabels(this->NumberOfYLabels);
      this->YAxis->SetBold(this->Bold);
      this->YAxis->SetItalic(this->Italic);
      this->YAxis->SetShadow(this->Shadow);
      this->YAxis->SetFontFamily(this->FontFamily);
      this->YAxis->SetLabelFormat(this->LabelFormat);

      if ( this->YRange[0] >= this->YRange[1] )
	{
	  if ( numDS > 0 ) //plotting data sets
	    {
	      this->ComputeYRange(yrange);
	    }
	}
      else
	{
	  yrange[0] = this->YRange[0];
	  yrange[1] = this->YRange[1];
	}
      vtkAxisActor2D::ComputeRange(yrange, yRange, this->NumberOfYLabels,
				   numTicks, interval);

      if ( !this->ExchangeAxes )
	{
	  this->YComputedRange[0] = yRange[0];
	  this->YComputedRange[1] = yRange[1];
	  if ( this->ReverseYAxis )
	    {
	      this->YAxis->SetRange(yrange[0],yrange[1]);
	    }
	  else
	    {
	      this->YAxis->SetRange(yrange[1],range[0]);
	    }
	}
      else
	{
	  this->YComputedRange[1] = yRange[0];
	  this->YComputedRange[0] = yRange[1];
	  if ( this->ReverseXAxis )
	    {
	      this->YAxis->SetRange(yrange[1],yrange[0]);
	    }
	  else
	    {
	      this->YAxis->SetRange(yrange[0],yrange[1]);
	    }
	}

      this->YAxis->Modified();
    
      vtkDebugMacro(<<"Creating Plot Data");
      // Okay, now create the plot data and set up the pipeline
      this->CreatePlotData(pos, pos2, xRange, yRange, lengths, numDS, numDO);
      delete [] lengths;
    
      this->BuildTime.Modified();
    }//if need to rebuild the plot

  vtkDebugMacro(<<"Rendering Axes");
  renderedSomething += this->XAxis->RenderOpaqueGeometry(viewport);
  renderedSomething += this->YAxis->RenderOpaqueGeometry(viewport);
  for (int i=0; i < this->NumberOfInputs; i++)
    {
      vtkDebugMacro(<<"Rendering plotactors");
      renderedSomething += this->PlotActor[i]->RenderOpaqueGeometry(viewport);
    }
  if ( this->Title != NULL )
    {
      vtkDebugMacro(<<"Rendering titleactors");
      renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }
  if ( this->Legend )
    {
      vtkDebugMacro(<<"Rendering legendeactors");
      renderedSomething += this->LegendActor->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}

const char *vtkXYPlotActor::GetXValuesAsString()
{
  switch (this->XValues)
    {
    case VTK_XYPLOT_INDEX:
      return "Index";
    case VTK_XYPLOT_ARC_LENGTH:
      return "ArcLength";
    case VTK_XYPLOT_NORMALIZED_ARC_LENGTH:
      return "NormalizedArcLength";
    default:
      return "Value";
    }
}

const char *vtkXYPlotActor::GetDataObjectPlotModeAsString()
{
  if ( this->XValues == VTK_XYPLOT_ROW ) 
    {
      return "Plot Rows";
    }
  else 
    {
      return "Plot Columns";
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
  for (int i=0; i < this->NumberOfInputs; i++)
    {
      this->PlotActor[i]->ReleaseGraphicsResources(win);
    }
  this->LegendActor->ReleaseGraphicsResources(win);
}

unsigned long vtkXYPlotActor::GetMTime()
{
  unsigned long mtime=this->LegendActor->GetMTime();
  unsigned long mtime2=this->vtkActor2D::GetMTime();
  return (mtime2 > mtime ? mtime2 : mtime);
}

void vtkXYPlotActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Input DataObjects:\n";
  this->DataObjectInputList->PrintSelf(os,indent.GetNextIndent());
  
  os << indent << "Data Object Plot Mode: " << this->GetDataObjectPlotModeAsString() << endl;

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "X Title: " 
     << (this->XTitle ? this->XTitle : "(none)") << "\n";
  os << indent << "Y Title: " 
     << (this->YTitle ? this->YTitle : "(none)") << "\n";
 
  os << indent << "X Values: " << this->GetXValuesAsString() << endl;
  os << indent << "Log X Values: " << (this->Logx ? "On\n" : "Off\n");

  os << indent << "Plot global-points: " << (this->PlotPoints ? "On\n" : "Off\n");
  os << indent << "Plot global-lines: " << (this->PlotLines ? "On\n" : "Off\n");
  os << indent << "Plot per-curve points: " << (this->PlotCurvePoints ? "On\n" : "Off\n");
  os << indent << "Plot per-curve lines: " << (this->PlotCurveLines ? "On\n" : "Off\n");
  os << indent << "Exchange Axes: " << (this->ExchangeAxes ? "On\n" : "Off\n");
  os << indent << "Reverse X Axis: " << (this->ReverseXAxis ? "On\n" : "Off\n");
  os << indent << "Reverse Y Axis: " << (this->ReverseYAxis ? "On\n" : "Off\n");

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

  os << indent << "Viewport Coordinate: ("
     << this->ViewportCoordinate[0] << ", " 
     << this->ViewportCoordinate[1] << ")\n";

  os << indent << "Plot Coordinate: ("
     << this->PlotCoordinate[0] << ", " 
     << this->PlotCoordinate[1] << ")\n";

  os << indent << "Legend: " << (this->Legend ? "On\n" : "Off\n");
  os << indent << "Legend Position: ("
     << this->LegendPosition[0] << ", " 
     << this->LegendPosition[1] << ")\n";
  os << indent << "Legend Position2: ("
     << this->LegendPosition2[0] << ", " 
     << this->LegendPosition2[1] << ")\n";

  os << indent << "Glyph Size: " << this->GlyphSize << endl;
}

void vtkXYPlotActor::ComputeXRange(float range[2], float *lengths)
{
  int dsNum;
  vtkIdType numPts, ptId, maxNum;
  float maxLength=0.0, xPrev[3], *x;
  vtkDataSet *ds;

  range[0] = VTK_LARGE_FLOAT;
  range[1] = -VTK_LARGE_FLOAT;
  for ( dsNum=0, maxNum=0, this->InputList->InitTraversal(); 
	(ds = this->InputList->GetNextItem()); dsNum++)
    {
      numPts = ds->GetNumberOfPoints();

      if ( this->XValues != VTK_XYPLOT_INDEX )
	{
	  ds->GetPoint(0, xPrev);
	  for ( lengths[dsNum]=0.0, ptId=0; ptId < numPts; ptId++ )
	    {
	      x = ds->GetPoint(ptId);
	      switch (this->XValues)
		{
		case VTK_XYPLOT_VALUE:
		  if (this->GetLogx() == 0)
		    {
		      if ( x[this->XComponent->GetValue(dsNum)] < range[0] )
			{
			  range[0] = x[this->XComponent->GetValue(dsNum)];
			}
		      if ( x[this->XComponent->GetValue(dsNum)] > range[1] )
			{
			  range[1] = x[this->XComponent->GetValue(dsNum)];
			}
		    }
		  else
		    {
		      //ensure range strictly > 0 for log
		      if ( (x[this->XComponent->GetValue(dsNum)]) < range[0] && 
			   (x[this->XComponent->GetValue(dsNum)] > 0))
			{
			  range[0] = x[this->XComponent->GetValue(dsNum)];
			}
		      if ( (x[this->XComponent->GetValue(dsNum)] > range[1]) && 
			   (x[this->XComponent->GetValue(dsNum)] > 0))
			{
			  range[1] = x[this->XComponent->GetValue(dsNum)];
			}
		    }
		  break;
		default:
		  lengths[dsNum] += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
		  xPrev[0] = x[0]; xPrev[1] = x[1]; xPrev[2] = x[2];
		}
	    }//for all points
	  if ( lengths[dsNum] > maxLength )
	    {
	      maxLength = lengths[dsNum];
	    }
	}//if need to visit all points
    
      else //if ( this->XValues == VTK_XYPLOT_INDEX )
	{
	  if ( numPts > maxNum )
	    {
	      maxNum = numPts;
	    }
	}
    }//over all datasets

  // determine the range
  switch (this->XValues)
    {
    case VTK_XYPLOT_ARC_LENGTH:
      range[0] = 0.0;
      range[1] = maxLength;
      break;
    case VTK_XYPLOT_NORMALIZED_ARC_LENGTH:
      range[0] = 0.0;
      range[1] = 1.0;
      break;
    case VTK_XYPLOT_INDEX:
      range[0] = 0.0;
      range[1] = (float)(maxNum - 1);
      break;
    case VTK_XYPLOT_VALUE:
      if (this->GetLogx() == 1)
	{
	  if (range[0] > range[1]) 
	    {
	      range[0] = 0;
	      range[1] = 0;
	    }
	  else
	    {
	      range[0] = log10(range[0]);
	      range[1] = log10(range[1]);
	    }
	}
      break; //range computed in for loop above
    default:
      vtkErrorMacro(<< "Unkown X-Value option.");
      return;
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

void vtkXYPlotActor::ComputeDORange(float xrange[2], float yrange[2], 
                                    float *lengths)
{
  int i;
  vtkDataObject *dobj;
  vtkFieldData *field;
  int doNum, numColumns;
  vtkIdType numTuples, numRows, num, ptId, maxNum;
  float maxLength=0.0, x, y, xPrev = 0.0;
  vtkDataArray *array;

  xrange[0] = yrange[0] = VTK_LARGE_FLOAT;
  xrange[1] = yrange[1] = -VTK_LARGE_FLOAT;
  for ( doNum=0, maxNum=0, this->DataObjectInputList->InitTraversal(); 
	(dobj = this->DataObjectInputList->GetNextItem()); doNum++)
    {
      field = dobj->GetFieldData();
      numColumns = field->GetNumberOfComponents(); //number of "columns"
      for (numRows = VTK_LARGE_INTEGER, i=0; i<field->GetNumberOfArrays(); i++)
	{
	  array = field->GetArray(i);
	  numTuples = array->GetNumberOfTuples();
	  if ( numTuples < numRows )
	    {
	      numRows = numTuples;
	    }
	}

      num = (this->DataObjectPlotMode == VTK_XYPLOT_ROW ? 
	     numColumns : numRows);

      if ( this->XValues != VTK_XYPLOT_INDEX )
	{
	  // gather the information to form a plot
	  for ( ptId=0; ptId < num; ptId++ )
	    {
	      if ( this->DataObjectPlotMode == VTK_XYPLOT_ROW )
		{
		  x = field->GetComponent(this->XComponent->GetValue(doNum), ptId);
		}
	      else //if ( this->DataObjectPlotMode == VTK_XYPLOT_COLUMN )
		{
		  x = field->GetComponent(ptId, this->XComponent->GetValue(doNum));
		}
	      if ( ptId == 0 )
		{
		  xPrev = x;
		}
	      
	      switch (this->XValues)
		{
		case VTK_XYPLOT_VALUE:
		  if (this->GetLogx() == 0)
		    {
		      if ( x < xrange[0] )
			{
			  xrange[0] = x;
			}
		      if ( x > xrange[1] )
			{
			  xrange[1] = x;
			}
		    }
		  else //ensure positive values
		    {
		      if ( (x < xrange[0]) && (x > 0) )
			{
			  xrange[0] = x;
			}
		      if ( x > xrange[1]  && (x > 0) )
			{
			  xrange[1] = x;
			}
		    }
		  break;
		default:
		  lengths[doNum] += fabs(x-xPrev);
		  xPrev = x;
		}
	    }//for all points
	  if ( lengths[doNum] > maxLength )
	    {
	      maxLength = lengths[doNum];
	    }
	}//if all data has to be visited
    
      else //if (this->XValues == VTK_XYPLOT_INDEX)
	{
	  if ( num > maxNum )
	    {
	      maxNum = num;
	    }
	}

      // Get the y-values
      for ( ptId=0; ptId < num; ptId++ )
	{
	  if ( this->DataObjectPlotMode == VTK_XYPLOT_ROW )
	    {
	      y = field->GetComponent(this->YComponent->GetValue(doNum), ptId);
	    }
	  else //if ( this->DataObjectPlotMode == VTK_XYPLOT_COLUMN )
	    {
	      y = field->GetComponent(ptId, this->YComponent->GetValue(doNum));
	    }
	  if ( y < yrange[0] )
	    {
	      yrange[0] = y;
	    }
	  if ( y > yrange[1] )
	    {
	      yrange[1] = y;
	    }
	}//over all y values
    }//over all dataobjects

  // determine the range
  switch (this->XValues)
    {
    case VTK_XYPLOT_ARC_LENGTH:
      xrange[0] = 0.0;
      xrange[1] = maxLength;
      break;
    case VTK_XYPLOT_NORMALIZED_ARC_LENGTH:
      xrange[0] = 0.0;
      xrange[1] = 1.0;
      break;
    case VTK_XYPLOT_INDEX:
      xrange[0] = 0.0;
      xrange[1] = (float)(maxNum - 1);
      break;
    case VTK_XYPLOT_VALUE:
      if (this->GetLogx() == 1)
	{
	  xrange[0] = log10(xrange[0]);
	  xrange[1] = log10(xrange[1]);
	}
      break;
    default:
      vtkErrorMacro(<< "Unknown X-Value option");
      return;
    }
}

void vtkXYPlotActor::CreatePlotData(int *pos, int *pos2, float xRange[2], 
                                    float yRange[2], float *lengths,
                                    int numDS, int numDO)
{
  float xyz[3]; xyz[2] = 0.0;
  int i, numLinePts, dsNum, doNum, num;
  vtkIdType numPts, ptId, id;
  float length, x[3], xPrev[3];
  vtkScalars *scalars;
  vtkDataSet *ds;
  vtkCellArray *lines;
  vtkPoints *pts;
  int clippingRequired = 0;

  // Allocate resources for the polygonal plots
  //
  num = (numDS > numDO ? numDS : numDO);
  this->InitializeEntries();
  this->NumberOfInputs = num;
  this->PlotData = new vtkPolyData* [num];
  this->PlotGlyph = new vtkGlyph2D* [num];
  this->PlotAppend = new vtkAppendPolyData* [num];
  this->PlotMapper = new vtkPolyDataMapper2D* [num];
  this->PlotActor = new vtkActor2D* [num];
  for (i=0; i<num; i++)
    {
      this->PlotData[i] = vtkPolyData::New();
      this->PlotGlyph[i] = vtkGlyph2D::New();
      this->PlotGlyph[i]->SetInput(this->PlotData[i]);
      this->PlotGlyph[i]->SetScaleModeToDataScalingOff();
      this->PlotAppend[i] = vtkAppendPolyData::New();
      this->PlotAppend[i]->AddInput(this->PlotData[i]);
      if ( this->LegendActor->GetEntrySymbol(i) != NULL &&
	   this->LegendActor->GetEntrySymbol(i) != this->GlyphSource->GetOutput() )
	{
	  this->PlotGlyph[i]->SetSource(this->LegendActor->GetEntrySymbol(i));
	  this->PlotGlyph[i]->SetScaleFactor(this->ComputeGlyphScale(i,pos,pos2));
	  this->PlotAppend[i]->AddInput(this->PlotGlyph[i]->GetOutput());
	}
      this->PlotMapper[i] = vtkPolyDataMapper2D::New();
      this->PlotMapper[i]->SetInput(this->PlotAppend[i]->GetOutput());
      this->PlotMapper[i]->ScalarVisibilityOff();
      this->PlotActor[i] = vtkActor2D::New();
      this->PlotActor[i]->SetMapper(this->PlotMapper[i]);
      this->PlotActor[i]->GetProperty()->DeepCopy(this->GetProperty());
      if ( this->LegendActor->GetEntryColor(i)[0] < 0.0 )
	{
	  this->PlotActor[i]->GetProperty()->SetColor(
						      this->GetProperty()->GetColor());
	}
      else
	{
	  this->PlotActor[i]->GetProperty()->SetColor(
						      this->LegendActor->GetEntryColor(i));
	}
    }

  // Prepare to receive data
  this->GenerateClipPlanes(pos,pos2);
  for (i=0; i<this->NumberOfInputs; i++)
    {
      lines = vtkCellArray::New();
      pts = vtkPoints::New();

      lines->Allocate(10,10);
      pts->Allocate(10,10);
      this->PlotData[i]->SetPoints(pts);
      this->PlotData[i]->SetVerts(lines);
      this->PlotData[i]->SetLines(lines);

      pts->Delete();
      lines->Delete();
    }
   
  // Okay, for each input generate plot data. Depending on the input
  // we use either dataset or data object.
  //
  if ( numDS > 0 )
    {
      for ( dsNum=0, this->InputList->InitTraversal(); 
	    (ds = this->InputList->GetNextItem()); dsNum++ )
	{
	  clippingRequired = 0;
	  numPts = ds->GetNumberOfPoints();
	  scalars = ds->GetPointData()->GetScalars();
	  if ( !scalars)
	    {
	      continue;
	    }

	  pts = this->PlotData[dsNum]->GetPoints();
	  lines = this->PlotData[dsNum]->GetLines();
	  lines->InsertNextCell(0); //update the count later

	  ds->GetPoint(0, xPrev);
	  for ( numLinePts=0, length=0.0, ptId=0; ptId < numPts; ptId++ )
	    {
	      xyz[1] = scalars->GetScalar(ptId);
	      ds->GetPoint(ptId, x);
	      switch (this->XValues)
		{
		case VTK_XYPLOT_NORMALIZED_ARC_LENGTH:
		  length += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
		  xyz[0] = length / lengths[dsNum];
		  xPrev[0] = x[0]; xPrev[1] = x[1]; xPrev[2] = x[2];
		  break;
		case VTK_XYPLOT_INDEX:
		  xyz[0] = (float)ptId;
		  break;
		case VTK_XYPLOT_ARC_LENGTH:
		  length += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
		  xyz[0] = length;
		  xPrev[0] = x[0]; xPrev[1] = x[1]; xPrev[2] = x[2];
		  break;
		case VTK_XYPLOT_VALUE:
		  xyz[0] = x[this->XComponent->GetValue(dsNum)];
		  break;
		default:
		  vtkErrorMacro(<< "Unknown X-Component option");
		}
        
	      if ( this->GetLogx() == 1 )
		{
		  if (xyz[0] > 0)
		    {
		      xyz[0] = log10(xyz[0]);
		      // normalize and position
		      if ( xyz[0] < xRange[0] || xyz[0] > xRange[1] ||
			   xyz[1] < yRange[0] || xyz[1] > yRange[1] )
			{
			  clippingRequired = 1;
			}

		      numLinePts++;
		      xyz[0] = pos[0] + 
			(xyz[0]-xRange[0])/(xRange[1]-xRange[0])*(pos2[0]-pos[0]);
		      xyz[1] = pos[1] + 
			(xyz[1]-yRange[0])/(yRange[1]-yRange[0])*(pos2[1]-pos[1]);
		      id = pts->InsertNextPoint(xyz);
		      lines->InsertCellPoint(id);
		    }
		} 
	      else
		{
		  // normalize and position
		  if ( xyz[0] < xRange[0] || xyz[0] > xRange[1] ||
		       xyz[1] < yRange[0] || xyz[1] > yRange[1] )
		    {
		      clippingRequired = 1;
		    }

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
	  if ( clippingRequired )
	    {
	      this->ClipPlotData(pos,pos2,this->PlotData[dsNum]);
	    }
	}//loop over all input data sets
    }//if plotting datasets

  else //plot data from data objects
    {
      vtkDataObject *dobj;
      int numColumns;
      vtkIdType numRows, numTuples;
      vtkDataArray *array;
      vtkFieldData *field;
      for ( doNum=0, this->DataObjectInputList->InitTraversal(); 
	    (dobj = this->DataObjectInputList->GetNextItem()); doNum++ )
	{
	  // determine the shape of the field
	  field = dobj->GetFieldData();
	  numColumns = field->GetNumberOfComponents(); //number of "columns"
	  for (numRows = VTK_LARGE_INTEGER, i=0; i<field->GetNumberOfArrays(); i++)
	    {
	      array = field->GetArray(i);
	      numTuples = array->GetNumberOfTuples();
	      if ( numTuples < numRows )
		{
		  numRows = numTuples;
		}
	    }

	  pts = this->PlotData[doNum]->GetPoints();
	  lines = this->PlotData[doNum]->GetLines();
	  lines->InsertNextCell(0); //update the count later

	  numPts = (this->DataObjectPlotMode == VTK_XYPLOT_ROW ? 
		    numColumns : numRows);

	  // gather the information to form a plot
	  for ( numLinePts=0, length=0.0, ptId=0; ptId < numPts; ptId++ )
	    {
	      if ( this->DataObjectPlotMode == VTK_XYPLOT_ROW )
		{
		  x[0] = field->GetComponent(this->XComponent->GetValue(doNum),ptId);
		  xyz[1] = field->GetComponent(this->YComponent->GetValue(doNum),ptId);
		}
	      else //if ( this->DataObjectPlotMode == VTK_XYPLOT_COLUMN )
		{
		  x[0] = field->GetComponent(ptId, this->XComponent->GetValue(doNum));
		  xyz[1] = field->GetComponent(ptId, this->YComponent->GetValue(doNum));
		}

	      switch (this->XValues)
		{
		case VTK_XYPLOT_NORMALIZED_ARC_LENGTH:
		  length += fabs(x[0]-xPrev[0]);
		  xyz[0] = length / lengths[doNum];
		  xPrev[0] = x[0];
		  break;
		case VTK_XYPLOT_INDEX:
		  xyz[0] = (float)ptId;
		  break;
		case VTK_XYPLOT_ARC_LENGTH:
		  length += fabs(x[0]-xPrev[0]);
		  xyz[0] = length;
		  xPrev[0] = x[0];
		  break;
		case VTK_XYPLOT_VALUE:
		  xyz[0] = x[0];
		  break;
		default:
		  vtkErrorMacro(<< "Unknown X-Value option");
		}
        


	      if ( this->GetLogx() == 1 )
		{
		  if (xyz[0] > 0)
		    {
		      xyz[0] = log10(xyz[0]);
		      // normalize and position
		      if ( xyz[0] < xRange[0] || xyz[0] > xRange[1] ||
			   xyz[1] < yRange[0] || xyz[1] > yRange[1] )
			{
			  clippingRequired = 1;
			}
		      numLinePts++;
		      xyz[0] = pos[0] + 
			(xyz[0]-xRange[0])/(xRange[1]-xRange[0])*(pos2[0]-pos[0]);
		      xyz[1] = pos[1] + 
			(xyz[1]-yRange[0])/(yRange[1]-yRange[0])*(pos2[1]-pos[1]);
		      id = pts->InsertNextPoint(xyz);
		      lines->InsertCellPoint(id);
		    }
		} 
	      else
		{
		  // normalize and position
		  if ( xyz[0] < xRange[0] || xyz[0] > xRange[1] ||
		       xyz[1] < yRange[0] || xyz[1] > yRange[1] )
		    {
		      clippingRequired = 1;
		    }    
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
	  if ( clippingRequired )
	    {
	      this->ClipPlotData(pos,pos2,this->PlotData[doNum]);
	    }
	}//loop over all input data sets
    }
  
  // Remove points/lines as directed by the user
  for ( i = 0; i < num; i++)
    {
      if (!this->PlotCurveLines) 
	{
	  if ( !this->PlotLines ) 
	    this->PlotData[i]->SetLines(NULL);
	}
      else
	{
	  if ( this->GetPlotLines(i) == 0)
	    this->PlotData[i]->SetLines(NULL);
	}

      if (!this->PlotCurvePoints) 
	{
	  if ( !this->PlotPoints || (this->LegendActor->GetEntrySymbol(i) &&
				     this->LegendActor->GetEntrySymbol(i) != 
				     this->GlyphSource->GetOutput()))
	    this->PlotData[i]->SetVerts(NULL);
	}
      else
	{
	  if ( this->GetPlotPoints(i) == 0 || 
	       (this->LegendActor->GetEntrySymbol(i) &&
		this->LegendActor->GetEntrySymbol(i) != 
		this->GlyphSource->GetOutput()))
	    this->PlotData[i]->SetVerts(NULL);
	}
    }
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
  if ( !this->ExchangeAxes )
    {
    textMapper->SetInput(this->YTitle);
    }
  else
    {
    textMapper->SetInput(this->XTitle);
    }
  vtkAxisActor2D::SetFontSize(viewport, textMapper, size, 1.0,
                              titleWidth, titleHeight);
  sprintf(string, this->LabelFormat, 0.0);
  textMapper->SetInput(string);
  vtkAxisActor2D::SetFontSize(viewport, textMapper, size, labelFactor,
                              labelWidth, labelHeight);
  
  // Okay, estimate the size
  pos[0] = (int)(p1[0] + titleWidth + 2.0*tickOffset + tickLength + 
                 labelWidth + this->Border);
  pos2[0] = (int)(p2[0] - labelWidth/2 - tickOffset - this->Border);
  pos[1] = (int)(p1[1] + titleHeight + 2.0*tickOffset + tickLength + 
                 labelHeight + this->Border);
  pos2[1] = (int)(p2[1] - labelHeight/2 - tickOffset - this->Border);

  // Now specify the location of the axes
  if ( !this->ExchangeAxes )
    {
      this->XAxis->GetPoint1Coordinate()->SetValue(pos[0], pos[1]);
      this->XAxis->GetPoint2Coordinate()->SetValue(pos2[0], pos[1]);
      this->YAxis->GetPoint1Coordinate()->SetValue(pos[0], pos2[1]);
      this->YAxis->GetPoint2Coordinate()->SetValue(pos[0], pos[1]);
    }
  else
    {
      this->XAxis->GetPoint1Coordinate()->SetValue(pos[0], pos2[1]);
      this->XAxis->GetPoint2Coordinate()->SetValue(pos[0], pos[1]);
      this->YAxis->GetPoint1Coordinate()->SetValue(pos[0], pos[1]);
      this->YAxis->GetPoint2Coordinate()->SetValue(pos2[0], pos[1]);
    }

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

void vtkXYPlotActor::ViewportToPlotCoordinate(vtkViewport *viewport)
{
  this->ViewportToPlotCoordinate(viewport, 
                                 this->PlotCoordinate[0],
                                 this->PlotCoordinate[1]);
}

void vtkXYPlotActor::PlotToViewportCoordinate(vtkViewport *viewport)
{
  this->PlotToViewportCoordinate(viewport, 
                                 this->ViewportCoordinate[0],
                                 this->ViewportCoordinate[1]);
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

void vtkXYPlotActor::SetPlotLines(int i, int isOn)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  int val = this->LinesOn->GetValue(i);
  if ( val != isOn )
    {
      this->Modified();
      this->LinesOn->SetValue(i, isOn);
    }
}

int vtkXYPlotActor::GetPlotLines(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->LinesOn->GetValue(i);
}

void vtkXYPlotActor::SetPlotPoints(int i, int isOn)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  int val = this->PointsOn->GetValue(i);
  if ( val != isOn )
    {
      this->Modified();
      this->PointsOn->SetValue(i, isOn);
    }
}

int vtkXYPlotActor::GetPlotPoints(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->PointsOn->GetValue(i);
}

void vtkXYPlotActor::SetPlotColor(int i, float r, float g, float b)
{
  this->LegendActor->SetEntryColor(i, r, g, b);
}

float *vtkXYPlotActor::GetPlotColor(int i)
{
  return this->LegendActor->GetEntryColor(i);
}

void vtkXYPlotActor::SetPlotSymbol(int i,vtkPolyData *input)
{
  this->LegendActor->SetEntrySymbol(i, input);
}

vtkPolyData *vtkXYPlotActor::GetPlotSymbol(int i)
{
  return this->LegendActor->GetEntrySymbol(i);
}

void vtkXYPlotActor::SetPlotLabel(int i, const char *label)
{
  this->LegendActor->SetEntryString(i, label);
}

const char *vtkXYPlotActor::GetPlotLabel(int i)
{
  return this->LegendActor->GetEntryString(i);
}

void vtkXYPlotActor::GenerateClipPlanes(int *pos, int *pos2)
{
  float n[3], x[3];
  vtkPoints *pts=this->ClipPlanes->GetPoints();
  vtkDataArray *normals=this->ClipPlanes->GetNormals();
  
  n[2] = x[2] = 0.0;
  
  //first
  n[0] = 0.0;
  n[1] = -1.0;
  normals->SetTuple(0,n);
  x[0] = (float)0.5*(pos[0]+pos2[0]);
  x[1] = (float)pos[1];
  pts->SetPoint(0,x);
  
  //second
  n[0] = 1.0;
  n[1] = 0.0;
  normals->SetTuple(1,n);
  x[0] = (float)pos2[0];
  x[1] = (float)0.5*(pos[1]+pos2[1]);
  pts->SetPoint(1,x);
  
  //third
  n[0] = 0.0;
  n[1] = 1.0;
  normals->SetTuple(2,n);
  x[0] = (float)0.5*(pos[0]+pos2[0]);
  x[1] = (float)pos2[1];
  pts->SetPoint(2,x);
  
  //fourth
  n[0] = -1.0;
  n[1] = 0.0;
  normals->SetTuple(3,n);
  x[0] = (float)pos[0];
  x[1] = (float)0.5*(pos[1]+pos2[1]);
  pts->SetPoint(3,x);
}

float vtkXYPlotActor::ComputeGlyphScale(int i, int *pos, int *pos2)
{
  vtkPolyData *pd=this->LegendActor->GetEntrySymbol(i);
  pd->Update();
  float length=pd->GetLength();
  float sf = this->GlyphSize * sqrt((double)(pos[0]-pos2[0])*(pos[0]-pos2[0]) + 
                                    (pos[1]-pos2[1])*(pos[1]-pos2[1])) / length;

  return sf;
}

//This assumes that there are multiple polylines
void vtkXYPlotActor::ClipPlotData(int *pos, int *pos2, vtkPolyData *pd)
{
  vtkPoints *points=pd->GetPoints();
  vtkPoints *newPoints;
  vtkCellArray *lines=pd->GetLines();
  vtkCellArray *newLines, *newVerts;
  vtkIdType numPts=pd->GetNumberOfPoints();
  vtkIdType npts;
  vtkIdType newPts[2], *pts, i, id;
  int j;
  float *x1, *x2, *px, *n, xint[3], t;
  float p1[2], p2[2];

  p1[0] = (float)pos[0]; p1[1] = (float)pos[1];
  p2[0] = (float)pos2[0]; p2[1] = (float)pos2[1];
  
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(lines->GetSize());
  newLines = vtkCellArray::New();
  newLines->Allocate(2*lines->GetSize());
  int *pointMap = new int [numPts];
  for (i=0; i<numPts; i++)
    {
    pointMap[i] = -1;
    }
  
  //Loop over polyverts eliminating those that are outside
  for ( lines->InitTraversal(); lines->GetNextCell(npts,pts); )
    {
    //loop over verts keeping only those that are not clipped
    for (i=0; i<npts; i++)
      {
      x1 = points->GetPoint(pts[i]);

      if (x1[0] >= p1[0] && x1[0] <= p2[0] && x1[1] >= p1[1] && x1[1] <= p2[1] )
        {
        id = newPoints->InsertNextPoint(x1);
        pointMap[i] = id;
        newPts[0] = id;
        newVerts->InsertNextCell(1,newPts);
        }
      }
    }

  //Loop over polylines clipping each line segment
  for ( lines->InitTraversal(); lines->GetNextCell(npts,pts); )
    {
    //loop over line segment making up the polyline
    for (i=0; i<(npts-1); i++)
      {
      x1 = points->GetPoint(pts[i]);
      x2 = points->GetPoint(pts[i+1]);

      //intersect each segment with the four planes
      if ( (x1[0] < p1[0] && x2[0] < p1[0]) || (x1[0] > p2[0] && x2[0] > p2[0]) ||
           (x1[1] < p1[1] && x2[1] < p1[1]) || (x1[1] > p2[1] && x2[1] > p2[1]) )
        {
        ;//trivial rejection
        }
      else if (x1[0] >= p1[0] && x2[0] >= p1[0] && x1[0] <= p2[0] && x2[0] <= p2[0] &&
               x1[1] >= p1[1] && x2[1] >= p1[1] && x1[1] <= p2[1] && x2[1] <= p2[1] )
        {//trivial acceptance
        newPts[0] = pointMap[pts[i]];
        newPts[1] = pointMap[pts[i+1]];
        newLines->InsertNextCell(2,newPts);
        }
      else
        {
        if (x1[0] >= p1[0] && x1[0] <= p2[0] && x1[1] >= p1[1] && x1[1] <= p2[1] )
          {//first point in
          newPts[0] = pointMap[pts[i]];
          }
        else
          {//second point in
          newPts[0] = pointMap[pts[i+1]];
          }
        for (j=0; j<4; j++)
          {
          px = this->ClipPlanes->GetPoints()->GetPoint(j);
          n = this->ClipPlanes->GetNormals()->GetTuple(j);
          if ( vtkPlane::IntersectWithLine(x1,x2,n,px,t,xint) && t >= 0 && t <= 1.0 )
            {
            newPts[1] = newPoints->InsertNextPoint(xint);
            break;
            }
          }
        newLines->InsertNextCell(2,newPts);
        }
      }
    }
  delete [] pointMap;
  
  //Update the lines
  pd->SetPoints(newPoints);
  pd->SetVerts(newVerts);
  pd->SetLines(newLines);
  
  newPoints->Delete();
  newVerts->Delete();
  newLines->Delete();
  
}

void vtkXYPlotActor::SetDataObjectXComponent(int i, int comp)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  int val=this->XComponent->GetValue(i);
  if ( val != comp )
    {
      this->Modified();
      this->XComponent->SetValue(i,comp);
    }
}

int vtkXYPlotActor::GetDataObjectXComponent(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->XComponent->GetValue(i);
}

void vtkXYPlotActor::SetDataObjectYComponent(int i, int comp)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  int val=this->YComponent->GetValue(i);
  if ( val != comp )
    {
      this->Modified();
      this->YComponent->SetValue(i,comp);
    }
}

int vtkXYPlotActor::GetDataObjectYComponent(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->YComponent->GetValue(i);
}

void vtkXYPlotActor::SetPointComponent(int i, int comp)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  int val=this->XComponent->GetValue(i);
  if ( val != comp )
    {
      this->Modified();
      this->XComponent->SetValue(i,comp);
    }
}

int vtkXYPlotActor::GetPointComponent(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->XComponent->GetValue(i);
}

float *vtkXYPlotActor::TransformPoint(int pos[2], int pos2[2], float x[3], float xNew[3])
{
  // First worry about exchanging axes
  if ( this->ExchangeAxes )
    {
    float sx = (x[0]-pos[0]) / (pos2[0]-pos[0]);
    float sy = (x[1]-pos[1]) / (pos2[1]-pos[1]);
    xNew[0] = sy*(pos2[0]-pos[0]) + pos[0];
    xNew[1] = sx*(pos2[1]-pos[1]) + pos[1];
    xNew[2] = x[2];
    }
  else
    {
    xNew[0] = x[0];
    xNew[1] = x[1];
    xNew[2] = x[2];
    }

  // Okay, now swap the axes around if reverse is on
  if ( this->ReverseXAxis )
    {
    xNew[0] = pos[0] + (pos2[0]-xNew[0]);
    }
  if ( this->ReverseYAxis )
    {
    xNew[1] = pos[1] + (pos2[1]-xNew[1]);
    }

  return xNew;
}
