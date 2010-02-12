/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotPoints.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkContextDevice2D.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkExecutive.h"
#include "vtkTimeStamp.h"
#include "vtkInformation.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPlotPoints, "1.3");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotPoints);

//-----------------------------------------------------------------------------
vtkPlotPoints::vtkPlotPoints()
{
  this->Points = 0;
  this->Label = 0;
}

//-----------------------------------------------------------------------------
vtkPlotPoints::~vtkPlotPoints()
{
  delete this->Label;
}

//-----------------------------------------------------------------------------
bool vtkPlotPoints::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotPoints.");

  // First check if we have an input
  vtkTable *table = this->Data->GetInput();
  if (!table)
    {
    vtkDebugMacro(<< "Paint event called with no input table set.");
    return false;
    }
  else if(this->GetMTime() > this->BuildTime ||
          table->GetMTime() > this->BuildTime)
    {
    vtkDebugMacro(<< "Paint event called with outdated table cache. Updating.");
    this->UpdateTableCache(table);
    }

  // Now add some decorations for our selected points...
  if (this->Selection)
    {
    vtkDebugMacro(<<"Selection set " << this->Selection->GetNumberOfTuples());
    for (int i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
      {
      painter->GetPen()->SetWidth(this->Pen->GetWidth()*15.0);
      painter->GetPen()->SetColor(0, 0, 255, 255);
      vtkIdType id = 0;
      this->Selection->GetTupleValue(i, &id);
      if (id < this->Points->GetNumberOfPoints())
        {
        double *point = this->Points->GetPoint(id);
        painter->DrawPoint(point[0], point[1]);
        }
      }
    }
  else
    {
    vtkDebugMacro("No selectionn set.");
    }

  // Now to plot the points
  if (this->Points)
    {
    painter->GetPen()->SetColor(this->Pen->GetColor());
    painter->GetPen()->SetOpacity(this->Pen->GetOpacity());
    painter->GetPen()->SetWidth(this->Pen->GetWidth());
    painter->DrawPoints(this->Points);
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::GetBounds(double bounds[4])
{
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkTable *table = this->Data->GetInput();
  vtkDataArray *x = this->Data->GetInputArrayToProcess(0, table);
  vtkDataArray *y = this->Data->GetInputArrayToProcess(1, table);

  if (x && y)
    {
    x->GetRange(&bounds[0]);
    y->GetRange(&bounds[2]);
    }
  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

//-----------------------------------------------------------------------------
template<class A, class B>
void CopyToPoints(vtkPoints2D *points, A *a, B *b, int n)
{
  points->SetNumberOfPoints(n);
  for (int i = 0; i < n; ++i)
    {
    points->SetPoint(i, a[i], b[i]);
    }
}

//-----------------------------------------------------------------------------
bool vtkPlotPoints::UpdateTableCache(vtkTable *table)
{
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkAbstractArray *x = this->Data->GetInputAbstractArrayToProcess(0, table);
  vtkAbstractArray *y = this->Data->GetInputAbstractArrayToProcess(1, table);
  if (!x)
    {
    vtkErrorMacro(<< "No X column is set (index 0).");
    return false;
    }
  else if (!y)
    {
    vtkErrorMacro(<< "No Y column is set (index 1).");
    return false;
    }
  else if (x->GetSize() != y->GetSize())
    {
    vtkErrorMacro("The x and y columns must have the same number of elements.");
    return false;
    }

  if (!this->Points)
    {
    this->Points = vtkPoints2D::New();
    }

  // Figure out the type and copy to our points
  if (x->IsA("vtkFloatArray") && y->IsA("vtkFloatArray"))
    {
    CopyToPoints(this->Points,
                 vtkFloatArray::SafeDownCast(x)->GetPointer(0),
                 vtkFloatArray::SafeDownCast(y)->GetPointer(0),
                 x->GetSize());
    this->BuildTime.Modified();
    double bounds[4];
    this->GetBounds(&bounds[0]);
    }
  else if (x->IsA("vtkDoubleArray") && y->IsA("vtkDoubleArray"))
    {
    CopyToPoints(this->Points,
                 vtkDoubleArray::SafeDownCast(x)->GetPointer(0),
                 vtkDoubleArray::SafeDownCast(y)->GetPointer(0),
                 x->GetSize());
    this->BuildTime.Modified();
    double bounds[4];
    this->GetBounds(&bounds[0]);
    }
  else
    {
    vtkErrorMacro(<< "Error the x or y array was not a valid type."
                  << endl << x->GetClassName()
                  << "\t" << y->GetClassName());
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
