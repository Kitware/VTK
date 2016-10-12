/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotSurface.cxx

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
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlotSurface.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotSurface)

//-----------------------------------------------------------------------------
vtkPlotSurface::vtkPlotSurface()
{
  this->NumberOfRows = 0;
  this->NumberOfColumns = 0;
  this->NumberOfVertices = 0;
  this->ColorComponents = 0;
  this->XAxisLabel = "X";
  this->YAxisLabel = "Y";
  this->ZAxisLabel = "Z";
  this->XMinimum = this->XMaximum = this->YMinimum = this->YMaximum = 0.0;
  this->DataHasBeenRescaled = true;
}

//-----------------------------------------------------------------------------
vtkPlotSurface::~vtkPlotSurface()
{
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkPlotSurface::Paint(vtkContext2D *painter)
{
  if (!this->Visible)
  {
    return false;
  }

  if (!this->DataHasBeenRescaled)
  {
    this->RescaleData();
  }

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();

  if (!context)
  {
    return false;
  }

  context->ApplyPen(this->Pen.GetPointer());

  // draw the surface
  if (this->Surface.size() > 0)
  {
    context->DrawTriangleMesh(this->Surface[0].GetData(),
                              static_cast<int>(this->Surface.size()),
                              this->Colors->GetPointer(0),
                              this->ColorComponents);
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::SetInputData(vtkTable *input)
{
  this->InputTable = input;
  this->NumberOfRows = input->GetNumberOfRows();
  this->NumberOfColumns = input->GetNumberOfColumns();
  this->NumberOfVertices =
    (this->NumberOfRows - 1) * (this->NumberOfColumns - 1) * 6;

  // initialize data ranges to row and column indices if they are not
  // already set.
  if (this->XMinimum == 0 && this->XMaximum == 0)
  {
    this->XMaximum = this->NumberOfColumns - 1;
  }
  if (this->YMinimum == 0 && this->YMaximum == 0)
  {
    this->YMaximum = this->NumberOfRows - 1;
  }

  this->Points.clear();
  this->Points.resize(this->NumberOfRows * this->NumberOfColumns);
  float *data = this->Points[0].GetData();
  int pos = 0;
  float surfaceMin = VTK_FLOAT_MAX;
  float surfaceMax = VTK_FLOAT_MIN;
  for (int i = 0; i < this->NumberOfRows; ++i)
  {
    for (int j = 0; j < this->NumberOfColumns; ++j)
    {
      // X (columns)
      data[pos] = this->ColumnToX(j);
      ++pos;

      // Y (rows)
      data[pos] = this->RowToY(i);
      ++pos;

      // Z (cell value)
      float k = input->GetValue(i, j).ToFloat();
      data[pos] = k;
      ++pos;

      if (k < surfaceMin)
      {
        surfaceMin = k;
      }
      if (k > surfaceMax)
      {
        surfaceMax = k;
      }
    }
  }

  if (this->Chart)
  {
    this->Chart->RecalculateBounds();
  }
  this->ComputeDataBounds();

  // setup lookup table
  this->LookupTable->SetNumberOfTableValues(256);
  this->LookupTable->SetRange(surfaceMin, surfaceMax);
  this->LookupTable->Build();
  this->ColorComponents = 3;

  // generate the surface that is used for rendering
  this->GenerateSurface();

  this->DataHasBeenRescaled = true;
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::SetInputData(vtkTable *input,
                          const vtkStdString& vtkNotUsed(xName),
                          const vtkStdString& vtkNotUsed(yName),
                          const vtkStdString& vtkNotUsed(zName))
{
  vtkWarningMacro("Warning: parameters beyond vtkTable are ignored");
  this->SetInputData(input);
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::SetInputData(vtkTable *input,
                          const vtkStdString& vtkNotUsed(xName),
                          const vtkStdString& vtkNotUsed(yName),
                          const vtkStdString& vtkNotUsed(zName),
                          const vtkStdString& vtkNotUsed(colorName))
{
  vtkWarningMacro("Warning: parameters beyond vtkTable are ignored");
  this->SetInputData(input);
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::SetInputData(vtkTable *input,
                                  vtkIdType vtkNotUsed(xColumn),
                                  vtkIdType vtkNotUsed(yColumn),
                                  vtkIdType vtkNotUsed(zColumn))
{
  vtkWarningMacro("Warning: parameters beyond vtkTable are ignored");
  this->SetInputData(input);
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::GenerateSurface()
{
  // clear out and initialize our surface & colors
  this->Surface.clear();
  this->Surface.resize(this->NumberOfVertices);
  this->Colors->Reset();
  this->Colors->Allocate(this->NumberOfVertices * 3);

  // collect vertices of triangles
  float *data = this->Surface[0].GetData();
  int pos = 0;
  for (int i = 0; i < this->NumberOfRows - 1; ++i)
  {
    for (int j = 0; j < this->NumberOfColumns - 1; ++j)
    {
      float value1 = this->InputTable->GetValue(i, j).ToFloat();
      float value2 = this->InputTable->GetValue(i, j + 1).ToFloat();
      float value3 = this->InputTable->GetValue(i + 1, j + 1).ToFloat();
      float value4 = this->InputTable->GetValue(i + 1, j).ToFloat();

      // bottom right triangle
      this->InsertSurfaceVertex(data, value1, i, j, pos);
      this->InsertSurfaceVertex(data, value2, i, j + 1, pos);
      this->InsertSurfaceVertex(data, value3, i + 1, j + 1, pos);

      // upper left triangle
      this->InsertSurfaceVertex(data, value1, i, j, pos);
      this->InsertSurfaceVertex(data, value3, i + 1, j + 1, pos);
      this->InsertSurfaceVertex(data, value4, i + 1, j, pos);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::InsertSurfaceVertex(float *data, float value, int i,
                                         int j, int &pos)
{
    data[pos] = this->ColumnToX(j);
    ++pos;
    data[pos] = this->RowToY(i);
    ++pos;
    data[pos] = value;
    ++pos;

    unsigned char *rgb = this->LookupTable->MapValue(data[pos-1]);
    const unsigned char constRGB[3] = { rgb[0], rgb[1], rgb[2] };
    this->Colors->InsertNextTypedTuple(&constRGB[0]);
    this->Colors->InsertNextTypedTuple(&constRGB[1]);
    this->Colors->InsertNextTypedTuple(&constRGB[2]);
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::SetXRange(float min, float max)
{
  this->XMinimum = min;
  this->XMaximum = max;
  this->DataHasBeenRescaled = false;
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::SetYRange(float min, float max)
{
  this->YMinimum = min;
  this->YMaximum = max;
  this->DataHasBeenRescaled = false;
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::RescaleData()
{
  float *data = this->Points[0].GetData();

  // rescale Points (used by ChartXYZ to generate axes scales).
  int pos = 0;
  for (int i = 0; i < this->NumberOfRows; ++i)
  {
    for (int j = 0; j < this->NumberOfColumns; ++j)
    {
      // X (columns)
      data[pos] = this->ColumnToX(j);
      ++pos;

      // Y (rows)
      data[pos] = this->RowToY(i);
      ++pos;

      // Z value doesn't change
      ++pos;
    }
  }
  this->Chart->RecalculateBounds();
  this->ComputeDataBounds();
  this->DataHasBeenRescaled = true;
}

//-----------------------------------------------------------------------------
float vtkPlotSurface::ColumnToX(int columnIndex)
{
  float newRange = this->XMaximum - this->XMinimum;
  return static_cast<float>(columnIndex) * (newRange / this->NumberOfColumns) +
    this->XMinimum;
}

//-----------------------------------------------------------------------------
float vtkPlotSurface::RowToY(int rowIndex)
{
  float newRange = this->YMaximum - this->YMinimum;
  return static_cast<float>(rowIndex) * (newRange / this->NumberOfRows) +
    this->YMinimum;
}
