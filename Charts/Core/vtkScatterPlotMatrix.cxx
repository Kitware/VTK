/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkScatterPlotMatrix.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkChartXY.h"
#include "vtkChartXYZ.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkPlotPoints.h"
#include "vtkPlotPoints3D.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTooltipItem.h"
#include "vtkVectorOperators.h"

// STL includes
#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

class vtkScatterPlotMatrix::PIMPL
{
public:
  PIMPL() : VisibleColumnsModified(true), BigChart(NULL),
    ResizingBigChart(false), AnimationCallbackInitialized(false), TimerId(0),
    TimerCallbackInitialized(false)
  {
    pimplChartSetting* scatterplotSettings = new pimplChartSetting();
    scatterplotSettings->BackgroundBrush->SetColor(255, 255, 255, 255);
    this->ChartSettings[vtkScatterPlotMatrix::SCATTERPLOT] =
        scatterplotSettings;
    pimplChartSetting* histogramSettings = new pimplChartSetting();
    histogramSettings->BackgroundBrush->SetColor(127, 127, 127, 102);
    histogramSettings->PlotPen->SetColor(255, 255, 255, 255);
    histogramSettings->ShowAxisLabels = true;
    this->ChartSettings[vtkScatterPlotMatrix::HISTOGRAM] = histogramSettings;
    pimplChartSetting* activeplotSettings = new pimplChartSetting();
    activeplotSettings->BackgroundBrush->SetColor(255, 255, 255, 255);
    activeplotSettings->ShowAxisLabels = true;
    this->ChartSettings[vtkScatterPlotMatrix::ACTIVEPLOT] = activeplotSettings;
    activeplotSettings->MarkerSize = 8.0;
    this->SelectedChartBGBrush->SetColor(0, 204, 0, 102);
    this->SelectedRowColumnBGBrush->SetColor(204, 0, 0, 102);
    this->TooltipItem = vtkSmartPointer<vtkTooltipItem>::New();
  }

  ~PIMPL()
  {
    delete this->ChartSettings[vtkScatterPlotMatrix::SCATTERPLOT];
    delete this->ChartSettings[vtkScatterPlotMatrix::HISTOGRAM];
    delete this->ChartSettings[vtkScatterPlotMatrix::ACTIVEPLOT];
  }

  // Store columns settings such as axis range, title, number of tick marks.
  class ColumnSetting
  {
  public:
    ColumnSetting() : min(0), max(0), nTicks(0), title("?!?")
    {
    }

    double min;
    double max;
    int    nTicks;
    std::string title;
  };

  class pimplChartSetting
  {
  public:
    pimplChartSetting()
    {
      this->PlotPen->SetColor(0, 0, 0, 255);
      this->MarkerStyle = vtkPlotPoints::CIRCLE;
      this->MarkerSize = 3.0;
      this->AxisColor.Set(0, 0, 0, 255);
      this->GridColor.Set(242, 242, 242, 255);
      this->LabelNotation = vtkAxis::STANDARD_NOTATION;
      this->LabelPrecision = 2;
      this->TooltipNotation = vtkAxis::STANDARD_NOTATION;
      this->TooltipPrecision = 2;
      this->ShowGrid = true;
      this->ShowAxisLabels = false;
      this->LabelFont = vtkSmartPointer<vtkTextProperty>::New();
      this->LabelFont->SetFontFamilyToArial();
      this->LabelFont->SetFontSize(12);
      this->LabelFont->SetColor(0.0, 0.0, 0.0);
      this->LabelFont->SetOpacity(1.0);
    }
    ~pimplChartSetting() {}

    int MarkerStyle;
    float MarkerSize;
    vtkColor4ub AxisColor;
    vtkColor4ub GridColor;
    int LabelNotation;
    int LabelPrecision;
    int TooltipNotation;
    int TooltipPrecision;
    bool ShowGrid;
    bool ShowAxisLabels;
    vtkSmartPointer<vtkTextProperty> LabelFont;
    vtkNew<vtkBrush> BackgroundBrush;
    vtkNew<vtkPen> PlotPen;
    vtkNew<vtkBrush> PlotBrush;
  };

  void UpdateAxis(vtkAxis* axis, pimplChartSetting* setting,
                  bool updateLabel = true)
  {
    if(axis && setting)
    {
      axis->GetPen()->SetColor(setting->AxisColor);
      axis->GetGridPen()->SetColor(setting->GridColor);
      axis->SetGridVisible(setting->ShowGrid);
      if (updateLabel)
      {
        vtkTextProperty *prop = setting->LabelFont.GetPointer();
        axis->SetNotation(setting->LabelNotation);
        axis->SetPrecision(setting->LabelPrecision);
        axis->SetLabelsVisible(setting->ShowAxisLabels);
        axis->GetLabelProperties()->SetFontSize(prop->GetFontSize());
        axis->GetLabelProperties()->SetColor(prop->GetColor());
        axis->GetLabelProperties()->SetOpacity(prop->GetOpacity());
        axis->GetLabelProperties()->SetFontFamilyAsString(
          prop->GetFontFamilyAsString());
        axis->GetLabelProperties()->SetBold(prop->GetBold());
        axis->GetLabelProperties()->SetItalic(prop->GetItalic());
      }
    }
  }

  void UpdateChart(vtkChart* chart, pimplChartSetting* setting)
  {
    if(chart && setting)
    {
      vtkPlot *plot = chart->GetPlot(0);
      if (plot)
      {
        plot->SetTooltipNotation(setting->TooltipNotation);
        plot->SetTooltipPrecision(setting->TooltipPrecision);
      }
    }
  }

  vtkNew<vtkTable> Histogram;
  bool VisibleColumnsModified;
  vtkWeakPointer<vtkChart> BigChart;
  vtkVector2i BigChartPos;
  bool ResizingBigChart;
  vtkNew<vtkAnnotationLink> Link;

  // Settings for the charts in the scatter plot matrix.
  std::map<int, pimplChartSetting*> ChartSettings;
  typedef std::map<int, pimplChartSetting*>::iterator chartIterator;

  // Axis ranges for the columns in the scatter plot matrix.
  std::map<std::string, ColumnSetting> ColumnSettings;

  vtkNew<vtkBrush> SelectedRowColumnBGBrush;
  vtkNew<vtkBrush> SelectedChartBGBrush;
  std::vector<vtkVector2i>           AnimationPath;
  std::vector<vtkVector2i>::iterator AnimationIter;
  vtkRenderWindowInteractor* Interactor;
  vtkNew<vtkCallbackCommand> AnimationCallback;
  bool                       AnimationCallbackInitialized;
  unsigned long int          TimerId;
  bool                       TimerCallbackInitialized;
  int                        AnimationPhase;
  float                      CurrentAngle;
  float                      IncAngle;
  float                      FinalAngle;
  vtkVector2i                NextActivePlot;

  vtkNew<vtkChartXYZ> BigChart3D;
  vtkNew<vtkAxis>     TestAxis;   // Used to get ranges/numer of ticks
  vtkSmartPointer<vtkTooltipItem> TooltipItem;
  vtkSmartPointer<vtkStringArray> IndexedLabelsArray;
};

