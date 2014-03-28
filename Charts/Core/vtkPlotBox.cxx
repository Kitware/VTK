/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotBox.h"

#include "vtkAbstractArray.h"
#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkChartBox.h"
#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextMapper2D.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkNew.h"
#include "vtkScalarsToColors.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTimeStamp.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"

#include <algorithm>
#include <vector>

class vtkPlotBox::Private :
    public std::vector< std::vector<double> >
{
public:
  Private()
  {
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotBox)

//-----------------------------------------------------------------------------
vtkPlotBox::vtkPlotBox()
{
  this->Storage = new vtkPlotBox::Private();
  this->Pen->SetColor(0, 0, 0);
  this->BoxWidth = 20.;
  this->LookupTable = 0;
  this->TooltipDefaultLabelFormat = "%y";

  this->TitleProperties = vtkTextProperty::New();
  this->TitleProperties->SetColor(0.0, 0.0, 0.0);
  this->TitleProperties->SetFontSize(12);
  this->TitleProperties->SetFontFamilyToArial();
  this->TitleProperties->SetBold(1);
  this->TitleProperties->SetJustificationToCentered();
}

//-----------------------------------------------------------------------------
vtkPlotBox::~vtkPlotBox()
{
  delete this->Storage;

  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }


  this->TitleProperties->Delete();
}

//-----------------------------------------------------------------------------
void vtkPlotBox::Update()
{
  if (!this->Visible)
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

  this->UpdateTableCache(table);
}

