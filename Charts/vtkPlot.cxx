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
#include "vtkStringArray.h"
#include "vtkNew.h"
#include "vtksys/ios/sstream"

vtkCxxSetObjectMacro(vtkPlot, Selection, vtkIdTypeArray);
vtkCxxSetObjectMacro(vtkPlot, XAxis, vtkAxis);
vtkCxxSetObjectMacro(vtkPlot, YAxis, vtkAxis);

//-----------------------------------------------------------------------------
vtkPlot::vtkPlot()
{
  this->Pen = vtkSmartPointer<vtkPen>::New();
  this->Pen->SetWidth(2.0);
  this->Brush = vtkSmartPointer<vtkBrush>::New();
  this->Labels = NULL;
  this->UseIndexForXSeries = false;
  this->Data = vtkSmartPointer<vtkContextMapper2D>::New();
  this->Selection = NULL;
  this->XAxis = NULL;
  this->YAxis = NULL;

  this->TooltipDefaultLabelFormat = "%l: %x,  %y";
  this->TooltipNotation = vtkAxis::STANDARD_NOTATION;
  this->TooltipPrecision = 6;
}

//-----------------------------------------------------------------------------
vtkPlot::~vtkPlot()
{
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
vtkIdType vtkPlot::GetNearestPoint(const vtkVector2f&, const vtkVector2f&,
                                   vtkVector2f*)
{
  return -1;
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlot::GetTooltipLabel(const vtkVector2f &plotPos,
                                      vtkIdType seriesIndex,
                                      vtkIdType)
{
  vtkStdString tooltipLabel;
  vtkStdString &format = this->TooltipLabelFormat.empty() ?
        this->TooltipDefaultLabelFormat : this->TooltipLabelFormat;
  // Parse TooltipLabelFormat and build tooltipLabel
  bool escapeNext = false;
  for (size_t i = 0; i < format.length(); ++i)
    {
    if (escapeNext)
      {
      switch (format[i])
        {
        case 'x':
          tooltipLabel += this->GetNumber(plotPos.X(), this->XAxis);
          break;
        case 'y':
          tooltipLabel += this->GetNumber(plotPos.Y(), this->YAxis);
          break;
        case 'i':
          if (this->IndexedLabels &&
              seriesIndex >= 0 &&
              seriesIndex < this->IndexedLabels->GetNumberOfTuples())
            {
            tooltipLabel += this->IndexedLabels->GetValue(seriesIndex);
            }
          break;
        case 'l':
          // GetLabel() is GetLabel(0) in this implementation
          tooltipLabel += this->GetLabel();
          break;
        default: // If no match, insert the entire format tag
          tooltipLabel += "%";
          tooltipLabel += format[i];
          break;
        }
      escapeNext = false;
      }
    else
      {
      if (format[i] == '%')
        {
        escapeNext = true;
        }
      else
        {
        tooltipLabel += format[i];
        }
      }
    }
  return tooltipLabel;
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlot::GetNumber(double position, vtkAxis *axis)
{
  // Determine and format the X and Y position in the chart
  vtksys_ios::ostringstream ostr;
  ostr.imbue(std::locale::classic());
  ostr.precision(this->GetTooltipPrecision());

  if(this->GetTooltipNotation() == vtkAxis::SCIENTIFIC_NOTATION)
    {
    ostr.setf(ios::scientific, ios::floatfield);
    }
  else if(this->GetTooltipNotation() == vtkAxis::FIXED_NOTATION)
    {
    ostr.setf(ios::fixed, ios::floatfield);
    }

  if (axis && axis->GetLogScale())
    {
    // If axes are set to logarithmic scale we need to convert the
    // axis value using 10^(axis value)
    ostr << pow(double(10.0), double(position));
    }
  else
    {
    ostr << position;
    }
  return ostr.str();
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
void vtkPlot::SetPen(vtkPen *pen)
{
  if (this->Pen != pen)
    {
    this->Pen = pen;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
vtkPen* vtkPlot::GetPen()
{
  return this->Pen.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkPlot::SetBrush(vtkBrush *brush)
{
  if (this->Brush != brush)
    {
    this->Brush = brush;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
vtkBrush* vtkPlot::GetBrush()
{
  return this->Brush.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkPlot::SetLabel(const vtkStdString& label)
{
  vtkNew<vtkStringArray> labels;
  labels->InsertNextValue(label);
  this->SetLabels(labels.GetPointer());
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
void vtkPlot::SetIndexedLabels(vtkStringArray *labels)
{
  if (this->IndexedLabels == labels)
    {
    return;
    }

  if (labels)
    {
    this->TooltipDefaultLabelFormat = "%i: %x,  %y";
    }
  else
    {
    this->TooltipDefaultLabelFormat = "%l: %x,  %y";
    }

  this->IndexedLabels = labels;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkStringArray * vtkPlot::GetIndexedLabels()
{
  return this->IndexedLabels.GetPointer();
}

//-----------------------------------------------------------------------------
vtkContextMapper2D * vtkPlot::GetData()
{
  return this->Data.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkPlot::SetTooltipLabelFormat(const vtkStdString &labelFormat)
{
  if (this->TooltipLabelFormat == labelFormat)
    {
    return;
    }

  this->TooltipLabelFormat = labelFormat;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlot::GetTooltipLabelFormat()
{
  return this->TooltipLabelFormat;
}

//-----------------------------------------------------------------------------
void vtkPlot::SetTooltipNotation(int notation)
{
  this->TooltipNotation = notation;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkPlot::GetTooltipNotation()
{
  return this->TooltipNotation;
}

//-----------------------------------------------------------------------------
void vtkPlot::SetTooltipPrecision(int precision)
{
  this->TooltipPrecision = precision;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkPlot::GetTooltipPrecision()
{
  return this->TooltipPrecision;
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
void vtkPlot::SetInput(vtkTable *table)
{
  this->Data->SetInput(table);
  this->AutoLabels = 0;  // No longer valid
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInput(vtkTable *table, const vtkStdString &xColumn,
                       const vtkStdString &yColumn)
{
  vtkDebugMacro(<< "Setting input, X column = \"" << xColumn.c_str()
                << "\", " << "Y column = \"" << yColumn.c_str() << "\"");

  this->Data->SetInput(table);
  this->Data->SetInputArrayToProcess(0, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     xColumn.c_str());
  this->Data->SetInputArrayToProcess(1, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     yColumn.c_str());
  this->AutoLabels = 0;  // No longer valid
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
