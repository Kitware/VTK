/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXYPlotActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXYPlotActor.h"

#include "vtkAppendPolyData.h"
#include "vtkAxisActor2D.h"
#include "vtkCellArray.h"
#include "vtkDataObjectCollection.h"
#include "vtkDataSetCollection.h"
#include "vtkFieldData.h"
#include "vtkDoubleArray.h"
#include "vtkGlyph2D.h"
#include "vtkGlyphSource2D.h"
#include "vtkIntArray.h"
#include "vtkLegendBoxActor.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

#define VTK_MAX_PLOTS 50

vtkStandardNewMacro(vtkXYPlotActor);

vtkCxxSetObjectMacro(vtkXYPlotActor,TitleTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkXYPlotActor,AxisLabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkXYPlotActor,AxisTitleTextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Instantiate object
vtkXYPlotActor::vtkXYPlotActor()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.25,0.25);
  this->Position2Coordinate->SetValue(0.5, 0.5);

  this->InputList = vtkDataSetCollection::New();
  this->SelectedInputScalars = NULL;
  this->SelectedInputScalarsComponent = vtkIntArray::New();
  this->DataObjectInputList = vtkDataObjectCollection::New();

  this->Title = NULL;
  this->XTitle = new char[7];
  sprintf(this->XTitle,"%s","X Axis");
  this->YTitle = new char[7];
  sprintf(this->YTitle,"%s","Y Axis");

  this->XValues = VTK_XYPLOT_INDEX;

  this->NumberOfXLabels = 5;
  this->NumberOfYLabels = 5;

  this->TitleTextProperty = vtkTextProperty::New();
  this->TitleTextProperty->SetBold(1);
  this->TitleTextProperty->SetItalic(1);
  this->TitleTextProperty->SetShadow(1);
  this->TitleTextProperty->SetFontFamilyToArial();

  this->AxisLabelTextProperty = vtkTextProperty::New();
  this->AxisLabelTextProperty->ShallowCopy(this->TitleTextProperty);

  this->AxisTitleTextProperty = vtkTextProperty::New();
  this->AxisTitleTextProperty->ShallowCopy(this->AxisLabelTextProperty);

  this->XLabelFormat = new char[8]; 
  sprintf(this->XLabelFormat,"%s","%-#6.3g");

  this->YLabelFormat = new char[8]; 
  sprintf(this->YLabelFormat,"%s","%-#6.3g");

  this->Logx = 0;
  
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
  this->XAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->XAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->XAxis->SetProperty(this->GetProperty());

  this->YAxis = vtkAxisActor2D::New();
  this->YAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->YAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
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
  vtkDoubleArray *n = vtkDoubleArray::New();
  n->SetNumberOfComponents(3);
  n->SetNumberOfTuples(4);
  this->ClipPlanes->SetNormals(n);
  n->Delete();

  // Construct the box
  this->ChartBox = 0;
  this->ChartBoxPolyData = vtkPolyData::New();
  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(4);
  this->ChartBoxPolyData->SetPoints(points);points->Delete();
  vtkCellArray *polys = vtkCellArray::New();
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->ChartBoxPolyData->SetPolys(polys); polys->Delete();
  this->ChartBoxMapper = vtkPolyDataMapper2D::New();
  this->ChartBoxMapper->SetInput(this->ChartBoxPolyData);
  this->ChartBoxActor = vtkActor2D::New();
  this->ChartBoxActor->SetMapper(this->ChartBoxMapper);
  // Box border
  this->ChartBorder = 0;
  this->ChartBorderPolyData = vtkPolyData::New();
  this->ChartBorderPolyData->SetPoints(points);
  vtkCellArray *lines = vtkCellArray::New();
  lines->InsertNextCell(5);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertCellPoint(0);
  this->ChartBorderPolyData->SetLines(lines); lines->Delete();
  this->ChartBorderMapper = vtkPolyDataMapper2D::New();
  this->ChartBorderMapper->SetInput(this->ChartBorderPolyData);
  this->ChartBorderActor = vtkActor2D::New();
  this->ChartBorderActor->SetMapper(this->ChartBorderMapper);

  // Reference lines
  this->ShowReferenceXLine = 0;
  this->ShowReferenceYLine = 0;
  this->ReferenceXValue = 0.;
  this->ReferenceYValue = 0.;
  points = vtkPoints::New();
  points->SetNumberOfPoints(4);
  lines = vtkCellArray::New();
  lines->InsertNextCell(2);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertNextCell(2);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  this->ReferenceLinesPolyData = vtkPolyData::New();
  this->ReferenceLinesPolyData->SetPoints(points);
  this->ReferenceLinesPolyData->SetLines(lines);
  points->Delete();
  lines->Delete();
  this->ReferenceLinesMapper = vtkPolyDataMapper2D::New();
  this->ReferenceLinesMapper->SetInput(this->ReferenceLinesPolyData);
  this->ReferenceLinesActor = vtkActor2D::New();
  this->ReferenceLinesActor->SetMapper(this->ReferenceLinesMapper);

  this->CachedSize[0] = 0;
  this->CachedSize[1] = 0;

  this->AdjustXLabels = 1;
  this->AdjustYLabels = 1;
  this->AdjustTitlePosition = 1;
  this->TitlePosition[0] = 0.5;
  this->TitlePosition[1] = 0.9;
  this->AdjustTitlePositionMode = vtkXYPlotActor::AlignHCenter
                                  | vtkXYPlotActor::AlignTop 
                                  | vtkXYPlotActor::AlignAxisHCenter
                                  | vtkXYPlotActor::AlignAxisVCenter;
  
}

//----------------------------------------------------------------------------
vtkXYPlotActor::~vtkXYPlotActor()
{
  // Get rid of the list of array names.
  int num = this->InputList->GetNumberOfItems();
  if (this->SelectedInputScalars)
    {
    for (int i = 0; i < num; ++i)
      {
      if (this->SelectedInputScalars[i])
        {
        delete [] this->SelectedInputScalars[i];
        this->SelectedInputScalars[i] = NULL;
        }
      }
    delete [] this->SelectedInputScalars;
    this->SelectedInputScalars = NULL;  
    }
  this->SelectedInputScalarsComponent->Delete();
  this->SelectedInputScalarsComponent = NULL;

  //  Now we can get rid of the inputs. 
  this->InputList->Delete();
  this->InputList = NULL;

  this->DataObjectInputList->Delete();
  this->DataObjectInputList = NULL;

  this->TitleMapper->Delete();
  this->TitleMapper = NULL;
  this->TitleActor->Delete();
  this->TitleActor = NULL;

  this->SetTitle(0);
  this->SetXTitle(0);
  this->SetYTitle(0);
  this->SetXLabelFormat(0);
  this->SetYLabelFormat(0);

  this->XAxis->Delete();
  this->YAxis->Delete();
  
  this->InitializeEntries();

  this->LegendActor->Delete();
  this->GlyphSource->Delete();
  this->ClipPlanes->Delete();
  
  this->ChartBoxActor->Delete();
  this->ChartBoxMapper->Delete();
  this->ChartBoxPolyData->Delete();

  this->ChartBorderActor->Delete();
  this->ChartBorderMapper->Delete();
  this->ChartBorderPolyData->Delete();
  
  this->ReferenceLinesActor->Delete();
  this->ReferenceLinesMapper->Delete();
  this->ReferenceLinesPolyData->Delete();

  this->XComponent->Delete();
  this->YComponent->Delete();

  this->LinesOn->Delete();
  this->PointsOn->Delete();

  this->SetTitleTextProperty(NULL);
  this->SetAxisLabelTextProperty(NULL);
  this->SetAxisTitleTextProperty(NULL);
}

