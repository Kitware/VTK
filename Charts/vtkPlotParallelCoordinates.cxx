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
#include "vtkDoubleArray.h"
#include "vtkVector.h"
#include "vtkTransform2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkStringArray.h"
#include "vtkTimeStamp.h"
#include "vtkInformation.h"
#include "vtkSmartPointer.h"

// Need to turn some arrays of strings into categories
#include "vtkStringToCategory.h"

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
  this->Pen->SetColor(0, 0, 0, 25);
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
  painter->ApplyPen(this->Pen);
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
void vtkPlotParallelCoordinates::SetInput(vtkTable* table)
{
  if (table == this->Data->GetInput() && (!table ||
                                          table->GetMTime() < this->BuildTime))
    {
    return;
    }

  this->vtkPlot::SetInput(table);
  if (this->Parent && table)
    {
    // By default make the first 10 columns visible in a plot.
    for (vtkIdType i = 0; i < table->GetNumberOfColumns() && i < 10; ++i)
      {
      this->Parent->SetColumnVisibility(table->GetColumnName(i), true);
      }
    }
  else if (this->Parent)
    {
    // No table, therefore no visible columns
    this->Parent->GetVisibleColumns()->SetNumberOfTuples(0);
    }
}

//-----------------------------------------------------------------------------
bool vtkPlotParallelCoordinates::UpdateTableCache(vtkTable *table)
{
  // Each axis is a column in our storage array, they are scaled from 0.0 to 1.0
  if (!this->Parent || !table || table->GetNumberOfColumns() == 0)
    {
    return false;
    }

  vtkStringArray* cols = this->Parent->GetVisibleColumns();
  this->Storage->resize(cols->GetNumberOfTuples());
  this->Storage->AxisPos.resize(cols->GetNumberOfTuples());
  vtkIdType rows = table->GetNumberOfRows();

  for (vtkIdType i = 0; i < cols->GetNumberOfTuples(); ++i)
    {
    vtkstd::vector<float>& col = this->Storage->at(i);
    vtkAxis* axis = this->Parent->GetAxis(i);
    col.resize(rows);
    vtkSmartPointer<vtkDataArray> data =
        vtkDataArray::SafeDownCast(table->GetColumnByName(cols->GetValue(i)));
    if (!data)
      {
      if (table->GetColumnByName(cols->GetValue(i))->IsA("vtkStringArray"))
        {
        // We have a different kind of column - attempt to make it into an enum
        vtkStringToCategory* stoc = vtkStringToCategory::New();
        stoc->SetInput(table);
        stoc->SetInputArrayToProcess(0, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     cols->GetValue(i));
        stoc->SetCategoryArrayName("enumPC");
        stoc->Update();
        vtkTable* table2 = vtkTable::SafeDownCast(stoc->GetOutput());
        vtkTable* stringTable = vtkTable::SafeDownCast(stoc->GetOutput(1));
        if (table2)
          {
          data = vtkDataArray::SafeDownCast(table2->GetColumnByName("enumPC"));
          }
        if (stringTable && stringTable->GetColumnByName("Strings"))
          {
          vtkStringArray* strings =
              vtkStringArray::SafeDownCast(stringTable->GetColumnByName("Strings"));
          vtkSmartPointer<vtkDoubleArray> arr =
              vtkSmartPointer<vtkDoubleArray>::New();
          for (vtkIdType j = 0; j < strings->GetNumberOfTuples(); ++j)
            {
            arr->InsertNextValue(j);
            }
          // Now we need to set the range on the string axis
          axis->SetTickLabels(strings);
          axis->SetTickPositions(arr);
          if (strings->GetNumberOfTuples() > 1)
            {
            axis->SetRange(0.0, strings->GetNumberOfTuples()-1);
            }
          else
            {
            axis->SetRange(-0.1, 0.1);
            }
          axis->Update();
          }
        stoc->Delete();
        }
      // If we still don't have a valid data array then skip this column.
      if (!data)
        {
        continue;
        }
      }

    // Also need the range from the appropriate axis, to normalize points
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
