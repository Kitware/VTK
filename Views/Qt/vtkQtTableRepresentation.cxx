/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkQtTableRepresentation.h"
#include "vtkQtTableModelAdapter.h"

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkLookupTable.h>
#include <vtkObjectFactory.h>
#include <vtkTable.h>

#include <QModelIndex>
#include <QColor>

#include <assert.h>

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkQtTableRepresentation, ColorTable, vtkLookupTable);

// ----------------------------------------------------------------------
vtkQtTableRepresentation::vtkQtTableRepresentation()
{
  this->ModelAdapter = new vtkQtTableModelAdapter;

  this->ColorTable = vtkLookupTable::New();
  this->ColorTable->Register(this);
  this->ColorTable->Delete();

  this->ColorTable->SetHueRange(0.0, 1.0);
  this->ColorTable->SetRange(0.0, 1.0);
  this->ColorTable->Build();

  this->SeriesColors = vtkDoubleArray::New();
  this->SeriesColors->SetNumberOfComponents(4);
  this->SeriesColors->Register(this);
  this->SeriesColors->Delete();

  this->KeyColumnInternal = NULL;
  this->FirstDataColumn = NULL;
  this->LastDataColumn = NULL;
}

// ----------------------------------------------------------------------

vtkQtTableRepresentation::~vtkQtTableRepresentation()
{
  if (this->ModelAdapter)
    {
    delete this->ModelAdapter;
    }
  this->ColorTable->UnRegister(this);
  this->SeriesColors->UnRegister(this);
  this->SetKeyColumnInternal(NULL);
  this->SetFirstDataColumn(NULL);
  this->SetLastDataColumn(NULL);
}

// ----------------------------------------------------------------------

int
vtkQtTableRepresentation::RequestData(vtkInformation*,
                                      vtkInformationVector**,
                                      vtkInformationVector*)
{
  this->UpdateTable();
  return 1;
}

// ----------------------------------------------------------------------

void
vtkQtTableRepresentation::SetKeyColumn(const char *col)
{
  if((!col && !this->KeyColumnInternal) ||
      (this->KeyColumnInternal && col && strcmp(this->KeyColumnInternal, col) == 0))
    {
    return;
    }

  this->SetKeyColumnInternal(col);
  this->ModelAdapter->SetKeyColumn(-1);
  this->Modified();
  // We don't call Update(), representations should not call Update() on
  // themselves when their ivars are changed. It's almost like a vtkAlgorithm
  // calling Update() on itself when an ivar change which is not recommended.
  //this->Update();
}

// ----------------------------------------------------------------------

char* vtkQtTableRepresentation::GetKeyColumn()
{
  return this->GetKeyColumnInternal();
}