namespace
{

// This is just here for now - quick and dirty historgram calculations...
bool PopulateHistograms(vtkTable *input, vtkTable *output, vtkStringArray *s,
                        int NumberOfBins)
{
  // The output table will have the twice the number of columns, they will be
  // the x and y for input column. This is the bin centers, and the population.
  for (vtkIdType i = 0; i < s->GetNumberOfTuples(); ++i)
  {
    double minmax[2] = { 0.0, 0.0 };
    vtkStdString name(s->GetValue(i));
    vtkDataArray *in =
        vtkArrayDownCast<vtkDataArray>(input->GetColumnByName(name.c_str()));
    if (in)
    {
      // The bin values are the centers, extending +/- half an inc either side
      in->GetRange(minmax);
      if (minmax[0] == minmax[1])
      {
        minmax[1] = minmax[0] + 1.0;
      }
      double inc = (minmax[1] - minmax[0]) / (NumberOfBins) * 1.001;
      double halfInc = inc / 2.0;
      vtkSmartPointer<vtkFloatArray> extents =
          vtkArrayDownCast<vtkFloatArray>(
            output->GetColumnByName(vtkStdString(name + "_extents").c_str()));
      if (!extents)
      {
        extents = vtkSmartPointer<vtkFloatArray>::New();
        extents->SetName(vtkStdString(name + "_extents").c_str());
      }
      extents->SetNumberOfTuples(NumberOfBins);
      float *centers = static_cast<float *>(extents->GetVoidPointer(0));
      double min = minmax[0] - 0.0005 * inc + halfInc;
      for (int j = 0; j < NumberOfBins; ++j)
      {
        extents->SetValue(j, min + j * inc);
      }
      vtkSmartPointer<vtkIntArray> populations =
          vtkArrayDownCast<vtkIntArray>(
            output->GetColumnByName(vtkStdString(name + "_pops").c_str()));
      if (!populations)
      {
        populations = vtkSmartPointer<vtkIntArray>::New();
        populations->SetName(vtkStdString(name + "_pops").c_str());
      }
      populations->SetNumberOfTuples(NumberOfBins);
      int *pops = static_cast<int *>(populations->GetVoidPointer(0));
      for (int k = 0; k < NumberOfBins; ++k)
      {
        pops[k] = 0;
      }
      for (vtkIdType j = 0; j < in->GetNumberOfTuples(); ++j)
      {
        double v(0.0);
        in->GetTuple(j, &v);
        for (int k = 0; k < NumberOfBins; ++k)
        {
          if (vtkMathUtilities::FuzzyCompare(v, double(centers[k]), halfInc))
          {
            ++pops[k];
            break;
          }
        }
      }
      output->AddColumn(extents.GetPointer());
      output->AddColumn(populations.GetPointer());
    }
  }
  return true;
}

bool MoveColumn(vtkStringArray* visCols, int fromCol, int toCol)
{
  if(!visCols || visCols->GetNumberOfTuples() == 0
    || fromCol == toCol || fromCol == (toCol-1) || fromCol < 0 || toCol < 0)
  {
    return false;
  }
  int numCols = visCols->GetNumberOfTuples();
  if( fromCol >= numCols || toCol > numCols)
  {
    return false;
  }

  std::vector<vtkStdString> newVisCols;
  vtkIdType c;
  if(toCol == numCols)
  {
    for(c=0; c<numCols; c++)
    {
      if(c!=fromCol)
      {
        newVisCols.push_back(visCols->GetValue(c));
      }
    }
    // move the fromCol to the end
    newVisCols.push_back(visCols->GetValue(fromCol));
  }
  // insert the fromCol before toCol
  else if(fromCol < toCol)
  {
    // move Cols in the middle up
    for(c=0; c<fromCol; c++)
    {
      newVisCols.push_back(visCols->GetValue(c));
    }
    for(c=fromCol+1; c<numCols; c++)
    {
      if(c == toCol)
      {
        newVisCols.push_back(visCols->GetValue(fromCol));
      }
      newVisCols.push_back(visCols->GetValue(c));
    }
  }
  else
  {
    for(c=0; c<toCol; c++)
    {
      newVisCols.push_back(visCols->GetValue(c));
    }
    newVisCols.push_back(visCols->GetValue(fromCol));
    for(c=toCol; c<numCols; c++)
    {
      if(c != fromCol)
      {
        newVisCols.push_back(visCols->GetValue(c));
      }
    }
  }

  // repopulate the visCols
  vtkIdType visId=0;
  std::vector<vtkStdString>::iterator arrayIt;
  for(arrayIt=newVisCols.begin(); arrayIt!=newVisCols.end(); ++arrayIt)
  {
    visCols->SetValue(visId++, *arrayIt);
  }
  return true;
}
} // End of anonymous namespace

vtkStandardNewMacro(vtkScatterPlotMatrix)

vtkScatterPlotMatrix::vtkScatterPlotMatrix()
  : NumberOfBins(10), NumberOfFrames(25),
  LayoutUpdatedTime(0)
{
  this->Private = new PIMPL;
  this->TitleProperties = vtkSmartPointer<vtkTextProperty>::New();
  this->TitleProperties->SetFontSize(12);
  this->SelectionMode = vtkContextScene::SELECTION_NONE;
}

vtkScatterPlotMatrix::~vtkScatterPlotMatrix()
{
  delete this->Private;
}

void vtkScatterPlotMatrix::Update()
{
  if (this->Private->VisibleColumnsModified)
  {
    // We need to handle layout changes due to modified visibility.
    // Build up our histograms data before updating the layout.
    PopulateHistograms(this->Input.GetPointer(),
                       this->Private->Histogram.GetPointer(),
                       this->VisibleColumns.GetPointer(),
                       this->NumberOfBins);
    this->UpdateLayout();
    this->Private->VisibleColumnsModified = false;
  }
  else if (this->GetMTime() > this->LayoutUpdatedTime)
  {
    this->UpdateLayout();
  }
}

bool vtkScatterPlotMatrix::Paint(vtkContext2D *painter)
{
  this->CurrentPainter = painter;
  this->Update();
  bool ret = this->Superclass::Paint(painter);
  this->ResizeBigChart();
  return ret;
}

void vtkScatterPlotMatrix::SetScene(vtkContextScene *scene)
{
  // The internal axis shouldn't be a child as it isn't rendered with the
  // chart, but it does need access to the scene.
  this->Private->TestAxis->SetScene(scene);

  this->Superclass::SetScene(scene);
}

