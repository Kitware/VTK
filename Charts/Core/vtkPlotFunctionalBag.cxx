/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotFunctionalBag.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotFunctionalBag.h"

#include "vtkAbstractArray.h"
#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkContextMapper2D.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlotLine.h"
#include "vtkPoints2D.h"
#include "vtkRect.h"
#include "vtkScalarsToColors.h"
#include "vtkTable.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotFunctionalBag);

//-----------------------------------------------------------------------------
vtkPlotFunctionalBag::vtkPlotFunctionalBag()
{
  this->LookupTable = 0;
  this->TooltipDefaultLabelFormat = "%l (%x, %y)";
  this->LogX = false;
  this->LogY = false;
}

//-----------------------------------------------------------------------------
vtkPlotFunctionalBag::~vtkPlotFunctionalBag()
{
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
}

//-----------------------------------------------------------------------------
bool vtkPlotFunctionalBag::IsBag()
{
  this->Update();
  return (this->BagPoints->GetNumberOfPoints() > 0);
}

//-----------------------------------------------------------------------------
bool vtkPlotFunctionalBag::GetVisible()
{
  return this->Superclass::GetVisible() || this->GetSelection() != 0;
}

//-----------------------------------------------------------------------------
void vtkPlotFunctionalBag::Update()
{
  if (!this->GetVisible())
    {
    return;
    }
  // Check if we have an input
  vtkTable *table = this->Data->GetInput();

  if (!table)
    {
    vtkDebugMacro(<< "Update event called with no input table set.");
    return;
    }
  else if(this->Data->GetMTime() > this->BuildTime ||
          table->GetMTime() > this->BuildTime ||
          (this->LookupTable && this->LookupTable->GetMTime() > this->BuildTime) ||
          this->MTime > this->BuildTime)
    {
    vtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(table);
    }
  else if ((this->XAxis->GetMTime() > this->BuildTime) ||
           (this->YAxis->GetMTime() > this->BuildTime))
    {
    if ((this->LogX != this->XAxis->GetLogScale()) ||
        (this->LogY != this->YAxis->GetLogScale()))
      {
      this->UpdateTableCache(table);
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkPlotFunctionalBag::UpdateTableCache(vtkTable *table)
{
  if (!this->LookupTable)
    {
    this->CreateDefaultLookupTable();
    this->LookupTable->SetRange(0, table->GetNumberOfColumns());
    this->LookupTable->Build();
    }

  this->BagPoints->Reset();

  vtkDataArray *array[2] = { 0, 0 };
  if (!this->GetDataArrays(table, array))
    {
    this->BuildTime.Modified();
    return false;
    }

  if (array[1]->GetNumberOfComponents() == 1)
    {
    // The input array has one component, manage it as a line
    this->Line->SetInputData(table,
      array[0] ? array[0]->GetName() : "", array[1]->GetName());
    this->Line->SetUseIndexForXSeries(this->UseIndexForXSeries);
    this->Line->SetMarkerStyle(vtkPlotPoints::NONE);
    this->Line->SetPen(this->Pen);
    this->Line->SetBrush(this->Brush);
    this->Line->Update();
    }
  else if (array[1]->GetNumberOfComponents() == 2)
    {
    // The input array has 2 components, this must be a bag
    // with {miny,maxy} tuples
    vtkDoubleArray* darr = vtkDoubleArray::SafeDownCast(array[1]);

    this->LogX = this->XAxis->GetLogScaleActive();
    this->LogY = this->YAxis->GetLogScaleActive();
    bool xAbs = this->XAxis->GetUnscaledMinimum() < 0.;
    bool yAbs = this->YAxis->GetUnscaledMinimum() < 0.;
    if (darr)
      {
      vtkIdType nbRows = array[1]->GetNumberOfTuples();
      this->BagPoints->SetNumberOfPoints(2 * nbRows);
      for (vtkIdType i = 0; i < nbRows; i++)
        {
        double y[2];
        darr->GetTuple(i, y);

        double x = (!this->UseIndexForXSeries && array[0]) ?
          array[0]->GetVariantValue(i).ToDouble() : static_cast<double>(i);
        if (this->LogX)
          {
          x = xAbs ? log10(fabs(x)) : log10(x);
          }

        if (this->LogY)
          {
          y[0] = yAbs ? log10(fabs(y[0])) : log10(y[0]);
          y[1] = yAbs ? log10(fabs(y[1])) : log10(y[1]);
          }

        this->BagPoints->SetPoint(2 * i, x, y[0]);
        this->BagPoints->SetPoint(2 * i + 1, x, y[1]);
        }
      this->BagPoints->Modified();
      }
    }

  this->BuildTime.Modified();

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotFunctionalBag::GetDataArrays(vtkTable *table, vtkDataArray *array[2])
{
  if (!table)
    {
    return false;
    }

  // Get the x and y arrays (index 0 and 1 respectively)
  array[0] = this->UseIndexForXSeries ?
        0 : this->Data->GetInputArrayToProcess(0, table);
  array[1] = this->Data->GetInputArrayToProcess(1, table);

  if (!array[0] && !this->UseIndexForXSeries)
    {
    vtkErrorMacro(<< "No X column is set (index 0).");
    return false;
    }
  else if (!array[1])
    {
    vtkErrorMacro(<< "No Y column is set (index 1).");
    return false;
    }
  else if (!this->UseIndexForXSeries &&
           array[0]->GetNumberOfTuples() != array[1]->GetNumberOfTuples())
    {
    vtkErrorMacro("The x and y columns must have the same number of elements. "
                  << array[0]->GetNumberOfTuples() << ", "
                  << array[1]->GetNumberOfTuples());
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotFunctionalBag::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotFunctionalBag.");

  if (!this->GetVisible())
    {
    return false;
    }

  vtkPen* pen = this->GetSelection() ? this->SelectionPen : this->Pen;

  if (this->IsBag())
    {
    double pwidth = pen->GetWidth();
    pen->SetWidth(0.);
    painter->ApplyPen(pen);
    unsigned char pcolor[4];
    pen->GetColor(pcolor);
    this->Brush->SetColor(pcolor);
    painter->ApplyBrush(this->Brush);
    painter->DrawQuadStrip(this->BagPoints.GetPointer());
    pen->SetWidth(pwidth);
    }
  else
    {
    this->Line->SetPen(pen);
    this->Line->Paint(painter);
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotFunctionalBag::PaintLegend(vtkContext2D *painter,
                                       const vtkRectf& rect, int index)
{
  if (this->BagPoints->GetNumberOfPoints() > 0)
    {
    vtkNew<vtkPen> blackPen;
    blackPen->SetWidth(1.0);
    blackPen->SetColor(0, 0, 0, 255);
    painter->ApplyPen(blackPen.GetPointer());
    painter->ApplyBrush(this->Brush);
    painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
    }
  else
    {
    this->Line->PaintLegend(painter, rect, index);
    }
  return true;
}

//-----------------------------------------------------------------------------
vtkIdType vtkPlotFunctionalBag::GetNearestPoint(const vtkVector2f& point,
                                                const vtkVector2f& tol,
                                                vtkVector2f* loc)
{
  if (this->BagPoints->GetNumberOfPoints() == 0)
    {
    return this->Line->GetNearestPoint(point, tol, loc);
    }
  return -1;
}

//-----------------------------------------------------------------------------
bool vtkPlotFunctionalBag::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  if (this->BagPoints->GetNumberOfPoints() == 0)
    {
    return this->Line->SelectPoints(min, max);
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkPlotFunctionalBag::SelectPointsInPolygon(const vtkContextPolygon &polygon)
{
  if (this->BagPoints->GetNumberOfPoints() == 0)
    {
    return this->Line->SelectPointsInPolygon(polygon);
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkPlotFunctionalBag::GetBounds(double bounds[4])
{
  if (this->BagPoints->GetNumberOfPoints() > 0)
    {
    this->BagPoints->GetBounds(bounds);
    if (this->LogX)
      {
      bounds[0] = log10(bounds[0]);
      bounds[1] = log10(bounds[1]);
    }
    if (this->LogY)
      {
      bounds[2] = log10(bounds[2]);
      bounds[3] = log10(bounds[3]);
      }
    }
  else
    {
    this->Line->GetBounds(bounds);
    }

  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

//-----------------------------------------------------------------------------
void vtkPlotFunctionalBag::GetUnscaledInputBounds(double bounds[4])
{
  if (this->BagPoints->GetNumberOfPoints() > 0)
    {
    this->BagPoints->GetBounds(bounds);
    }
  else
    {
    this->Line->GetUnscaledInputBounds(bounds);
    }

  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

//-----------------------------------------------------------------------------
void vtkPlotFunctionalBag::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkPlotFunctionalBag::SetLookupTable(vtkScalarsToColors *lut)
{
  if ( this->LookupTable != lut )
    {
    if ( this->LookupTable)
      {
      this->LookupTable->UnRegister(this);
      }
    this->LookupTable = lut;
    if (lut)
      {
      lut->Register(this);
      }
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
vtkScalarsToColors *vtkPlotFunctionalBag::GetLookupTable()
{
  if ( this->LookupTable == 0 )
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void vtkPlotFunctionalBag::CreateDefaultLookupTable()
{
  if ( this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  this->LookupTable = vtkLookupTable::New();
  // Consistent Register/UnRegisters.
  this->LookupTable->Register(this);
  this->LookupTable->Delete();
}
