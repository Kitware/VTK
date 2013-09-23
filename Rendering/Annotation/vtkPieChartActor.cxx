/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieChartActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPieChartActor.h"

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
#include "vtkTrivialProducer.h"

#include <string>
#include <vector>

vtkStandardNewMacro(vtkPieChartActor);

vtkCxxSetObjectMacro(vtkPieChartActor,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkPieChartActor,TitleTextProperty,vtkTextProperty);

// PIMPL'd list of labels
class vtkPieceLabelArray : public std::vector<std::string> {};

class vtkPieChartActorConnection : public vtkAlgorithm
{
public:
  static vtkPieChartActorConnection *New();
  vtkTypeMacro(vtkPieChartActorConnection,vtkAlgorithm);

  vtkPieChartActorConnection()
    {
      this->SetNumberOfInputPorts(1);
    }
};

vtkStandardNewMacro(vtkPieChartActorConnection);


//----------------------------------------------------------------------------
// Instantiate object
vtkPieChartActor::vtkPieChartActor()
{
  // Actor2D positions
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.1,0.1);
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.9, 0.8);
  this->Position2Coordinate->SetReferenceCoordinate(NULL);

  this->ConnectionHolder = vtkPieChartActorConnection::New();

  this->ArrayNumber = 0;
  this->ComponentNumber = 0;
  this->TitleVisibility = 1;
  this->Title = NULL;
  this->Labels = new vtkPieceLabelArray;
  this->PieceMappers = NULL;
  this->PieceActors = NULL;
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
  this->Total = 0.0;
  this->Fractions = NULL;

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
vtkPieChartActor::~vtkPieChartActor()
{
  this->ConnectionHolder->Delete();
  this->ConnectionHolder = 0;

  delete [] this->Title;
  this->Title = NULL;

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

  this->WebData->Delete();
  this->WebMapper->Delete();
  this->WebActor->Delete();

  this->PlotData->Delete();
  this->PlotMapper->Delete();
  this->PlotActor->Delete();
}

//----------------------------------------------------------------------------
void vtkPieChartActor::SetInputConnection(vtkAlgorithmOutput* ao)
{
  this->ConnectionHolder->SetInputConnection(ao);
}

