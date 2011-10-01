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
    // Set the input, then update the size of the scatter plot matrix, set their
    // inputs and all the other stuff needed.
    this->Input = table;

    this->SetSize(vtkVector2i(this->Input->GetNumberOfColumns(),
                              this->Input->GetNumberOfColumns()));
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
    for (int i = 0; i < this->Size.X(); ++i)
      {
      for (int j = 0; j < this->Size.Y(); ++j)
        {
        vtkVector2i pos(i, j);
        if (this->Size.X() - j > 0)
          {
          vtkPlot *plot = this->GetChart(pos)->AddPlot(vtkChart::POINTS);
          plot->SetInput(table, i, this->Size.X() - j - 1);
          }
        // Only show bottom axis label for bottom plots
        if (j > 0)
          {
          vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::BOTTOM);
          axis->SetTitle("");
          axis->SetLabelsVisible(false);
          }
        // Only show the left axis labels for left-most plots
        if (i > 0)
          {
          vtkAxis *axis = this->GetChart(pos)->GetAxis(vtkAxis::LEFT);
          axis->SetTitle("");
          axis->SetLabelsVisible(false);
          }
        }
      }

    this->Modified();
    }
}

void vtkScatterPlotMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