//----------------------------------------------------------------------------
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
  
//----------------------------------------------------------------------------
// Add a dataset and array to the list of data to plot.
void vtkXYPlotActor::AddInput(vtkDataSet *ds, const char *arrayName, int component)
{
  int idx, num;
  char** newNames;

  // I cannot change the input list, because the user has direct 
  // access to the collection.  I cannot store the index of the array, 
  // because the index might change from render to render ...
  // I have to store the list of string array names.

  // I believe idx starts at 1 and goes to "NumberOfItems".
  idx = this->InputList->IsItemPresent(ds);
  if (idx > 0)
    { // Return if arrays are the same.
    if (arrayName == NULL && this->SelectedInputScalars[idx-1] == NULL &&
        component == this->SelectedInputScalarsComponent->GetValue(idx-1))
      {
      return;
      }
    if (arrayName != NULL && this->SelectedInputScalars[idx-1] != NULL &&
        strcmp(arrayName, this->SelectedInputScalars[idx-1]) == 0 &&
        component == this->SelectedInputScalarsComponent->GetValue(idx-1))
      {
      return;
      }
    }

  // The input/array/component must be a unique combination.  Add it to our input list.

  // Now reallocate the list of strings and add the new value.
  num = this->InputList->GetNumberOfItems();
  newNames = new char*[num+1];
  for (idx = 0; idx < num; ++idx)
    {
    newNames[idx] = this->SelectedInputScalars[idx];
    }
  if (arrayName == NULL)
    {
    newNames[num] = NULL;
    }
  else
    {
    newNames[num] = new char[strlen(arrayName)+1];
    strcpy(newNames[num],arrayName);
    }
  delete [] this->SelectedInputScalars;
  this->SelectedInputScalars = newNames;

  // Save the component in the int array.
  this->SelectedInputScalarsComponent->InsertValue(num, component);

  // Add the data set to the collection
  this->InputList->AddItem(ds);

  // In case of multiple use of a XYPlotActor the NumberOfEntries could be set
  // to n. Then when a call to SetEntryString(n+1, bla) was done the string was lost
  // Need to update the number of entries for the legend actor
  this->LegendActor->SetNumberOfEntries(this->LegendActor->GetNumberOfEntries()+1);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::RemoveAllInputs()
{
  int idx, num;

  num = this->InputList->GetNumberOfItems();
  this->InputList->RemoveAllItems();

  for (idx = 0; idx < num; ++idx)
    {
    if (this->SelectedInputScalars[idx])
      {
      delete [] this->SelectedInputScalars[idx];
      this->SelectedInputScalars[idx] = NULL;
      }
    }
  this->SelectedInputScalarsComponent->Reset();

  this->DataObjectInputList->RemoveAllItems();
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to plot.
void vtkXYPlotActor::RemoveInput(vtkDataSet *ds, const char *arrayName, int component)
{
  int idx, num;
  vtkDataSet *input;
  int found = -1;

  // This is my own find routine, because the array names have to match also.
  num = this->InputList->GetNumberOfItems();
  vtkCollectionSimpleIterator dsit;
  this->InputList->InitTraversal(dsit);
  for (idx = 0; idx < num && found == -1; ++idx)
    {
    input = this->InputList->GetNextDataSet(dsit);
    if (input == ds)
      {
      if (arrayName == NULL && this->SelectedInputScalars[idx] == NULL &&
          component == this->SelectedInputScalarsComponent->GetValue(idx))
        {
        found = idx;
        }
      if (arrayName != NULL && this->SelectedInputScalars[idx] != NULL &&
          strcmp(arrayName, this->SelectedInputScalars[idx]) == 0 &&
          component == this->SelectedInputScalarsComponent->GetValue(idx))
        {
        found = idx;
        }
      }
    }

  if (found == -1)
    {
    return;
    }
  
  this->Modified();
  // Collections index their items starting at 1.
  this->InputList->RemoveItem(found);

  // Do not bother reallocating the SelectedInputScalars 
  // string array to make it smaller.
  if (this->SelectedInputScalars[found])
    {
    delete [] this->SelectedInputScalars[found];
    this->SelectedInputScalars[found] = NULL;
    }
  for (idx = found+1; idx < num; ++idx)
    {
    this->SelectedInputScalars[idx-1] = this->SelectedInputScalars[idx];
    this->SelectedInputScalarsComponent->SetValue(idx-1, 
                          this->SelectedInputScalarsComponent->GetValue(idx));
    }
  // Reseting the last item is not really necessary, 
  // but to be clean we do it anyway.
  this->SelectedInputScalarsComponent->SetValue(num-1, -1); 
  this->SelectedInputScalars[num-1] = NULL;
}

//----------------------------------------------------------------------------
// Add a data object to the list of data to plot.
void vtkXYPlotActor::AddDataObjectInput(vtkDataObject *in)
{
  if ( ! this->DataObjectInputList->IsItemPresent(in) )
    {
    this->Modified();
    this->DataObjectInputList->AddItem(in);
    }
}

//----------------------------------------------------------------------------
// Remove a data object from the list of data to plot.
void vtkXYPlotActor::RemoveDataObjectInput(vtkDataObject *in)
{
  if ( this->DataObjectInputList->IsItemPresent(in) )
    {
    this->Modified();
    this->DataObjectInputList->RemoveItem(in);
    }
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int vtkXYPlotActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething = 0;

  // Make sure input is up to date.
  if ( this->InputList->GetNumberOfItems() < 1 && 
       this->DataObjectInputList->GetNumberOfItems() < 1 )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  if ( this->ChartBox )
    {
    renderedSomething += this->ChartBoxActor->RenderOverlay(viewport);
    }
  if ( this->ChartBorder )
    {
    renderedSomething += this->ChartBorderActor->RenderOverlay(viewport);
    }

  renderedSomething += this->XAxis->RenderOverlay(viewport);
  renderedSomething += this->YAxis->RenderOverlay(viewport);
  if ( this->Title )
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }
  for (int i=0; i < this->NumberOfInputs; i++)
    {
    renderedSomething += this->PlotActor[i]->RenderOverlay(viewport);
    }
  if ( this->ShowReferenceXLine || this->ShowReferenceYLine )
    {
    renderedSomething += this->ReferenceLinesActor->RenderOverlay(viewport);
    }
  if ( this->Legend )
    {
    renderedSomething += this->LegendActor->RenderOverlay(viewport);
    }

  return renderedSomething;
}

//----------------------------------------------------------------------------
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
    vtkCollectionSimpleIterator dsit;
    for (mtime=0, this->InputList->InitTraversal(dsit); 
         (ds = this->InputList->GetNextDataSet(dsit)); )
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
    vtkCollectionSimpleIterator doit;
    for (mtime=0, this->DataObjectInputList->InitTraversal(doit); 
         (dobj = this->DataObjectInputList->GetNextDataObject(doit)); )
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

  if (this->Title && this->Title[0] && !this->TitleTextProperty)
    {
    vtkErrorMacro(<< "Need a title text property to render plot title");
    return 0;
    }

  // Check modified time to see whether we have to rebuild.
  // Pay attention that GetMTime() has been redefined (see below)

  int *size=viewport->GetSize();
  if (mtime > this->BuildTime || 
      size[0] != this->CachedSize[0] || size[1] != this->CachedSize[1] ||
      this->GetMTime() > this->BuildTime ||
      (this->Title && this->Title[0] && 
       this->TitleTextProperty->GetMTime() > this->BuildTime) ||
      (this->AxisLabelTextProperty &&
       this->AxisLabelTextProperty->GetMTime() > this->BuildTime) ||
      (this->AxisTitleTextProperty &&
       this->AxisTitleTextProperty->GetMTime() > this->BuildTime))
    {
    double range[2], yrange[2], xRange[2], yRange[2], interval, *lengths=NULL;
    int pos[2], pos2[2], numTicks;
    int stringSize[2];
    int num = ( numDS > 0 ? numDS : numDO );

    vtkDebugMacro(<<"Rebuilding plot");
    this->CachedSize[0] = size[0];
    this->CachedSize[1] = size[1];

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
      
      this->LegendActor->GetPositionCoordinate()->SetValue(
        (double)legPos[0], (double)legPos[1]);
      this->LegendActor->GetPosition2Coordinate()->SetValue(
        (double)legPos2[0], (double)legPos2[1]);
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

    // Rebuid text props
    // Perform shallow copy here since each individual axis can be
    // accessed through the class API (i.e. each individual axis text prop
    // can be changed). Therefore, we can not just assign pointers otherwise
    // each individual axis text prop would point to the same text prop.

    if (this->AxisLabelTextProperty &&
        this->AxisLabelTextProperty->GetMTime() > this->BuildTime)
      {
      if (this->XAxis->GetLabelTextProperty())
        {
        this->XAxis->GetLabelTextProperty()->ShallowCopy(
          this->AxisLabelTextProperty);
        }
      if (this->YAxis->GetLabelTextProperty())
        {
        this->YAxis->GetLabelTextProperty()->ShallowCopy(
          this->AxisLabelTextProperty);
        }
      }
    
    if (this->AxisTitleTextProperty &&
        this->AxisTitleTextProperty->GetMTime() > this->BuildTime)
      {
      if (this->XAxis->GetTitleTextProperty())
        {
        this->XAxis->GetTitleTextProperty()->ShallowCopy(
          this->AxisTitleTextProperty);
        }
      if (this->YAxis->GetTitleTextProperty())
        {
        this->YAxis->GetTitleTextProperty()->ShallowCopy(
          this->AxisTitleTextProperty);
        }
      }
    
    // setup x-axis
    vtkDebugMacro(<<"Rebuilding x-axis");
    
    this->XAxis->SetTitle(this->XTitle);
    this->XAxis->SetNumberOfLabels(this->NumberOfXLabels);
    this->XAxis->SetProperty(this->GetProperty());

    lengths = new double[num];
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
    
    if ( this->AdjustXLabels )
      {
      vtkAxisActor2D::ComputeRange(range, xRange, this->NumberOfXLabels,
                                   numTicks, interval);
      }
    else
      {
      xRange[0] = range[0];
      xRange[1] = range[1];
      }

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
    
    // setup y-axis
    vtkDebugMacro(<<"Rebuilding y-axis");
    this->YAxis->SetTitle(this->YTitle);
    this->YAxis->SetNumberOfLabels(this->NumberOfYLabels);

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

    if ( this->AdjustYLabels )
      {
      vtkAxisActor2D::ComputeRange(yrange, yRange, this->NumberOfYLabels,
                                   numTicks, interval);
      }
    else
      {
      yRange[0] = yrange[0];
      yRange[1] = yrange[1];
      }

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
        this->YAxis->SetRange(yrange[1],yrange[0]);
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

    this->PlaceAxes(viewport, size, pos, pos2);
    
    // manage title
    if (this->Title != NULL && this->Title[0])
      {
      this->TitleMapper->SetInput(this->Title);
      if (this->TitleTextProperty->GetMTime() > this->BuildTime)
        {
        this->TitleMapper->GetTextProperty()->ShallowCopy(
          this->TitleTextProperty);
        }

      vtkTextMapper::SetRelativeFontSize(this->TitleMapper, viewport, 
                                         size, stringSize, 0.015);

      if (this->AdjustTitlePosition)
        {
        this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
        double titlePos[2];
        switch (this->AdjustTitlePositionMode & (AlignLeft | AlignRight | AlignHCenter))
          {
          default:
          case AlignLeft:
            titlePos[0] = pos[0];
            break;
          case AlignRight:
            titlePos[0] = pos2[0];
            break;
          case AlignHCenter:
            titlePos[0] = pos[0] + 0.5 * (pos2[0] - pos[0]);
            break;
          };
        switch (this->AdjustTitlePositionMode & (AlignAxisLeft | AlignAxisRight | AlignAxisHCenter))
          {
          case AlignAxisLeft:
            titlePos[0] -= stringSize[0];
            break;
          case AlignAxisRight:
            break;
          case AlignAxisHCenter:
            titlePos[0] -= stringSize[0] / 2;
            break;
          default:
            titlePos[0] -= (this->AdjustTitlePositionMode & AlignLeft) ? stringSize[0] : 0;
            break;
          };
        switch (this->AdjustTitlePositionMode & (AlignTop | AlignBottom | AlignVCenter))
          {
          default:
          case AlignTop:
            titlePos[1] = pos2[1];
            break;
          case AlignBottom:
            titlePos[1] = pos[1];
            break;
          case AlignVCenter:
            titlePos[1] = pos[1] + 0.5 * (pos2[1] - pos[1]);
          };

        switch (this->AdjustTitlePositionMode & (AlignAxisTop | AlignAxisBottom | AlignAxisVCenter))
          {
          case AlignAxisTop:
            titlePos[1] += this->AdjustTitlePositionMode & AlignTop ? this->Border : -this->Border;
            break;
          case AlignAxisBottom:
            titlePos[1] -= stringSize[1];
            break;
          case AlignAxisVCenter:
            titlePos[1] -= stringSize[1] / 2;
            break;
          default:
            titlePos[1] += (this->AdjustTitlePositionMode & AlignTop) ? stringSize[1] : 0;
            break;
          };
        this->TitleActor->GetPositionCoordinate()->SetValue(titlePos[0], titlePos[1]);
        //this->TitleActor->GetPositionCoordinate()->SetValue(
        //  pos[0] + 0.5 * (pos2[0] - pos[0]) - stringSize[0] / 2.0, 
        //  pos2[1] - stringSize[1] / 2.0);
        }
      else
        {
        this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
        this->TitleActor->GetPositionCoordinate()->SetValue(
          this->TitlePosition[0], this->TitlePosition[1]);
        }

      this->TitleActor->SetProperty(this->GetProperty());
      }

    //Border and box - may adjust spacing based on font size relationship
    //to the proportions relative to the border
    //
    if (this->ChartBox || this->ChartBorder)
      {
      double doubleP1[3], doubleP2[3];
      
      doubleP1[0] = static_cast<double>(pos[0]); 
      doubleP1[1] = static_cast<double>(pos[1]); 
      doubleP1[2] = 0.0;
      doubleP2[0] = static_cast<double>(pos2[0]);
      doubleP2[1] = static_cast<double>(pos2[1]); 
      doubleP2[2] = 0.0;
      
      vtkPoints *pts = this->ChartBoxPolyData->GetPoints();
      pts->SetPoint(0, doubleP1);
      pts->SetPoint(1, doubleP2[0],doubleP1[1],0.0);
      pts->SetPoint(2, doubleP2);
      pts->SetPoint(3, doubleP1[0],doubleP2[1],0.0);

      this->ChartBorderActor->SetProperty(this->GetProperty());
      }
    // Reference lines
    if (this->ShowReferenceXLine || this->ShowReferenceYLine)
      {
      double doubleP1[3], doubleP2[3];
      
      doubleP1[0] = static_cast<double>(pos[0]); 
      doubleP1[1] = static_cast<double>(pos[1]); 
      doubleP1[2] = 0.0;
      doubleP2[0] = static_cast<double>(pos2[0]);
      doubleP2[1] = static_cast<double>(pos2[1]); 
      doubleP2[2] = 0.0;
      
      vtkPoints *pts = this->ReferenceLinesPolyData->GetPoints();
      if (this->ShowReferenceXLine && 
          this->ReferenceXValue >= xRange[0] && 
          this->ReferenceXValue < xRange[1])
        {
        double xRefPos = doubleP1[0] + (this->ReferenceXValue - xRange[0]) / (xRange[1] - xRange[0]) * (doubleP2[0] - doubleP1[0]);
        pts->SetPoint(0, xRefPos, doubleP1[1], 0.0);
        pts->SetPoint(1, xRefPos, doubleP2[1], 0.0);
        }
      else
        {
        pts->SetPoint(0, doubleP1);
        pts->SetPoint(1, doubleP1);
        }
      if (this->ShowReferenceYLine && 
          this->ReferenceYValue >= yRange[0] && 
          this->ReferenceYValue < yRange[1])
        {
        double yRefPos = doubleP1[1] + (this->ReferenceYValue - yRange[0]) / (yRange[1] - yRange[0])* (doubleP2[1] - doubleP1[1]);
        pts->SetPoint(2, doubleP1[0], yRefPos, 0.);
        pts->SetPoint(3, doubleP2[0], yRefPos, 0.);
        }
      else
        {
        pts->SetPoint(2, doubleP1);
        pts->SetPoint(3, doubleP1);
        }
      // copy the color/linewidth/opacity...
      this->ReferenceLinesActor->SetProperty(this->GetProperty());
      }
    vtkDebugMacro(<<"Creating Plot Data");
    // Okay, now create the plot data and set up the pipeline
    this->CreatePlotData(pos, pos2, xRange, yRange, lengths, numDS, numDO);
    delete [] lengths;
    
    this->BuildTime.Modified();

    }//if need to rebuild the plot
  
  vtkDebugMacro(<<"Rendering Box");
  if ( this->ChartBox )
    {
    renderedSomething += this->ChartBoxActor->RenderOpaqueGeometry(viewport);
    }
  if ( this->ChartBorder )
    {
    renderedSomething += this->ChartBorderActor->RenderOpaqueGeometry(viewport);
    }
  if ( this->ShowReferenceXLine || this->ShowReferenceYLine)
    {
    renderedSomething += this->ReferenceLinesActor->RenderOpaqueGeometry(viewport);
    }
  vtkDebugMacro(<<"Rendering Axes");
  renderedSomething += this->XAxis->RenderOpaqueGeometry(viewport);
  renderedSomething += this->YAxis->RenderOpaqueGeometry(viewport);
  for (int i=0; i < this->NumberOfInputs; i++)
    {
    vtkDebugMacro(<<"Rendering plotactors");
    renderedSomething += this->PlotActor[i]->RenderOpaqueGeometry(viewport);
    }
  if ( this->Title )
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

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkXYPlotActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
const char *vtkXYPlotActor::GetDataObjectPlotModeAsString()
{
  if ( this->DataObjectPlotMode == VTK_XYPLOT_ROW )
    {
    return "Plot Rows";
    }
  else 
    {
    return "Plot Columns";
    }
}

//----------------------------------------------------------------------------
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
  if (this->ChartBoxActor) 
   { 
   this->ChartBoxActor->ReleaseGraphicsResources(win); 
   }
  if (this->ChartBorderActor) 
   { 
   this->ChartBorderActor->ReleaseGraphicsResources(win); 
   }
  if (this->ReferenceLinesActor) 
   { 
   this->ReferenceLinesActor->ReleaseGraphicsResources(win); 
   }
}

