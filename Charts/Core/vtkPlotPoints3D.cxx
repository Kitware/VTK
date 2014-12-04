/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotPoints3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartXYZ.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlotPoints3D.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotPoints3D)

//-----------------------------------------------------------------------------
vtkPlotPoints3D::vtkPlotPoints3D()
{
  this->Pen->SetWidth(5);
  this->Pen->SetColor(0, 0, 0, 255);
  this->SelectionPen->SetWidth(7);
}

//-----------------------------------------------------------------------------
vtkPlotPoints3D::~vtkPlotPoints3D()
{
}

//-----------------------------------------------------------------------------
void vtkPlotPoints3D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkPlotPoints3D::Paint(vtkContext2D *painter)
{
  if (!this->Visible || this->Points.size() == 0)
    {
    return false;
    }

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();

  if (!context)
    {
    return false;
    }

  this->Update();

  if (this->Points.size() > 0)
    {

    // Draw the points in 3d.
    context->ApplyPen(this->Pen.GetPointer());
    if (this->NumberOfComponents == 0)
      {
      context->DrawPoints(
        this->Points[0].GetData(), static_cast<int>(this->Points.size()));
      }
    else
      {
      context->DrawPoints(
        this->Points[0].GetData(),
        static_cast<int>(this->Points.size()),
        this->Colors->GetPointer(0), this->NumberOfComponents);
      }

    }

  // Now add some decorations for our selected points...
  if (this->Selection && this->Selection->GetNumberOfTuples())
    {
    if (this->Selection->GetMTime() > this->SelectedPointsBuildTime ||
        this->GetMTime() > this->SelectedPointsBuildTime)
      {
      size_t nSelected(static_cast<size_t>(this->Selection->GetNumberOfTuples()));
      this->SelectedPoints.reserve(nSelected);
      for (size_t i = 0; i < nSelected; ++i)
        {
        this->SelectedPoints.push_back(this->Points[this->Selection->GetValue(i)]);
        }
      this->SelectedPointsBuildTime.Modified();
      }

    // Now to render the selected points.
    if (!this->SelectedPoints.empty())
      {
      context->ApplyPen(this->SelectionPen.GetPointer());
      context->DrawPoints(this->SelectedPoints[0].GetData(),
                          static_cast<int>(this->SelectedPoints.size()));
      }
    }

  return true;
}