bool vtkScatterPlotMatrix::SetActivePlot(const vtkVector2i &pos)
{
  if (pos.GetX() + pos.GetY() + 1 < this->Size.GetX() && pos.GetX() < this->Size.GetX() &&
      pos.GetY() < this->Size.GetY())
  {
    // The supplied index is valid (in the lower quadrant).
    this->ActivePlot = pos;

    // Invoke an interaction event, to let observers know something changed.
    this->InvokeEvent(vtkCommand::AnnotationChangedEvent);

    // set background colors for plots
    if (this->GetChart(this->ActivePlot)->GetPlot(0))
    {
      int plotCount = this->GetSize().GetX();
      for (int i = 0; i < plotCount; ++i)
      {
        for (int j = 0; j < plotCount; ++j)
        {
          if (this->GetPlotType(i, j) == SCATTERPLOT)
          {
            vtkChartXY *chart =
                vtkChartXY::SafeDownCast(this->GetChart(vtkVector2i(i, j)));

            if (pos[0] == i && pos[1] == j)
            {
              // set the new active chart background color to light green
              chart->SetBackgroundBrush(
                    this->Private->SelectedChartBGBrush.GetPointer());
            }
            else if (pos[0] == i || pos[1] == j)
            {
              // set background color for all other charts in the selected
              // chart's row and column to light red
              chart->SetBackgroundBrush(
                    this->Private->SelectedRowColumnBGBrush.GetPointer());
            }
            else
            {
              // set all else to white
              chart->SetBackgroundBrush(
                    this->Private->ChartSettings[SCATTERPLOT]
                    ->BackgroundBrush.GetPointer());
            }
          }
        }
      }
    }
    if (this->Private->BigChart)
    {
      vtkPlot *plot = this->Private->BigChart->GetPlot(0);
      vtkStdString column = this->GetColumnName(pos.GetX());
      vtkStdString row = this->GetRowName(pos.GetY());
      if (!plot)
      {
        plot = this->Private->BigChart->AddPlot(vtkChart::POINTS);
        vtkChart *active = this->GetChart(this->ActivePlot);
        vtkChartXY *xy = vtkChartXY::SafeDownCast(this->Private->BigChart);
        if (xy)
        {
          // Set plot corner, and axis visibility
          xy->SetPlotCorner(plot, 2);
          xy->SetAutoAxes(false);
          xy->GetAxis(vtkAxis::TOP)->SetVisible(true);
          xy->GetAxis(vtkAxis::RIGHT)->SetVisible(true);
          xy->GetAxis(vtkAxis::BOTTOM)->SetLabelsVisible(false);
          xy->GetAxis(vtkAxis::BOTTOM)->SetGridVisible(false);
          xy->GetAxis(vtkAxis::BOTTOM)->SetTicksVisible(false);
          xy->GetAxis(vtkAxis::BOTTOM)->SetVisible(true);
          xy->GetAxis(vtkAxis::LEFT)->SetLabelsVisible(false);
          xy->GetAxis(vtkAxis::LEFT)->SetGridVisible(false);
          xy->GetAxis(vtkAxis::LEFT)->SetTicksVisible(false);
          xy->GetAxis(vtkAxis::LEFT)->SetVisible(true);

          // set labels array
          if(this->Private->IndexedLabelsArray)
          {
              plot->SetIndexedLabels(this->Private->IndexedLabelsArray);
              plot->SetTooltipLabelFormat("%i");
          }
        }
        if (xy && active)
        {
          vtkAxis *a = active->GetAxis(vtkAxis::BOTTOM);
          xy->GetAxis(vtkAxis::TOP)->SetUnscaledRange(
            a->GetUnscaledMinimum(), a->GetUnscaledMaximum());
          a = active->GetAxis(vtkAxis::LEFT);
          xy->GetAxis(vtkAxis::RIGHT)->SetUnscaledRange(
            a->GetUnscaledMinimum(), a->GetUnscaledMaximum());
        }
      }
      else
      {
        this->Private->BigChart->ClearPlots();
        plot = this->Private->BigChart->AddPlot(vtkChart::POINTS);
        vtkChartXY *xy = vtkChartXY::SafeDownCast(this->Private->BigChart);
        if (xy)
        {
          xy->SetPlotCorner(plot, 2);
        }

        // set labels array
        if(this->Private->IndexedLabelsArray)
        {
          plot->SetIndexedLabels(this->Private->IndexedLabelsArray);
          plot->SetTooltipLabelFormat("%i");
        }
      }
      plot->SetInputData(this->Input.GetPointer(), column, row);
      plot->SetPen(this->Private->ChartSettings[ACTIVEPLOT]
                   ->PlotPen.GetPointer());
      this->ApplyAxisSetting(this->Private->BigChart.GetPointer(), column, row);

      // Set marker size and style.
      vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast(plot);
      plotPoints->SetMarkerSize(this->Private->ChartSettings[ACTIVEPLOT]
                                ->MarkerSize);
      plotPoints->SetMarkerStyle(this->Private->ChartSettings[ACTIVEPLOT]
                                 ->MarkerStyle);
      // Set background color.
      this->Private->BigChart->SetBackgroundBrush(
            this->Private->ChartSettings[ACTIVEPLOT]
            ->BackgroundBrush.GetPointer());
      this->Private->BigChart->GetAxis(vtkAxis::TOP)->SetTitle(
            this->VisibleColumns->GetValue(pos.GetX()));
      this->Private->BigChart->GetAxis(vtkAxis::RIGHT)->SetTitle(
            this->VisibleColumns->GetValue(this->GetSize().GetX() - pos.GetY() - 1));
      // Calculate the ideal range.
      //this->Private->BigChart->RecalculateBounds();
    }
    return true;
  }
  else
  {
    return false;
  }
}

vtkVector2i vtkScatterPlotMatrix::GetActivePlot()
{
  return this->ActivePlot;
}

void vtkScatterPlotMatrix::UpdateAnimationPath(const vtkVector2i& newActivePos)
{
  this->Private->AnimationPath.clear();
  if (newActivePos[0] != this->ActivePlot[0] ||
      newActivePos[1] != this->ActivePlot[1])
  {
    if (newActivePos[1] >= this->ActivePlot[1])
    {
      // x direction first
      if (this->ActivePlot[0]>newActivePos[0])
      {
        for(int r = this->ActivePlot[0] - 1; r >= newActivePos[0]; r--)
        {
          this->Private->AnimationPath.push_back(vtkVector2i(r,
                                                             this->ActivePlot[1]));
        }
      }
      else
      {
        for (int r = this->ActivePlot[0] + 1; r <= newActivePos[0]; r++)
        {
          this->Private->AnimationPath.push_back(vtkVector2i(r,
                                                             this->ActivePlot[1]));
        }
      }
      // then y direction
      for (int c = this->ActivePlot[1] + 1; c <= newActivePos[1]; c++)
      {
        this->Private->AnimationPath.push_back(vtkVector2i(newActivePos[0], c));
      }
    }
    else
    {
      // y direction first
      for (int c = this->ActivePlot[1] - 1; c >= newActivePos[1]; c--)
      {
        this->Private->AnimationPath.push_back(vtkVector2i(this->ActivePlot[0],
                                                           c));
      }
      // then x direction
      if (this->ActivePlot[0]>newActivePos[0])
      {
        for (int r = this->ActivePlot[0] - 1; r >= newActivePos[0]; r--)
        {
          this->Private->AnimationPath.push_back(vtkVector2i(r,
                                                             newActivePos[1]));
        }
      }
      else
      {
        for (int r = this->ActivePlot[0] + 1; r <= newActivePos[0]; r++)
        {
          this->Private->AnimationPath.push_back(vtkVector2i(r, newActivePos[1]));
        }
      }
    }
  }
}

void vtkScatterPlotMatrix::StartAnimation(vtkRenderWindowInteractor* interactor)
{
  // Start a simple repeating timer to advance along the path until completion.
  if (!this->Private->TimerCallbackInitialized && interactor)
  {
    if (!this->Private->AnimationCallbackInitialized)
    {
      this->Private->AnimationCallback->SetClientData(this);
      this->Private->AnimationCallback->SetCallback(
            vtkScatterPlotMatrix::ProcessEvents);
      interactor->AddObserver(vtkCommand::TimerEvent,
                              this->Private->AnimationCallback.GetPointer(),
                              0);
      this->Private->Interactor = interactor;
      this->Private->AnimationCallbackInitialized = true;
    }
    this->Private->TimerCallbackInitialized = true;
    // This defines the interval at which the animation will proceed. 25Hz?
    this->Private->TimerId = interactor->CreateRepeatingTimer(1000 / 50);
    this->Private->AnimationIter = this->Private->AnimationPath.begin();
    this->Private->AnimationPhase = 0;
  }
}

