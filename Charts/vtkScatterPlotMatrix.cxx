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
#include "vtkChart.h"
#include "vtkPlot.h"
#include "vtkAxis.h"
#include "vtkObjectFactory.h"

class vtkScatterPlotMatrix::PIMPL
{
public:
  PIMPL() {}
  ~PIMPL() {}
};

vtkStandardNewMacro(vtkScatterPlotMatrix)

vtkScatterPlotMatrix::vtkScatterPlotMatrix()
{
}

vtkScatterPlotMatrix::~vtkScatterPlotMatrix()
{
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
          vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::LINE);
          plot->SetInput(table, i, n - j - 1);
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