// ----------------------------------------------------------------------
void vtkQtTableRepresentation::UpdateTable()
{
  this->ResetModel();

  if (!this->GetInput())
    {
    return;
    }

  vtkTable *table = vtkTable::SafeDownCast(this->GetInput());
  if (!table)
    {
    vtkErrorMacro(<<"vtkQtTableRepresentation: I need a vtkTable as input.  You supplied a " << this->GetInput()->GetClassName() << ".");
    return;
    }

  // Set first/last data column names if they
  // have not already been set.
  const char* firstDataColumn = this->FirstDataColumn;
  const char* lastDataColumn = this->LastDataColumn;
  if (!firstDataColumn)
    {
    firstDataColumn = table->GetColumnName(0);
    }
  if (!lastDataColumn)
    {
    lastDataColumn = table->GetColumnName(table->GetNumberOfColumns()-1);
    }

  // Now that we're sure of having data, put it into a Qt model
  // adapter that we can push into the QListView.  Before we hand that
  // off, though, we'll need to come up with colors for
  // each series.
  //int keyColumnIndex = -1;
  int firstDataColumnIndex = -1;
  int lastDataColumnIndex = -1;
  //if (this->KeyColumnInternal != NULL)
  //  {
  //  table->GetRowData()->GetAbstractArray(this->KeyColumnInternal, keyColumnIndex);
  //  if (keyColumnIndex >= 0)
  //    {
  //    this->ModelAdapter->SetKeyColumn(keyColumnIndex);
  //    }
  //  else
  //    {
  //    // Either the user didn't specify a key column or else it wasn't
  //    // found.  We'll do the best we can.
  //    vtkWarningMacro(<<"vtkQtTableRepresentation: Key column "
  //                    << (this->KeyColumnInternal ? this->KeyColumnInternal : "(NULL)")
  //                    << " not found.  Defaulting to column 0.");
  //    this->ModelAdapter->SetKeyColumn(0);
  //    }
  //  }
  if (firstDataColumn != NULL)
    {
    table->GetRowData()->GetAbstractArray(firstDataColumn,
                                          firstDataColumnIndex);
    }
  if (lastDataColumn != NULL)
    {
    table->GetRowData()->GetAbstractArray(lastDataColumn,
                                          lastDataColumnIndex);
    }
  this->ModelAdapter->SetDataColumnRange(firstDataColumnIndex, lastDataColumnIndex);

  // The view will try to do this when we add the representation, but
  // we need the model to be populated before that so we'll just do it
  // here.

  this->ModelAdapter->SetVTKDataObject(table);
  if (this->KeyColumnInternal != NULL)
    {
    this->ModelAdapter->SetKeyColumnName(this->KeyColumnInternal);
    }
  this->CreateSeriesColors();
}

// ----------------------------------------------------------------------

void
vtkQtTableRepresentation::ResetModel()
{
  this->SetModelType();
  if (this->ModelAdapter)
    {
    // FIXME
    // Need to alert the model of potential changes to the vtkTable
    // in different way than disconnecting/reconnecting the vtkTable from
    // the model adapter
    //this->ModelAdapter->SetVTKDataObject(NULL);
    }
  this->SeriesColors->Reset();
  this->SeriesColors->SetNumberOfComponents(4);
}

// ----------------------------------------------------------------------

void
vtkQtTableRepresentation::CreateSeriesColors()
{
  this->SeriesColors->Reset();
  this->SeriesColors->SetNumberOfComponents(4);

  int size = this->ModelAdapter->rowCount(QModelIndex());

  this->SeriesColors->SetNumberOfTuples(size);

  for (int i = 0; i < size; ++i)
    {
    double seriesValue = 1;
    if (size > 1)
      {
      seriesValue = static_cast<double>(i) / (size-1);
      }
    QColor c;
    if (this->ColorTable)
      {
      double rgb[3];
      double opacity;
      this->ColorTable->GetColor(seriesValue, rgb);
      opacity = this->ColorTable->GetOpacity(seriesValue);
      c.setRgbF(rgb[0], rgb[1], rgb[2], opacity);
      }
    else
      {
      c.setHsvF(seriesValue, 1, 0.7);
      }

    this->SeriesColors->SetComponent(i, 0, c.redF());
    this->SeriesColors->SetComponent(i, 1, c.greenF());
    this->SeriesColors->SetComponent(i, 2, c.blueF());
    this->SeriesColors->SetComponent(i, 3, c.alphaF());
    }
}


// ----------------------------------------------------------------------

void
vtkQtTableRepresentation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "First data column: "
     << (this->FirstDataColumn ? this->FirstDataColumn : "(NULL)")
     << "\n";

  os << indent << "Last data column: "
     << (this->LastDataColumn ? this->LastDataColumn : "(NULL)")
     << "\n";

  os << indent << "Key column: "
     << (this->KeyColumnInternal ? this->KeyColumnInternal : "(NULL)")
     << "\n";

  os << indent << "Model adapter: Qt object " << this->ModelAdapter
     << "\n";

  os << indent << "Color creation table: ";
  this->ColorTable->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Series color table: ";
  this->SeriesColors->PrintSelf(os, indent.GetNextIndent());
}
