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

class vtkScatterPlotMatrix::PIMPL
{
public:
  PIMPL() : VisibleColumnsModified(true), BigChart(NULL)
  {
    // default colors to black
    for(int i = 0; i < 3; i++)
      {
      this->ScatterPlotColor[i] = 0.0;
      this->ActivePlotColor[i] = 0.0;
      this->HistogramColor[i] = 0.0;
      }

    this->ScatterPlotMarkerSize = 5;
    this->ActivePlotMarkerSize = 8;
    this->ScatterPlotMarkerStyle = vtkPlotPoints::CIRCLE;
    this->ActivePlotMarkerStyle = vtkPlotPoints::CIRCLE;
  }

  ~PIMPL()
  {
  }

  vtkNew<vtkTable> Histogram;
  bool VisibleColumnsModified;
  vtkChart* BigChart;
  vtkNew<vtkAnnotationLink> Link;
  double ScatterPlotColor[3];
  double ActivePlotColor[3];
  double HistogramColor[3];
  int ScatterPlotMarkerStyle;
  int ActivePlotMarkerStyle;
  double ScatterPlotMarkerSize;
  double ActivePlotMarkerSize;
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
}

vtkStandardNewMacro(vtkScatterPlotMatrix)

vtkScatterPlotMatrix::vtkScatterPlotMatrix() : NumberOfBins(10)
{
  this->Private = new PIMPL;
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
      for(int i = 0; i < plotCount; i++)
        {
        for(int j = 0; j < plotCount; j++)
          {
          if(this->GetPlotType(i, j) == SCATTERPLOT)
            {
            vtkChartXY *chart = vtkChartXY::SafeDownCast(this->GetChart(vtkVector2i(i, j)));

            if(pos[0] == i && pos[1] == j)
              {
              // set the new active chart background color to light green
              chart->GetBackgroundBrush()->SetColorF(0, 0.8, 0, 0.4);
              }
            else if(pos[0] == i || pos[1] == j)
              {
              // set background color for all other charts in the selected chart's row
              // and column to light red
              chart->GetBackgroundBrush()->SetColorF(0.8, 0, 0, 0.4);
              }
            else
              {
              // set all else to white
              chart->GetBackgroundBrush()->SetColorF(1, 1, 1, 0);
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
          xy->GetAxis(vtkAxis::RIGHT)->SetRange(a->GetMinimum(), a->GetMaximum());
          }
        }
      plot->SetInputData(this->Input.GetPointer(),
                         this->VisibleColumns->GetValue(pos.X()),
                         this->VisibleColumns->GetValue(this->Size.X() -
                                                        pos.Y() - 1));
      plot->SetColor(this->Private->ActivePlotColor[0],
                     this->Private->ActivePlotColor[1],
                     this->Private->ActivePlotColor[2]);

      // set marker size and style
      vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast(plot);
      plotPoints->SetMarkerSize(this->Private->ActivePlotMarkerSize);
      plotPoints->SetMarkerStyle(this->Private->ActivePlotMarkerStyle);

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

vtkAnnotationLink* vtkScatterPlotMatrix::GetActiveAnnotationLink()
{
  return this->Private->BigChart ?
    this->Private->BigChart->GetAnnotationLink() : NULL;
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

void vtkScatterPlotMatrix::SetColor(double r, double g, double b)
{
  this->Private->ScatterPlotColor[0] = r;
  this->Private->ScatterPlotColor[1] = g;
  this->Private->ScatterPlotColor[2] = b;

  int plotCount = this->GetSize().X();

  for(int i = 0; i < plotCount - 1; i++)
    {
    for(int j = 0; j < plotCount - 1; j++)
      {
      if(this->GetPlotType(i, j) == SCATTERPLOT)
        {
        vtkChart *chart = this->GetChart(vtkVector2i(i, j));
        vtkPlot *plot = chart->GetPlot(0);
        plot->SetColor(r, g, b);
        plot->Update();
        }
      }
    }

  this->Modified();
}

void vtkScatterPlotMatrix::SetActivePlotColor(double r, double g, double b)
{
  this->Private->ActivePlotColor[0] = r;
  this->Private->ActivePlotColor[1] = g;
  this->Private->ActivePlotColor[2] = b;

  // update color on current active plot
  vtkChart *chart = this->Private->BigChart;
  if(chart)
    {
    chart->GetPlot(0)->SetColor(r, g, b);
    chart->GetPlot(0)->Update();
    }

  this->Modified();
}

void vtkScatterPlotMatrix::SetHistogramColor(double r, double g, double b)
{
  this->Private->HistogramColor[0] = r;
  this->Private->HistogramColor[1] = g;
  this->Private->HistogramColor[2] = b;

  int plotCount = this->GetSize().X();

  for(int i = 0; i < plotCount; i++)
    {
    vtkChart *chart = this->GetChart(vtkVector2i(i, plotCount - i - 1));
    vtkPlot *plot = chart->GetPlot(0);

    plot->GetBrush()->SetColorF(this->Private->HistogramColor);
    plot->Update();
    }

  this->Modified();
}

void vtkScatterPlotMatrix::SetMarkerStyle(int style)
{
  if(style != this->Private->ScatterPlotMarkerStyle)
    {
    this->Private->ScatterPlotMarkerStyle = style;

    int plotCount = this->GetSize().X();

    for(int i = 0; i < plotCount - 1; i++)
      {
      for(int j = 0; j < plotCount - 1; j++)
        {
        if(this->GetPlotType(i, j) == SCATTERPLOT)
          {
          vtkChart *chart = this->GetChart(vtkVector2i(i, j));
          vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast(chart->GetPlot(0));
          plot->SetMarkerStyle(style);
          plot->Update();
          }
        }
      }

    this->Modified();
    }
}

void vtkScatterPlotMatrix::SetActivePlotMarkerStyle(int style)
{
  if(style != this->Private->ActivePlotMarkerStyle)
    {
    this->Private->ActivePlotMarkerStyle = style;

    // update marker style on current active plot
    vtkChart *chart = this->Private->BigChart;
    if(chart)
      {
      vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast(chart->GetPlot(0));
      plot->SetMarkerStyle(style);
      plot->Update();
      }

    this->Modified();
    }
}

void vtkScatterPlotMatrix::SetMarkerSize(double size)
{
  if(size != this->Private->ScatterPlotMarkerSize)
    {
    this->Private->ScatterPlotMarkerSize = size;

    int plotCount = this->GetSize().X();

    for(int i = 0; i < plotCount - 1; i++)
      {
      for(int j = 0; j < plotCount - 1; j++)
        {
        if(this->GetPlotType(i, j) == SCATTERPLOT)
          {
          vtkChart *chart = this->GetChart(vtkVector2i(i, j));
          vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast(chart->GetPlot(0));
          plot->SetMarkerSize(size);
          plot->Update();
          }
        }
      }

    this->Modified();
    }
}

void vtkScatterPlotMatrix::SetActivePlotMarkerSize(double size)
{
  if(size != this->Private->ActivePlotMarkerSize)
    {
    this->Private->ActivePlotMarkerSize = size;

    // update marker size on current active plot
    vtkChart *chart = this->Private->BigChart;
    if(chart)
      {
      vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast(chart->GetPlot(0));
      plot->SetMarkerSize(size);
      plot->Update();
      }

    this->Modified();
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

bool vtkScatterPlotMatrix::MouseButtonPressEvent(
    const vtkContextMouseEvent &)
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
        this->SetActivePlot(vtkVector2i(i, j));
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
        this->GetChart(pos)->SetAnnotationLink(this->Private->Link.GetPointer());
        // Lower-left triangle - scatter plots.
        this->GetChart(pos)->SetActionToButton(vtkChart::PAN, -1);
        this->GetChart(pos)->SetActionToButton(vtkChart::ZOOM, -1);
        this->GetChart(pos)->SetActionToButton(vtkChart::SELECT, -1);
        vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::POINTS);
        plot->SetInputData(this->Input.GetPointer(),
                           this->VisibleColumns->GetValue(i),
                           this->VisibleColumns->GetValue(n - j - 1));
        plot->SetColor(this->Private->ScatterPlotColor[0],
                       this->Private->ScatterPlotColor[1],
                       this->Private->ScatterPlotColor[2]);

        // set plot marker size and style
        vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast(plot);
        plotPoints->SetMarkerSize(this->Private->ScatterPlotMarkerSize);
        plotPoints->SetMarkerStyle(this->Private->ScatterPlotMarkerStyle);
        }
      else if (this->GetPlotType(pos) == HISTOGRAM)
        {
        // We are on the diagonal - need a histogram plot.
        vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::BAR);
        plot->GetBrush()->SetColorF(this->Private->HistogramColor);
        plot->GetPen()->SetColor(255, 255, 255);
        vtkStdString name(this->VisibleColumns->GetValue(i));
        plot->SetInputData(this->Private->Histogram.GetPointer(),
                           name + "_extents", name + "_pops");
        vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::TOP);
        axis->SetTitle(name.c_str());
        if (i != n - 1)
          {
          axis->SetBehavior(vtkAxis::FIXED);
          }
        // Set the plot corner to the top-right
        vtkChartXY *xy = vtkChartXY::SafeDownCast(this->GetChart(pos));
        if (xy)
          {
          xy->SetBarWidthFraction(1.0);
          xy->SetPlotCorner(plot, 2);
          }

        // set background color to light gray
        xy->GetBackgroundBrush()->SetColorF(0.5, 0.5, 0.5, 0.4);
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

void vtkScatterPlotMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
