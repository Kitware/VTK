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
      this->GetScene()->GetSceneWidth() != this->Private->Geometry.GetX() ||
      this->GetScene()->GetSceneHeight() != this->Private->Geometry.GetY())
  {
    // Update the chart element positions
    this->Private->Geometry.Set(this->GetScene()->GetSceneWidth(),
                                this->GetScene()->GetSceneHeight());
    if (this->Size.GetX() > 0 && this->Size.GetY() > 0)
    {
      // Calculate the increments without the gutters/borders that must be left
      vtkVector2f increments;
      increments.SetX((this->Private->Geometry.GetX() - (this->Size.GetX() - 1) *
                       this->Gutter.GetX() - this->Borders[vtkAxis::LEFT] -
                       this->Borders[vtkAxis::RIGHT]) /
                      this->Size.GetX());
      increments.SetY((this->Private->Geometry.GetY() - (this->Size.GetY() - 1) *
                       this->Gutter.GetY() - this->Borders[vtkAxis::TOP] -
                       this->Borders[vtkAxis::BOTTOM]) /
                      this->Size.GetY());

      float x = this->Borders[vtkAxis::LEFT];
      float y = this->Borders[vtkAxis::BOTTOM];
      for (int i = 0; i < this->Size.GetX(); ++i)
      {
        if (i > 0)
        {
          x += increments.GetX() + this->Gutter.GetX();
        }
        for (int j = 0; j < this->Size.GetY(); ++j)
        {
          if (j > 0)
          {
            y += increments.GetY() + this->Gutter.GetY();
          }
          else
          {
            y = this->Borders[vtkAxis::BOTTOM];
          }
          vtkVector2f resize(0., 0.);
          vtkVector2i key(i, j);
          if (this->SpecificResize.find(key) != this->SpecificResize.end())
          {
            resize = this->SpecificResize[key];
          }
          size_t index = j * this->Size.GetX() + i;
          if (this->Private->Charts[index])
          {
            vtkChart *chart = this->Private->Charts[index];
            vtkVector2i &span = this->Private->Spans[index];
            chart->SetSize(vtkRectf(x + resize.GetX(), y + resize.GetY(),
                                    increments.GetX() * span.GetX() - resize.GetX() +
                                    (span.GetX() - 1) * this->Gutter.GetX(),
                                    increments.GetY() * span.GetY() - resize.GetY() +
                                    (span.GetY() - 1) * this->Gutter.GetY()));
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
  if (this->Size.GetX() != size.GetX() || this->Size.GetY() != size.GetY())
  {
    this->Size = size;
    if (size.GetX() * size.GetY() < static_cast<int>(this->Private->Charts.size()))
    {
      for (int i = static_cast<int>(this->Private->Charts.size() - 1);
           i >= size.GetX() * size.GetY(); --i)
      {
        this->RemoveItem(this->Private->Charts[i]);
      }
    }
    this->Private->Charts.resize(size.GetX() * size.GetY());
    this->Private->Spans.resize(size.GetX() * size.GetY(), vtkVector2i(1, 1));
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

void vtkChartMatrix::SetBorderLeft(int value)
{
  this->Borders[vtkAxis::LEFT] = value;
  this->LayoutIsDirty = true;
}

void vtkChartMatrix::SetBorderBottom(int value)
{
  this->Borders[vtkAxis::BOTTOM] = value;
  this->LayoutIsDirty = true;
}

void vtkChartMatrix::SetBorderRight(int value)
{
  this->Borders[vtkAxis::RIGHT] = value;
  this->LayoutIsDirty = true;
}

void vtkChartMatrix::SetBorderTop(int value)
{
  this->Borders[vtkAxis::TOP] = value;
  this->LayoutIsDirty = true;
}


void vtkChartMatrix::SetGutter(const vtkVector2f &gutter)
{
  this->Gutter = gutter;
  this->LayoutIsDirty = true;
}

void vtkChartMatrix::SetGutterX(float value)
{
  this->Gutter.SetX (value);
  this->LayoutIsDirty = true;
}

void vtkChartMatrix::SetGutterY(float value)
{
  this->Gutter.SetY (value);
  this->LayoutIsDirty = true;
}

void vtkChartMatrix::SetSpecificResize(const vtkVector2i& index, const vtkVector2f& resize)
{
  if (this->SpecificResize.find(index) == this->SpecificResize.end() ||
    this->SpecificResize[index] != resize)
    {
    this->SpecificResize[index] = resize;
    this->LayoutIsDirty = true;
    }
}

void vtkChartMatrix::ClearSpecificResizes()
{
  if (this->SpecificResize.size() != 0)
    {
    this->SpecificResize.clear();
    this->LayoutIsDirty = true;
    }
}

void vtkChartMatrix::Allocate()
{
  // Force allocation of all objects as vtkChartXY.
}

bool vtkChartMatrix::SetChart(const vtkVector2i &position, vtkChart *chart)
{
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    size_t index = position.GetY() * this->Size.GetX() + position.GetX();
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
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    size_t index = position.GetY() * this->Size.GetX() + position.GetX();
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
  if (this->Size.GetX() - position.GetX() - span.GetX() < 0 ||
      this->Size.GetY() - position.GetY() - span.GetY() < 0)
  {
    return false;
  }
  else
  {
    this->Private->Spans[position.GetY() * this->Size.GetX() + position.GetX()] = span;
    this->LayoutIsDirty = true;
    return true;
  }
}

vtkVector2i vtkChartMatrix::GetChartSpan(const vtkVector2i& position)
{
  size_t index = position.GetY() * this->Size.GetX() + position.GetX();
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    return this->Private->Spans[index];
  }
  else
  {
    return vtkVector2i(0, 0);
  }
}

vtkVector2i vtkChartMatrix::GetChartIndex(const vtkVector2f &position)
{
  if (this->Size.GetX() > 0 && this->Size.GetY() > 0)
  {
    // Calculate the increments without the gutters/borders that must be left.
    vtkVector2f increments;
    increments.SetX((this->Private->Geometry.GetX() - (this->Size.GetX() - 1) *
                     this->Gutter.GetX() - this->Borders[vtkAxis::LEFT] -
                     this->Borders[vtkAxis::RIGHT]) /
                     this->Size.GetX());
    increments.SetY((this->Private->Geometry.GetY() - (this->Size.GetY() - 1) *
                     this->Gutter.GetY() - this->Borders[vtkAxis::TOP] -
                     this->Borders[vtkAxis::BOTTOM]) /
                     this->Size.GetY());

    float x = this->Borders[vtkAxis::LEFT];
    float y = this->Borders[vtkAxis::BOTTOM];
    for (int i = 0; i < this->Size.GetX(); ++i)
    {
      if (i > 0)
      {
        x += increments.GetX() + this->Gutter.GetX();
      }
      for (int j = 0; j < this->Size.GetY(); ++j)
      {
        if (j > 0)
        {
          y += increments.GetY() + this->Gutter.GetY();
        }
        else
        {
          y = this->Borders[vtkAxis::BOTTOM];
        }
        vtkVector2f resize(0., 0.);
        vtkVector2i key(i, j);
        if (this->SpecificResize.find(key) != this->SpecificResize.end())
        {
          resize = this->SpecificResize[key];
        }
        size_t index = j * this->Size.GetX() + i;
        if (this->Private->Charts[index])
        {
          vtkVector2i &span = this->Private->Spans[index];
          // Check if the supplied location is within this charts area.
          float x2 = x + resize.GetX();
          float y2 = y + resize.GetY();
          if (position.GetX() > x2 &&
              position.GetX() < (x2 + increments.GetX() * span.GetX() - resize.GetY()
                              + (span.GetX() - 1) * this->Gutter.GetX()) &&
              position.GetY() > y2 &&
              position.GetY() < (y2 + increments.GetY() * span.GetY() - resize.GetY()
                              + (span.GetY() - 1) * this->Gutter.GetY()))
            return vtkVector2i(i, j);
        }
      }
    }
  }
  return vtkVector2i(-1, -1);
}

void vtkChartMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
