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
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkNew.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"

class vtkScatterPlotMatrix::PIMPL
{
public:
  PIMPL() : VisibleColumnsModified(true)
  {
  }

  ~PIMPL()
  {
  }

  vtkNew<vtkTable> Histogram;
  bool VisibleColumnsModified;
};

namespace
{

int NumberOfBins = 10;

// This is just here for now - quick and dirty historgram calculations...
bool PopulateHistograms(vtkTable *input, vtkTable *output, vtkStringArray *s)
{
  // The output table will have the twice the number of columns, they will be
  // the x and y for input column. This is the bin centers, and the population.
  for (vtkIdType i = output->GetNumberOfColumns() - 1; i >= 0; --i)
    {
    output->RemoveColumn(i);
    }
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
      double inc = (minmax[1] - minmax[0]) / NumberOfBins;
      double halfInc = inc / 2.0;
      vtkNew<vtkFloatArray> extents;
      extents->SetName(vtkStdString(name + "_extents").c_str());
      extents->SetNumberOfTuples(NumberOfBins);
      float *centers = static_cast<float *>(extents->GetVoidPointer(0));
      for (int j = 0; j < NumberOfBins; ++j)
        {
        extents->SetValue(j, minmax[0] + j * inc);
        }
      vtkNew<vtkIntArray> populations;
      populations->SetName(vtkStdString(name + "_pops").c_str());
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
          if (vtkFuzzyCompare(v, double(centers[k]), halfInc))
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

vtkScatterPlotMatrix::vtkScatterPlotMatrix()
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
                       this->VisibleColumns.GetPointer());
    this->UpdateLayout();
    this->Private->VisibleColumnsModified = false;
    }
}

bool vtkScatterPlotMatrix::Paint(vtkContext2D *painter)
{
  this->Update();
  return Superclass::Paint(painter);
}

void vtkScatterPlotMatrix::SetInput(vtkTable *table)
{
  if (this->Input != table)
    {
    // Set the input, then update the size of the scatter plot matrix, set
    // their inputs and all the other stuff needed.
    this->Input = table;
    this->Modified();

    if (table == NULL)
      {
      this->SetSize(vtkVector2i(0, 0));
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
        this->SetSize(vtkVector2i(this->VisibleColumns->GetNumberOfTuples(),
                                  this->VisibleColumns->GetNumberOfTuples()));
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
}

vtkStringArray* vtkScatterPlotMatrix::GetVisibleColumns()
{
  return this->VisibleColumns.GetPointer();
}

void vtkScatterPlotMatrix::UpdateLayout()
{
  // We want scatter plots on the lower-left triangle, then histograms along
  // the diagonal and a big plot in the top-right. The basic layout is,
  //
  // 0 H   +++
  // 1 S H +++
  // 2 S S H
  // 3 S S S H
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
      if (i + j + 1 < n)
        {
        // Lower-left triangle - scatter plots.
        vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::POINTS);
        plot->SetInput(this->Input.GetPointer(), i, n - j - 1);
        }
      else if (i == n - j - 1)
        {
        // We are on the diagonal - need a histogram plot.
        vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::BAR);
        vtkStdString name(this->VisibleColumns->GetValue(i));
        plot->SetInput(this->Private->Histogram.GetPointer(),
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
          xy->SetPlotCorner(plot, 2);
          }
        }
      else if (i == static_cast<int>(n / 2.0) + n % 2 && i == j)
        {
        // This big plot in the top-right
        vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::POINTS);
        plot->SetInput(this->Input.GetPointer(), i, n - j - 1);
        this->SetChartSpan(pos, vtkVector2i(n - i, n - i));
        vtkChartXY *xy = vtkChartXY::SafeDownCast(this->GetChart(pos));
        if (xy)
          {
          xy->SetPlotCorner(plot, 2);
          }
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

void vtkScatterPlotMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