void vtkScatterPlotMatrix::AdvanceAnimation()
{
  // The animation has several phases, and we must track where we are.

  // 1: Remove decoration from the big chart.
  // 2: Set three dimensions to plot in the BigChart3D.
  // 3: Make BigChart inivisible, and BigChart3D visible.
  // 4: Rotate between the two dimensions we are transitioning between.
  //    -> Loop from start to end angle to complete the effect.
  // 5: Make the new dimensionality active, update BigChart.
  // 5: Make BigChart3D invisible and BigChart visible.
  // 6: Stop the timer.
  this->InvokeEvent(vtkCommand::AnimationCueTickEvent);
  switch (this->Private->AnimationPhase)
  {
  case 0: // Remove decoration from the big chart, load up the 3D chart
  {
    this->Private->NextActivePlot = *this->Private->AnimationIter;
    vtkChartXYZ *chart = this->Private->BigChart3D.GetPointer();
    chart->SetVisible(false);
    chart->SetAutoRotate(true);
    chart->SetDecorateAxes(false);
    chart->SetFitToScene(false);

    int yColumn = this->GetSize().GetY() - this->ActivePlot.GetY() - 1;
    bool isX = false;
    int zColumn = 0;

    vtkRectf size = this->Private->BigChart->GetSize();
    float zSize;
    this->Private->FinalAngle = 90.0;
    this->Private->IncAngle = this->Private->FinalAngle / this->NumberOfFrames;

    if (this->Private->NextActivePlot.GetY() == this->ActivePlot.GetY())
    {
      // Horizontal move.
      zColumn = this->Private->NextActivePlot.GetX();
      isX = false;
      if (this->ActivePlot.GetX() < zColumn)
      {
        this->Private->IncAngle *= 1.0;
        zSize = size.GetWidth();
      }
      else
      {
        this->Private->IncAngle *= -1.0;
        zSize = -size.GetWidth();
      }
    }
    else
    {
      // Vertical move.
      zColumn = this->GetSize().GetY() - this->Private->NextActivePlot.GetY() - 1;
      isX = true;
      if (this->GetSize().GetY() - this->ActivePlot.GetY() - 1 < zColumn)
      {
        this->Private->IncAngle *= -1.0;
        zSize = size.GetHeight();
      }
      else
      {
        this->Private->IncAngle *= 1.0;
        zSize = -size.GetHeight();
      }
    }
    chart->SetAroundX(isX);
    chart->SetGeometry(size);

    vtkStdString names[3];
    names[0] = this->VisibleColumns->GetValue(this->ActivePlot.GetX());
    names[1] = this->VisibleColumns->GetValue(yColumn);
    names[2] = this->VisibleColumns->GetValue(zColumn);

    // Setup the 3D chart
    this->Private->BigChart3D->ClearPlots();
    vtkNew<vtkPlotPoints3D> scatterPlot3D;
    scatterPlot3D->SetInputData(
      this->Input.GetPointer(), names[0], names[1], names[2]);
    this->Private->BigChart3D->AddPlot(scatterPlot3D.GetPointer());

    // Set the z axis up so that it ends in the right orientation.
    chart->GetAxis(2)->SetPoint2(0, zSize);
    // Now set the ranges for the three axes.
    for (int i = 0; i < 3; ++i)
    {
      PIMPL::ColumnSetting &settings = this->Private->ColumnSettings[names[i]];
      chart->GetAxis(i)->SetUnscaledRange(settings.min, settings.max);
    }
    chart->RecalculateTransform();
    this->GetScene()->SetDirty(true);
    ++this->Private->AnimationPhase;
    return;
  }
  case 1: // Make BigChart invisible, and BigChart3D visible.
    this->Private->BigChart->SetVisible(false);
    this->AddItem(this->Private->BigChart3D.GetPointer());
    this->Private->BigChart3D->SetVisible(true);
    this->GetScene()->SetDirty(true);
    ++this->Private->AnimationPhase;
    this->Private->CurrentAngle = 0.0;
    return;
  case 2: // Rotation of the 3D chart from start to end angle.
    if (fabs(this->Private->CurrentAngle) < (this->Private->FinalAngle - 0.001))
    {
      this->Private->CurrentAngle += this->Private->IncAngle;
      this->Private->BigChart3D->SetAngle(this->Private->CurrentAngle);
    }
    else
    {
      ++this->Private->AnimationPhase;
    }
    this->GetScene()->SetDirty(true);
    return;
  case 3: // Transition to new dimensionality, update the big chart.
    this->SetActivePlot(this->Private->NextActivePlot);
    this->Private->BigChart->Update();
    this->GetScene()->SetDirty(true);
    ++this->Private->AnimationPhase;
    break;
  case 4:
    this->GetScene()->SetDirty(true);
    ++this->Private->AnimationIter;
    // Clean up - we are done.
    this->Private->AnimationPhase = 0;
    if (this->Private->AnimationIter == this->Private->AnimationPath.end())
    {
      this->Private->BigChart->SetVisible(true);
      this->RemoveItem(this->Private->BigChart3D.GetPointer());
      this->Private->BigChart3D->SetVisible(false);
      this->Private->Interactor->DestroyTimer(this->Private->TimerId);
      this->Private->TimerId = 0;
      this->Private->TimerCallbackInitialized = false;
    }
  }
}

void vtkScatterPlotMatrix::ProcessEvents(vtkObject *, unsigned long event,
                                         void *clientData, void *callerData)
{
  vtkScatterPlotMatrix *self =
      reinterpret_cast<vtkScatterPlotMatrix *>(clientData);
  switch (event)
  {
    case vtkCommand::TimerEvent:
    {
      // We must filter the events to ensure we actually get the timer event we
      // created. I would love signals and slots...
      int timerId = *reinterpret_cast<int *>(callerData);   // Seems to work.
      if (self->Private->TimerCallbackInitialized &&
          timerId == static_cast<int>(self->Private->TimerId))
      {
        self->AdvanceAnimation();
      }
      break;
    }
    default:
      break;
  }
}

vtkAnnotationLink* vtkScatterPlotMatrix::GetAnnotationLink()
{
  return this->Private->Link.GetPointer();
}

void vtkScatterPlotMatrix::SetInput(vtkTable *table)
{
  if(table && table->GetNumberOfRows() == 0)
  {
    // do nothing if the table is emtpy
    return;
  }

  if (this->Input != table)
  {
    // Set the input, then update the size of the scatter plot matrix, set
    // their inputs and all the other stuff needed.
    this->Input = table;
    this->SetSize(vtkVector2i(0, 0));
    this->Modified();

    if (table == NULL)
    {
      this->SetColumnVisibilityAll(true);
      return;
    }
    int n = static_cast<int>(this->Input->GetNumberOfColumns());
    this->SetColumnVisibilityAll(true);
    this->SetSize(vtkVector2i(n, n));
  }
}

void vtkScatterPlotMatrix::SetColumnVisibility(const vtkStdString &name,
                                               bool visible)
{
  if (visible)
  {
    for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      if (this->VisibleColumns->GetValue(i) == name)
      {
        // Already there, nothing more needs to be done
        return;
      }
    }
    // Add the column to the end of the list if it is a numeric column
    if (this->Input && this->Input->GetColumnByName(name.c_str()) &&
        vtkArrayDownCast<vtkDataArray>(this->Input->GetColumnByName(name.c_str())))
    {
      this->VisibleColumns->InsertNextValue(name);
      this->Private->VisibleColumnsModified = true;
      this->SetSize(vtkVector2i(0, 0));
      this->SetSize(vtkVector2i(this->VisibleColumns->GetNumberOfTuples(),
                                this->VisibleColumns->GetNumberOfTuples()));
      this->Modified();
    }
  }
  else
  {
    // Remove the value if present
    for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      if (this->VisibleColumns->GetValue(i) == name)
      {
        // Move all the later elements down by one, and reduce the size
        while (i < this->VisibleColumns->GetNumberOfTuples() - 1)
        {
          this->VisibleColumns->SetValue(i,
                                         this->VisibleColumns->GetValue(i + 1));
          ++i;
        }
        this->VisibleColumns->SetNumberOfTuples(
            this->VisibleColumns->GetNumberOfTuples() - 1);
        this->SetSize(vtkVector2i(0, 0));
        this->SetSize(vtkVector2i(this->VisibleColumns->GetNumberOfTuples(),
                                  this->VisibleColumns->GetNumberOfTuples()));
        if (this->ActivePlot.GetX() + this->ActivePlot.GetY() + 1 >=
            this->VisibleColumns->GetNumberOfTuples())
        {
          this->ActivePlot.Set(0, this->VisibleColumns->GetNumberOfTuples() - 1);
        }
        this->Private->VisibleColumnsModified = true;
        this->Modified();
      }
    }
  }
}

