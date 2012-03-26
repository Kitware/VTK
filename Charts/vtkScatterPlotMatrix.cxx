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

#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkAxis.h"
#include "vtkContextMouseEvent.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkMathUtilities.h"
#include "vtkAnnotationLink.h"
#include "vtkObjectFactory.h"
#include "vtkBrush.h"
#include "vtkPlotPoints.h"
#include "vtkCommand.h"
#include "vtkTextProperty.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"

// STL includes
#include <map>
#include <cassert>
#include <vector>

class vtkScatterPlotMatrix::PIMPL
{
public:
  PIMPL() : VisibleColumnsModified(true), BigChart(NULL)
  {
    pimplChartSetting* scatterplotSettings = new pimplChartSetting();
    scatterplotSettings->BackgroundBrush->SetColor(255, 255, 255, 255);
    this->ChartSettings[vtkScatterPlotMatrix::SCATTERPLOT] =
        scatterplotSettings;
    pimplChartSetting* histogramSettings = new pimplChartSetting();
    histogramSettings->BackgroundBrush->SetColor(127, 127, 127, 102);
    histogramSettings->PlotPen->SetColor(255, 255, 255, 255);
    this->ChartSettings[vtkScatterPlotMatrix::HISTOGRAM] = histogramSettings;
    pimplChartSetting* activeplotSettings = new pimplChartSetting();
    activeplotSettings->BackgroundBrush->SetColor(255, 255, 255, 255);
    this->ChartSettings[vtkScatterPlotMatrix::ACTIVEPLOT] = activeplotSettings;
    activeplotSettings->MarkerSize = 8.0;
    this->SelectedChartBGBrush->SetColor(0, 204, 0, 102);
    this->SelectedRowColumnBGBrush->SetColor(204, 0, 0, 102);
  }

  ~PIMPL()
  {
    delete this->ChartSettings[vtkScatterPlotMatrix::SCATTERPLOT];
    delete this->ChartSettings[vtkScatterPlotMatrix::HISTOGRAM];
    delete this->ChartSettings[vtkScatterPlotMatrix::ACTIVEPLOT];
  }

