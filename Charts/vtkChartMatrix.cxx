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
#include "vtkObjectFactory.h"

#include <vector>

class vtkChartMatrix::PIMPL
{
public:
  PIMPL() {}
  ~PIMPL() {}

  std::vector< vtkSmartPointer<vtkChart> > Charts;
  vtkVector2i Geometry;
};

vtkStandardNewMacro(vtkChartMatrix)

vtkChartMatrix::vtkChartMatrix()
{
  this->Private = new PIMPL;
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
  if (this->GetScene()->GetSceneWidth() != this->Private->Geometry.X() ||
      this->GetScene()->GetSceneHeight() != this->Private->Geometry.Y())
    {
    // Update the chart element positions
    this->Private->Geometry.Set(this->GetScene()->GetSceneWidth(),
                                this->GetScene()->GetSceneHeight());
    vtkVector2f increments(this->Private->Geometry.X() / this->Size.X(),
                           this->Private->Geometry.Y() / this->Size.Y());
    for (int i = 0; i < this->Size.X(); ++i)
      {
      for (int j = 0; j < this->Size.Y(); ++j)
        {
        size_t index = j * this->Size.X() + i;
        if (this->Private->Charts[index])
          {
          vtkChart *chart = this->Private->Charts[index];
          chart->SetSize(vtkRectf(i * increments.X(),
                                  j * increments.Y(),
                                  increments.X(),
                                  increments.Y()));
          }
        }
      }
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
    }
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
    chart->SetAutoSize(false);
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
      chart->SetAutoSize(false);
      }
    return this->Private->Charts[index];
    }
  else
    {
    return NULL;
    }
}

void vtkChartMatrix::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