void vtkScatterPlotMatrix::InsertVisibleColumn(const vtkStdString &name,
                                               int index)
{
  if(!this->Input || !this->Input->GetColumnByName(name.c_str()))
  {
    return;
  }

  // Check if the column is already in the list. If yes,
  // we may need to rearrange the order of the columns.
  vtkIdType currIdx = -1;
  vtkIdType numCols = this->VisibleColumns->GetNumberOfTuples();
  for (vtkIdType i = 0; i < numCols; ++i)
  {
    if (this->VisibleColumns->GetValue(i) == name)
    {
      currIdx = i;
      break;
    }
  }

  if(currIdx > 0 && currIdx == index)
  {
    //This column is already there.
    return;
  }

  if(currIdx < 0)
  {
    this->VisibleColumns->SetNumberOfTuples(numCols+1);
    if(index >= numCols)
    {
      this->VisibleColumns->SetValue(numCols, name);
    }
    else // move all the values after index down 1
    {
      vtkIdType startidx = numCols;
      vtkIdType idx = (index < 0) ? 0 : index;
      while (startidx > idx)
      {
        this->VisibleColumns->SetValue(startidx,
          this->VisibleColumns->GetValue(startidx-1));
        startidx--;
      }
      this->VisibleColumns->SetValue(idx, name);
    }
    this->Private->VisibleColumnsModified = true;
  }
  else // need to rearrange table columns
  {
    vtkIdType toIdx = (index < 0) ? 0 : index;
    toIdx = toIdx>numCols ? numCols : toIdx;
    this->Private->VisibleColumnsModified =
      MoveColumn(this->VisibleColumns.GetPointer(), currIdx, toIdx);
  }
  this->LayoutIsDirty = true;
}

bool vtkScatterPlotMatrix::GetColumnVisibility(const vtkStdString &name)
{
  for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
  {
    if (this->VisibleColumns->GetValue(i) == name)
    {
      return true;
    }
  }
  return false;
}

void vtkScatterPlotMatrix::SetColumnVisibilityAll(bool visible)
{
  if (visible && this->Input)
  {
    vtkIdType n = this->Input->GetNumberOfColumns();
    this->VisibleColumns->SetNumberOfTuples(n);
    for (vtkIdType i = 0; i < n; ++i)
    {
      this->VisibleColumns->SetValue(i, this->Input->GetColumnName(i));
    }
  }
  else
  {
    this->SetSize(vtkVector2i(0, 0));
    this->VisibleColumns->SetNumberOfTuples(0);
  }

  this->Private->VisibleColumnsModified = true;
}

vtkStringArray* vtkScatterPlotMatrix::GetVisibleColumns()
{
  return this->VisibleColumns.GetPointer();
}

void vtkScatterPlotMatrix::SetVisibleColumns(vtkStringArray* visColumns)
{
  if(!visColumns || visColumns->GetNumberOfTuples() == 0)
  {
    this->SetSize(vtkVector2i(0, 0));
    this->VisibleColumns->SetNumberOfTuples(0);
  }
  else
  {
    this->VisibleColumns->SetNumberOfTuples(
      visColumns->GetNumberOfTuples());
    this->VisibleColumns->DeepCopy(visColumns);
  }
  this->Private->VisibleColumnsModified = true;
  this->LayoutIsDirty = true;
}

void vtkScatterPlotMatrix::SetNumberOfBins(int numberOfBins)
{
  if (this->NumberOfBins != numberOfBins)
  {
    this->NumberOfBins = numberOfBins;
    if (this->Input)
    {
      PopulateHistograms(this->Input.GetPointer(),
                         this->Private->Histogram.GetPointer(),
                         this->VisibleColumns.GetPointer(),
                         this->NumberOfBins);
    }
    this->Modified();
  }
}

void vtkScatterPlotMatrix::SetPlotColor(int plotType, const vtkColor4ub& color)
{
  if(plotType >= 0 && plotType < NOPLOT)
  {
    if (plotType == ACTIVEPLOT || plotType == SCATTERPLOT)
    {
      this->Private->ChartSettings[plotType]->PlotPen->SetColor(color);
    }
    else
    {
      this->Private->ChartSettings[HISTOGRAM]->PlotBrush->SetColor(color);
    }
    this->Modified();
  }
}

void vtkScatterPlotMatrix::SetPlotMarkerStyle(int plotType, int style)
{
  if(plotType >= 0 && plotType < NOPLOT &&
     style != this->Private->ChartSettings[plotType]->MarkerStyle)
  {
    this->Private->ChartSettings[plotType]->MarkerStyle = style;

    if (plotType == ACTIVEPLOT)
    {
      vtkChart *chart = this->Private->BigChart;
      if (chart)
      {
        vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast(chart->GetPlot(0));
        if (plot)
        {
          plot->SetMarkerStyle(style);
        }
      }
      this->Modified();
    }
    else if (plotType == SCATTERPLOT)
    {
      int plotCount = this->GetSize().GetX();
      for (int i = 0; i < plotCount - 1; ++i)
      {
        for(int j = 0; j < plotCount - 1; ++j)
        {
          if(this->GetPlotType(i, j) == SCATTERPLOT &&
             this->GetChart(vtkVector2i(i, j)))
          {
            vtkChart *chart = this->GetChart(vtkVector2i(i, j));
            vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast(chart->GetPlot(0));
            if (plot)
            {
              plot->SetMarkerStyle(style);
            }
          }
        }
      }
      this->Modified();
    }
  }
}

void vtkScatterPlotMatrix::SetPlotMarkerSize(int plotType, float size)
{
  if(plotType >= 0 && plotType < NOPLOT &&
     size != this->Private->ChartSettings[plotType]->MarkerSize)
  {
    this->Private->ChartSettings[plotType]->MarkerSize = size;

    if (plotType == ACTIVEPLOT)
    {
      // update marker size on current active plot
      vtkChart *chart = this->Private->BigChart;
      if(chart)
      {
        vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast(chart->GetPlot(0));
        if (plot)
        {
          plot->SetMarkerSize(size);
        }
      }
      this->Modified();
    }
    else if (plotType == SCATTERPLOT)
    {
      int plotCount = this->GetSize().GetX();

      for(int i = 0; i < plotCount - 1; i++)
      {
        for(int j = 0; j < plotCount - 1; j++)
        {
          if(this->GetPlotType(i, j) == SCATTERPLOT &&
             this->GetChart(vtkVector2i(i, j)))
          {
            vtkChart *chart = this->GetChart(vtkVector2i(i, j));
            vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast(chart
                                                              ->GetPlot(0));
            if (plot)
            {
              plot->SetMarkerSize(size);
            }
          }
        }
      }
      this->Modified();
    }
  }
}

bool vtkScatterPlotMatrix::Hit(const vtkContextMouseEvent &)
{
  return true;
}

bool vtkScatterPlotMatrix::MouseMoveEvent(const vtkContextMouseEvent &)
{
  // Eat the event, don't do anything for now...
  return true;
}

bool vtkScatterPlotMatrix::MouseButtonPressEvent(const vtkContextMouseEvent &)
{
  return true;
}

bool vtkScatterPlotMatrix::MouseButtonReleaseEvent(
    const vtkContextMouseEvent &mouse)
{
  // Check we are not currently already animating
  if (this->Private->TimerCallbackInitialized)
  {
    return true;
  }

  // Work out which scatter plot was clicked - make that one the active plot.
  vtkVector2i pos = this->GetChartIndex(mouse.GetPos());

  if(pos.GetX() == -1 || pos.GetX() + pos.GetY() + 1 >= this->Size.GetX())
  {
    // We didn't click a chart in the bottom-left triangle of the matrix.
    return true;
  }

  // If the left button was used, hyperjump, if the right was used full path.
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    if (this->NumberOfFrames == 0)
    {
      this->SetActivePlot(pos);
      return true;
    }
    this->Private->AnimationPath.clear();
    bool horizontalFirst = pos[0] > this->ActivePlot[0] ? false : true;
    if (horizontalFirst)
    {
      if (pos[0] != this->ActivePlot[0])
      {
        this->Private->AnimationPath.push_back(vtkVector2i(pos[0],
                                                           this->ActivePlot[1]));
      }
    }
    else
    {
      if (pos[1] != this->ActivePlot[1])
      {
        this->Private->AnimationPath.push_back(vtkVector2i(this->ActivePlot[0],
                                                           pos[1]));
      }
    }
    if ((this->Private->AnimationPath.size() == 1 &&
         this->Private->AnimationPath.back() != pos) ||
        (this->Private->AnimationPath.size() == 0 && this->ActivePlot != pos))
    {
      this->Private->AnimationPath.push_back(pos);
    }
    if (this->Private->AnimationPath.size() > 0)
    {
      this->InvokeEvent(vtkCommand::CreateTimerEvent);
      this->StartAnimation(mouse.GetInteractor());
    }
  }
  else if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON)
  {
    if (this->NumberOfFrames == 0)
    {
      this->SetActivePlot(pos);
      return true;
    }
    this->UpdateAnimationPath(pos);
    if (this->Private->AnimationPath.size() > 0)
    {
      this->InvokeEvent(vtkCommand::CreateTimerEvent);
      this->StartAnimation(mouse.GetInteractor());
    }
    else
    {
      this->SetActivePlot(pos);
    }
  }

  return true;
}

