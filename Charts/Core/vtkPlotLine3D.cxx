/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotLine3D.h"

#include "vtkPen.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotLine3D);

//-----------------------------------------------------------------------------
vtkPlotLine3D::vtkPlotLine3D()
{
}

//-----------------------------------------------------------------------------
vtkPlotLine3D::~vtkPlotLine3D()
{
}

//-----------------------------------------------------------------------------
bool vtkPlotLine3D::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotLine3D.");

  if (!this->Visible || this->Points.size() == 0)
  {
    return false;
  }

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();
  if(context == NULL)
  {
    return false;
  }

  // Draw the line between the points
  context->ApplyPen(this->Pen);
  context->DrawPoly(this->Points[0].GetData(), static_cast< int >(this->Points.size()));

  return this->vtkPlotPoints3D::Paint(painter);
}

//-----------------------------------------------------------------------------
void vtkPlotLine3D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
