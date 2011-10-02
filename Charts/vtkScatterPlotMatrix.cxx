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
#include "vtkNew.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"

class vtkScatterPlotMatrix::PIMPL
{
public:
  PIMPL() {}
  ~PIMPL() {}

  vtkNew<vtkTable> Histogram;
};

namespace
{

int NumberOfBins = 10;

// This is just here for now - quick and dirty historgram calculations...
bool PopulateHistograms(vtkTable *input, vtkTable *output)
{
  // The output table will have the twice the number of columns, they will be
  // the x and y for input column. This is the bin centers, and the population.
  for (vtkIdType i = output->GetNumberOfColumns() - 1; i >= 0; --i)
    {
    output->RemoveColumn(i);
    }
  for (vtkIdType i = 0; i < input->GetNumberOfColumns(); ++i)
    {
    double minmax[2] = { 0.0, 0.0 };
    vtkDataArray *in = vtkDataArray::SafeDownCast(input->GetColumn(i));
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
      vtkStdString name(input->GetColumnName(i));
      vtkNew<vtkFloatArray> extents;
      extents->SetName(vtkStdString(name + "_extents").c_str());
      extents->SetNumberOfTuples(NumberOfBins);
      float *centers = static_cast<float *>(extents->GetVoidPointer(0));
      for (int j = 0; j < NumberOfBins; ++j)
        {
        extents->SetValue(j, j * inc);
        }
      vtkNew<vtkIntArray> populations;
      populations->SetName(vtkStdString(name + "_pops").c_str());
      populations->SetNumberOfTuples(NumberOfBins);
      int *pops = static_cast<int *>(populations->GetVoidPointer(0));
      for (int k = 0; k < NumberOfBins; ++k)
        {
        pops[k] = 0;
        }
      cout << "Attempting to bin " << name << "(" << minmax[0] <<
              "->" << minmax[1] << ")" << endl;
      cout << "inc: " << inc << " / 2 -> " << halfInc << endl;
      for (vtkIdType j = 0; j < in->GetNumberOfTuples(); ++j)
        {
        double v(0.0);
        in->GetTuple(j, &v);
        for (int k = 0; k < NumberOfBins; ++k)
          {
          if (vtkFuzzyCompare(v, double(centers[k]), halfInc))
            {
            cout << "FuzzyCompare true: " << v << ", " << centers[k] << ": "
                    << halfInc << endl;
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
}

bool vtkScatterPlotMatrix::Paint(vtkContext2D *painter)
{
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
      return;
      }

    // Build up our histograms
    PopulateHistograms(table, this->Private->Histogram.GetPointer());

    int n = static_cast<int>(this->Input->GetNumberOfColumns());
    this->SetSize(vtkVector2i(n, n));
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
    for (int i = 0; i < n; ++i)
      {
      for (int j = 0; j < n; ++j)
        {
        vtkVector2i pos(i, j);
        if (i + j + 1 < n)
          {
          vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::POINTS);
          plot->SetInput(table, i, n - j - 1);
          }
        else if (i == n - j - 1)
          {
          // We are on the diagonal - need a histogram plot.
          vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::BAR);
          vtkStdString name(table->GetColumnName(i));
          plot->SetInput(this->Private->Histogram.GetPointer(),
                         name + "_extents", name + "_pops");
          vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::TOP);
          axis->SetTitle(name.c_str());
          // Set the plot corner to the top-right
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
          }
        else
          {
          vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::BOTTOM);
          axis->SetTitle(table->GetColumnName(i));
          }
        // Only show the left axis labels for left-most plots
        if (i > 0)
          {
          vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::LEFT);
          axis->SetTitle("");
          axis->SetLabelsVisible(false);
          }
        else
          {
          vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::LEFT);
          axis->SetTitle(table->GetColumnName(n - j - 1));
          }
        }
      }
    }
}

void vtkScatterPlotMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
