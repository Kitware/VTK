/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotBag.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkContextMapper2D.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlotBag.h"
#include "vtkPoints.h"
#include "vtkPoints2D.h"
#include "vtkPointsProjectedHull.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimeStamp.h"

#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotBag);

//-----------------------------------------------------------------------------
vtkPlotBag::vtkPlotBag()
{
  this->MedianPoints = vtkPoints2D::New();
  this->Q3Points = vtkPoints2D::New();
  this->TooltipDefaultLabelFormat = "%l (%x, %y): %z";
}

//-----------------------------------------------------------------------------
vtkPlotBag::~vtkPlotBag()
{
  if (this->MedianPoints)
    {
    this->MedianPoints->Delete();
    this->MedianPoints = 0;
    }
  if (this->Q3Points)
    {
    this->Q3Points->Delete();
    this->Q3Points = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPlotBag::Update()
{
  if (!this->Visible)
    {
    return;
    }

  // Check if we have an input
  vtkTable *table = this->Data->GetInput();
  vtkDataArray *density = vtkDataArray::SafeDownCast(
    this->Data->GetInputAbstractArrayToProcess(2, this->GetInput()));
  if (!table || !density)
    {
    vtkDebugMacro(<< "Update event called with no input table or density column set.");
    return;
    }
  bool update = (this->Data->GetMTime() > this->BuildTime ||
    table->GetMTime() > this->BuildTime ||
    this->MTime > this->BuildTime);

  this->Superclass::Update();

  if (update)
    {
    vtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(density);
    }
}

//-----------------------------------------------------------------------------
class ArraySorter
{
public:
  ArraySorter(vtkDataArray* arr) : Array(arr) {}
  bool operator()(const vtkIdType& a, const vtkIdType& b)
  {
    return this->Array->GetTuple1(a) > this->Array->GetTuple1(b);
  }
  vtkDataArray* Array;
};

//-----------------------------------------------------------------------------
void vtkPlotBag::UpdateTableCache(vtkDataArray* density)
{
  this->MedianPoints->Reset();
  this->Q3Points->Reset();

  if (!this->Points)
    {
    return;
    }
  vtkIdType nbPoints = density->GetNumberOfTuples();

  // Sort the density array
  std::vector<vtkIdType> ids;
  ids.resize(nbPoints);
  double sum = 0.0;
  for (vtkIdType i = 0; i < nbPoints; i++)
    {
    sum += density->GetTuple1(i);
    ids[i] = i;
    }

  vtkNew<vtkDoubleArray> nDensity;
  // Normalize the density array if needed
  if (fabs(sum - 1.0) > 1.0e-12)
    {
    sum = 1.0 / sum;
    nDensity->SetNumberOfComponents(1);
    nDensity->SetNumberOfTuples(nbPoints);
    for (vtkIdType i = 0; i < nbPoints; i++)
      {
      nDensity->SetTuple1(i, density->GetTuple1(ids[i]) * sum);
      }
    density = nDensity.GetPointer();
    }

  // Sort array by density
  ArraySorter arraySorter(density);
  std::sort(ids.begin(), ids.end(), arraySorter);

  vtkNew<vtkPointsProjectedHull> q3Points;
  q3Points->Allocate(nbPoints);
  vtkNew<vtkPointsProjectedHull> medianPoints;
  medianPoints->Allocate(nbPoints);

  for (vtkIdType i = 0; i < nbPoints; i++)
    {
    double x[3];
    this->Points->GetPoint(ids[i], x);
    if (i < static_cast<vtkIdType>(nbPoints * 0.5))
      {
      medianPoints->InsertNextPoint(x);
      }
    if (i < static_cast<vtkIdType>(nbPoints * 0.75))
      {
      q3Points->InsertNextPoint(x);
      }
    else
      {
      break;
      }
    }

  // Compute the convex hull for the median points
  vtkIdType nbMedPoints = medianPoints->GetNumberOfPoints();
  if (nbMedPoints > 2)
    {
    int size = medianPoints->GetSizeCCWHullZ();
    this->MedianPoints->SetDataTypeToFloat();
    this->MedianPoints->SetNumberOfPoints(size+1);
    medianPoints->GetCCWHullZ(
      static_cast<float*>(this->MedianPoints->GetData()->GetVoidPointer(0)), size);
    double x[3];
    this->MedianPoints->GetPoint(0, x);
    this->MedianPoints->SetPoint(size, x);
    }
  else if (nbMedPoints > 0)
    {
    this->MedianPoints->SetNumberOfPoints(nbMedPoints);
    for (int j = 0; j < nbMedPoints; j++)
      {
      double x[3];
      medianPoints->GetPoint(j, x);
      this->MedianPoints->SetPoint(j, x);
      }
    }

  // Compute the convex hull for the first quartile points
  vtkIdType nbQ3Points = q3Points->GetNumberOfPoints();
  if (nbQ3Points > 2)
    {
    int size = q3Points->GetSizeCCWHullZ();
    this->Q3Points->SetDataTypeToFloat();
    this->Q3Points->SetNumberOfPoints(size+1);
    q3Points->GetCCWHullZ(
      static_cast<float*>(this->Q3Points->GetData()->GetVoidPointer(0)), size);
    double x[3];
    this->Q3Points->GetPoint(0, x);
    this->Q3Points->SetPoint(size, x);
    }
  else if (nbQ3Points > 0)
    {
    this->Q3Points->SetNumberOfPoints(nbQ3Points);
    for (int j = 0; j < nbQ3Points; j++)
      {
      double x[3];
      q3Points->GetPoint(j, x);
      this->Q3Points->SetPoint(j, x);
      }
    }

  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkPlotBag::Paint(vtkContext2D *painter)
{
  vtkDebugMacro(<< "Paint event called in vtkPlotBag.");

  vtkTable *table = this->Data->GetInput();

  if (!this->Visible || !this->Points || !table)
    {
    return false;
    }

  unsigned char pcolor[4];
  this->Pen->GetColor(pcolor);
  unsigned char bcolor[4];
  this->Brush->GetColor(bcolor);

  // Draw the 2 bags
  this->Pen->SetColor(0, 0, 0);
  this->Brush->SetOpacity(255);
  this->Brush->SetColor(pcolor[0] / 2, pcolor[1] / 2, pcolor[2] / 2);
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  if (this->Q3Points->GetNumberOfPoints() > 2)
    {
    painter->DrawPolygon(this->Q3Points);
    }
  else if (this->Q3Points->GetNumberOfPoints() == 2)
    {
    painter->DrawLine(this->Q3Points);
    }

  this->Brush->SetColor(pcolor);
  this->Brush->SetOpacity(128);
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);

  if (this->MedianPoints->GetNumberOfPoints() > 2)
    {
    painter->DrawPolygon(this->MedianPoints);
    }
  else if (this->MedianPoints->GetNumberOfPoints() == 2)
    {
    painter->DrawLine(this->MedianPoints);
    }

  this->Brush->SetColor(bcolor);
  this->Pen->SetColor(pcolor);

  // Let PlotPoints draw the points as usual
  return this->Superclass::Paint(painter);
}

//-----------------------------------------------------------------------------
bool vtkPlotBag::PaintLegend(vtkContext2D *painter, const vtkRectf& rect, int)
{
  vtkNew<vtkPen> blackPen;
  blackPen->SetWidth(1.0);
  blackPen->SetColor(0, 0, 0, 255);
  painter->ApplyPen(blackPen.GetPointer());

  unsigned char pcolor[4];
  this->Pen->GetColor(pcolor);

  this->Brush->SetColor(pcolor[0] / 2, pcolor[1] / 2, pcolor[2] / 2);
  this->Brush->SetOpacity(255);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2]/2, rect[3]);

  this->Brush->SetColor(pcolor);
  this->Brush->SetOpacity(255);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0] + rect[2] / 2.f, rect[1], rect[2]/2, rect[3]);

  return true;
}

