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
#include "vtkStringArray.h"

vtkCxxSetObjectMacro(vtkPlot, Selection, vtkIdTypeArray);
vtkCxxSetObjectMacro(vtkPlot, XAxis, vtkAxis);
vtkCxxSetObjectMacro(vtkPlot, YAxis, vtkAxis);

//-----------------------------------------------------------------------------
vtkPlot::vtkPlot()
{
  this->Pen = vtkPen::New();
  this->Pen->SetWidth(2.0);
  this->Brush = vtkBrush::New();
  this->Labels = NULL;
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
  this->SetLabels(NULL);
  this->SetXAxis(NULL);
  this->SetYAxis(NULL);
}

//-----------------------------------------------------------------------------
bool vtkPlot::PaintLegend(vtkContext2D*, const vtkRectf&, int)
{
  return false;
}

//-----------------------------------------------------------------------------
int vtkPlot::GetNearestPoint(const vtkVector2f&, const vtkVector2f&,
                              vtkVector2f*)
{
  return -1;
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
void vtkPlot::GetColor(unsigned char rgb[3])
{
  double rgbF[3];
  this->GetColor(rgbF);
  rgb[0] = static_cast<unsigned char>(255. * rgbF[0] + 0.5);
  rgb[1] = static_cast<unsigned char>(255. * rgbF[1] + 0.5);
  rgb[2] = static_cast<unsigned char>(255. * rgbF[2] + 0.5);
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
void vtkPlot::SetLabel(const vtkStdString& label)
{
  vtkStringArray *labels = vtkStringArray::New();
  labels->InsertNextValue(label);
  this->SetLabels(labels);
  labels->Delete();
}

//-----------------------------------------------------------------------------

vtkStdString vtkPlot::GetLabel()
{
  return this->GetLabel(0);
}


//-----------------------------------------------------------------------------

void vtkPlot::SetLabels(vtkStringArray *labels)
{
  if (this->Labels == labels)
    {
    return;
    }

  this->Labels = labels;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkStringArray * vtkPlot::GetLabels()
{
  // If the label string is empty, return the y column name
  if (this->Labels)
    {
    return this->Labels;
    }
  else if (this->AutoLabels)
    {
    return this->AutoLabels;
    }
  else if (this->Data->GetInput() &&
           this->Data->GetInputArrayToProcess(1, this->Data->GetInput()))
    {
    this->AutoLabels = vtkSmartPointer<vtkStringArray>::New();
    this->AutoLabels->InsertNextValue(this->Data->GetInputArrayToProcess(1, this->Data->GetInput())->GetName());
    return this->AutoLabels;
    }
  else
    {
    return NULL;
    }
}
//-----------------------------------------------------------------------------
int vtkPlot::GetNumberOfLabels()
{
  vtkStringArray *labels = this->GetLabels();
  if (labels)
    {
    return labels->GetNumberOfValues();
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlot::GetLabel(vtkIdType index)
{
  vtkStringArray *labels = this->GetLabels();
  if (labels && index >= 0 && index < labels->GetNumberOfValues())
    {
    return labels->GetValue(index);
    }
  else
    {
    return vtkStdString();
    }
}
//-----------------------------------------------------------------------------
void vtkPlot::SetInputData(vtkTable *table)
{
  this->Data->SetInputData(table);
  this->AutoLabels = 0;  // No longer valid
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInputData(vtkTable *table, const vtkStdString &xColumn,
                           const vtkStdString &yColumn)
{
  vtkDebugMacro(<< "Setting input, X column = \"" << xColumn.c_str()
                << "\", " << "Y column = \"" << yColumn.c_str() << "\"");

  this->Data->SetInputData(table);
  this->Data->SetInputArrayToProcess(0, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     xColumn.c_str());
  this->Data->SetInputArrayToProcess(1, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     yColumn.c_str());
  this->AutoLabels = 0;  // No longer valid
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInputData(vtkTable *table, vtkIdType xColumn,
                           vtkIdType yColumn)
{
  this->SetInputData(table,
                     table->GetColumnName(xColumn),
                     table->GetColumnName(yColumn));
}

//-----------------------------------------------------------------------------
vtkTable* vtkPlot::GetInput()
{
  return this->Data->GetInput();
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInputArray(int index, const vtkStdString &name)
{
  this->Data->SetInputArrayToProcess(index, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     name.c_str());
  this->AutoLabels = 0; // No longer valid
}

//-----------------------------------------------------------------------------
void vtkPlot::SetProperty(const vtkStdString&, const vtkVariant&)
{
}

//-----------------------------------------------------------------------------
vtkVariant vtkPlot::GetProperty(const vtkStdString&)
{
  return vtkVariant();
}

//-----------------------------------------------------------------------------
void vtkPlot::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
