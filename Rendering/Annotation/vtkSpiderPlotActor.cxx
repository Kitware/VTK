/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpiderPlotActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpiderPlotActor.h"

#include "vtkAlgorithmOutput.h"
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
#include "vtkTrivialProducer.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkLegendBoxActor.h"
#include "vtkGlyphSource2D.h"
#include "vtkProperty2D.h"
#include <string>
#include <vector>


vtkStandardNewMacro(vtkSpiderPlotActor);

vtkCxxSetObjectMacro(vtkSpiderPlotActor,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkSpiderPlotActor,TitleTextProperty,vtkTextProperty);

// PIMPL'd list of labels
class vtkAxisLabelArray : public std::vector<std::string> {};

// PIMPL'd list of ranges
struct vtkAxisRange
{
  double Min;
  double Max;
  vtkAxisRange() : Min(0.0), Max(0.0) {}
  vtkAxisRange(double min, double max) : Min(min), Max(max) {}
};
class vtkAxisRanges : public std::vector<vtkAxisRange> {};

class vtkSpiderPlotActorConnection : public vtkAlgorithm
{
public:
  static vtkSpiderPlotActorConnection *New();
  vtkTypeMacro(vtkSpiderPlotActorConnection,vtkAlgorithm);

  vtkSpiderPlotActorConnection()
    {
      this->SetNumberOfInputPorts(1);
    }
};

vtkStandardNewMacro(vtkSpiderPlotActorConnection);


//----------------------------------------------------------------------------
// Instantiate object
vtkSpiderPlotActor::vtkSpiderPlotActor()
{
  // Actor2D positions
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.1,0.1);
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.9, 0.8);
  this->Position2Coordinate->SetReferenceCoordinate(NULL);

  this->ConnectionHolder = vtkSpiderPlotActorConnection::New();

  this->IndependentVariables = VTK_IV_COLUMN;
  this->TitleVisibility = 1;
  this->Title = NULL;
  this->Labels = new vtkAxisLabelArray;
  this->Ranges = new vtkAxisRanges;
  this->LabelMappers = NULL;
  this->LabelActors = NULL;
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
  this->NumberOfRings = 2;

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
  this->GlyphSource->Update();

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
  this->Mins = NULL;
  this->Maxs = NULL;

  this->WebData = vtkPolyData::New();
  this->WebMapper = vtkPolyDataMapper2D::New();
  this->WebMapper->SetInputData(this->WebData);
  this->WebActor = vtkActor2D::New();
  this->WebActor->SetMapper(this->WebMapper);

  this->LastPosition[0] =
    this->LastPosition[1] =
    this->LastPosition2[0] =
    this->LastPosition2[1] = 0;

  this->P1[0] = this->P1[1] = this->P2[0] = this->P2[1] = 0.0;
}

//----------------------------------------------------------------------------
vtkSpiderPlotActor::~vtkSpiderPlotActor()
{
  this->ConnectionHolder->Delete();
  this->ConnectionHolder = 0;

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }

  delete this->Labels;
  delete this->Ranges;
  this->SetLabelTextProperty(NULL);
  this->SetTitleTextProperty(NULL);

  this->LegendActor->Delete();
  this->GlyphSource->Delete();

  this->Initialize();

  this->TitleMapper->Delete();
  this->TitleMapper = NULL;
  this->TitleActor->Delete();
  this->TitleActor = NULL;

  this->WebData->Delete();
  this->WebMapper->Delete();
  this->WebActor->Delete();

  this->PlotData->Delete();
  this->PlotMapper->Delete();
  this->PlotActor->Delete();
}

//----------------------------------------------------------------------------
void vtkSpiderPlotActor::SetInputConnection(vtkAlgorithmOutput* ao)
{
  this->ConnectionHolder->SetInputConnection(ao);
}