//-----------------------------------------------------------------------------
vtkStringArray* vtkPlotBag::GetLabels()
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
  else if (this->Data->GetInput())
    {
    this->AutoLabels = vtkSmartPointer<vtkStringArray>::New();
    vtkDataArray *density = vtkDataArray::SafeDownCast(
      this->Data->GetInputAbstractArrayToProcess(2, this->GetInput()));
    this->AutoLabels->InsertNextValue(density->GetName());
    return this->AutoLabels;
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlotBag::GetTooltipLabel(const vtkVector2d &plotPos,
                                         vtkIdType seriesIndex,
                                         vtkIdType)
{
  vtkStdString tooltipLabel;
  vtkStdString &format = this->TooltipLabelFormat.empty() ?
        this->TooltipDefaultLabelFormat : this->TooltipLabelFormat;
  // Parse TooltipLabelFormat and build tooltipLabel
  bool escapeNext = false;
  vtkDataArray *density = vtkDataArray::SafeDownCast(
    this->Data->GetInputAbstractArrayToProcess(2, this->GetInput()));
  for (size_t i = 0; i < format.length(); ++i)
    {
    if (escapeNext)
      {
      switch (format[i])
        {
        case 'x':
          tooltipLabel += this->GetNumber(plotPos.GetX(), this->XAxis);
          break;
        case 'y':
          tooltipLabel += this->GetNumber(plotPos.GetY(), this->YAxis);
          break;
        case 'z':
          tooltipLabel += density ?
            density->GetVariantValue(seriesIndex).ToString() :
            vtkStdString("?");
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
void vtkPlotBag::SetInputData(vtkTable *table)
{
  this->Data->SetInputData(table);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPlotBag::SetInputData(vtkTable *table, const vtkStdString &yColumn,
                              const vtkStdString &densityColumn)
{

  vtkDebugMacro(<< "Setting input, Y column = \"" << yColumn.c_str() << "\", "
                << "Density column = \"" << densityColumn.c_str() << "\"");

  if (table->GetColumnByName(densityColumn.c_str())->GetNumberOfTuples()
    != table->GetColumnByName(yColumn.c_str())->GetNumberOfTuples())
    {
    vtkErrorMacro(<< "Input table not correctly initialized!");
    return;
    }

  this->SetInputData(table, yColumn, yColumn, densityColumn);
  this->UseIndexForXSeries = true;
}

//-----------------------------------------------------------------------------
void vtkPlotBag::SetInputData(vtkTable *table, const vtkStdString &xColumn,
                              const vtkStdString &yColumn,
                              const vtkStdString &densityColumn)
{
  vtkDebugMacro(<< "Setting input, X column = \"" << xColumn.c_str()
                << "\", " << "Y column = \""
                << yColumn.c_str() << "\""
                << "\", " << "Density column = \""
                << densityColumn.c_str() << "\"");

  this->Data->SetInputData(table);
  this->Data->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, xColumn.c_str());
  this->Data->SetInputArrayToProcess(1, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, yColumn.c_str());
  this->Data->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, densityColumn.c_str());
}

//-----------------------------------------------------------------------------
void vtkPlotBag::SetInputData(vtkTable *table, vtkIdType xColumn,
                              vtkIdType yColumn,
                              vtkIdType densityColumn)
{
  this->SetInputData(table,
    table->GetColumnName(xColumn),
    table->GetColumnName(yColumn),
    table->GetColumnName(densityColumn));
}

//-----------------------------------------------------------------------------
void vtkPlotBag::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
