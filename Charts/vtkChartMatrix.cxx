/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartMatrix.h"

#include "vtkChartXY.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkAxis.h"
#include "vtkObjectFactory.h"

#include <vector>

class vtkChartMatrix::PIMPL
{
public:
  PIMPL() : Geometry(0, 0) {}
  ~PIMPL() {}

  // Container for the vtkChart objects that make up the matrix.
  std::vector< vtkSmartPointer<vtkChart> > Charts;
  // Spans of the charts in the matrix, default is 1x1.
  std::vector< vtkVector2i > Spans;
  vtkVector2i Geometry;
};

vtkStandardNewMacro(vtkChartMatrix)

vtkChartMatrix::vtkChartMatrix() : Size(0, 0), Gutter(15.0, 15.0)
{
  this->Private = new PIMPL;
  this->Borders[vtkAxis::LEFT] = 50;
  this->Borders[vtkAxis::BOTTOM] = 40;
  this->Borders[vtkAxis::RIGHT] = 50;
  this->Borders[vtkAxis::TOP] = 40;
  this->LayoutIsDirty = true;
}

vtkChartMatrix::~vtkChartMatrix()
{
  delete this->Private;
}

void vtkChartMatrix::Update()
{

}

bool vtkChartMatrix::Paint(vtkContext2D *painter)
{
  if (this->LayoutIsDirty ||
      this->GetScene()->GetSceneWidth() != this->Private->Geometry.X() ||
      this->GetScene()->GetSceneHeight() != this->Private->Geometry.Y())
    {
    // Update the chart element positions
    this->Private->Geometry.Set(this->GetScene()->GetSceneWidth(),
                                this->GetScene()->GetSceneHeight());
    if (this->Size.X() > 0 && this->Size.Y() > 0)
      {
      // Calculate the increments without the gutters/borders that must be left
      vtkVector2f increments;
      increments.SetX((this->Private->Geometry.X() - (this->Size.X() - 1) *
                       this->Gutter.X() - this->Borders[vtkAxis::LEFT] -
                       this->Borders[vtkAxis::RIGHT]) /
                      this->Size.X());
      increments.SetY((this->Private->Geometry.Y() - (this->Size.Y() - 1) *
                       this->Gutter.Y() - this->Borders[vtkAxis::TOP] -
                       this->Borders[vtkAxis::BOTTOM]) /
                      this->Size.Y());

      float x = this->Borders[vtkAxis::LEFT];
      float y = this->Borders[vtkAxis::BOTTOM];
      for (int i = 0; i < this->Size.X(); ++i)
        {
        if (i > 0)
          {
          x += increments.X() + this->Gutter.X();
          }
        for (int j = 0; j < this->Size.Y(); ++j)
          {
          if (j > 0)
            {
            y += increments.Y() + this->Gutter.Y();
            }
          else
            {
            y = this->Borders[vtkAxis::BOTTOM];
            }
          size_t index = j * this->Size.X() + i;
          if (this->Private->Charts[index])
            {
            vtkChart *chart = this->Private->Charts[index];
            vtkVector2i &span = this->Private->Spans[index];
            chart->SetSize(vtkRectf(x, y,
                                    increments.X() * span.X() +
                                    (span.X() - 1) * this->Gutter.X(),
                                    increments.Y() * span.Y() +
                                    (span.Y() - 1) * this->Gutter.Y()));
            }
          }
        }
      }
    this->LayoutIsDirty = false;
    }
  return Superclass::Paint(painter);
}

void vtkChartMatrix::SetSize(const vtkVector2i &size)
{
  if (this->Size.X() != size.X() || this->Size.Y() != size.Y())
    {
    this->Size = size;
    if (size.X() * size.Y() < static_cast<int>(this->Private->Charts.size()))
      {
      for (int i = static_cast<int>(this->Private->Charts.size() - 1);
           i >= size.X() * size.Y(); --i)
        {
        this->RemoveItem(this->Private->Charts[i]);
        }
      }
    this->Private->Charts.resize(size.X() * size.Y());
    this->Private->Spans.resize(size.X() * size.Y(), vtkVector2i(1, 1));
    this->LayoutIsDirty = true;
    }
}

void vtkChartMatrix::SetBorders(int left, int bottom, int right, int top)
{
  this->Borders[vtkAxis::LEFT] = left;
  this->Borders[vtkAxis::BOTTOM] = bottom;
  this->Borders[vtkAxis::RIGHT] = right;
  this->Borders[vtkAxis::TOP] = top;
  this->LayoutIsDirty = true;
}

void vtkChartMatrix::SetGutter(const vtkVector2f &gutter)
{
  this->Gutter = gutter;
  this->LayoutIsDirty = true;
}

void vtkChartMatrix::Allocate()
{
  // Force allocation of all objects as vtkChartXY.
}

bool vtkChartMatrix::SetChart(const vtkVector2i &position, vtkChart *chart)
{
  if (position.X() < this->Size.X() && position.Y() < this->Size.Y())
    {
    size_t index = position.Y() * this->Size.X() + position.X();
    if (this->Private->Charts[index])
      {
      this->RemoveItem(this->Private->Charts[index]);
      }
    this->Private->Charts[index] = chart;
    this->AddItem(chart);
    chart->SetLayoutStrategy(vtkChart::AXES_TO_RECT);
    return true;
    }
  else
    {
    return false;
    }
}

vtkChart* vtkChartMatrix::GetChart(const vtkVector2i &position)
{
  if (position.X() < this->Size.X() && position.Y() < this->Size.Y())
    {
    size_t index = position.Y() * this->Size.X() + position.X();
    if (this->Private->Charts[index] == NULL)
      {
      vtkNew<vtkChartXY> chart;
      this->Private->Charts[index] = chart.GetPointer();
      this->AddItem(chart.GetPointer());
      chart->SetLayoutStrategy(vtkChart::AXES_TO_RECT);
      }
    return this->Private->Charts[index];
    }
  else
    {
    return NULL;
    }
}

bool vtkChartMatrix::SetChartSpan(const vtkVector2i& position,
                                  const vtkVector2i& span)
{
  if (this->Size.X() - position.X() - span.X() < 0 ||
      this->Size.Y() - position.Y() - span.Y() < 0)
    {
    return false;
    }
  else
    {
    this->Private->Spans[position.Y() * this->Size.X() + position.X()] = span;
    this->LayoutIsDirty = true;
    return true;
    }
}

vtkVector2i vtkChartMatrix::GetChartSpan(const vtkVector2i& position)
{
  size_t index = position.Y() * this->Size.X() + position.X();
  if (position.X() < this->Size.X() && position.Y() < this->Size.Y())
    {
    return this->Private->Spans[index];
    }
  else
    {
    return vtkVector2i(0, 0);
    }
}

void vtkChartMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
