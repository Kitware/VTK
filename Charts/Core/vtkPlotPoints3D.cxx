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

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotPoints3D)

//-----------------------------------------------------------------------------
vtkPlotPoints3D::vtkPlotPoints3D()
{
  this->Pen->SetWidth(5);
  this->Pen->SetColor(0, 0, 0, 255);
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
    return false;

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();

  if (!context)
    return false;

  this->Update();

  // Update the points that fall inside our axes
  if (this->Chart->ShouldCheckClipping())
    {
    this->UpdateClippedPoints();
    }

  if (this->PointsThatSurviveClipping.size() > 0)
    {

    // Draw the points in 3d.
    context->ApplyPen(this->Pen.GetPointer());
    if (this->NumberOfComponents == 0)
      {
      context->DrawPoints(
        this->PointsThatSurviveClipping[0].GetData(),
        static_cast<int>(this->PointsThatSurviveClipping.size()));
      }
    else
      {
      context->DrawPoints(
        this->PointsThatSurviveClipping[0].GetData(),
        static_cast<int>(this->PointsThatSurviveClipping.size()),
        this->ClippedColors->GetPointer(0), this->NumberOfComponents);
      }

    }

  return true;
}