//----------------------------------------------------------------------------
void vtkSpiderPlotActor::SetInputData(vtkDataObject* dobj)
{
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(dobj);
  this->SetInputConnection(tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSpiderPlotActor::GetInput()
{
  return this->ConnectionHolder->GetInputDataObject(0, 0);
}

//----------------------------------------------------------------------------
// Free-up axes and related stuff
void vtkSpiderPlotActor::Initialize()
{
  if ( this->LabelActors )
    {
    for (int i=0; i<this->N; i++)
      {
      this->LabelMappers[i]->Delete();
      this->LabelActors[i]->Delete();
      }
    delete [] this->LabelMappers;
    this->LabelMappers = NULL;
    delete [] this->LabelActors;
    this->LabelActors = NULL;
    }

  if ( this->Mins )
    {
    delete [] this->Mins;
    this->Mins = NULL;
    delete [] this->Maxs;
    this->Maxs = NULL;
    }

  this->N = 0;
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int vtkSpiderPlotActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething=0;

  if ( !this->BuildPlot(viewport) )
    {
    return 0;
    }

  // Done rebuilding, render as appropriate.
  if ( this->GetInput() == NULL || this->N <= 0 )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  if ( this->TitleVisibility )
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }

  this->WebActor->SetProperty(this->GetProperty());
  renderedSomething += this->WebActor->RenderOverlay(viewport);
  renderedSomething += this->PlotActor->RenderOverlay(viewport);

  if ( this->LabelVisibility )
    {
    for (int i=0; i<this->N; i++)
      {
      renderedSomething += this->LabelActors[i]->RenderOverlay(viewport);
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
int vtkSpiderPlotActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething=0;

  if ( !this->BuildPlot(viewport) )
    {
    return 0;
    }

  // Done rebuilding, render as appropriate.
  if ( this->GetInput() == NULL || this->N <= 0 )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  if ( this->TitleVisibility )
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }

  this->WebActor->SetProperty(this->GetProperty());
  renderedSomething += this->WebActor->RenderOpaqueGeometry(viewport);
  renderedSomething += this->PlotActor->RenderOpaqueGeometry(viewport);

  if ( this->LabelVisibility )
    {
    for (int i=0; i<this->N; i++)
      {
      renderedSomething += this->LabelActors[i]->RenderOpaqueGeometry(viewport);
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
int vtkSpiderPlotActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSpiderPlotActor::BuildPlot(vtkViewport *viewport)
{
  // Initialize
  vtkDebugMacro(<<"Building spider plot");

  // Make sure input is up to date, and that the data is the correct shape to
  // plot.
  if (!this->GetInput())
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
  this->ConnectionHolder->GetInputAlgorithm()->Update();

  if (positionsHaveChanged ||
      this->GetMTime() > this->BuildTime ||
      this->GetInput()->GetMTime() > this->BuildTime ||
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
static inline int vtkSpiderPlotActorGetComponent(vtkFieldData* field,
  vtkIdType tuple, int component, double* val)
{
  int array_comp;
  int array_index = field->GetArrayContainingComponent(component, array_comp);
  if (array_index < 0)
    {
    return 0;
    }
  vtkDataArray* da = field->GetArray(array_index);
  if (!da)
    {
    // non-numeric array.
    return 0;
    }
  *val = da->GetComponent(tuple, array_comp);
  return 1;
}

#define VTK_RING_PTS 64
//----------------------------------------------------------------------------
int vtkSpiderPlotActor::PlaceAxes(vtkViewport *viewport, int* vtkNotUsed(size))
{
  vtkIdType i, j, k;
  vtkDataObject *input = this->GetInput();
  vtkFieldData *field = input->GetFieldData();
  double v = 0.0;

  this->Initialize();

  if ( ! field )
    {
    return 0;
    }

  // Determine the shape of the field
  int numComponents = field->GetNumberOfComponents(); //number of components
  // Note: numComponents also includes the non-numeric arrays.

  int numColumns = 0; //number of "columns" -- includes only numeric arrays.
  vtkIdType numRows = VTK_LARGE_ID; //figure out number of rows
  vtkIdType numTuples;
  vtkDataArray *array;
  for (i=0; i<field->GetNumberOfArrays(); i++)
    {
    array = field->GetArray(i);
    if (!array)
      {
      // skip over non-numeric arrays.
      continue;
      }
    numColumns += array->GetNumberOfComponents();
    numTuples = array->GetNumberOfTuples();
    if ( numTuples < numRows )
      {
      numRows = numTuples;
      }
    }

  // Determine the number of independent variables
  if ( this->IndependentVariables == VTK_IV_COLUMN )
    {
    this->N = numColumns;
    }
  else //row
    {
    this->N = numRows;
    }

  if ( this->N <= 0 || this->N >= VTK_LARGE_ID )
    {
    this->N = 0;
    vtkErrorMacro(<<"No field data to plot");
    return 0;
    }

  // We need to loop over the field to determine the range of
  // each independent variable.
  this->Mins = new double [this->N];
  this->Maxs = new double [this->N];
  for (i=0; i<this->N; i++)
    {
    this->Mins[i] =  VTK_DOUBLE_MAX;
    this->Maxs[i] = -VTK_DOUBLE_MAX;
    }

  if ( this->Ranges->size() < static_cast<unsigned int>(this->N) )
    {//ranges not specified
    if ( this->IndependentVariables == VTK_IV_COLUMN )
      {
      k = 0;
      for (j=0; j<numComponents; j++)
        {
        int array_comp, array_index;
        array_index = field->GetArrayContainingComponent(j, array_comp);
        if (array_index < 0 || !field->GetArray(array_index))
          {
          // non-numeric component, simply skip it.
          continue;
          }
        for (i=0; i<numRows; i++)
          {
          //v = field->GetComponent(i,j);
          ::vtkSpiderPlotActorGetComponent(field, i, j, &v);
          if ( v < this->Mins[k] )
            {
            this->Mins[k] = v;
            }
          if ( v > this->Maxs[k] )
            {
            this->Maxs[k] = v;
            }
          }
        k++;
        }
      }
    else //row
      {
      for (j=0; j<numRows; j++)
        {
        for (i=0; i<numComponents; i++)
          {
          //v = field->GetComponent(j,i);
          if (::vtkSpiderPlotActorGetComponent(field,
              j, i, &v) == 0)
            {
            // non-numeric component, simply skip.
            continue;
            }
          if ( v < this->Mins[j] )
            {
            this->Mins[j] = v;
            }
          if ( v > this->Maxs[j] )
            {
            this->Maxs[j] = v;
            }
          }
        }
      }
    for (i=0; i < this->N; i++)
      {
      this->Ranges->push_back(vtkAxisRange(this->Mins[i],this->Maxs[i]));
      }
    }//if automatic range computation
  else//specified correctly
    {
    vtkAxisRange range;
    for (i=0; i < this->N; i++)
      {
      range = this->Ranges->at(i);
      this->Mins[i] = range.Min;
      this->Maxs[i] = range.Max;
      }
    }

  // Get the location of the corners of the box
  double *p1 = this->PositionCoordinate->GetComputedDoubleViewportValue(viewport);
  double *p2 = this->Position2Coordinate->GetComputedDoubleViewportValue(viewport);
  this->P1[0] = (p1[0] < p2[0] ? p1[0] : p2[0]);
  this->P1[1] = (p1[1] < p2[1] ? p1[1] : p2[1]);
  this->P2[0] = (p1[0] > p2[0] ? p1[0] : p2[0]);
  this->P2[1] = (p1[1] > p2[1] ? p1[1] : p2[1]);
  p1 = this->P1;
  p2 = this->P2;

  // Create the spider web
  // Determine the center of the spider plot. Leave room for the title and the legend
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

  this->Center[0] = p1[0] + d1/2.0;
  this->Center[1] = p1[1] + d2/2.0;
  this->Center[2] = 0.0;
  this->Radius = (d1 < d2 ? d1 : d2);
  this->Radius /= 2.0;

  // Now generate the web points
  this->WebData->Initialize(); //remove old polydata, if any
  vtkPoints *webPts = vtkPoints::New();
  webPts->Allocate(this->NumberOfRings*VTK_RING_PTS);
  vtkCellArray *webLines = vtkCellArray::New();
  webLines->Allocate(this->N+this->NumberOfRings,VTK_RING_PTS);
  this->WebData->SetPoints(webPts);
  this->WebData->SetLines(webLines);
  vtkIdType ptId, pIds[VTK_RING_PTS+1];
  double currentTheta, x[3]; x[2] = 0.0;

  // Specify the positions for the axes
  this->Theta = 2.0*vtkMath::Pi()/this->N;
  pIds[0] = webPts->InsertNextPoint(this->Center);
  for (i=0; i<this->N; i++)
    {
    x[0] = this->Center[0] + this->Radius*cos(i*this->Theta);
    x[1] = this->Center[1] + this->Radius*sin(i*this->Theta);
    pIds[1] = webPts->InsertNextPoint(x);
    webLines->InsertNextCell(2,pIds);
    }
  if ( this->NumberOfRings > 0 )
    {
    double currentRadius, deltaRadius = this->Radius / this->NumberOfRings;
    currentTheta = 2.0*vtkMath::Pi()/VTK_RING_PTS;
    for (i=0; i<this->NumberOfRings; i++)
      {
      currentRadius = static_cast<double>(i+1) * deltaRadius;
      for (j=0; j<VTK_RING_PTS; j++)
        {
        x[0] = this->Center[0] + currentRadius*cos(j*currentTheta);
        x[1] = this->Center[1] + currentRadius*sin(j*currentTheta);
        pIds[j] = webPts->InsertNextPoint(x);
        }
      pIds[VTK_RING_PTS] = pIds[0];
      webLines->InsertNextCell(VTK_RING_PTS+1,pIds);
      }
    }

  // Produce labels around the rim of the plot
  int minFontSize=1000, fontSize, tsize[2];
  if ( this->LabelVisibility )
    {
    this->LabelActors = new vtkActor2D* [this->N];
    this->LabelMappers = new vtkTextMapper* [this->N];
    char label[1024];
    const char *str;
    for (i=0; i<this->N; i++)
      {
      x[0] = this->Center[0] + (this->Radius+5)*cos(i*this->Theta);
      x[1] = this->Center[1] + (this->Radius+5)*sin(i*this->Theta);
      this->LabelMappers[i] = vtkTextMapper::New();
      if ( (str=this->GetAxisLabel(i)) != NULL )
        {
        this->LabelMappers[i]->SetInput(str);
        }
      else
        {
        sprintf(label,"%d",static_cast<int>(i));
        this->LabelMappers[i]->SetInput(label);
        }
      this->LabelMappers[i]->GetTextProperty()->
        ShallowCopy(this->LabelTextProperty);
      tsize[0] = static_cast<int>(0.15*d1);
      tsize[1] = static_cast<int>(0.15*d2);
      fontSize = this->LabelMappers[i]->SetConstrainedFontSize(
        viewport, tsize[0], tsize[1]);
      minFontSize = (fontSize < minFontSize ? fontSize : minFontSize);

      this->LabelActors[i] = vtkActor2D::New();
      this->LabelActors[i]->SetMapper(this->LabelMappers[i]);
      this->LabelActors[i]->GetPositionCoordinate()->
        SetCoordinateSystemToViewport();
      this->LabelActors[i]->SetPosition(x);
      // depending on the qudrant, the text is aligned differently
      if ( x[0] >= this->Center[0] && x[1] >= this->Center[1] )
        {
        this->LabelMappers[i]->GetTextProperty()->SetJustificationToLeft();
        this->LabelMappers[i]->GetTextProperty()->SetVerticalJustificationToBottom();
        }
      else if ( x[0] < this->Center[0] && x[1] >= this->Center[1] )
        {
        this->LabelMappers[i]->GetTextProperty()->SetJustificationToRight();
        this->LabelMappers[i]->GetTextProperty()->SetVerticalJustificationToBottom();
        }
      else if ( x[0] < this->Center[0] && x[1] < this->Center[1] )
        {
        this->LabelMappers[i]->GetTextProperty()->SetJustificationToRight();
        this->LabelMappers[i]->GetTextProperty()->SetVerticalJustificationToTop();
        }
      else if ( x[0] >= this->Center[0] && x[1] < this->Center[1] )
        {
        this->LabelMappers[i]->GetTextProperty()->SetJustificationToLeft();
        this->LabelMappers[i]->GetTextProperty()->SetVerticalJustificationToTop();
        }
      }//for all axes
    //Now reset font sizes to the same value
    for (i=0; i<this->N; i++)
      {
      this->LabelMappers[i]->GetTextProperty()->SetFontSize(minFontSize);
      }
    }

  // Now generate the lines to plot
  this->PlotData->Initialize(); //remove old polydata, if any
  vtkPoints *pts = vtkPoints::New();
  pts->Allocate(numRows*numColumns);
  vtkCellArray *lines = vtkCellArray::New();
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  this->PlotData->SetPoints(pts);
  this->PlotData->SetLines(lines);
  this->PlotData->GetCellData()->SetScalars(colors);
  colors->Delete();

  this->LegendActor->GetProperty()->DeepCopy(this->GetProperty());

  double r, *color;
  vtkIdType firstId=0;
  char buf[1024];
  if ( this->IndependentVariables == VTK_IV_COLUMN )
    {
    lines->Allocate(lines->EstimateSize(numRows,numColumns));
    for (j=0; j<numRows; j++)
      {
      lines->InsertNextCell(numColumns+1);
      color = this->LegendActor->GetEntryColor(j);
      colors->InsertNextTuple3(255*color[0],255*color[1],255*color[2]);
      this->LegendActor->SetEntrySymbol(j,this->GlyphSource->GetOutput());
      sprintf(buf,"%d",static_cast<int>(j));
      this->LegendActor->SetEntryString(j,buf);
      for (i=0,k=0; i<numColumns && k < numComponents; k++)
        {
        if (::vtkSpiderPlotActorGetComponent(field, j, k, &v) == 0)
          {
          // skip non-numeric components.
          continue;
          }
        if ( (this->Maxs[i]-this->Mins[i]) == 0.0 )
          {
          r = 0.0;
          }
        else
          {
          r = this->Radius*(v - this->Mins[i]) / (this->Maxs[i] - this->Mins[i]);
          }
        x[0]  = this->Center[0] + r * cos(i*this->Theta);
        x[1]  = this->Center[1] + r * sin(i*this->Theta);
        ptId = pts->InsertNextPoint(x);
        firstId = (i==0 ? ptId : firstId);
        lines->InsertCellPoint(ptId);
        i++;
        }
      lines->InsertCellPoint(firstId);
      }
    }
  else //row
    {
    lines->Allocate(lines->EstimateSize(numColumns,numRows));
    for (j=0; j<numComponents; j++)
      {
      int array_comp;
      int array_index = field->GetArrayContainingComponent(j, array_comp);
      if (!field->GetArray(array_index))
        {
        // non-numeric component, skip it.
        continue;
        }
      lines->InsertNextCell(numColumns+1);
      color = this->LegendActor->GetEntryColor(j);
      colors->InsertNextTuple3(255*color[0],255*color[1],255*color[2]);
      this->LegendActor->SetEntrySymbol(j,this->GlyphSource->GetOutput());
      sprintf(buf,"%d",static_cast<int>(j));
      this->LegendActor->SetEntryString(j,buf);
      for (i=0; i<numRows; i++)
        {
        vtkSpiderPlotActorGetComponent(field, i, j, &v);
        if ( (this->Maxs[i]-this->Mins[i]) == 0.0 )
          {
          r = 0.0;
          }
        else
          {
          r = this->Radius*(v - this->Mins[i]) / (this->Maxs[i] - this->Mins[i]);
          }
        x[0]  = this->Center[0] + r * cos(i*this->Theta);
        x[1]  = this->Center[1] + r * sin(i*this->Theta);
        ptId = pts->InsertNextPoint(x);
        firstId = (i==0 ? ptId : firstId);
        lines->InsertCellPoint(ptId);
        }
      lines->InsertCellPoint(firstId);
      }
    }

  //Display the legend
  if ( this->LegendVisibility )
    {
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
    SetValue(this->Center[0],this->Center[1]+this->Radius+tsize[1]);
  this->TitleActor->SetProperty(this->GetProperty());

  //Clean up
  webPts->Delete();
  webLines->Delete();
  pts->Delete();
  lines->Delete();

  return 1;
}
#undef VTK_RING_PTS

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkSpiderPlotActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  this->LegendActor->ReleaseGraphicsResources(win);
  this->WebActor->ReleaseGraphicsResources(win);
  this->PlotActor->ReleaseGraphicsResources(win);
  for (int i=0; this->LabelActors && i<this->N; i++)
    {
    this->LabelActors[i]->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
void vtkSpiderPlotActor::SetAxisLabel(const int i, const char *label)
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
const char* vtkSpiderPlotActor::GetAxisLabel(int i)
{
  if ( i < 0 )
    {
    return NULL;
    }

  return this->Labels->at(i).c_str();
}

//----------------------------------------------------------------------------
void vtkSpiderPlotActor::SetAxisRange(int i, double min, double max)
{
  if ( i < 0 )
    {
    return;
    }

  if ( static_cast<unsigned int>(i) >= this->Ranges->size() )
    {
    this->Ranges->resize(i+1);
    }
  (*this->Ranges)[i] = vtkAxisRange(min,max);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSpiderPlotActor::SetAxisRange(int i, double range[2])
{
  this->SetAxisRange(i, range[0],range[1]);
}

//----------------------------------------------------------------------------
void vtkSpiderPlotActor::GetAxisRange(int i, double range[2])
{
  if ( i < 0 )
    {
    return;
    }

  vtkAxisRange arange = this->Ranges->at(i);
  range[0] = arange.Min;
  range[1] = arange.Max;
}


//----------------------------------------------------------------------------
void vtkSpiderPlotActor::SetPlotColor(int i, double r, double g, double b)
{
  this->LegendActor->SetEntryColor(i, r, g, b);
}

//----------------------------------------------------------------------------
double *vtkSpiderPlotActor::GetPlotColor(int i)
{
  return this->LegendActor->GetEntryColor(i);
}

//----------------------------------------------------------------------------
void vtkSpiderPlotActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";

  os << indent << "Number Of Independent Variables: " << this->N << "\n";
  os << indent << "Independent Variables: ";
  if ( this->IndependentVariables == VTK_IV_COLUMN )
    {
    os << "Columns\n";
    }
  else
    {
    os << "Rows\n";
    }

  os << indent << "Title Visibility: "
     << (this->TitleVisibility ? "On\n" : "Off\n");

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";

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

  os << indent << "Number of Rings: " << this->NumberOfRings << "\n";

  os << indent << "Legend Visibility: "
     << (this->LegendVisibility ? "On\n" : "Off\n");

  os << indent << "Legend Actor: "
     << this->LegendActor << "\n";
  this->LegendActor->PrintSelf(os, indent.GetNextIndent());

}