void vtkScatterPlotMatrix::SetNumberOfFrames(int frames)
{
  this->NumberOfFrames = frames;
}

int vtkScatterPlotMatrix::GetNumberOfFrames()
{
  return this->NumberOfFrames;
}

void vtkScatterPlotMatrix::ClearAnimationPath()
{
  this->Private->AnimationPath.clear();
}

vtkIdType vtkScatterPlotMatrix::GetNumberOfAnimationPathElements()
{
  return static_cast<vtkIdType>(this->Private->AnimationPath.size());
}

vtkVector2i vtkScatterPlotMatrix::GetAnimationPathElement(vtkIdType i)
{
  return this->Private->AnimationPath.at(i);
}

bool vtkScatterPlotMatrix::AddAnimationPath(const vtkVector2i &move)
{
  vtkVector2i pos = this->ActivePlot;
  if (this->Private->AnimationPath.size())
  {
    pos = this->Private->AnimationPath.back();
  }
  if (move.GetX() != pos.GetX() && move.GetY() != pos.GetY())
  {
    // Can only move in x or y, not both. Do not append the element.
    return false;
  }
  else
  {
    this->Private->AnimationPath.push_back(move);
    return true;
  }
}

bool vtkScatterPlotMatrix::BeginAnimationPath(vtkRenderWindowInteractor* interactor)
{
  if (interactor && this->Private->AnimationPath.size())
  {
    this->StartAnimation(interactor);
    return true;
  }
  else
  {
    return false;
  }
}

int vtkScatterPlotMatrix::GetPlotType(const vtkVector2i &pos)
{
  int plotCount = this->GetSize().GetX();

  if(pos.GetX() + pos.GetY() + 1 < plotCount)
  {
    return SCATTERPLOT;
  }
  else if(pos.GetX() + pos.GetY() + 1 == plotCount)
  {
    return HISTOGRAM;
  }
  else if(pos.GetX() == pos.GetY() &&
          pos.GetX() == static_cast<int>(plotCount / 2.0) + plotCount % 2)
  {
    return ACTIVEPLOT;
  }
  else
  {
      return NOPLOT;
  }
}

int vtkScatterPlotMatrix::GetPlotType(int row, int column)
{
  return this->GetPlotType(vtkVector2i(row, column));
}

void vtkScatterPlotMatrix::UpdateAxes()
{
  if (!this->Input)
  {
    return;
  }
  // We need to iterate through all visible columns and set up the axis ranges.
  vtkAxis *axis(this->Private->TestAxis.GetPointer());
  axis->SetPoint1(0, 0);
  axis->SetPoint2(0, 200);
  for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
  {
    double range[2] = { 0, 0 };
    std::string name(this->VisibleColumns->GetValue(i));
    vtkDataArray *arr =
        vtkArrayDownCast<vtkDataArray>(this->Input->GetColumnByName(name.c_str()));
    if (arr)
    {
      PIMPL::ColumnSetting settings;
      arr->GetRange(range);
      // Apply a little padding either side of the ranges.
      range[0] = range[0] - (0.01 * range[0]);
      range[1] = range[1] + (0.01 * range[1]);
      axis->SetUnscaledRange(range);
      axis->AutoScale();
      settings.min = axis->GetUnscaledMinimum();
      settings.max = axis->GetUnscaledMaximum();
      settings.nTicks = axis->GetNumberOfTicks();
      settings.title = name;
      this->Private->ColumnSettings[name] = settings;
    }
    else
    {
      vtkDebugMacro(<< "No valid data array available. " << name);
    }
  }
}

vtkStdString vtkScatterPlotMatrix::GetColumnName(int column)
{
  assert(column < this->VisibleColumns->GetNumberOfTuples());
  return this->VisibleColumns->GetValue(column);
}

vtkStdString vtkScatterPlotMatrix::GetRowName(int row)
{
  assert(row < this->VisibleColumns->GetNumberOfTuples());
  return this->VisibleColumns->GetValue(this->Size.GetY() - row - 1);
}

void vtkScatterPlotMatrix::ApplyAxisSetting(vtkChart *chart,
                                            const vtkStdString &x,
                                            const vtkStdString &y)
{
  PIMPL::ColumnSetting &xSettings = this->Private->ColumnSettings[x];
  PIMPL::ColumnSetting &ySettings = this->Private->ColumnSettings[y];
  vtkAxis *axis = chart->GetAxis(vtkAxis::BOTTOM);
  axis->SetUnscaledRange(xSettings.min, xSettings.max);
  axis->SetBehavior(vtkAxis::FIXED);
  axis = chart->GetAxis(vtkAxis::TOP);
  axis->SetUnscaledRange(xSettings.min, xSettings.max);
  axis->SetBehavior(vtkAxis::FIXED);
  axis = chart->GetAxis(vtkAxis::LEFT);
  axis->SetUnscaledRange(ySettings.min, ySettings.max);
  axis->SetBehavior(vtkAxis::FIXED);
  axis = chart->GetAxis(vtkAxis::RIGHT);
  axis->SetUnscaledRange(ySettings.min, ySettings.max);
  axis->SetBehavior(vtkAxis::FIXED);
}

