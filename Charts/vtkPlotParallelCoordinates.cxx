/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotParallelCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotParallelCoordinates.h"

#include "vtkChartParallelCoordinates.h"
#include "vtkContext2D.h"
#include "vtkAxis.h"
#include "vtkPen.h"
#include "vtkFloatArray.h"
#include "vtkVector.h"
#include "vtkTransform2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkTimeStamp.h"
#include "vtkInformation.h"

#include "vtkObjectFactory.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"

class vtkPlotParallelCoordinates::Private :
    public vtkstd::vector< vtkstd::vector<float> >
{
public:
  Private()
  {
    this->SelectionInitialized = false;
  }

  vtkstd::vector<float> AxisPos;
  bool SelectionInitialized;
};


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotParallelCoordinates);

//-----------------------------------------------------------------------------
vtkPlotParallelCoordinates::vtkPlotParallelCoordinates()
{
  this->Points = NULL;
  this->Storage = new vtkPlotParallelCoordinates::Private;
  this->Parent = NULL;
}

//-----------------------------------------------------------------------------
vtkPlotParallelCoordinates::~vtkPlotParallelCoordinates()
{
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = NULL;
    }
  delete this->Storage;
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::Update()
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
bool vtkPlotParallelCoordinates::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotParallelCoordinates.");

  if (!this->Visible)
    {
    return false;
    }

  // Now to plot the points
  if (this->Points)
    {
    painter->ApplyPen(this->Pen);
    painter->DrawPoly(this->Points);
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    }

  painter->ApplyPen(this->Pen);

  if (this->Storage->size() == 0)
    {
    return false;
    }

  size_t cols = this->Storage->size();
  size_t rows = this->Storage->at(0).size();
  vtkVector2f* line = new vtkVector2f[cols];

  // Update the axis positions
  for (size_t i = 0; i < cols; ++i)
    {
    this->Storage->AxisPos[i] = this->Parent->GetAxis(int(i)) ?
                                this->Parent->GetAxis(int(i))->GetPoint1()[0] :
                                0;
    }

  vtkIdType selection = 0;
  vtkIdType id = 0;
  vtkIdType selectionSize = 0;
  if (this->Selection)
    {
    selectionSize = this->Selection->GetNumberOfTuples();
    if (selectionSize)
      {
      this->Selection->GetTupleValue(selection, &id);
      }
    }

  // Draw all of the lines
  //painter->GetPen()->SetColor(230, 230, 230, 255);
  painter->GetPen()->SetColor(0, 0, 0, 25);
  for (size_t i = 0; i < rows; ++i)
    {
    for (size_t j = 0; j < cols; ++j)
      {
      line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][i]);
      }
    painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
    }

  // Now draw the selected lines
  if (this->Selection)
    {
    painter->GetPen()->SetColor(255, 0, 0, 100);
    for (vtkIdType i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
      {
      for (size_t j = 0; j < cols; ++j)
        {
        this->Selection->GetTupleValue(i, &id);
        line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][id]);
        }
      painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
      }
    }

  delete[] line;

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotParallelCoordinates::PaintLegend(vtkContext2D *painter, float rect[4])
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0], rect[1]+0.5*rect[3],
                    rect[0]+rect[2], rect[1]+0.5*rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::GetBounds(double *)
{

}

//-----------------------------------------------------------------------------
bool vtkPlotParallelCoordinates::GetNearestPoint(const vtkVector2f& ,
                                  const vtkVector2f& ,
                                  vtkVector2f* )
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::SetParent(vtkChartParallelCoordinates* parent)
{
  this->Parent = parent;
}

//-----------------------------------------------------------------------------
bool vtkPlotParallelCoordinates::SetSelectionRange(int axis, float low,
                                                   float high)
{
  if (!this->Selection)
    {
    return false;
    }
  if (this->Storage->SelectionInitialized)
    {
    // Further refine the selection that has already been made
    vtkIdTypeArray *array = vtkIdTypeArray::New();
    vtkstd::vector<float>& col = this->Storage->at(axis);
    for (vtkIdType i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
      {
      vtkIdType id = 0;
      this->Selection->GetTupleValue(i, &id);
      if (col[id] >= low && col[id] <= high)
        {
        // Remove this point - no longer selected
        array->InsertNextValue(id);
        }
      }
    this->Selection->DeepCopy(array);
    array->Delete();
    }
  else
    {
    // First run - ensure the selection list is empty and build it up
    vtkstd::vector<float>& col = this->Storage->at(axis);
    for (size_t i = 0; i < col.size(); ++i)
      {
      if (col[i] >= low && col[i] <= high)
        {
        // Remove this point - no longer selected
        this->Selection->InsertNextValue(i);
        }
      }
    this->Storage->SelectionInitialized = true;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotParallelCoordinates::ResetSelectionRange()
{
  this->Storage->SelectionInitialized = false;
  if (this->Selection)
    {
    this->Selection->SetNumberOfTuples(0);
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotParallelCoordinates::UpdateTableCache(vtkTable *table)
{
  // Each axis is a column in our storage array, they are scaled from 0.0 to 1.0
  if (!this->Parent || !table || table->GetNumberOfColumns() == 0)
    {
    return false;
    }

  this->Storage->resize(table->GetNumberOfColumns());
  this->Storage->AxisPos.resize(table->GetNumberOfColumns());
  vtkIdType rows = table->GetNumberOfRows();
  for (vtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
    {
    vtkstd::vector<float>& col = this->Storage->at(i);
    col.resize(rows);
    vtkDataArray* data = vtkDataArray::SafeDownCast(table->GetColumn(i));
    if (!data)
      {
      continue;
      }

    // Also need the range from the appropriate axis, to normalize points
    vtkAxis* axis = this->Parent->GetAxis(i);
    float min = axis->GetMinimum();
    float max = axis->GetMaximum();
    float scale = 1.0f / (max - min);

    for (vtkIdType j = 0; j < rows; ++j)
      {
      col[j] = (data->GetTuple1(j)-min) * scale;
      }
    }

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
