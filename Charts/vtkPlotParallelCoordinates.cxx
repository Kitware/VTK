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
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkStringArray.h"
#include "vtkTimeStamp.h"
#include "vtkInformation.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkLookupTable.h"

// Need to turn some arrays of strings into categories
#include "vtkStringToCategory.h"

#include "vtkObjectFactory.h"

#include <vector>
#include <algorithm>

class vtkPlotParallelCoordinates::Private :
    public std::vector< std::vector<float> >
{
public:
  Private()
  {
    this->SelectionInitialized = false;
  }

  std::vector<float> AxisPos;
  bool SelectionInitialized;
};


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotParallelCoordinates);

//-----------------------------------------------------------------------------
vtkPlotParallelCoordinates::vtkPlotParallelCoordinates()
{
  this->Storage = new vtkPlotParallelCoordinates::Private;
  this->Pen->SetColor(0, 0, 0, 25);

  this->LookupTable = 0;
  this->Colors = 0;
  this->ScalarVisibility = 0;
}

//-----------------------------------------------------------------------------
vtkPlotParallelCoordinates::~vtkPlotParallelCoordinates()
{
  delete this->Storage;
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  if ( this->Colors != 0 )
    {
    this->Colors->UnRegister(this);
    }
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

  painter->ApplyPen(this->Pen);

  if (this->Storage->size() == 0)
    {
    return false;
    }

  size_t cols = this->Storage->size();
  size_t rows = this->Storage->at(0).size();
  vtkVector2f* line = new vtkVector2f[cols];

  // Update the axis positions
  vtkChartParallelCoordinates *parent =
      vtkChartParallelCoordinates::SafeDownCast(this->Parent);

  for (size_t i = 0; i < cols; ++i)
    {
    this->Storage->AxisPos[i] = parent->GetAxis(int(i)) ?
                                parent->GetAxis(int(i))->GetPoint1()[0] :
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
  int nc_comps(0);
  if (this->ScalarVisibility && this->Colors)
    {
    nc_comps = static_cast<int>(this->Colors->GetNumberOfComponents());
    }
  if (this->ScalarVisibility && this->Colors && nc_comps == 4)
    {
    for (size_t i = 0, nc = 0; i < rows; ++i, nc += nc_comps)
      {
      for (size_t j = 0; j < cols; ++j)
        {
        line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][i]);
        }
      painter->GetPen()->SetColor(this->Colors->GetPointer(nc));
      painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
      }
    }
  else
    {
    for (size_t i = 0; i < rows; ++i)
      {
      for (size_t j = 0; j < cols; ++j)
        {
        line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][i]);
        }
      painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
      }
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
bool vtkPlotParallelCoordinates::PaintLegend(vtkContext2D *painter,
                                             const vtkRectf& rect, int)
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0]          , rect[1] + 0.5 * rect[3],
                    rect[0] + rect[2], rect[1] + 0.5 * rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::GetBounds(double *)
{

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
    std::vector<float>& col = this->Storage->at(axis);
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
    std::vector<float>& col = this->Storage->at(axis);
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

  bool updateVisibility = table != this->Data->GetInput();
  this->vtkPlot::SetInput(table);
  vtkChartParallelCoordinates *parent =
      vtkChartParallelCoordinates::SafeDownCast(this->Parent);

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
}

//-----------------------------------------------------------------------------
bool vtkPlotParallelCoordinates::UpdateTableCache(vtkTable *table)
{
  // Each axis is a column in our storage array, they are scaled from 0.0 to 1.0
  vtkChartParallelCoordinates *parent =
      vtkChartParallelCoordinates::SafeDownCast(this->Parent);
  if (!parent || !table || table->GetNumberOfColumns() == 0)
    {
    return false;
    }

  vtkStringArray* cols = parent->GetVisibleColumns();

  this->Storage->resize(cols->GetNumberOfTuples());
  this->Storage->AxisPos.resize(cols->GetNumberOfTuples());
  vtkIdType rows = table->GetNumberOfRows();

  for (vtkIdType i = 0; i < cols->GetNumberOfTuples(); ++i)
    {
    std::vector<float>& col = this->Storage->at(i);
    vtkAxis* axis = parent->GetAxis(i);
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

  // Additions for color mapping
  if (this->ScalarVisibility && !this->ColorArrayName.empty())
    {
    vtkDataArray* c =
      vtkDataArray::SafeDownCast(table->GetColumnByName(this->ColorArrayName));
    // TODO: Should add support for categorical coloring & try enum lookup
    if (c)
      {
      if (!this->LookupTable)
        {
        this->CreateDefaultLookupTable();
        }
      this->Colors = this->LookupTable->MapScalars(c, VTK_COLOR_MODE_MAP_SCALARS, -1);
      // Consistent register and unregisters
      this->Colors->Register(this);
      this->Colors->Delete();
      }
    else
      {
      this->Colors->UnRegister(this);
      this->Colors = 0;
      }
    }

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::SetLookupTable(vtkScalarsToColors *lut)
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
vtkScalarsToColors *vtkPlotParallelCoordinates::GetLookupTable()
{
  if ( this->LookupTable == 0 )
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::CreateDefaultLookupTable()
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

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::SelectColorArray(const vtkStdString &arrayName)
{
  vtkTable *table = this->Data->GetInput();
  if (!table)
    {
    vtkDebugMacro(<< "SelectColorArray called with no input table set.");
    return;
    }
  if (this->ColorArrayName == arrayName)
    {
    return;
    }
  for (vtkIdType c = 0; c < table->GetNumberOfColumns(); ++c)
    {
    if (table->GetColumnName(c) == arrayName)
      {
      this->ColorArrayName = arrayName;
      this->Modified();
      return;
      }
    }
  vtkDebugMacro(<< "SelectColorArray called with invalid column name.");
  this->ColorArrayName = "";
  this->Modified();
  return;
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlotParallelCoordinates::GetColorArrayName()
{
  return this->ColorArrayName;
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::SelectColorArray(vtkIdType arrayNum)
{
  vtkTable *table = this->Data->GetInput();
  if (!table)
    {
    vtkDebugMacro(<< "SelectColorArray called with no input table set.");
    return;
    }
  vtkDataArray *col = vtkDataArray::SafeDownCast(table->GetColumn(arrayNum));
  // TODO: Should add support for categorical coloring & try enum lookup
  if (!col)
    {
    vtkDebugMacro(<< "SelectColorArray called with invalid column index");
    return;
    }
  else
    {
    if (this->ColorArrayName == table->GetColumnName(arrayNum))
      {
      return;
      }
    else
      {
      this->ColorArrayName = table->GetColumnName(arrayNum);
      this->Modified();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPlotParallelCoordinates::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