//----------------------------------------------------------------------------
void vtkPieChartActor::SetInputData(vtkDataObject* dobj)
{
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(dobj);
  this->SetInputConnection(tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPieChartActor::GetInput()
{
  return this->ConnectionHolder->GetInputDataObject(0, 0);
}

//----------------------------------------------------------------------------
// Free-up axes and related stuff
void vtkPieChartActor::Initialize()
{
  if ( this->PieceActors )
    {
    for (int i=0; i<this->N; i++)
      {
      this->PieceMappers[i]->Delete();
      this->PieceActors[i]->Delete();
      }
    delete [] this->PieceMappers;
    this->PieceMappers = NULL;
    delete [] this->PieceActors;
    this->PieceActors = NULL;
    }

  this->N = 0;
  this->Total = 0.0;
  delete [] this->Fractions;
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int vtkPieChartActor::RenderOverlay(vtkViewport *viewport)
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
  renderedSomething += this->PlotActor->RenderOverlay(viewport);
  renderedSomething += this->WebActor->RenderOverlay(viewport);

  if ( this->LabelVisibility )
    {
    for (int i=0; i<this->N; i++)
      {
      renderedSomething += this->PieceActors[i]->RenderOverlay(viewport);
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
int vtkPieChartActor::RenderOpaqueGeometry(vtkViewport *viewport)
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
  renderedSomething += this->PlotActor->RenderOpaqueGeometry(viewport);
  renderedSomething += this->WebActor->RenderOpaqueGeometry(viewport);

  if ( this->LabelVisibility )
    {
    for (int i=0; i<this->N; i++)
      {
      renderedSomething += this->PieceActors[i]->RenderOpaqueGeometry(viewport);
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
int vtkPieChartActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}


//-----------------------------------------------------------------------------
int vtkPieChartActor::BuildPlot(vtkViewport *viewport)
{
  // Initialize
  vtkDebugMacro(<<"Building pie chart plot");

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
int vtkPieChartActor::PlaceAxes(vtkViewport *viewport, int* vtkNotUsed(size))
{
  vtkIdType i, j;
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
  if ( this->N <= 0 || this->N >= VTK_ID_MAX )
    {
    this->N = 0;
    vtkErrorMacro(<<"No field data to plot");
    return 0;
    }

  // We need to loop over the field to determine the total
  this->Total = 0.0;
  this->Fractions = new double[this->N];
  for (i=0; i<this->N; i++)
    {
    v = fabs(da->GetComponent(i,this->ComponentNumber));
    this->Fractions[i] = v;
    this->Total += v;
    }
  if ( this->Total > 0.0 )
    {
    double total=0.0;
    for (i=0; i<this->N; i++)
      {
      total += this->Fractions[i];
      this->Fractions[i] = total/this->Total;
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

  // Create the borders of the pie pieces.
  // Determine the center of the pie. Leave room for the title and the legend.
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
  webPts->Allocate(this->N+1);
  vtkCellArray *webLines = vtkCellArray::New();
  webLines->Allocate(this->N,2);
  this->WebData->SetPoints(webPts);
  this->WebData->SetLines(webLines);
  vtkIdType ptId, pIds[2];
  double theta, x[3]; x[2] = 0.0;

  // Specify the positions for the axes
  pIds[0] = webPts->InsertNextPoint(this->Center);
  for (i=0; i<this->N; i++)
    {
    theta = this->Fractions[i] * 2.0*vtkMath::Pi();
    x[0] = this->Center[0] + this->Radius*cos(theta);
    x[1] = this->Center[1] + this->Radius*sin(theta);
    pIds[1] = webPts->InsertNextPoint(x);
    webLines->InsertNextCell(2,pIds);
    }

  // Draw a bounding ring
  webLines->InsertNextCell(65);
  theta = 2.0*vtkMath::Pi() / 64;
  for (j=0; j<65; j++)
    {
    x[0] = this->Center[0] + this->Radius*cos(j*theta);
    x[1] = this->Center[1] + this->Radius*sin(j*theta);
    ptId = webPts->InsertNextPoint(x);
    webLines->InsertCellPoint(ptId);
    }

  // Produce labels around the rim of the plot
  double thetaM;
  char label[1024];
  const char *str;
  int minFontSize=1000, fontSize, tsize[2];
  if ( this->LabelVisibility )
    {
    this->PieceActors = new vtkActor2D* [this->N];
    this->PieceMappers = new vtkTextMapper* [this->N];
    for (i=0; i<this->N; i++)
      {
      thetaM = (i==0 ? 0.0 : this->Fractions[i-1] * 2.0*vtkMath::Pi());
      theta = this->Fractions[i] * 2.0*vtkMath::Pi();
      x[0] = this->Center[0] + (this->Radius+5)*cos((theta+thetaM)/2.0);
      x[1] = this->Center[1] + (this->Radius+5)*sin((theta+thetaM)/2.0);
      this->PieceMappers[i] = vtkTextMapper::New();
      if ( (str=this->GetPieceLabel(i)) != NULL )
        {
        this->PieceMappers[i]->SetInput(str);
        }
      else
        {
        sprintf(label,"%d",static_cast<int>(i));
        this->PieceMappers[i]->SetInput(label);
        }
      this->PieceMappers[i]->GetTextProperty()->
        ShallowCopy(this->LabelTextProperty);
      tsize[0] = static_cast<int>(0.15*d1);
      tsize[1] = static_cast<int>(0.15*d2);
      fontSize = this->PieceMappers[i]->SetConstrainedFontSize(
        viewport, tsize[0], tsize[1]);
      minFontSize = (fontSize < minFontSize ? fontSize : minFontSize);
      this->PieceActors[i] = vtkActor2D::New();
      this->PieceActors[i]->SetMapper(this->PieceMappers[i]);
      this->PieceActors[i]->GetPositionCoordinate()->
        SetCoordinateSystemToViewport();
      this->PieceActors[i]->SetPosition(x);
      // depending on the qudrant, the text is aligned differently
      if ( x[0] >= this->Center[0] && x[1] >= this->Center[1] )
        {
        this->PieceMappers[i]->GetTextProperty()->SetJustificationToLeft();
        this->PieceMappers[i]->GetTextProperty()->SetVerticalJustificationToBottom();
        }
      else if ( x[0] < this->Center[0] && x[1] >= this->Center[1] )
        {
        this->PieceMappers[i]->GetTextProperty()->SetJustificationToRight();
        this->PieceMappers[i]->GetTextProperty()->SetVerticalJustificationToBottom();
        }
      else if ( x[0] < this->Center[0] && x[1] < this->Center[1] )
        {
        this->PieceMappers[i]->GetTextProperty()->SetJustificationToRight();
        this->PieceMappers[i]->GetTextProperty()->SetVerticalJustificationToTop();
        }
      else if ( x[0] >= this->Center[0] && x[1] < this->Center[1] )
        {
        this->PieceMappers[i]->GetTextProperty()->SetJustificationToLeft();
        this->PieceMappers[i]->GetTextProperty()->SetVerticalJustificationToTop();
        }
      }//for all pieces of pie
    //Now reset font sizes to the same value
    for (i=0; i<this->N; i++)
      {
      this->PieceMappers[i]->GetTextProperty()->SetFontSize(minFontSize);
      }
    }

  // Now generate the pie polygons
  this->PlotData->Initialize(); //remove old polydata, if any
  vtkPoints *pts = vtkPoints::New();
  pts->Allocate(this->N*2);
  vtkCellArray *polys = vtkCellArray::New();
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  this->PlotData->SetPoints(pts);
  this->PlotData->SetPolys(polys);
  this->PlotData->GetCellData()->SetScalars(colors);
  colors->Delete();

  double *color, delTheta;
  vtkIdType numDivs;
  polys->Allocate(polys->EstimateSize(this->N,12));

  pIds[0] = pts->InsertNextPoint(this->Center);
  for (i=0; i<this->N; i++)
    {
    thetaM = (i==0 ? 0.0 : this->Fractions[i-1] * 2.0*vtkMath::Pi());
    theta = this->Fractions[i] * 2.0*vtkMath::Pi();
    numDivs = static_cast<vtkIdType>(32 * (theta-thetaM) / vtkMath::Pi());
    numDivs = (numDivs < 2 ? 2 : numDivs);
    delTheta = (theta - thetaM) / numDivs;

    polys->InsertNextCell(numDivs+2);
    polys->InsertCellPoint(pIds[0]);
    color = this->LegendActor->GetEntryColor(i);
    colors->InsertNextTuple3(255*color[0],255*color[1],255*color[2]);
    this->LegendActor->SetEntrySymbol(i,this->GlyphSource->GetOutput());
    if ( (str=this->GetPieceLabel(i)) != NULL )
      {
      this->LegendActor->SetEntryString(i,str);
      }
    else
      {
      sprintf(label,"%d",static_cast<int>(i));
      this->LegendActor->SetEntryString(i,label);
      }

    for (j=0; j<=numDivs; j++)
      {
      theta = thetaM + j*delTheta;
      x[0]  = this->Center[0] + this->Radius * cos(theta);
      x[1]  = this->Center[1] + this->Radius * sin(theta);
      ptId = pts->InsertNextPoint(x);
      polys->InsertCellPoint(ptId);
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
    SetValue(this->Center[0],this->Center[1]+this->Radius+tsize[1]);
  this->TitleActor->SetProperty(this->GetProperty());

  //Clean up
  webPts->Delete();
  webLines->Delete();
  pts->Delete();
  polys->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkPieChartActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  this->LegendActor->ReleaseGraphicsResources(win);
  this->WebActor->ReleaseGraphicsResources(win);
  this->PlotActor->ReleaseGraphicsResources(win);
  for (int i=0; this->PieceActors && i<this->N; i++)
    {
    this->PieceActors[i]->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
void vtkPieChartActor::SetPieceLabel(const int i, const char *label)
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
const char* vtkPieChartActor::GetPieceLabel(int i)
{
  if ( i < 0 || static_cast<unsigned int>(i) >= this->Labels->size())
    {
    return NULL;
    }

  return this->Labels->at(i).c_str();
}

//----------------------------------------------------------------------------
void vtkPieChartActor::SetPieceColor(int i, double r, double g, double b)
{
  this->LegendActor->SetEntryColor(i, r, g, b);
}

//----------------------------------------------------------------------------
double *vtkPieChartActor::GetPieceColor(int i)
{
  return this->LegendActor->GetEntryColor(i);
}

//----------------------------------------------------------------------------
void vtkPieChartActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";

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

  os << indent << "Legend Visibility: "
     << (this->LegendVisibility ? "On\n" : "Off\n");

  os << indent << "Legend Actor: "
     << this->LegendActor << "\n";
  this->LegendActor->PrintSelf(os, indent.GetNextIndent());

}
