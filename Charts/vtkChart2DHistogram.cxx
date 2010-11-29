/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChart2DHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChart2DHistogram.h"

#include "vtkContext2D.h"
#include "vtkBrush.h"
#include "vtkPen.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkTextProperty.h"
#include "vtkAxis.h"
#include "vtk2DHistogramItem.h"
#include "vtkContextTransform.h"
#include "vtkSmartPointer.h"
#include "vtkTransform2D.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#include <vector>

// Minimal storage class for STL containers etc.
class vtkChart2DHistogram::Private
{
public:
  Private()
    {
    this->Transform = vtkSmartPointer<vtkContextTransform>::New();
    }
  ~Private()
    {
    for(std::vector<vtkAxis *>::iterator it = this->Axes.begin();
        it != this->Axes.end(); ++it)
      {
      (*it)->Delete();
      }
    }
  vtkSmartPointer<vtkContextTransform> Transform;
  std::vector<vtkAxis *> Axes;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChart2DHistogram);

//-----------------------------------------------------------------------------
vtkChart2DHistogram::vtkChart2DHistogram()
{
  this->Storage = new vtkChart2DHistogram::Private;

  // Now for the 2D histogram
  this->AddItem(this->Storage->Transform);
  this->Histogram = vtkSmartPointer<vtk2DHistogramItem>::New();
  this->Storage->Transform->AddItem(this->Histogram);

  // Setup the axes
  for (int i = 0; i < 2; ++i)
    {
    this->Storage->Axes.push_back(vtkAxis::New());
    this->AddItem(this->Storage->Axes.back());
    this->Storage->Axes[i]->SetPosition(i);
    }
}

//-----------------------------------------------------------------------------
vtkChart2DHistogram::~vtkChart2DHistogram()
{
  delete this->Storage;
}

//-----------------------------------------------------------------------------
void vtkChart2DHistogram::Update()
{
  for(std::vector<vtkAxis *>::iterator it = this->Storage->Axes.begin();
      it != this->Storage->Axes.end(); ++it)
    {
    (*it)->Update();
    }
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::Paint(vtkContext2D *painter)
{
  int geometry[] = { this->GetScene()->GetSceneWidth(),
                     this->GetScene()->GetSceneHeight() };
  if (geometry[0] == 0 || geometry[1] == 0 || !this->Visible)
    {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
    }

  this->Update();

  if (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1] ||
      this->MTime > this->Storage->Axes[0]->GetMTime())
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);
    }

  this->UpdateGeometry();

  this->PaintChildren(painter);

  return true;
}

void vtkChart2DHistogram::SetInput(vtkImageData *data, vtkIdType z)
{
  this->Histogram->SetInput(data, z);
}

void vtkChart2DHistogram::SetTransferFunction(vtkColorTransferFunction *function)
{
  this->Histogram->SetTransferFunction(function);
}

//-----------------------------------------------------------------------------
vtkPlot* vtkChart2DHistogram::GetPlot(vtkIdType)
{
  return NULL;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChart2DHistogram::GetNumberOfPlots()
{
  return 1;
}

//-----------------------------------------------------------------------------
vtkAxis* vtkChart2DHistogram::GetAxis(int index)
{
  if (index < this->GetNumberOfAxes())
    {
    return this->Storage->Axes[index];
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkChart2DHistogram::GetNumberOfAxes()
{
  return this->Storage->Axes.size();
}

//-----------------------------------------------------------------------------
void vtkChart2DHistogram::UpdateGeometry()
{
  this->SetBorders(20, 20, 20, 20);

  double bounds[4];
  this->Histogram->GetBounds(bounds);

  vtkAxis *axis = this->Storage->Axes[vtkAxis::LEFT];
  axis->SetRange(bounds[2], bounds[3]);
  axis->SetPoint1(this->Point1[0], this->Point1[1]);
  axis->SetPoint2(this->Point1[0], this->Point2[1]);
  axis->AutoScale();
  axis->Update();
  axis = this->Storage->Axes[vtkAxis::BOTTOM];
  axis->SetRange(bounds[0], bounds[1]);
  axis->SetPoint1(this->Point1[0], this->Point1[1]);
  axis->SetPoint2(this->Point2[0], this->Point1[1]);
  axis->AutoScale();
  axis->Update();

  this->CalculatePlotTransform(this->Storage->Axes[vtkAxis::BOTTOM],
                               this->Storage->Axes[vtkAxis::LEFT],
                               this->Storage->Transform->GetTransform());
}

//-----------------------------------------------------------------------------
void vtkChart2DHistogram::RecalculateBounds()
{
  return;
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::Hit(const vtkContextMouseEvent &mouse)
{
  if (mouse.ScreenPos[0] > this->Point1[0]-10 &&
      mouse.ScreenPos[0] < this->Point2[0]+10 &&
      mouse.ScreenPos[1] > this->Point1[1] &&
      mouse.ScreenPos[1] < this->Point2[1])
    {
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::MouseEnterEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::MouseButtonPressEvent(
    const vtkContextMouseEvent& mouse)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::MouseButtonReleaseEvent(
    const vtkContextMouseEvent& mouse)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::MouseWheelEvent(const vtkContextMouseEvent &,
                                                  int)
{
  return true;
}

//-----------------------------------------------------------------------------
void vtkChart2DHistogram::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