void vtkScatterPlotMatrix::UpdateLayout()
{
  // We want scatter plots on the lower-left triangle, then histograms along
  // the diagonal and a big plot in the top-right. The basic layout is,
  //
  // 3 H   +++
  // 2 S H +++
  // 1 S S H
  // 0 S S S H
  //   0 1 2 3
  //
  // Where the indices are those of the columns. The indices of the charts
  // originate in the bottom-left. S = scatter plot, H = histogram and + is the
  // big chart.
  this->LayoutUpdatedTime = this->GetMTime();
  int n = this->Size.GetX();
  this->UpdateAxes();
  this->Private->BigChart3D->SetAnnotationLink(this->Private->Link.GetPointer());
  for (int i = 0; i < n; ++i)
    {
    vtkStdString column = this->GetColumnName(i);
    for (int j = 0; j < n; ++j)
      {
      vtkStdString row = this->GetRowName(j);
      vtkVector2i pos(i, j);
      if (this->GetPlotType(pos) == SCATTERPLOT)
        {
        vtkChart* chart = this->GetChart(pos);
        this->ApplyAxisSetting(chart, column, row);
        chart->ClearPlots();
        chart->SetInteractive(false);
        chart->SetAnnotationLink(this->Private->Link.GetPointer());
        // Lower-left triangle - scatter plots.
        chart->SetActionToButton(vtkChart::PAN, -1);
        chart->SetActionToButton(vtkChart::ZOOM, -1);
        chart->SetActionToButton(vtkChart::SELECT, -1);
        vtkPlot *plot = chart->AddPlot(vtkChart::POINTS);
        plot->SetInputData(this->Input.GetPointer(), column, row);
        plot->SetPen(this->Private->ChartSettings[SCATTERPLOT]
                     ->PlotPen.GetPointer());
        // set plot marker size and style
        vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast(plot);
        plotPoints->SetMarkerSize(this->Private->ChartSettings[SCATTERPLOT]
                                  ->MarkerSize);
        plotPoints->SetMarkerStyle(this->Private->ChartSettings[SCATTERPLOT]
                                   ->MarkerStyle);
        }
      else if (this->GetPlotType(pos) == HISTOGRAM)
        {
        // We are on the diagonal - need a histogram plot.
        vtkChart* chart = this->GetChart(pos);
        chart->SetInteractive(false);
        this->ApplyAxisSetting(chart, column, row);
        chart->ClearPlots();
        vtkPlot *plot = chart->AddPlot(vtkChart::BAR);
        plot->SetPen(this->Private->ChartSettings[HISTOGRAM]
                     ->PlotPen.GetPointer());
        plot->SetBrush(this->Private->ChartSettings[HISTOGRAM]
                       ->PlotBrush.GetPointer());
        vtkStdString name(this->VisibleColumns->GetValue(i));
        plot->SetInputData(this->Private->Histogram.GetPointer(),
                           name + "_extents", name + "_pops");
        vtkAxis *axis = chart->GetAxis(vtkAxis::TOP);
        axis->SetTitle(name);
        axis->SetLabelsVisible(false);
        // Show the labels on the right for populations of bins.
        axis = chart->GetAxis(vtkAxis::RIGHT);
        axis->SetLabelsVisible(true);
        axis->SetBehavior(vtkAxis::AUTO);
        axis->AutoScale();
        // Set the plot corner to the top-right
        vtkChartXY *xy = vtkChartXY::SafeDownCast(chart);
        if (xy)
          {
          xy->SetBarWidthFraction(1.0);
          xy->SetPlotCorner(plot, 2);
          }

        // set background color to light gray
        xy->SetBackgroundBrush(this->Private->ChartSettings[HISTOGRAM]
                               ->BackgroundBrush.GetPointer());
        }
      else if (this->GetPlotType(pos) == ACTIVEPLOT)
        {
        // This big plot in the top-right
        this->Private->BigChart = this->GetChart(pos);
        this->Private->BigChartPos = pos;
        this->Private->BigChart->SetAnnotationLink(
              this->Private->Link.GetPointer());
        this->Private->BigChart->AddObserver(
          vtkCommand::SelectionChangedEvent, this,
          &vtkScatterPlotMatrix::BigChartSelectionCallback);

        // set tooltip item
        vtkChartXY *chartXY =
          vtkChartXY::SafeDownCast(this->Private->BigChart.GetPointer());
        if(chartXY)
          {
          chartXY->SetTooltip(this->Private->TooltipItem);
          }

        this->SetChartSpan(pos, vtkVector2i(n - i, n - j));
        this->SetActivePlot(vtkVector2i(0, n - 2));
        }
      // Only show bottom axis label for bottom plots
      if (j > 0)
        {
        vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::BOTTOM);
        axis->SetTitle("");
        axis->SetLabelsVisible(false);
        axis->SetBehavior(vtkAxis::FIXED);
        }
      else
        {
        vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::BOTTOM);
        axis->SetTitle(this->VisibleColumns->GetValue(i));
        axis->SetLabelsVisible(false);
        this->AttachAxisRangeListener(axis);
        }
      // Only show the left axis labels for left-most plots
      if (i > 0)
        {
        vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::LEFT);
        axis->SetTitle("");
        axis->SetLabelsVisible(false);
        axis->SetBehavior(vtkAxis::FIXED);
        }
      else
        {
        vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::LEFT);
        axis->SetTitle(this->VisibleColumns->GetValue(n - j - 1));
        axis->SetLabelsVisible(false);
        this->AttachAxisRangeListener(axis);
        }
      }
    }
}

void vtkScatterPlotMatrix::ResizeBigChart()
{
  if (!this->Private->ResizingBigChart)
    {
    this->ClearSpecificResizes();
    int n = this->Size.GetX();
    // The big chart need to be resized only when it is
    // "between" the histograms, ie. when n is even.
    if (n%2 == 0)
      {
      // 30*30 is an acceptable default size to resize with
      int resizeX = 30;
      int resizeY = 30;
      if (this->CurrentPainter)
        {
        // Try to use painter to resize the big plot
        int i = this->Private->BigChartPos.GetX();
        int j = this->Private->BigChartPos.GetY();
        vtkVector2i posLeft(i - 1, j);
        vtkVector2i posBottom(i, j - 1);
        vtkChart* leftChart = this->GetChart(posLeft);
        vtkChart* bottomChart = this->GetChart(posLeft);
        if (leftChart)
          {
          vtkAxis* leftAxis = leftChart->GetAxis(vtkAxis::RIGHT);
          if (leftAxis)
            {
            resizeX = std::max(leftAxis->GetBoundingRect(
              this->CurrentPainter).GetWidth() - this->Gutter.GetX(), this->Gutter.GetX());
            }
          }
        if (bottomChart)
          {
          vtkAxis* bottomAxis = bottomChart->GetAxis(vtkAxis::TOP);
          if (bottomAxis)
            {
            resizeY = std::max(bottomAxis->GetBoundingRect(
              this->CurrentPainter).GetHeight() - this->Gutter.GetY(), this->Gutter.GetY());
            }
          }
        }

      // Move big plot bottom left point to avoid overlap
      vtkVector2f resize(resizeX, resizeY);
      this->SetSpecificResize(this->Private->BigChartPos, resize);
      if (this->LayoutIsDirty)
        {
        this->Private->ResizingBigChart = true;
        this->GetScene()->SetDirty(true);
        }
      }
    }
  else
    {
    this->Private->ResizingBigChart = false;
    }
}

void vtkScatterPlotMatrix::AttachAxisRangeListener(vtkAxis* axis)
{
  axis->AddObserver(vtkChart::UpdateRange, this,
                    &vtkScatterPlotMatrix::AxisRangeForwarderCallback);
}

void vtkScatterPlotMatrix::AxisRangeForwarderCallback(vtkObject*,
                                                      unsigned long, void*)
{
  // Only set on the end axes, and propagated to all other matching axes.
  double r[2];
  int n = this->GetSize().GetX() - 1;
  for (int i = 0; i < n; ++i)
  {
    this->GetChart(vtkVector2i(i, 0))
      ->GetAxis(vtkAxis::BOTTOM)
      ->GetUnscaledRange(r);
    for (int j = 1; j < n - i; ++j)
    {
      this->GetChart(vtkVector2i(i, j))
        ->GetAxis(vtkAxis::BOTTOM)
        ->SetUnscaledRange(r);
    }
    this->GetChart(vtkVector2i(i, n-i))
      ->GetAxis(vtkAxis::TOP)
      ->SetUnscaledRange(r);
    this->GetChart(vtkVector2i(0, i))
      ->GetAxis(vtkAxis::LEFT)
      ->GetUnscaledRange(r);
    for (int j = 1; j < n - i; ++j)
    {
      this->GetChart(vtkVector2i(j, i))
        ->GetAxis(vtkAxis::LEFT)
        ->SetUnscaledRange(r);
    }
  }
}

void vtkScatterPlotMatrix::BigChartSelectionCallback(vtkObject*,
  unsigned long event, void*)
{
  // forward the SelectionChangedEvent from the Big Chart plot
  this->InvokeEvent(event);
}

void vtkScatterPlotMatrix::SetTitle(const vtkStdString& title)
{
  if (this->Title != title)
  {
    this->Title = title;
    this->Modified();
  }
}

vtkStdString vtkScatterPlotMatrix::GetTitle()
{
  return this->Title;
}

void vtkScatterPlotMatrix::SetTitleProperties(vtkTextProperty *prop)
{
  if (this->TitleProperties != prop)
  {
    this->TitleProperties = prop;
    this->Modified();
  }
}

vtkTextProperty* vtkScatterPlotMatrix::GetTitleProperties()
{
  return this->TitleProperties.GetPointer();
}

void vtkScatterPlotMatrix::SetAxisLabelProperties(int plotType,
                                                  vtkTextProperty *prop)
{
  if (plotType >= 0 && plotType < vtkScatterPlotMatrix::NOPLOT &&
      this->Private->ChartSettings[plotType]->LabelFont != prop)
  {
    this->Private->ChartSettings[plotType]->LabelFont = prop;
    this->Modified();
  }
}