//----------------------------------------------------------------------------
unsigned long vtkXYPlotActor::GetMTime()
{
  unsigned long mtime, mtime2;
  mtime = this->vtkActor2D::GetMTime();

  if (this->Legend)
    {
    mtime2 = this->LegendActor->GetMTime();
    if (mtime2 > mtime)
      {
      mtime = mtime2;
      }
    }

  return mtime;
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  vtkDataSet *input;
  char *array;
  int component;
  int idx, num;

  this->Superclass::PrintSelf(os,indent);

  vtkCollectionSimpleIterator dsit;
  this->InputList->InitTraversal(dsit);
  num = this->InputList->GetNumberOfItems();
  os << indent << "DataSetInputs: " << endl;
  for (idx = 0; idx < num; ++idx)
    {
    input = this->InputList->GetNextDataSet(dsit);
    array = this->SelectedInputScalars[idx];
    component = this->SelectedInputScalarsComponent->GetValue((vtkIdType)idx);
    if (array == NULL)
      {
      os << i2 << "(" << input << ") Default Scalars,  Component = " << component << endl;
      }
    else
      {
      os << i2 << "(" << input << ") " << array << ",  Component = " << component << endl;
      }
    }

  os << indent << "Input DataObjects:\n";
  this->DataObjectInputList->PrintSelf(os,indent.GetNextIndent());
  
  if (this->TitleTextProperty)
    {
    os << indent << "Title Text Property:\n";
    this->TitleTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Title Text Property: (none)\n";
    }

  if (this->AxisTitleTextProperty)
    {
    os << indent << "Axis Title Text Property:\n";
    this->AxisTitleTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Axis Title Text Property: (none)\n";
    }

  if (this->AxisLabelTextProperty)
    {
    os << indent << "Axis Label Text Property:\n";
    this->AxisLabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Axis Label Text Property: (none)\n";
    }

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

  os << indent << "X Label Format: " << this->XLabelFormat << "\n";
  os << indent << "Y Label Format: " << this->YLabelFormat << "\n";
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

  os << indent << "Legend Actor:";
  this->LegendActor->PrintSelf( os << endl, i2);
  os << indent << "Glyph Source:";
  this->GlyphSource->PrintSelf( os << endl, i2);

  os << indent << "AdjustXLabels: " 
     << this->AdjustXLabels << endl;
  os << indent << "AdjustYLabels: " 
     << this->AdjustYLabels << endl;
  os << indent << "AdjustTitlePosition: " 
     << this->AdjustTitlePosition << endl;
  os << indent << "TitlePosition: " 
     << this->TitlePosition[0] << " "
     << this->TitlePosition[1] << " "
     << endl;
  os << indent << "AdjustTitlePositionMode: "
     << this->AdjustTitlePositionMode << endl;
  os << indent << "ChartBox: " << (this->ChartBox ? "On\n" : "Off\n");  
  os << indent << "ChartBorder: " << (this->ChartBorder ? "On\n" : "Off\n");
  os << indent << "ShowReferenceXLine: " 
     << (this->ShowReferenceXLine ? "On\n" : "Off\n");
  os << indent << "ReferenceXValue: " << this->ReferenceXValue << endl;
  os << indent << "ShowReferenceYLine: " 
     << (this->ShowReferenceYLine ? "On\n" : "Off\n");
  os << indent << "ReferenceYValue: " << this->ReferenceYValue << endl;
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::ComputeXRange(double range[2], double *lengths)
{
  int dsNum;
  vtkIdType numPts, ptId, maxNum;
  double maxLength=0.0, xPrev[3], x[3];
  vtkDataSet *ds;

  range[0] = VTK_DOUBLE_MAX, range[1] = VTK_DOUBLE_MIN;

  vtkCollectionSimpleIterator dsit;
  for ( dsNum=0, maxNum=0, this->InputList->InitTraversal(dsit); 
        (ds = this->InputList->GetNextDataSet(dsit)); dsNum++)
    {
    numPts = ds->GetNumberOfPoints();
    if (numPts == 0)
      {
      vtkErrorMacro(<<"No scalar data to plot!");
      continue;
      }

    if ( this->XValues != VTK_XYPLOT_INDEX )
      {
      ds->GetPoint(0, xPrev);
      for ( lengths[dsNum]=0.0, ptId=0; ptId < numPts; ptId++ )
        {
        ds->GetPoint(ptId, x);
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
      range[1] = (double)(maxNum - 1);
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

//----------------------------------------------------------------------------
void vtkXYPlotActor::ComputeYRange(double range[2])
{
  vtkDataSet *ds;
  vtkDataArray *scalars;
  double sRange[2];
  int count;
  int component;

  range[0]=VTK_DOUBLE_MAX, range[1]=VTK_DOUBLE_MIN;

  vtkCollectionSimpleIterator dsit;
  for ( this->InputList->InitTraversal(dsit), count = 0; 
        (ds = this->InputList->GetNextDataSet(dsit)); ++count)
    {
    scalars = ds->GetPointData()->GetScalars(this->SelectedInputScalars[count]);
    component = this->SelectedInputScalarsComponent->GetValue(count);
    if ( !scalars)
      {
      vtkErrorMacro(<<"No scalar data to plot!");
      continue;
      }
    if ( component < 0 || component >= scalars->GetNumberOfComponents())
      {
      vtkErrorMacro(<<"Bad component!");
      continue;
      }
    
    scalars->GetRange(sRange, component);
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

//----------------------------------------------------------------------------
static inline int vtkXYPlotActorGetComponent(vtkFieldData* field,
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

//----------------------------------------------------------------------------
void vtkXYPlotActor::ComputeDORange(double xrange[2], double yrange[2], 
                                    double *lengths)
{
  int i;
  vtkDataObject *dobj;
  vtkFieldData *field;
  int doNum, numColumns;
  vtkIdType numTuples, numRows, num, ptId, maxNum;
  double maxLength=0.0;
  double x = 0.0;
  double y = 0.0;
  double xPrev = 0.0;
  vtkDataArray *array;
  
  // NOTE: FieldData can have non-numeric arrays. However, XY plot can only 
  // work on numeric arrays (or vtkDataArray subclasses). 
  
  xrange[0] = yrange[0] = VTK_DOUBLE_MAX;
  xrange[1] = yrange[1] = -VTK_DOUBLE_MAX;
  vtkCollectionSimpleIterator doit;
  for ( doNum=0, maxNum=0, this->DataObjectInputList->InitTraversal(doit); 
        (dobj = this->DataObjectInputList->GetNextDataObject(doit)); doNum++)
    {

    lengths[doNum] = 0.0;
    field = dobj->GetFieldData();
    numColumns = field->GetNumberOfComponents();  //number of "columns"
      // numColumns includes the components for non-numeric arrays as well.
    for (numRows = VTK_LARGE_ID, i=0; i<field->GetNumberOfArrays(); i++)
      {
      array = field->GetArray(i);
      if (!array)
        {
        // non-numeric array, skip.
        continue;
        }
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
        int status = 0;
      
        if ( this->DataObjectPlotMode == VTK_XYPLOT_ROW )
          {
          // x = field->GetComponent(this->XComponent->GetValue(doNum), ptId);
          status = ::vtkXYPlotActorGetComponent(field,
            this->XComponent->GetValue(doNum), ptId, &x);
          }
        else //if ( this->DataObjectPlotMode == VTK_XYPLOT_COLUMN )
          {
          // x = field->GetComponent(ptId, this->XComponent->GetValue(doNum));
          status = ::vtkXYPlotActorGetComponent(field,
            ptId, this->XComponent->GetValue(doNum), &x);
          }
        if (!status)
          {
          // requested component falls in a non-numeric array, skip it.
          continue;
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
      int status = 0;
      if ( this->DataObjectPlotMode == VTK_XYPLOT_ROW )
        {
        //y = field->GetComponent(this->YComponent->GetValue(doNum), ptId);
        status = ::vtkXYPlotActorGetComponent(field,
          this->YComponent->GetValue(doNum), ptId, &y);
        }
      else //if ( this->DataObjectPlotMode == VTK_XYPLOT_COLUMN )
        {
        //y = field->GetComponent(ptId, this->YComponent->GetValue(doNum));
        status = ::vtkXYPlotActorGetComponent(field,
          ptId, this->YComponent->GetValue(doNum), &y);
        }
      if (!status)
        {
        // requested component falls in non-numeric array.
        // skip.
        continue;
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
      xrange[1] = (double)(maxNum - 1);
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

//----------------------------------------------------------------------------
void vtkXYPlotActor::CreatePlotData(int *pos, int *pos2, double xRange[2], 
                                    double yRange[2], double *lengths,
                                    int numDS, int numDO)
{
  double xyz[3]; xyz[2] = 0.0;
  int i, numLinePts, dsNum, doNum, num;
  vtkIdType numPts, ptId, id;
  double length, x[3], xPrev[3];
  vtkDataArray *scalars;
  int component;
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
    vtkCollectionSimpleIterator dsit;
    for ( dsNum=0, this->InputList->InitTraversal(dsit); 
          (ds = this->InputList->GetNextDataSet(dsit)); dsNum++ )
      {
      clippingRequired = 0;
      numPts = ds->GetNumberOfPoints();
      scalars = ds->GetPointData()->GetScalars(this->SelectedInputScalars[dsNum]);
      if ( !scalars)
        {
        continue;
        }
      component = this->SelectedInputScalarsComponent->GetValue(dsNum);
      if ( component < 0 || component >= scalars->GetNumberOfComponents())
        {
        continue;
        }

      pts = this->PlotData[dsNum]->GetPoints();
      lines = this->PlotData[dsNum]->GetLines();
      lines->InsertNextCell(0); //update the count later

      ds->GetPoint(0, xPrev);
      for ( numLinePts=0, length=0.0, ptId=0; ptId < numPts; ptId++ )
        {
        xyz[1] = scalars->GetComponent(ptId, component);
        ds->GetPoint(ptId, x);
        switch (this->XValues)
          {
          case VTK_XYPLOT_NORMALIZED_ARC_LENGTH:
            length += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
            xyz[0] = length / lengths[dsNum];
            xPrev[0] = x[0]; xPrev[1] = x[1]; xPrev[2] = x[2];
            break;
          case VTK_XYPLOT_INDEX:
            xyz[0] = (double)ptId;
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
    vtkCollectionSimpleIterator doit;
    for ( doNum=0, this->DataObjectInputList->InitTraversal(doit); 
          (dobj = this->DataObjectInputList->GetNextDataObject(doit)); 
          doNum++ )
      {
      // determine the shape of the field
      field = dobj->GetFieldData();
      numColumns = field->GetNumberOfComponents(); //number of "columns"
      // numColumns also includes non-numeric array components.
      for (numRows = VTK_LARGE_ID, i=0; i<field->GetNumberOfArrays(); i++)
        {
        array = field->GetArray(i);
        if (!array)
          {
          // skip non-numeric arrays.
          continue;
          }
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
        int status1, status2;
        if ( this->DataObjectPlotMode == VTK_XYPLOT_ROW )
          {
          //x[0] = field->GetComponent(this->XComponent->GetValue(doNum),ptId);
          //xyz[1] = field->GetComponent(this->YComponent->GetValue(doNum),ptId);
          status1 = ::vtkXYPlotActorGetComponent(field,
            this->XComponent->GetValue(doNum), ptId, &x[0]);
          status2 = ::vtkXYPlotActorGetComponent(field,
            this->YComponent->GetValue(doNum), ptId, &xyz[1]);
          }
        else //if ( this->DataObjectPlotMode == VTK_XYPLOT_COLUMN )
          {
          //x[0] = field->GetComponent(ptId, this->XComponent->GetValue(doNum));
          //xyz[1] = field->GetComponent(ptId, this->YComponent->GetValue(doNum));

          status1 = ::vtkXYPlotActorGetComponent(field,
            ptId, this->XComponent->GetValue(doNum), &x[0]);

          if (!status1)
            {
            vtkWarningMacro(<< this->XComponent->GetValue(doNum) << " is a non-numeric component.");
            }
          
          status2 = ::vtkXYPlotActorGetComponent(field,
            ptId, this->YComponent->GetValue(doNum), &xyz[1]);

          if (!status2)
            {
            vtkWarningMacro(<< this->YComponent->GetValue(doNum) << " is a non-numeric component.");
            }
          }
        if (!status1 || !status2)
          {
          // component is non-numeric.
          // Skip it.
          continue;
          }

        switch (this->XValues)
          {
          case VTK_XYPLOT_NORMALIZED_ARC_LENGTH:
            length += fabs(x[0]-xPrev[0]);
            xyz[0] = length / lengths[doNum];
            xPrev[0] = x[0];
            break;
          case VTK_XYPLOT_INDEX:
            xyz[0] = (double)ptId;
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
        {
        this->PlotData[i]->SetLines(NULL);
        }
      }
    else
      {
      if ( this->GetPlotLines(i) == 0)
        {
        this->PlotData[i]->SetLines(NULL);
        }
      }

    if (!this->PlotCurvePoints) 
      {
      if ( !this->PlotPoints || (this->LegendActor->GetEntrySymbol(i) &&
                                 this->LegendActor->GetEntrySymbol(i) != 
                                 this->GlyphSource->GetOutput()))
        {
        this->PlotData[i]->SetVerts(NULL);
        }
      }
    else
      {
      if ( this->GetPlotPoints(i) == 0 || 
          (this->LegendActor->GetEntrySymbol(i) &&
           this->LegendActor->GetEntrySymbol(i) != 
           this->GlyphSource->GetOutput()))
        {
        this->PlotData[i]->SetVerts(NULL);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Position the axes taking into account the expected padding due to labels
// and titles. We want the result to fit in the box specified. This method
// knows something about how the vtkAxisActor2D functions, so it may have 
// to change if that class changes dramatically.
//
void vtkXYPlotActor::PlaceAxes(vtkViewport *viewport, int *size,
                               int pos[2], int pos2[2])
{
  int titleSizeX[2], titleSizeY[2], labelSizeX[2], labelSizeY[2];
  double labelFactorX, labelFactorY;
  double fontFactorX, fontFactorY;
  double tickOffsetX, tickOffsetY;
  double tickLengthX, tickLengthY;

  vtkAxisActor2D *axisX;
  vtkAxisActor2D *axisY;

  char str1[512], str2[512];

  if (this->ExchangeAxes)
    {
    axisX = this->YAxis;
    axisY = this->XAxis;
    }
  else
    {
    axisX = this->XAxis;
    axisY = this->YAxis;
    }

  fontFactorY = axisY->GetFontFactor();
  fontFactorX = axisX->GetFontFactor();

  labelFactorY = axisY->GetLabelFactor();
  labelFactorX = axisX->GetLabelFactor();

  // Create a dummy text mapper for getting font sizes
  vtkTextMapper *textMapper = vtkTextMapper::New();
  vtkTextProperty *tprop = textMapper->GetTextProperty();

  // Get the location of the corners of the box
  int *p1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
  int *p2 = this->Position2Coordinate->GetComputedViewportValue(viewport);

  // Estimate the padding around the X and Y axes
  tprop->ShallowCopy(axisX->GetTitleTextProperty());
  textMapper->SetInput(axisX->GetTitle());
  vtkTextMapper::SetRelativeFontSize(textMapper, viewport, size, titleSizeX, 0.015*fontFactorX);

  tprop->ShallowCopy(axisY->GetTitleTextProperty());
  textMapper->SetInput(axisY->GetTitle());
  vtkTextMapper::SetRelativeFontSize(textMapper, viewport, size, titleSizeY, 0.015*fontFactorY);

  // At this point the thing to do would be to actually ask the Y axis
  // actor to return the largest label.
  // In the meantime, let's try with the min and max
  sprintf(str1, axisY->GetLabelFormat(), axisY->GetAdjustedRange()[0]);
  sprintf(str2, axisY->GetLabelFormat(), axisY->GetAdjustedRange()[1]);
  tprop->ShallowCopy(axisY->GetLabelTextProperty());
  textMapper->SetInput(strlen(str1) > strlen(str2) ? str1 : str2);
  vtkTextMapper::SetRelativeFontSize(textMapper, viewport, size, labelSizeY, 0.015*labelFactorY*fontFactorY);

  // We do only care of the height of the label in the X axis, so let's
  // use the min for example
  sprintf(str1, axisX->GetLabelFormat(), axisX->GetAdjustedRange()[0]);
  tprop->ShallowCopy(axisX->GetLabelTextProperty());
  textMapper->SetInput(str1);
  vtkTextMapper::SetRelativeFontSize(textMapper, viewport, size, labelSizeX, 0.015*labelFactorX*fontFactorX);

  tickOffsetX = axisX->GetTickOffset();
  tickOffsetY = axisY->GetTickOffset();
  tickLengthX = axisX->GetTickLength();
  tickLengthY = axisY->GetTickLength();

  // Okay, estimate the size
  pos[0] = (int)(p1[0] + titleSizeY[0] + 2.0 * tickOffsetY + tickLengthY + 
                 labelSizeY[0] + this->Border);

  pos[1] = (int)(p1[1] + titleSizeX[1] + 2.0 * tickOffsetX + tickLengthX + 
                 labelSizeX[1] + this->Border);

  pos2[0] = (int)(p2[0] - labelSizeY[0] / 2 - tickOffsetY - this->Border);

  pos2[1] = (int)(p2[1] - labelSizeX[1] / 2 - tickOffsetX - this->Border);

  // Now specify the location of the axes
  axisX->GetPositionCoordinate()->SetValue(
    (double)pos[0], (double)pos[1]);
  axisX->GetPosition2Coordinate()->SetValue(
    (double)pos2[0], (double)pos[1]);
  axisY->GetPositionCoordinate()->SetValue(
    (double)pos[0], (double)pos2[1]);
  axisY->GetPosition2Coordinate()->SetValue(
    (double)pos[0], (double)pos[1]);

  textMapper->Delete();
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::ViewportToPlotCoordinate(vtkViewport *viewport, double &u, double &v)
{
  int *p0, *p1, *p2;

  // XAxis, YAxis are in viewport coordinates already
  p0 = this->XAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPosition2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  u = ((u - p0[0]) / (double)(p1[0] - p0[0]))
    *(this->XComputedRange[1] - this->XComputedRange[0])
    + this->XComputedRange[0];
  v = ((v - p0[1]) / (double)(p2[1] - p0[1]))
    *(this->YComputedRange[1] - this->YComputedRange[0])
    + this->YComputedRange[0];
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::PlotToViewportCoordinate(vtkViewport *viewport,
                                              double &u, double &v)
{
  int *p0, *p1, *p2;

  // XAxis, YAxis are in viewport coordinates already
  p0 = this->XAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPosition2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  u = (((u - this->XComputedRange[0])
        / (this->XComputedRange[1] - this->XComputedRange[0]))
       * (double)(p1[0] - p0[0])) + p0[0];
  v = (((v - this->YComputedRange[0])
        / (this->YComputedRange[1] - this->YComputedRange[0]))
       * (double)(p2[1] - p0[1])) + p0[1];
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::ViewportToPlotCoordinate(vtkViewport *viewport)
{
  this->ViewportToPlotCoordinate(viewport, 
                                 this->ViewportCoordinate[0],
                                 this->ViewportCoordinate[1]);
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::PlotToViewportCoordinate(vtkViewport *viewport)
{
  this->PlotToViewportCoordinate(viewport, 
                                 this->PlotCoordinate[0],
                                 this->PlotCoordinate[1]);
}

//----------------------------------------------------------------------------
int vtkXYPlotActor::IsInPlot(vtkViewport *viewport, double u, double v)
{
  int *p0, *p1, *p2;

  // Bounds of the plot are based on the axes...
  p0 = this->XAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPosition2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  
  if (u >= p0[0] && u <= p1[0] && v >= p0[1] && v <= p2[1])
    {
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkXYPlotActor::GetPlotLines(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->LinesOn->GetValue(i);
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkXYPlotActor::GetPlotPoints(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->PointsOn->GetValue(i);
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetPlotColor(int i, double r, double g, double b)
{
  this->LegendActor->SetEntryColor(i, r, g, b);
}

//----------------------------------------------------------------------------
double *vtkXYPlotActor::GetPlotColor(int i)
{
  return this->LegendActor->GetEntryColor(i);
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetPlotSymbol(int i,vtkPolyData *input)
{
  this->LegendActor->SetEntrySymbol(i, input);
}

//----------------------------------------------------------------------------
vtkPolyData *vtkXYPlotActor::GetPlotSymbol(int i)
{
  return this->LegendActor->GetEntrySymbol(i);
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetPlotLabel(int i, const char *label)
{
  this->LegendActor->SetEntryString(i, label);
}

//----------------------------------------------------------------------------
const char *vtkXYPlotActor::GetPlotLabel(int i)
{
  return this->LegendActor->GetEntryString(i);
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::GenerateClipPlanes(int *pos, int *pos2)
{
  double n[3], x[3];
  vtkPoints *pts=this->ClipPlanes->GetPoints();
  vtkDataArray *normals=this->ClipPlanes->GetNormals();
  
  n[2] = x[2] = 0.0;
  
  //first
  n[0] = 0.0;
  n[1] = -1.0;
  normals->SetTuple(0,n);
  x[0] = (double)0.5*(pos[0]+pos2[0]);
  x[1] = (double)pos[1];
  pts->SetPoint(0,x);
  
  //second
  n[0] = 1.0;
  n[1] = 0.0;
  normals->SetTuple(1,n);
  x[0] = (double)pos2[0];
  x[1] = (double)0.5*(pos[1]+pos2[1]);
  pts->SetPoint(1,x);
  
  //third
  n[0] = 0.0;
  n[1] = 1.0;
  normals->SetTuple(2,n);
  x[0] = (double)0.5*(pos[0]+pos2[0]);
  x[1] = (double)pos2[1];
  pts->SetPoint(2,x);
  
  //fourth
  n[0] = -1.0;
  n[1] = 0.0;
  normals->SetTuple(3,n);
  x[0] = (double)pos[0];
  x[1] = (double)0.5*(pos[1]+pos2[1]);
  pts->SetPoint(3,x);
}

//----------------------------------------------------------------------------
double vtkXYPlotActor::ComputeGlyphScale(int i, int *pos, int *pos2)
{
  vtkPolyData *pd=this->LegendActor->GetEntrySymbol(i);
  pd->Update();
  double length=pd->GetLength();
  double sf = this->GlyphSize * sqrt((double)(pos[0]-pos2[0])*(pos[0]-pos2[0]) + 
                                    (pos[1]-pos2[1])*(pos[1]-pos2[1])) / length;

  return sf;
}

//----------------------------------------------------------------------------
//This assumes that there are multiple polylines
void vtkXYPlotActor::ClipPlotData(int *pos, int *pos2, vtkPolyData *pd)
{
  vtkPoints *points=pd->GetPoints();
  vtkPoints *newPoints;
  vtkCellArray *lines=pd->GetLines();
  vtkCellArray *newLines, *newVerts;
  vtkIdType numPts=pd->GetNumberOfPoints();
  vtkIdType npts = 0;
  vtkIdType newPts[2];
  vtkIdType *pts=0;
  vtkIdType i, id;
  int j;
  double x1[3], x2[3], px[3], n[3], xint[3], t;
  double p1[2], p2[2];

  p1[0] = (double)pos[0]; p1[1] = (double)pos[1];
  p2[0] = (double)pos2[0]; p2[1] = (double)pos2[1];
  
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
      points->GetPoint(pts[i], x1);

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
      points->GetPoint(pts[i], x1);
      points->GetPoint(pts[i+1], x2);

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
        newPts[0] = -1;
        newPts[1] = -1;
        if (x1[0] >= p1[0] && x1[0] <= p2[0] && x1[1] >= p1[1] && x1[1] <= p2[1] )
          {//first point in
          newPts[0] = pointMap[pts[i]];
          }
        else if (x2[0] >= p1[0] && x2[0] <= p2[0] && x2[1] >= p1[1] && x2[1] <= p2[1] )
          {//second point in
          newPts[0] = pointMap[pts[i+1]];
          }

        //only create cell if either x1 or x2 is inside the range
        if (newPts[0] >= 0)
          {
          for (j=0; j<4; j++)
            {
            this->ClipPlanes->GetPoints()->GetPoint(j, px);
            this->ClipPlanes->GetNormals()->GetTuple(j, n);
            if ( vtkPlane::IntersectWithLine(x1,x2,n,px,t,xint) && t >= 0 && t <= 1.0 )
              {
              newPts[1] = newPoints->InsertNextPoint(xint);
              break;
              }
            }
          if (newPts[1] >= 0)
            {
            newLines->InsertNextCell(2,newPts);
            }
          }
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkXYPlotActor::GetDataObjectXComponent(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->XComponent->GetValue(i);
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkXYPlotActor::GetDataObjectYComponent(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->YComponent->GetValue(i);
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetPointComponent(int i, int comp)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  int val = this->XComponent->GetValue(i);
  if ( val != comp )
    {
    this->Modified();
    this->XComponent->SetValue(i,comp);
    }
}

//----------------------------------------------------------------------------
int vtkXYPlotActor::GetPointComponent(int i)
{
  i = ( i < 0 ? 0 : (i >=VTK_MAX_PLOTS ? VTK_MAX_PLOTS-1 : i));
  return this->XComponent->GetValue(i);
}

//----------------------------------------------------------------------------
double *vtkXYPlotActor::TransformPoint(int pos[2], int pos2[2],
                                       double x[3], double xNew[3])
{
  // First worry about exchanging axes
  if ( this->ExchangeAxes )
    {
    double sx = (x[0]-pos[0]) / (pos2[0]-pos[0]);
    double sy = (x[1]-pos[1]) / (pos2[1]-pos[1]);
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

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetXTitlePosition(double position)
{
  this->XAxis->SetTitlePosition(position);
}

//----------------------------------------------------------------------------
double vtkXYPlotActor::GetXTitlePosition()
{
  return this->XAxis->GetTitlePosition();
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetYTitlePosition(double position)
{
  this->YAxis->SetTitlePosition(1.0-position);
}

//----------------------------------------------------------------------------
double vtkXYPlotActor::GetYTitlePosition()
{
  return this->YAxis->GetTitlePosition();
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetAdjustXLabels(int adjust)
{
  this->AdjustXLabels = adjust;
  this->XAxis->SetAdjustLabels(adjust);
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetAdjustYLabels(int adjust)
{
  this->AdjustYLabels = adjust;
  this->YAxis->SetAdjustLabels(adjust);
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetLabelFormat(const char* _arg)
{
  this->SetXLabelFormat(_arg);
  this->SetYLabelFormat(_arg);
}
    
//----------------------------------------------------------------------------
void vtkXYPlotActor::SetXLabelFormat(const char* _arg)
{
  if (this->XLabelFormat == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->XLabelFormat && _arg && (!strcmp(this->XLabelFormat,_arg))) 
    { 
    return;
    }

  if (this->XLabelFormat) 
    { 
    delete [] this->XLabelFormat; 
    }

  if (_arg)
    {
    this->XLabelFormat = new char[strlen(_arg)+1];
    strcpy(this->XLabelFormat,_arg);
    }
  else
    {
    this->XLabelFormat = NULL;
    }

  this->XAxis->SetLabelFormat(this->XLabelFormat);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetYLabelFormat(const char* _arg)
{
  if (this->YLabelFormat == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->YLabelFormat && _arg && (!strcmp(this->YLabelFormat,_arg))) 
    { 
    return;
    }

  if (this->YLabelFormat) 
    { 
    delete [] this->YLabelFormat; 
    }

  if (_arg)
    {
    this->YLabelFormat = new char[strlen(_arg)+1];
    strcpy(this->YLabelFormat,_arg);
    }
  else
    {
    this->YLabelFormat = NULL;
    }

  this->YAxis->SetLabelFormat(this->YLabelFormat);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetNumberOfXMinorTicks(int num)
{
  this->XAxis->SetNumberOfMinorTicks(num);
}

//----------------------------------------------------------------------------
int vtkXYPlotActor::GetNumberOfXMinorTicks()
{
  return this->XAxis->GetNumberOfMinorTicks();
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::SetNumberOfYMinorTicks(int num)
{
  this->YAxis->SetNumberOfMinorTicks(num);
}

//----------------------------------------------------------------------------
int vtkXYPlotActor::GetNumberOfYMinorTicks()
{
  return this->YAxis->GetNumberOfMinorTicks();
}

//----------------------------------------------------------------------------
void vtkXYPlotActor::PrintAsCSV(ostream &os)
{
  vtkDataArray *scalars;
  vtkDataSet *ds;
  vtkCollectionSimpleIterator dsit;
  double s;
  int dsNum,component;
  for ( dsNum=0, this->InputList->InitTraversal(dsit); 
    (ds = this->InputList->GetNextDataSet(dsit)); dsNum++ )
    {
    vtkIdType numPts = ds->GetNumberOfPoints();
    scalars = ds->GetPointData()->GetScalars(this->SelectedInputScalars[dsNum]);
    os << this->SelectedInputScalars[dsNum] << ",";

    component = this->SelectedInputScalarsComponent->GetValue(dsNum);
    for ( vtkIdType ptId=0; ptId < numPts; ptId++ )
      {
      s = scalars->GetComponent(ptId, component);
      if( ptId == 0 )
        {
        os << s;
        }
      else
        {
        os << "," << s;
        }
      }
    os << endl;

    if (dsNum == this->InputList->GetNumberOfItems()-1)
      {
      os << "X or T,";
      for ( vtkIdType ptId=0; ptId < numPts; ptId++ )
        {
        double *x = ds->GetPoint(ptId);
        if( ptId == 0 )
          {
          os << x[0];
          }
        else
          {
          os << "," << x[0];
          }
        }
      os << endl;
      }

    }
}
