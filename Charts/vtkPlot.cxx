/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlot.h"

#include "vtkAxis.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTable.h"
#include "vtkDataObject.h"
#include "vtkIdTypeArray.h"
#include "vtkContextMapper2D.h"
#include "vtkObjectFactory.h"

#include "vtkStdString.h"

vtkCxxSetObjectMacro(vtkPlot, Selection, vtkIdTypeArray);
vtkCxxSetObjectMacro(vtkPlot, XAxis, vtkAxis);
vtkCxxSetObjectMacro(vtkPlot, YAxis, vtkAxis);

//-----------------------------------------------------------------------------
vtkPlot::vtkPlot()
{
  this->Pen = vtkPen::New();
  this->Pen->SetWidth(2.0);
  this->Brush = vtkBrush::New();
  this->Label = NULL;
  this->UseIndexForXSeries = false;
  this->Data = vtkContextMapper2D::New();
  this->Selection = NULL;
  this->XAxis = NULL;
  this->YAxis = NULL;
}

//-----------------------------------------------------------------------------
vtkPlot::~vtkPlot()
{
  if (this->Pen)
    {
    this->Pen->Delete();
    this->Pen = NULL;
    }
  if (this->Brush)
    {
    this->Brush->Delete();
    this->Brush = NULL;
    }
  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }
  if (this->Selection)
    {
    this->Selection->Delete();
    this->Selection = NULL;
    }
  this->SetLabel(NULL);
  this->SetXAxis(NULL);
  this->SetYAxis(NULL);
}

//-----------------------------------------------------------------------------
bool vtkPlot::PaintLegend(vtkContext2D*, float*)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkPlot::GetNearestPoint(const vtkVector2f&, const vtkVector2f&,
                              vtkVector2f*)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkPlot::SelectPoints(const vtkVector2f&, const vtkVector2f&)
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkPlot::SetColor(unsigned char r, unsigned char g, unsigned char b,
                       unsigned char a)
{
  this->Pen->SetColor(r, g, b, a);
}

//-----------------------------------------------------------------------------
void vtkPlot::SetColor(double r, double g, double b)
{
  this->Pen->SetColorF(r, g, b);
}

//-----------------------------------------------------------------------------
void vtkPlot::GetColor(double rgb[3])
{
  this->Pen->GetColorF(rgb);
}

//-----------------------------------------------------------------------------
void vtkPlot::SetWidth(float width)
{
  this->Pen->SetWidth(width);
}

//-----------------------------------------------------------------------------
float vtkPlot::GetWidth()
{
  return this->Pen->GetWidth();
}

//-----------------------------------------------------------------------------
const char* vtkPlot::GetLabel()
{
  // If the label string is empty, return the y column name
  if (this->Label)
    {
    return this->Label;
    }
  else if (this->Data->GetInput() &&
           this->Data->GetInputArrayToProcess(1, this->Data->GetInput()))
    {
    return this->Data->GetInputArrayToProcess(1, this->Data->GetInput())->GetName();
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInput(vtkTable *table)
{
  this->Data->SetInput(table);
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInput(vtkTable *table, const char *xColumn,
                       const char *yColumn)
{
  if (!xColumn || !yColumn)
    {
    vtkErrorMacro(<< "Called with null arguments for X or Y column.")
    }
  vtkDebugMacro(<< "Setting input, X column = \"" << vtkstd::string(xColumn)
                << "\", " << "Y column = \"" << vtkstd::string(yColumn) << "\"");

  this->Data->SetInput(table);
  this->Data->SetInputArrayToProcess(0, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     xColumn);
  this->Data->SetInputArrayToProcess(1, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     yColumn);
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInput(vtkTable *table, vtkIdType xColumn,
                       vtkIdType yColumn)
{
  this->SetInput(table,
                 table->GetColumnName(xColumn),
                 table->GetColumnName(yColumn));
}

//-----------------------------------------------------------------------------
vtkTable* vtkPlot::GetInput()
{
  return this->Data->GetInput();
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInputArray(int index, const char *name)
{
  this->Data->SetInputArrayToProcess(index, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     name);
}

//-----------------------------------------------------------------------------
void vtkPlot::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