//-----------------------------------------------------------------------------
bool vtkPlotBox::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotBox.");

  if (!this->Visible)
    {
    return false;
    }

  if (this->Storage->size() == 0 || this->Storage->at(0).size() < 5)
    {
    return false;
    }

  vtkChartBox *parent = vtkChartBox::SafeDownCast(this->Parent);

  int nbCols = static_cast<int>(this->Storage->size());
  for (int i = 0; i < nbCols; i++)
    {
    vtkStdString colName = parent->GetVisibleColumns()->GetValue(i);
    int index;
    this->GetInput()->GetRowData()->GetAbstractArray(colName.c_str(), index);
    double rgb[4];
    this->LookupTable->GetIndexedColor(index, rgb);
    unsigned char crgba[4] =
      {
      static_cast<unsigned char>(rgb[0] * 255.),
      static_cast<unsigned char>(rgb[1] * 255.),
      static_cast<unsigned char>(rgb[2] * 255.),
      255
      };

    if (parent->GetSelectedColumn() == i)
      {
      crgba[0] = crgba[0]^255;
      crgba[1] = crgba[1]^255;
      crgba[2] = crgba[2]^255;
      }
    DrawBoxPlot(i, crgba, parent->GetXPosition(i), painter);
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotBox::DrawBoxPlot(int i, unsigned char *rgba, double x,
                             vtkContext2D *painter)
{
  std::vector<double>& colQuartiles = this->Storage->at(i);
  if (colQuartiles.size() < 5)
    {
    return;
    }

  painter->ApplyPen(this->Pen);

  vtkNew<vtkBrush> brush;
  brush->SetColor(rgba);
  painter->ApplyBrush(brush.GetPointer());

  // Helper variables for x position
  double xpos = x + 0.5 * this->BoxWidth;
  double xneg = x - 0.5 * this->BoxWidth;
  double hBoxW = this->BoxWidth * 0.25;

  // Fetch the quartiles and median
  double* q = &colQuartiles[0];

  // Draw the box
  painter->DrawQuad(xpos, q[1], xneg, q[1], xneg, q[3], xpos, q[3]);

  // Draw the whiskers: ends of the whiskers match the
  // extremum values of the quartiles
  painter->DrawLine(x, q[0], x, q[1]);
  painter->DrawLine(x - hBoxW, q[0], x + hBoxW, q[0]);
  painter->DrawLine(x, q[3], x, q[4]);
  painter->DrawLine(x - hBoxW, q[4], x + hBoxW, q[4]);

  // Draw the median
  vtkNew<vtkPen> whitePen;
  unsigned char brushColor[4];
  brush->GetColor(brushColor);
  // Use a gray pen if the brush is black so the median is always visible
  if (brushColor[0] == 0 && brushColor[1] == 0 && brushColor[2] == 0)
    {
    whitePen->SetWidth(this->Pen->GetWidth());
    whitePen->SetColor(128, 128, 128, 128);
    whitePen->SetOpacity(this->Pen->GetOpacity());
    painter->ApplyPen(whitePen.GetPointer());
    }

  painter->DrawLine(xneg, q[2], xpos, q[2]);
}

//-----------------------------------------------------------------------------
vtkStringArray* vtkPlotBox::GetLabels()
{
  if (this->Labels)
    {
    return this->Labels;
    }
  return 0;
}

//-----------------------------------------------------------------------------
bool vtkPlotBox::PaintLegend(vtkContext2D* painter, const vtkRectf& rec, int)
{
  if (this->Storage->size() == 0 || this->Storage->at(0).size() < 5)
    {
    return false;
    }

  vtkChartBox *parent = vtkChartBox::SafeDownCast(this->Parent);

  painter->ApplyTextProp(this->TitleProperties);

  int nbCols = static_cast<int>(this->Storage->size());
  for (int i = 0; i < nbCols; i++)
    {
    vtkStdString colName = parent->GetVisibleColumns()->GetValue(i);
    if (this->GetLabels() && this->GetLabels()->GetNumberOfValues() > i)
      {
      colName = this->GetLabels()->GetValue(parent->GetColumnId(colName));
      }
    painter->DrawString(parent->GetXPosition(i), rec.GetY(), colName);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotBox::SetInputData(vtkTable* table)
{
  if (table == this->Data->GetInput() &&
    (!table || table->GetMTime() < this->BuildTime))
    {
    return;
    }

  this->vtkPlot::SetInputData(table);

  bool updateVisibility = table != this->Data->GetInput();
  vtkChartBox *parent = vtkChartBox::SafeDownCast(this->Parent);

  if (parent && table && updateVisibility)
    {
    parent->SetColumnVisibilityAll(false);
    // By default make the first 10 columns visible in a plot.
    for (vtkIdType i = 0; i < table->GetNumberOfColumns() && i < 10; ++i)
      {
      parent->SetColumnVisibility(table->GetColumnName(i), true);
      }
    }
  else if (parent && updateVisibility)
    {
    // No table, therefore no visible columns
    parent->GetVisibleColumns()->SetNumberOfTuples(0);
    }
  // Create a default lookup table is non set yet
  if (!this->LookupTable)
    {
    this->CreateDefaultLookupTable();
    }
}

//-----------------------------------------------------------------------------
namespace
{
// See if the point is within tolerance.
bool inRange(const vtkVector2f& point, const vtkVector2f& tol,
             const vtkVector2f& current)
{
  return current.GetX() > point.GetX() - tol.GetX() &&
         current.GetX() < point.GetX() + tol.GetX() &&
         current.GetY() > point.GetY() - tol.GetY() &&
         current.GetY() < point.GetY() + tol.GetY();
}
}

//-----------------------------------------------------------------------------
vtkIdType vtkPlotBox::GetNearestPoint(const vtkVector2f& point,
                                      const vtkVector2f& tol,
                                      vtkVector2f* location)
{
  vtkChartBox *parent = vtkChartBox::SafeDownCast(this->Parent);

  int nbCols = static_cast<int>(this->Storage->size());
  for (int i = 0; i < nbCols; i++)
    {
    vtkVector2f v;
    v.SetX(parent->GetXPosition(i));
    for (int j = 0; j < 5; j++)
      {
      v.SetY((*this->Storage)[i][j]);
      if (inRange(point, tol, v))
        {
        vtkAxis* axis = parent->GetYAxis();
        double min = axis->GetUnscaledMinimum();
        double max = axis->GetUnscaledMaximum();
        double scale = 1.0f / (max - min);
        double y = (*this->Storage)[i][j] / scale + min;
        location->SetX(i);
        location->SetY(y);
        return static_cast<int>(i);
        }
      }
    }
  return -1;

}
//-----------------------------------------------------------------------------
bool vtkPlotBox::UpdateTableCache(vtkTable *table)
{
  // Each boxplot is a column in our storage array,
  // they are scaled from 0.0 to 1.0
  vtkChartBox *parent = vtkChartBox::SafeDownCast(this->Parent);

  if (!parent || !table || table->GetNumberOfColumns() == 0)
    {
    return false;
    }

  vtkStringArray* cols = parent->GetVisibleColumns();

  this->Storage->resize(cols->GetNumberOfTuples());
  vtkIdType rows = table->GetNumberOfRows();

  for (vtkIdType i = 0; i < cols->GetNumberOfTuples(); ++i)
    {
    std::vector<double>& col = this->Storage->at(i);
    col.resize(rows);
    vtkSmartPointer<vtkDataArray> data =
        vtkDataArray::SafeDownCast(table->GetColumnByName(cols->GetValue(i)));
    if (!data)
      {
      continue;
      }

    vtkAxis* axis = parent->GetYAxis();
    // Also need the range from the appropriate axis, to normalize points
    double min = axis->GetUnscaledMinimum();
    double max = axis->GetUnscaledMaximum();
    double scale = 1.0f / (max - min);

    for (vtkIdType j = 0; j < rows; ++j)
      {
      col[j] = (data->GetTuple1(j) - min) * scale;
      }
    std::sort(col.begin(), col.end());
    }

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotBox::SetLookupTable(vtkScalarsToColors *lut)
{
  if (this->LookupTable != lut)
    {
    if (this->LookupTable)
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
vtkScalarsToColors *vtkPlotBox::GetLookupTable()
{
  if (this->LookupTable == 0)
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void vtkPlotBox::SetColumnColor(const vtkStdString& colName, double *rgb)
{
  if (this->LookupTable == 0)
    {
    this->CreateDefaultLookupTable();
    }
  int index;
  this->GetInput()->GetRowData()->GetAbstractArray(colName.c_str(), index);
  vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->LookupTable);
  if (index >= 0 && lut)
    {
    lut->SetTableValue(index, rgb[0], rgb[1], rgb[2]);
    lut->Build();
    }
}

//-----------------------------------------------------------------------------
void vtkPlotBox::CreateDefaultLookupTable()
{
  // There must be an input to create a lookup table
  if (this->GetInput())
    {
    if (this->LookupTable)
      {
      this->LookupTable->UnRegister(this);
      }
    vtkLookupTable* lut = vtkLookupTable::New();
    this->LookupTable = lut;
    // Consistent Register/UnRegisters.
    this->LookupTable->Register(this);
    this->LookupTable->Delete();
    vtkTable *table = this->GetInput();
    lut->SetNumberOfColors(table->GetNumberOfColumns());
    this->LookupTable->Build();
    }
}

//-----------------------------------------------------------------------------
void vtkPlotBox::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
