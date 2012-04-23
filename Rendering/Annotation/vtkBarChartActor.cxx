/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBarChartActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBarChartActor.h"

#include "vtkAxisActor2D.h"
#include "vtkCellArray.h"
#include "vtkFieldData.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkLegendBoxActor.h"
#include "vtkGlyphSource2D.h"
#include "vtkProperty2D.h"
#include <string>
#include <vector>


vtkStandardNewMacro(vtkBarChartActor);

vtkCxxSetObjectMacro(vtkBarChartActor,Input,vtkDataObject);
vtkCxxSetObjectMacro(vtkBarChartActor,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkBarChartActor,TitleTextProperty,vtkTextProperty);

// PIMPL'd list of labels
class vtkBarLabelArray : public std::vector<std::string> {};


//----------------------------------------------------------------------------
// Instantiate object
vtkBarChartActor::vtkBarChartActor()
{
  // Actor2D positions
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.1,0.1);
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.9, 0.8);
  this->Position2Coordinate->SetReferenceCoordinate(NULL);

  this->Input = NULL;
  this->ArrayNumber = 0;
  this->ComponentNumber = 0;
  this->TitleVisibility = 1;
  this->Title = NULL;
  this->Labels = new vtkBarLabelArray;
  this->BarMappers = NULL;
  this->BarActors = NULL;
  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetFontSize(12);
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(0);
  this->LabelTextProperty->SetFontFamilyToArial();
  this->TitleTextProperty = vtkTextProperty::New();
  this->TitleTextProperty->ShallowCopy(this->LabelTextProperty);
  this->TitleTextProperty->SetFontSize(24);
  this->TitleTextProperty->SetBold(1);
  this->TitleTextProperty->SetItalic(0);
  this->TitleTextProperty->SetShadow(1);
  this->TitleTextProperty->SetFontFamilyToArial();
  this->LabelVisibility = 1;

  this->LegendVisibility = 1;

  this->LegendActor = vtkLegendBoxActor::New();
  this->LegendActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->LegendActor->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->LegendActor->GetPosition2Coordinate()->SetReferenceCoordinate(NULL);
  this->LegendActor->BorderOff();
  this->LegendActor->SetNumberOfEntries(100); //initial allocation
  this->LegendActor->SetPadding(2);
  this->LegendActor->ScalarVisibilityOff();
  this->GlyphSource = vtkGlyphSource2D::New();
  this->GlyphSource->SetGlyphTypeToNone();
  this->GlyphSource->DashOn();
  this->GlyphSource->FilledOff();

  this->YAxis = vtkAxisActor2D::New();
  this->YAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->YAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->YAxis->SetProperty(this->GetProperty());
  this->YAxis->SizeFontRelativeToAxisOn();
  this->YTitle = new char[1];
  sprintf(this->YTitle,"%s","");

  this->PlotData = vtkPolyData::New();
  this->PlotMapper = vtkPolyDataMapper2D::New();
  this->PlotMapper->SetInputData(this->PlotData);
  this->PlotActor = vtkActor2D::New();
  this->PlotActor->SetMapper(this->PlotMapper);
  this->TitleMapper = vtkTextMapper::New();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();

  this->N = 0;
  this->Heights = NULL;
  this->MinHeight =  VTK_LARGE_FLOAT;
  this->MaxHeight = -VTK_LARGE_FLOAT;

  this->LowerLeft[0] = this->LowerLeft[1] = 0.0;
  this->UpperRight[0] = this->UpperRight[1] = 0.0;

  this->LastPosition[0] =
    this->LastPosition[1] =
    this->LastPosition2[0] =
    this->LastPosition2[1] = 0;

  this->P1[0] = this->P1[1] = this->P2[0] = this->P2[1] = 0.0;
}

//----------------------------------------------------------------------------
vtkBarChartActor::~vtkBarChartActor()
{
  if ( this->Input )
    {
    this->Input->Delete();
    this->Input = NULL;
    }

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }

  delete this->Labels;
  this->SetLabelTextProperty(NULL);
  this->SetTitleTextProperty(NULL);

  this->LegendActor->Delete();
  this->GlyphSource->Delete();

  this->Initialize();

  this->TitleMapper->Delete();
  this->TitleMapper = NULL;
  this->TitleActor->Delete();
  this->TitleActor = NULL;

  this->YAxis->Delete();
  if ( this->YTitle )
    {
    delete [] this->YTitle;
    }

  this->PlotData->Delete();
  this->PlotMapper->Delete();
  this->PlotActor->Delete();
}

