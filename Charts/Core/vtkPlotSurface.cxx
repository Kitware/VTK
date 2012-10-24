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
  this->PointIsClipped = NULL;
  this->XAxisLabel = "X";
  this->YAxisLabel = "Y";
  this->ZAxisLabel = "Z";
  this->XMinimum = this->XMaximum = this->YMinimum = this->YMaximum = 0.0;
  this->DataHasBeenRescaled = true;
}

//-----------------------------------------------------------------------------
vtkPlotSurface::~vtkPlotSurface()
{
  if (this->PointIsClipped != NULL)
    {
    for (int i = 0; i < this->NumberOfRows; ++i)
      {
      delete [] this->PointIsClipped[i];
      }
      delete [] this->PointIsClipped;
    }
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

  // Update the points that fall inside our axes
  // Update the points that fall inside our axes
  if (this->Chart->ShouldCheckClipping())
    {
    this->UpdateClippedPoints();
    }

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

  this->PointIsClipped = new bool*[this->NumberOfRows];

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
    this->PointIsClipped[i] = new bool[this->NumberOfColumns];

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

      // set PointIsClipped to true so the surface will be (re)generated.
      this->PointIsClipped[i][j] = true;

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
  this->UpdateClippedPoints();

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
void vtkPlotSurface::GenerateClippedSurface()
{
  // clear out and initialize our surface & colors
  this->Surface.clear();
  this->Surface.resize(this->NumberOfVertices);
  this->Colors->Reset();
  this->Colors->Allocate(this->NumberOfVertices * 3);

  // collect vertices of unclipped triangles
  float *data = this->Surface[0].GetData();
  int pos = 0;
  for (int i = 0; i < this->NumberOfRows - 1; ++i)
    {
    for (int j = 0; j < this->NumberOfColumns - 1; ++j)
      {
      // we need these two points to draw either triangle
      if (this->PointIsClipped[i][j] || this->PointIsClipped[i + 1][j + 1])
        {
        continue;
        }

      bool drawBottomRightTriangle = true;
      if (this->PointIsClipped[i][j + 1])
        {
        drawBottomRightTriangle = false;
        }

      bool drawUpperLeftTriangle = true;
      if (this->PointIsClipped[i + 1][j])
        {
        drawUpperLeftTriangle = false;
        }

      if (!drawBottomRightTriangle && !drawUpperLeftTriangle)
        {
        continue;
        }

      float value1 = this->InputTable->GetValue(i, j).ToFloat();
      float value3 = this->InputTable->GetValue(i + 1, j + 1).ToFloat();

      if (drawBottomRightTriangle)
        {
        float value2 = this->InputTable->GetValue(i, j + 1).ToFloat();
        this->InsertSurfaceVertex(data, value1, i, j, pos);
        this->InsertSurfaceVertex(data, value2, i, j + 1, pos);
        this->InsertSurfaceVertex(data, value3, i + 1, j + 1, pos);
        }

      if (drawUpperLeftTriangle)
        {
        float value4 = this->InputTable->GetValue(i + 1, j).ToFloat();
        this->InsertSurfaceVertex(data, value1, i, j, pos);
        this->InsertSurfaceVertex(data, value3, i + 1, j + 1, pos);
        this->InsertSurfaceVertex(data, value4, i + 1, j, pos);
        }
      }
    }
  this->Colors->Squeeze();
}

//-----------------------------------------------------------------------------
void vtkPlotSurface::UpdateClippedPoints()
{
  if (this->Chart == NULL)
    {
    return;
    }

  vtkIdType row, col;
  bool clippingChanged = false;
  bool priorValue;

  for( size_t i = 0; i < this->Points.size(); ++i )
    {
    row = i / this->NumberOfColumns;
    col = i % this->NumberOfColumns;
    priorValue = this->PointIsClipped[row][col];
    if( this->Chart->PointShouldBeClipped(this->Points[i]) )
      {
      this->PointIsClipped[row][col] = true;
      }
    else
      {
      this->PointIsClipped[row][col] = false;
      }
    if (!clippingChanged && priorValue != this->PointIsClipped[row][col])
      {
      clippingChanged = true;
      }
    }
  if (clippingChanged)
    {
    this->GenerateClippedSurface();
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
    this->Colors->InsertNextTupleValue(&constRGB[0]);
    this->Colors->InsertNextTupleValue(&constRGB[1]);
    this->Colors->InsertNextTupleValue(&constRGB[2]);
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

  // rescale the surface that is used for actual rendering
  this->UpdateClippedPoints();

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