  class pimplChartSetting
  {
  public:
    pimplChartSetting()
    {
      this->PlotPen->SetColor(0, 0, 0, 255);
      this->MarkerStyle = vtkPlotPoints::CIRCLE;
      this->MarkerSize = 5.0;
      this->AxisColor.Set(0, 0, 0, 1);
      this->GridColor.Set(242, 242, 242, 255);
      this->LabelNotation = vtkAxis::STANDARD_NOTATION;
      this->LabelPrecision = 2;
      this->TooltipNotation = vtkAxis::STANDARD_NOTATION;
      this->TooltipPrecision = 2;
      this->ShowGrid = true;
      this->ShowAxisLabels = true;
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
                  bool updateLabel=true)
    {
    if(axis && setting)
      {
      axis->GetPen()->SetColor(setting->AxisColor);
      axis->GetGridPen()->SetColor(setting->GridColor);
      axis->SetGridVisible(setting->ShowGrid);
      if(updateLabel)
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
  vtkNew<vtkAnnotationLink> Link;

  std::map<int, pimplChartSetting*> ChartSettings;
  typedef std::map<int, pimplChartSetting*>::iterator chartIterator;

  vtkNew<vtkBrush> SelectedRowColumnBGBrush;
  vtkNew<vtkBrush> SelectedChartBGBrush;
  std::vector< vtkVector2i > AnimationPath;
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
        vtkDataArray::SafeDownCast(input->GetColumnByName(name.c_str()));
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
          vtkFloatArray::SafeDownCast(
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
          vtkIntArray::SafeDownCast(
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
}

vtkStandardNewMacro(vtkScatterPlotMatrix)

vtkScatterPlotMatrix::vtkScatterPlotMatrix() : NumberOfBins(10)
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
}

bool vtkScatterPlotMatrix::Paint(vtkContext2D *painter)
{
  this->Update();
  return Superclass::Paint(painter);
}

bool vtkScatterPlotMatrix::SetActivePlot(const vtkVector2i &pos)
{
  if (pos.X() + pos.Y() + 1 < this->Size.X() && pos.X() < this->Size.X() &&
      pos.Y() < this->Size.Y())
    {
    // The supplied index is valid (in the lower quadrant).
    this->ActivePlot = pos;

    // set background colors for plots
    if (this->GetChart(this->ActivePlot)->GetPlot(0))
      {
      int plotCount = this->GetSize().X();
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
      if (!plot)
        {
        plot = this->Private->BigChart->AddPlot(vtkChart::POINTS);
        vtkChart *active = this->GetChart(this->ActivePlot);
        vtkChartXY *xy = vtkChartXY::SafeDownCast(this->Private->BigChart);
        if (xy)
          {
          xy->SetPlotCorner(plot, 2);
          }
        if (xy && active)
          {
          vtkAxis *a = active->GetAxis(vtkAxis::BOTTOM);
          xy->GetAxis(vtkAxis::TOP)->SetRange(a->GetMinimum(), a->GetMaximum());
          a = active->GetAxis(vtkAxis::LEFT);
          xy->GetAxis(vtkAxis::RIGHT)->SetRange(a->GetMinimum(),
                                                a->GetMaximum());
          }
        }
      plot->SetInput(this->Input.GetPointer(),
                     this->VisibleColumns->GetValue(pos.X()),
                     this->VisibleColumns->GetValue(this->Size.X() -
                                                    pos.Y() - 1));
      plot->SetPen(this->Private->ChartSettings[ACTIVEPLOT]
                   ->PlotPen.GetPointer());

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
      // Calculate the ideal range.
      this->Private->BigChart->RecalculateBounds();
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

void vtkScatterPlotMatrix::UpdateAnimationPath(
  const vtkVector2i& newActivePos)
{
  this->Private->AnimationPath.clear();
  if(newActivePos[0] != this->ActivePlot[0] ||
    newActivePos[1] != this->ActivePlot[1])
    {
    if(newActivePos[1] >= this->ActivePlot[1])
      {
      // x direction first
      if(this->ActivePlot[0]>newActivePos[0])
        {
        for(int r=this->ActivePlot[0]-1; r>=newActivePos[0]; r--)
          this->Private->AnimationPath.push_back(
          vtkVector2i(r, this->ActivePlot[1]));
        }
      else
        {
        for(int r=this->ActivePlot[0]+1; r<=newActivePos[0]; r++)
          this->Private->AnimationPath.push_back(
          vtkVector2i(r, this->ActivePlot[1]));
        }
      // then y direction
      for(int c=this->ActivePlot[1]+1; c<=newActivePos[1]; c++)
        this->Private->AnimationPath.push_back(
          vtkVector2i(newActivePos[0], c));
      }
    else
      {
      // y direction first
      for(int c=this->ActivePlot[1]-1; c>=newActivePos[1]; c--)
        this->Private->AnimationPath.push_back(
        vtkVector2i(this->ActivePlot[0], c));       
      // then x direction
      if(this->ActivePlot[0]>newActivePos[0])
        {
        for(int r=this->ActivePlot[0]-1; r>=newActivePos[0]; r--)
          this->Private->AnimationPath.push_back(
          vtkVector2i(r, newActivePos[1]));
        }
      else
        {
        for(int r=this->ActivePlot[0]+1; r<=newActivePos[0]; r++)
          this->Private->AnimationPath.push_back(
          vtkVector2i(r, newActivePos[1]));
        }
      }
    }
}

void vtkScatterPlotMatrix::StartAnimation(
  vtkRenderWindowInteractor* interactor)
{
  for(std::vector<vtkVector2i>::iterator iter =
    this->Private->AnimationPath.begin();
    iter != this->Private->AnimationPath.end(); iter++)
    {
    this->SetActivePlot(*iter);
    this->GetScene()->SetDirty(true);
    interactor->Render();
    }
}

#ifndef VTK_LEGACY_REMOVE
vtkAnnotationLink* vtkScatterPlotMatrix::GetActiveAnnotationLink()
{
  // Never made it into a release, deprecating for shorter, more consistent
  // naming of the function.
  VTK_LEGACY_REPLACED_BODY(vtkScatterPlotMatrix::GetActiveAnnotationLink,
                           "VTK 5.8",
                           vtkScatterPlotMatrix::GetAnnotationLink);
  return this->GetAnnotationLink();
}
#endif

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
    // Add the column to the end of the list
    this->VisibleColumns->InsertNextValue(name);
    this->Private->VisibleColumnsModified = true;
    this->SetSize(vtkVector2i(0, 0));
    this->SetSize(vtkVector2i(this->VisibleColumns->GetNumberOfTuples(),
                              this->VisibleColumns->GetNumberOfTuples()));
    this->Modified();
    }
  else
    {
    // Remove the value if present
    for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
      {
      if (this->VisibleColumns->GetValue(i) == name)
        {
        // Move all the later elements down by one, and reduce the size
        while (i < this->VisibleColumns->GetNumberOfTuples()-1)
          {
          this->VisibleColumns->SetValue(i,
                                         this->VisibleColumns->GetValue(i+1));
          ++i;
          }
        this->VisibleColumns->SetNumberOfTuples(
            this->VisibleColumns->GetNumberOfTuples()-1);
        this->SetSize(vtkVector2i(0, 0));
        this->SetSize(vtkVector2i(this->VisibleColumns->GetNumberOfTuples(),
                                  this->VisibleColumns->GetNumberOfTuples()));
        if (this->ActivePlot.X() + this->ActivePlot.Y() + 1 >=
            this->VisibleColumns->GetNumberOfTuples())
          {
          this->ActivePlot.Set(0, this->VisibleColumns->GetNumberOfTuples() - 1);
          }
        this->Private->VisibleColumnsModified = true;

        this->Modified();
        return;
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

  this->Update();
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
  this->Update();
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
      int plotCount = this->GetSize().X();
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
      int plotCount = this->GetSize().X();

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
  // Work out which scatter plot was clicked - make that one the active plot.
  int n = this->GetSize().X();
  for (int i = 0; i < n; ++i)
    {
    for (int j = 0; j < n; ++j)
      {
      if (i + j + 1 < n && this->GetChart(vtkVector2i(i, j))->Hit(mouse))
        {
        vtkVector2i pos(i, j);
        this->UpdateAnimationPath(pos);
        if(this->Private->AnimationPath.size()>0)
          {
          this->StartAnimation(mouse.GetInteractor());
          }
        else
          {
          this->SetActivePlot(pos);
          }

        return true;
        }
      }
    }
  return false;
}

int vtkScatterPlotMatrix::GetPlotType(const vtkVector2i &pos)
{
  int plotCount = this->GetSize().X();

  if(pos.X() + pos.Y() + 1 < plotCount)
    {
    return SCATTERPLOT;
    }
  else if(pos.X() + pos.Y() + 1 == plotCount)
    {
    return HISTOGRAM;
    }
  else if(pos.X() == pos.Y() &&
          pos.X() == static_cast<int>(plotCount / 2.0) + plotCount % 2)
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
  // originate in the bottom-left.
  int n = this->Size.X();
  for (int i = 0; i < n; ++i)
    {
    for (int j = 0; j < n; ++j)
      {
      vtkVector2i pos(i, j);
      if (this->GetPlotType(pos) == SCATTERPLOT)
        {
        vtkChart* chart = this->GetChart(pos);
        chart->ClearPlots();
        chart->SetAnnotationLink(this->Private->Link.GetPointer());
        // Lower-left triangle - scatter plots.
        chart->SetActionToButton(vtkChart::PAN, -1);
        chart->SetActionToButton(vtkChart::ZOOM, -1);
        chart->SetActionToButton(vtkChart::SELECT, -1);
        vtkPlot *plot = chart->AddPlot(vtkChart::POINTS);
        plot->SetInput(this->Input.GetPointer(),
                       this->VisibleColumns->GetValue(i),
                       this->VisibleColumns->GetValue(n - j - 1));
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
        chart->ClearPlots();
        vtkPlot *plot = chart->AddPlot(vtkChart::BAR);
        plot->SetPen(this->Private->ChartSettings[HISTOGRAM]
                     ->PlotPen.GetPointer());
        plot->SetBrush(this->Private->ChartSettings[HISTOGRAM]
                       ->PlotBrush.GetPointer());
        vtkStdString name(this->VisibleColumns->GetValue(i));
        plot->SetInput(this->Private->Histogram.GetPointer(),
                       name + "_extents", name + "_pops");
        vtkAxis *axis = chart->GetAxis(vtkAxis::TOP);
        axis->SetTitle(name);
        if (i != n - 1)
          {
          axis->SetBehavior(vtkAxis::FIXED);
          }
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
        this->Private->BigChart->SetAnnotationLink(
              this->Private->Link.GetPointer());
        this->Private->BigChart->AddObserver(
          vtkCommand::SelectionChangedEvent, this,
          &vtkScatterPlotMatrix::BigChartSelectionCallback);

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
        this->AttachAxisRangeListener(axis);
        }
      }
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
  int n = this->GetSize().X() - 1;
  for (int i = 0; i < n; ++i)
    {
    this->GetChart(vtkVector2i(i, 0))->GetAxis(vtkAxis::BOTTOM)->GetRange(r);
    for (int j = 1; j < n - i; ++j)
      {
      this->GetChart(vtkVector2i(i, j))->GetAxis(vtkAxis::BOTTOM)->SetRange(r);
      }
    this->GetChart(vtkVector2i(i, n-i))->GetAxis(vtkAxis::TOP)->SetRange(r);
    this->GetChart(vtkVector2i(0, i))->GetAxis(vtkAxis::LEFT)->GetRange(r);
    for (int j = 1; j < n - i; ++j)
      {
      this->GetChart(vtkVector2i(j, i))->GetAxis(vtkAxis::LEFT)->SetRange(r);
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
  if(plotType == HISTOGRAM)
    {
    int plotCount = this->GetSize().X();

    for(int i = 0; i < plotCount; i++)
      {
      vtkChart *chart = this->GetChart(vtkVector2i(i, plotCount - i - 1));
      this->Private->UpdateAxis(chart->GetAxis(vtkAxis::TOP),
        this->Private->ChartSettings[HISTOGRAM]);
      this->Private->UpdateAxis(chart->GetAxis(vtkAxis::RIGHT),
        this->Private->ChartSettings[HISTOGRAM]);
      this->Private->UpdateChart(chart,
        this->Private->ChartSettings[HISTOGRAM]);
      }
    }
  else if(plotType == SCATTERPLOT)
    {
    int plotCount = this->GetSize().X();

    for(int i = 0; i < plotCount - 1; i++)
      {
      for(int j = 0; j < plotCount - 1; j++)
        {
        if(this->GetPlotType(i, j) == SCATTERPLOT)
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
  else if(plotType == ACTIVEPLOT && this->Private->BigChart)
    {
    this->Private->UpdateAxis(this->Private->BigChart->GetAxis(
      vtkAxis::TOP), this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->UpdateAxis(this->Private->BigChart->GetAxis(
      vtkAxis::RIGHT), this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->UpdateChart(this->Private->BigChart,
      this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->BigChart->SetSelectionMode(this->SelectionMode);
    }

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
void vtkScatterPlotMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfBins: " << this->NumberOfBins << endl;
  os << indent << "Title: " << this->Title << endl;
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
}