//----------------------------------------------------------------------------
// Free-up axes and related stuff
void vtkBarChartActor::Initialize()
{
  if ( this->BarActors )
    {
    for (int i=0; i<this->N; i++)
      {
      this->BarMappers[i]->Delete();
      this->BarActors[i]->Delete();
      }
    delete [] this->BarMappers;
    this->BarMappers = NULL;
    delete [] this->BarActors;
    this->BarActors = NULL;
    }

  this->N = 0;
  if ( this->Heights )
    {
    delete [] this->Heights;
    this->Heights = NULL;
    }
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int vtkBarChartActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething=0;

  if ( !this->BuildPlot(viewport) )
    {
    return 0;
    }

  // Done rebuilding, render as appropriate.
  if ( this->Input == NULL || this->N <= 0 )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  if ( this->TitleVisibility )
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }

  renderedSomething += this->YAxis->RenderOverlay(viewport);
  renderedSomething += this->PlotActor->RenderOverlay(viewport);

  if ( this->LabelVisibility )
    {
    for (int i=0; i<this->N; i++)
      {
      renderedSomething += this->BarActors[i]->RenderOverlay(viewport);
      }
    }

  if ( this->LegendVisibility )
    {
    renderedSomething += this->LegendActor->RenderOverlay(viewport);
    }

  return renderedSomething;
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int vtkBarChartActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething=0;

  if ( !this->BuildPlot(viewport) )
    {
    return 0;
    }

  // Done rebuilding, render as appropriate.
  if ( this->Input == NULL || this->N <= 0 )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  if ( this->TitleVisibility )
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }

  renderedSomething += this->YAxis->RenderOpaqueGeometry(viewport);
  renderedSomething += this->PlotActor->RenderOpaqueGeometry(viewport);

  if ( this->LabelVisibility )
    {
    for (int i=0; i<this->N; i++)
      {
      renderedSomething += this->BarActors[i]->RenderOpaqueGeometry(viewport);
      }
    }

  if ( this->LegendVisibility )
    {
    renderedSomething += this->LegendActor->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkBarChartActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//-----------------------------------------------------------------------------
int vtkBarChartActor::BuildPlot(vtkViewport *viewport)
{
  // Initialize
  vtkDebugMacro(<<"Building pie chart plot");

  // Make sure input is up to date, and that the data is the correct shape to
  // plot.
  if (!this->Input)
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  if (!this->TitleTextProperty)
    {
    vtkErrorMacro(<<"Need title text property to render plot");
    return 0;
    }
  if (!this->LabelTextProperty)
    {
    vtkErrorMacro(<<"Need label text property to render plot");
    return 0;
    }

  // Viewport change may not require rebuild
  int positionsHaveChanged = 0;
  if (viewport->GetMTime() > this->BuildTime ||
      (viewport->GetVTKWindow() &&
       viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
    {
    int *lastPosition =
      this->PositionCoordinate->GetComputedViewportValue(viewport);
    int *lastPosition2 =
      this->Position2Coordinate->GetComputedViewportValue(viewport);
    if (lastPosition[0] != this->LastPosition[0] ||
        lastPosition[1] != this->LastPosition[1] ||
        lastPosition2[0] != this->LastPosition2[0] ||
        lastPosition2[1] != this->LastPosition2[1] )
      {
      this->LastPosition[0] = lastPosition[0];
      this->LastPosition[1] = lastPosition[1];
      this->LastPosition2[0] = lastPosition2[0];
      this->LastPosition2[1] = lastPosition2[1];
      positionsHaveChanged = 1;
      }
    }

  // Check modified time to see whether we have to rebuild.
  if (positionsHaveChanged ||
      this->GetMTime() > this->BuildTime ||
      this->Input->GetMTime() > this->BuildTime ||
      this->LabelTextProperty->GetMTime() > this->BuildTime ||
      this->TitleTextProperty->GetMTime() > this->BuildTime)
    {
    vtkDebugMacro(<<"Rebuilding plot");

    // Build axes
    int *size = viewport->GetSize();
    if (!this->PlaceAxes(viewport, size))
      {
      return 0;
      }

    this->BuildTime.Modified();
    } // If need to rebuild the plot

  return 1;
}

//----------------------------------------------------------------------------
int vtkBarChartActor::PlaceAxes(vtkViewport *viewport, int *vtkNotUsed(size))
{
  vtkIdType i;
  vtkDataObject *input = this->GetInput();
  vtkFieldData *field = input->GetFieldData();
  double v = 0.0;

  this->Initialize();

  if ( ! field )
    {
    return 0;
    }

  // Retrieve the appropriate data array
  vtkDataArray *da = field->GetArray(this->ArrayNumber);
  if ( ! da )
    {
    return 0;
    }

  // Determine the number of independent variables
  this->N = da->GetNumberOfTuples();
  if ( this->N <= 0 || this->N >= VTK_LARGE_ID )
    {
    this->N = 0;
    vtkErrorMacro(<<"No field data to plot");
    return 0;
    }

  // We need to loop over the field to determine the total
  this->Heights = new double[this->N];
  this->MaxHeight = -VTK_LARGE_FLOAT;
  this->MinHeight =  VTK_LARGE_FLOAT;
  for (i=0; i<this->N; i++)
    {
    v = fabs(da->GetComponent(i,this->ComponentNumber));
    this->Heights[i] = v;
    this->MinHeight = (v < this->MinHeight ? v : this->MinHeight);
    this->MaxHeight = (v > this->MaxHeight ? v : this->MaxHeight);
    }
  if ( this->MaxHeight > 0.0 )
    {
    // compress the heights into the (0.10->1.0) range for aesthetic reasons
    // (i.e., you always want to see the minimum bar).
    for (i=0; i<this->N; i++)
      {
      this->Heights[i] = 0.10 + 0.90*(this->Heights[i] - this->MinHeight) /
        (this->MaxHeight - this->MinHeight);
      }
    this->MinHeight -= 0.10*(this->MaxHeight-this->MinHeight);
    }

  // Get the location of the corners of the box; make sure they are sane
  double *p1 = this->PositionCoordinate->GetComputedDoubleViewportValue(viewport);
  double *p2 = this->Position2Coordinate->GetComputedDoubleViewportValue(viewport);
  this->P1[0] = (p1[0] < p2[0] ? p1[0] : p2[0]);
  this->P1[1] = (p1[1] < p2[1] ? p1[1] : p2[1]);
  this->P2[0] = (p1[0] > p2[0] ? p1[0] : p2[0]);
  this->P2[1] = (p1[1] > p2[1] ? p1[1] : p2[1]);
  p1 = this->P1;
  p2 = this->P2;

  // Create the bar plot.
  // Determine the boundaries of the plot.
  double titleSpace=0.0, legendSpace=0.0;
  if ( this->TitleVisibility )
    {
    titleSpace = 0.1;
    }
  if ( this->LegendVisibility )
    {
    legendSpace = 0.15;
    }

  double d1 = p2[0] - legendSpace*(p2[0]-p1[0]) - p1[0];
  double d2 = p2[1] - titleSpace*(p2[1]-p1[1]) - p1[1];

  this->LowerLeft[0] = p1[0] + 25;
  this->LowerLeft[1] = p1[1] + 15;
  this->UpperRight[0] = p1[0] + d1 - 15;
  this->UpperRight[1] = p1[1] + d2 - 15;
  // Make sue layout is sane
  if ( this->LowerLeft[0] > this->UpperRight[0] )
    {
    this->LowerLeft[0] = p1[0];
    this->UpperRight[0] = p2[0];
    }
  if ( this->LowerLeft[1] > this->UpperRight[1] )
    {
    this->LowerLeft[1] = p1[1];
    this->UpperRight[1] = p2[1];
    }

  // First configure the y-axis
  this->YAxis->SetProperty(this->Property);
  this->YAxis->GetLabelTextProperty()->ShallowCopy(this->LabelTextProperty);
  this->YAxis->SetTitle(this->YTitle);
  this->YAxis->SetNumberOfLabels(5);
  this->YAxis->SetRange(this->MaxHeight,this->MinHeight);
  this->YAxis->GetPosition2Coordinate()->SetValue(this->LowerLeft[0],this->LowerLeft[1]);
  this->YAxis->GetPositionCoordinate()->SetValue(this->LowerLeft[0],this->UpperRight[1]);

  // Now generate the bar polygons
  this->PlotData->Initialize(); //remove old polydata, if any
  vtkPoints *pts = vtkPoints::New();
  pts->Allocate(this->N*4);
  vtkCellArray *xaxis = vtkCellArray::New();
  xaxis->Allocate(xaxis->EstimateSize(1,2));
  vtkCellArray *polys = vtkCellArray::New();
  polys->Allocate(polys->EstimateSize(this->N,4));
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  this->PlotData->SetPoints(pts);
  this->PlotData->SetLines(xaxis);
  this->PlotData->SetPolys(polys);
  this->PlotData->GetCellData()->SetScalars(colors);
  colors->Delete();

  double *color, c[4];
  vtkIdType pIds[4];

  // Create the x-axis
  pIds[0] = pts->InsertNextPoint(this->LowerLeft[0],this->LowerLeft[1],0.0);
  pIds[1] = pts->InsertNextPoint(this->UpperRight[0],this->LowerLeft[1],0.0);
  xaxis->InsertNextCell(2,pIds);
  this->GetProperty()->GetColor(c);
  colors->InsertNextTuple3(255*c[0],255*c[1],255*c[2]);

  // Create the bars. Make sure there is some spacing.
  char label[1024];
  const char *str;
  double x[3]; x[2]=0.0;
  double space = 0.25*(this->UpperRight[0]-this->LowerLeft[0]) / this->N;
  double barWidth = 0.75*(this->UpperRight[0]-this->LowerLeft[0]) / this->N;
  for (i=0; i<this->N; i++)
    {
    x[0] = this->LowerLeft[0] + (i+1)*space + i*barWidth;
    x[1] = this->LowerLeft[1] + 1;
    pIds[0] = pts->InsertNextPoint(x);

    x[0] += barWidth;
    pIds[1] = pts->InsertNextPoint(x);

    x[1] += this->Heights[i]*(this->UpperRight[1]-this->LowerLeft[1]) - 1;
    pIds[2] = pts->InsertNextPoint(x);

    x[0] -= barWidth;
    pIds[3] = pts->InsertNextPoint(x);

    polys->InsertNextCell(4,pIds);
    color = this->LegendActor->GetEntryColor(i);
    colors->InsertNextTuple3(255*color[0],255*color[1],255*color[2]);
    this->LegendActor->SetEntrySymbol(i,this->GlyphSource->GetOutput());
    if ( (str=this->GetBarLabel(i)) != NULL )
      {
      this->LegendActor->SetEntryString(i,str);
      }
    else
      {
      sprintf(label,"%d",static_cast<int>(i));
      this->LegendActor->SetEntryString(i,label);
      }
    }

  // Produce labels along the bars
  int minFontSize=1000, fontSize, tsize[2];
  if ( this->LabelVisibility )
    {
    this->BarActors = new vtkActor2D* [this->N];
    this->BarMappers = new vtkTextMapper* [this->N];
    for (i=0; i<this->N; i++)
      {
      this->BarMappers[i] = vtkTextMapper::New();
      if ( (str=this->GetBarLabel(i)) != NULL )
        {
        this->BarMappers[i]->SetInput(str);
        }
      else
        {
        sprintf(label,"%d",static_cast<int>(i));
        this->BarMappers[i]->SetInput(label);
        }
      this->BarMappers[i]->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
      this->BarMappers[i]->GetTextProperty()->SetJustificationToCentered();
      this->BarMappers[i]->GetTextProperty()->SetVerticalJustificationToTop();
      tsize[0] = static_cast<int>(barWidth);
      tsize[1] = static_cast<int>(barWidth);
      fontSize = this->BarMappers[i]->SetConstrainedFontSize(
        viewport, tsize[0], tsize[1]);
      minFontSize = (fontSize < minFontSize ? fontSize : minFontSize);

      this->BarActors[i] = vtkActor2D::New();
      this->BarActors[i]->SetMapper(this->BarMappers[i]);
      this->BarActors[i]->GetPositionCoordinate()->
        SetCoordinateSystemToViewport();
      x[0] = this->LowerLeft[0] + (i+1)*space + i*barWidth + barWidth/2.0;
      x[1] = this->LowerLeft[1]-3;
      this->BarActors[i]->SetPosition(x);
      }
    //Now reset font sizes to the same value
    for (i=0; i<this->N; i++)
      {
      this->BarMappers[i]->GetTextProperty()->SetFontSize(minFontSize);
      }
    }

  //Display the legend
  if ( this->LegendVisibility )
    {
    this->LegendActor->GetProperty()->DeepCopy(this->GetProperty());
    this->LegendActor->GetPositionCoordinate()->SetValue(
      p1[0] + 0.85*(p2[0]-p1[0]),p1[1] + 0.20*(p2[1]-p1[1]));
    this->LegendActor->GetPosition2Coordinate()->SetValue(
      p2[0], p1[1] + 0.80*(p2[1]-p1[1]));
    }

  // Build title
  this->TitleMapper->SetInput(this->Title);
  if (this->TitleTextProperty->GetMTime() > this->BuildTime)
    {
    // Shallow copy here since the justification is changed but we still
    // want to allow actors to share the same text property, and in that case
    // specifically allow the title and label text prop to be the same.
    this->TitleMapper->GetTextProperty()->ShallowCopy(
      this->TitleTextProperty);
    this->TitleMapper->GetTextProperty()->SetJustificationToCentered();
    }

  // We could do some caching here, but hey, that's just the title
  tsize[0] = static_cast<int>(0.25*d1);
  tsize[1] = static_cast<int>(0.15*d2);
  this->TitleMapper->SetConstrainedFontSize(viewport, tsize[0], tsize[1]);

  this->TitleActor->GetPositionCoordinate()->
    SetValue((this->LowerLeft[0]+this->UpperRight[0])/2.0,
             this->UpperRight[1]+tsize[1]);
  this->TitleActor->SetProperty(this->GetProperty());

  // Clean up
  pts->Delete();
  xaxis->Delete();
  polys->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkBarChartActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  this->LegendActor->ReleaseGraphicsResources(win);
  this->YAxis->ReleaseGraphicsResources(win);
  this->PlotActor->ReleaseGraphicsResources(win);
  for (int i=0; this->BarActors && i<this->N; i++)
    {
    this->BarActors[i]->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
void vtkBarChartActor::SetBarLabel(const int i, const char *label)
{
  if ( i < 0 )
    {
    return;
    }

  if ( static_cast<unsigned int>(i) >= this->Labels->size() )
    {
    this->Labels->resize(i+1);
    }
  (*this->Labels)[i] = std::string(label);
  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkBarChartActor::GetBarLabel(int i)
{
  if ( i < 0 || static_cast<unsigned int>(i) >= this->Labels->size())
    {
    return NULL;
    }

  return this->Labels->at(i).c_str();
}

//----------------------------------------------------------------------------
void vtkBarChartActor::SetBarColor(int i, double r, double g, double b)
{
  this->LegendActor->SetEntryColor(i, r, g, b);
}

//----------------------------------------------------------------------------
double *vtkBarChartActor::GetBarColor(int i)
{
  return this->LegendActor->GetEntryColor(i);
}

//----------------------------------------------------------------------------
void vtkBarChartActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->Input << "\n";

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";

  os << indent << "Title Visibility: "
     << (this->TitleVisibility ? "On\n" : "Off\n");

  if (this->TitleTextProperty)
    {
    os << indent << "Title Text Property:\n";
    this->TitleTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Title Text Property: (none)\n";
    }

  os << indent << "Label Visibility: "
     << (this->LabelVisibility ? "On\n" : "Off\n");

  if (this->LabelTextProperty)
    {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Label Text Property: (none)\n";
    }

  os << indent << "Legend Visibility: "
     << (this->LegendVisibility ? "On\n" : "Off\n");

  os << indent << "Legend Actor: "
     << this->LegendActor << "\n";
  this->LegendActor->PrintSelf(os, indent.GetNextIndent());

  os << indent << "YTitle: " << (this->YTitle ? this->YTitle : "(none)") << "\n";

}