vtkTextProperty* vtkScatterPlotMatrix::GetAxisLabelProperties(int plotType)
{
  if (plotType >= 0 && plotType < vtkScatterPlotMatrix::NOPLOT)
  {
    return this->Private->ChartSettings[plotType]->LabelFont.GetPointer();
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetBackgroundColor(int plotType,
                                              const vtkColor4ub& color)
{
  if (plotType >= 0 && plotType < vtkScatterPlotMatrix::NOPLOT)
  {
    this->Private->ChartSettings[plotType]->BackgroundBrush->SetColor(color);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetAxisColor(int plotType,
                                        const vtkColor4ub& color)
{
  if (plotType >= 0 && plotType < vtkScatterPlotMatrix::NOPLOT)
  {
    this->Private->ChartSettings[plotType]->AxisColor = color;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetGridVisibility(int plotType, bool visible)
{
  if(plotType!= NOPLOT)
  {
    this->Private->ChartSettings[plotType]->ShowGrid = visible;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetGridColor(int plotType,
                                        const vtkColor4ub& color)
{
  if (plotType >= 0 && plotType < vtkScatterPlotMatrix::NOPLOT)
  {
    this->Private->ChartSettings[plotType]->GridColor = color;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetAxisLabelVisibility(int plotType, bool visible)
{
  if(plotType!= NOPLOT)
  {
    this->Private->ChartSettings[plotType]->ShowAxisLabels = visible;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetAxisLabelNotation(int plotType, int notation)
{
  if(plotType!= NOPLOT)
  {
    this->Private->ChartSettings[plotType]->LabelNotation = notation;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetAxisLabelPrecision(int plotType, int precision)
{
  if(plotType!= NOPLOT)
  {
    this->Private->ChartSettings[plotType]->LabelPrecision = precision;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetTooltipNotation(int plotType, int notation)
{
  if(plotType!= NOPLOT)
  {
    this->Private->ChartSettings[plotType]->TooltipNotation = notation;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetTooltipPrecision(int plotType, int precision)
{
  if(plotType!= NOPLOT)
  {
    this->Private->ChartSettings[plotType]->TooltipPrecision = precision;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetScatterPlotSelectedRowColumnColor(
    const vtkColor4ub& color)
{
  this->Private->SelectedRowColumnBGBrush->SetColor(color);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetScatterPlotSelectedActiveColor(
    const vtkColor4ub& color)
{
  this->Private->SelectedChartBGBrush->SetColor(color);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::UpdateChartSettings(int plotType)
{
  if (plotType == HISTOGRAM)
  {
    int plotCount = this->GetSize().GetX();

    for(int i = 0; i < plotCount; i++)
    {
      vtkChart *chart = this->GetChart(vtkVector2i(i, plotCount - i - 1));
      this->Private->UpdateAxis(chart->GetAxis(vtkAxis::TOP),
                                this->Private->ChartSettings[HISTOGRAM]);
      this->Private->UpdateAxis(chart->GetAxis(vtkAxis::RIGHT),
                                this->Private->ChartSettings[HISTOGRAM]);
      this->Private->UpdateChart(chart, this->Private->ChartSettings[HISTOGRAM]);
    }
  }
  else if (plotType == SCATTERPLOT)
  {
    int plotCount = this->GetSize().GetX();

    for (int i = 0; i < plotCount - 1; i++)
    {
      for (int j = 0; j < plotCount - 1; j++)
      {
        if (this->GetPlotType(i, j) == SCATTERPLOT)
        {
          vtkChart *chart = this->GetChart(vtkVector2i(i, j));
          bool updateleft = i==0 ? true : false;
          bool updatebottom = j==0 ? true : false;
          this->Private->UpdateAxis(chart->GetAxis(vtkAxis::LEFT),
            this->Private->ChartSettings[SCATTERPLOT], updateleft);
          this->Private->UpdateAxis(chart->GetAxis(vtkAxis::BOTTOM),
            this->Private->ChartSettings[SCATTERPLOT], updatebottom);
        }
      }
    }
  }
  else if (plotType == ACTIVEPLOT && this->Private->BigChart)
  {
    this->Private->UpdateAxis(this->Private->BigChart->GetAxis(vtkAxis::TOP),
                              this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->UpdateAxis(this->Private->BigChart->GetAxis(vtkAxis::RIGHT),
                              this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->UpdateChart(this->Private->BigChart,
                               this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->BigChart->SetSelectionMode(this->SelectionMode);
  }
  this->Modified();
}
//-----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetSelectionMode(int selMode)
{
  if (this->SelectionMode == selMode ||
      selMode < vtkContextScene::SELECTION_NONE ||
      selMode > vtkContextScene::SELECTION_TOGGLE)
  {
    return;
  }
  this->SelectionMode = selMode;
  if(this->Private->BigChart)
  {
    this->Private->BigChart->SetSelectionMode(selMode);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::UpdateSettings()
{

// TODO: Should update the Scatter plot title

  this->UpdateChartSettings(ACTIVEPLOT);
  this->UpdateChartSettings(HISTOGRAM);
  this->UpdateChartSettings(SCATTERPLOT);
}

//----------------------------------------------------------------------------
bool vtkScatterPlotMatrix::GetGridVisibility(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->ShowGrid;
}

//----------------------------------------------------------------------------
vtkColor4ub vtkScatterPlotMatrix::GetBackgroundColor(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->BackgroundBrush
      ->GetColorObject();
}

//----------------------------------------------------------------------------
vtkColor4ub vtkScatterPlotMatrix::GetAxisColor(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->AxisColor;
}

//----------------------------------------------------------------------------
vtkColor4ub vtkScatterPlotMatrix::GetGridColor(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->GridColor;
}

//----------------------------------------------------------------------------
bool vtkScatterPlotMatrix::GetAxisLabelVisibility(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->ShowAxisLabels;
}

//----------------------------------------------------------------------------
int vtkScatterPlotMatrix::GetAxisLabelNotation(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->LabelNotation;
}

//----------------------------------------------------------------------------
int vtkScatterPlotMatrix::GetAxisLabelPrecision(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->LabelPrecision;
}

//----------------------------------------------------------------------------
int vtkScatterPlotMatrix::GetTooltipNotation(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->TooltipNotation;
}

int vtkScatterPlotMatrix::GetTooltipPrecision(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->TooltipPrecision;
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetTooltip(vtkTooltipItem *tooltip)
{
  if(tooltip != this->Private->TooltipItem)
  {
    this->Private->TooltipItem = tooltip;
    this->Modified();

    vtkChartXY *chartXY =
      vtkChartXY::SafeDownCast(this->Private->BigChart.GetPointer());

    if(chartXY)
    {
      chartXY->SetTooltip(tooltip);
    }
  }
}

//----------------------------------------------------------------------------
vtkTooltipItem* vtkScatterPlotMatrix::GetTooltip() const
{
  return this->Private->TooltipItem;
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::SetIndexedLabels(vtkStringArray *labels)
{
  if(labels != this->Private->IndexedLabelsArray)
  {
    this->Private->IndexedLabelsArray = labels;
    this->Modified();

    if(this->Private->BigChart)
    {
      vtkPlot *plot = this->Private->BigChart->GetPlot(0);

      if(plot)
      {
        plot->SetIndexedLabels(labels);
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkStringArray* vtkScatterPlotMatrix::GetIndexedLabels() const
{
  return this->Private->IndexedLabelsArray;
}

//----------------------------------------------------------------------------
vtkColor4ub vtkScatterPlotMatrix::GetScatterPlotSelectedRowColumnColor()
{
  return this->Private->SelectedRowColumnBGBrush->GetColorObject();
}

//----------------------------------------------------------------------------
vtkColor4ub vtkScatterPlotMatrix::GetScatterPlotSelectedActiveColor()
{
  return this->Private->SelectedChartBGBrush->GetColorObject();
}

//----------------------------------------------------------------------------
vtkChart * vtkScatterPlotMatrix::GetMainChart()
{
  return this->Private->BigChart.GetPointer();
}

//----------------------------------------------------------------------------
void vtkScatterPlotMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfBins: " << this->NumberOfBins << endl;
  os << indent << "Title: " << this->Title << endl;
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
}
