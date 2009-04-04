/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableModelAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkQtTableModelAdapter.h"

#include "vtkDataSetAttributes.h"
#include "vtkTable.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkVariant.h"

#include <QIcon>
#include <QPixmap>

vtkQtTableModelAdapter::vtkQtTableModelAdapter(QObject* p)
  : vtkQtAbstractModelAdapter(p)
{
  this->Table = NULL;
  this->SplitMultiComponentColumns = false;
} 
  
vtkQtTableModelAdapter::vtkQtTableModelAdapter(vtkTable* t, QObject* p)
  : vtkQtAbstractModelAdapter(p), Table(t)
{
  this->SplitMultiComponentColumns = false;
  if (this->Table != NULL)
    {
    this->Table->Register(0);
    }
}

vtkQtTableModelAdapter::~vtkQtTableModelAdapter()
{
  if (this->Table != NULL)
    {
    this->Table->Delete();
    }
}

void vtkQtTableModelAdapter::SetKeyColumnName(const char* name)
{
  if (name == 0)
    {
    this->KeyColumn = -1;
    }
  else
    {
    this->KeyColumn = -1;
    for (int i = 0; i < static_cast<int>(this->Table->GetNumberOfColumns()); i++)
      {
      if (!strcmp(name, this->Table->GetColumn(i)->GetName()))
        {
        this->KeyColumn = i;
        break;
        }
      }
    }
}

void vtkQtTableModelAdapter::SetVTKDataObject(vtkDataObject *obj)
{
  vtkTable *t = vtkTable::SafeDownCast(obj);
  if (obj && !t)
    {
    qWarning("vtkQtTableModelAdapter needs a vtkTable for SetVTKDataObject");
    return;
    }
    
  // Okay it's a table so set it :)
  this->setTable(t);
}

vtkDataObject* vtkQtTableModelAdapter::GetVTKDataObject() const
{
  return this->Table;
}

void vtkQtTableModelAdapter::updateModelColumnHashTables() 
{
  // Clear the hash tables
  this->ModelColumnToTableColumn.clear();
  this->ModelColumnNames.clear();

  // Do not continue if SplitMultiComponentColumns is false
  // or our table is null.
  if (!this->SplitMultiComponentColumns || !this->Table)
    {
    return;
    }

  // Get the start and end columns.
  int startColumn = 0;
  int endColumn = this->Table->GetNumberOfColumns() - 1;
  if (this->GetViewType() == DATA_VIEW)
    {
    startColumn = this->DataStartColumn;
    endColumn = this->DataEndColumn;
    }

  // Double check to make sure startColumn and endColumn are within bounds
  int maxColumn = this->Table->GetNumberOfColumns()-1;
  if ((startColumn < 0 || startColumn > maxColumn) ||
      (endColumn   < 0 || endColumn   > maxColumn))
    {
    return;
    }

  // For each column in the vtkTable, iterate over the column's number of
  // components to construct a mapping from qt model columns to
  // vtkTable columns-component pairs.  Also generate qt model column names.
  int modelColumn = 0;
  for (int tableColumn = startColumn; tableColumn <= endColumn; ++tableColumn)
    {
    int nComponents = this->Table->GetColumn(tableColumn)->GetNumberOfComponents();
    for (int c = 0; c < nComponents; ++c)
      {
      QString columnName = this->Table->GetColumnName(tableColumn);
      if (nComponents != 1)
        {
        columnName = QString("%1 (%2)").arg(columnName).arg(c);
        }
      this->ModelColumnNames[modelColumn] = columnName;
      this->ModelColumnToTableColumn[modelColumn++] = QPair<vtkIdType, int>(tableColumn, c);
      }
    }
}

void vtkQtTableModelAdapter::setTable(vtkTable* t) 
{
  if (this->Table != NULL)
    {
    this->Table->Delete();
    }
  this->Table = t;
  if (this->Table != NULL)
    {
    this->Table->Register(0);

    // When setting a table, update the QHash tables for column mapping.
    // If SplitMultiComponentColumns is disabled, this call will just clear
    // the tables and return.
    this->updateModelColumnHashTables();

    // We will assume the table is totally
    // new and any views should update completely
    emit this->reset();
    }
}

bool vtkQtTableModelAdapter::noTableCheck() const
{
  if (this->Table == NULL)
    {
    // It's not necessarily an error to have a null pointer for the
    // table.  It just means that the model is empty.
    return true;
    }
  if (this->Table->GetNumberOfRows() == 0)
    {
    return true;
    }
  return false;
}

// Description:
// Selection conversion from VTK land to Qt land
vtkSelection* vtkQtTableModelAdapter::QModelIndexListToVTKIndexSelection(
  const QModelIndexList qmil) const
{
  // Create vtk index selection
  vtkSelection* IndexSelection = vtkSelection::New(); // Caller needs to delete
  vtkSmartPointer<vtkSelectionNode> node =
    vtkSmartPointer<vtkSelectionNode>::New();
  node->SetContentType(vtkSelectionNode::INDICES);
  node->SetFieldType(vtkSelectionNode::ROW);
  vtkSmartPointer<vtkIdTypeArray> index_arr =
    vtkSmartPointer<vtkIdTypeArray>::New();
  node->SetSelectionList(index_arr);
  IndexSelection->AddNode(node);
  
  // Run through the QModelIndexList pulling out vtk indexes
  for (int i = 0; i < qmil.size(); i++)
    {
    vtkIdType vtk_index = qmil.at(i).internalId();
    index_arr->InsertNextValue(vtk_index);
    }  
  return IndexSelection;
}

QItemSelection vtkQtTableModelAdapter::VTKIndexSelectionToQItemSelection(
  vtkSelection *vtksel) const
{

  QItemSelection qis_list;
  vtkSelectionNode* node = vtksel->GetNode(0);
  if (node)
    {
    vtkIdTypeArray* arr = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
    if (arr)
      {
      for (vtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
        {
        vtkIdType vtk_index = arr->GetValue(i);
        QModelIndex qmodel_index =
          this->createIndex(vtk_index, 0, static_cast<int>(vtk_index));
        qis_list.select(qmodel_index, qmodel_index);
        }
      }
    }
  return qis_list;
}

bool vtkQtTableModelAdapter::GetSplitMultiComponentColumns() const
{
  return this->SplitMultiComponentColumns;
}

void vtkQtTableModelAdapter::SetSplitMultiComponentColumns(bool value)
{
  if (value != this->SplitMultiComponentColumns)
    {
    this->SplitMultiComponentColumns = value;
    this->updateModelColumnHashTables();
    }
}

QVariant vtkQtTableModelAdapter::data(const QModelIndex &idx, int role) const
{
  if (this->noTableCheck())
    {
    return QVariant();
    }
  if (!idx.isValid())
    {
    return QVariant();
    }

  if (role == Qt::DecorationRole)
    {
    return this->IndexToDecoration[idx];
    }

  // Map the qt model column to a column in the vtk table
  int column;
  if (this->GetSplitMultiComponentColumns())
    {
    column = this->ModelColumnToTableColumn[idx.column()].first;
    }
  else
    {
    column = this->ModelColumnToFieldDataColumn(idx.column());
    }

  // Get the value from the table as a vtkVariant
  vtkVariant v = this->Table->GetValue(idx.row(), column);

  // Special case- if the variant is an array and we are splitting
  // multi-component columns:
  if (v.IsArray() && this->GetSplitMultiComponentColumns())
    {
    // SplitMultiComponentColumns only works for vtkDataArray
    vtkDataArray* dataArray = vtkDataArray::SafeDownCast(v.ToArray());
    if (dataArray)
      {
      // Map the qt model column to the corresponding component in the vtk column
      int component = this->ModelColumnToTableColumn[idx.column()].second;

      // Reconstruct the variant using a single scalar from the array
      double value = dataArray->GetComponent(0, component);
      v = vtkVariant(value);
      }
    }
  
  // Return a string if they ask for a display role 
  if (role == Qt::DisplayRole)
    {
    bool ok;
    double value = v.ToDouble(&ok);
    if (ok)
      {
      return QVariant(value);
      }
    else
      {
      vtkStdString s = v.ToString();
      const char* const whitespace = " \t\r\n\v\f";
      s.erase(0, s.find_first_not_of(whitespace));
      s.erase(s.find_last_not_of(whitespace) + 1);
      return QVariant(s.c_str());
      }
    }

  // Return a byte array if they ask for a decorate role 
  if (role == Qt::DecorationRole)
    {
    // Create a QBtyeArray out of the variant
    vtkStdString s = v.ToString();
    QByteArray byteArray(s, static_cast<int>(s.length()));
    return QVariant(byteArray);
    }
  
  // Return a raw value if they ask for a user role
  if (role == Qt::UserRole)
    {
    if (v.IsNumeric())
      {
      return QVariant(v.ToDouble());
      }
    return QVariant(v.ToString().c_str());
    }

  // Hmmm... if the role isn't decorate, user or display
  // then just punt and return a empty qvariant
  return QVariant();
   
}

bool vtkQtTableModelAdapter::setData(const QModelIndex &idx, const QVariant &value, int role)
{
  if (role == Qt::DecorationRole)
    {
    this->IndexToDecoration[idx] = value;
    emit this->dataChanged(idx, idx);
    return true;
    }
  return false;
}

Qt::ItemFlags vtkQtTableModelAdapter::flags(const QModelIndex &idx) const
{
  if (!idx.isValid())
    {
    return Qt::ItemIsEnabled;
    }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant vtkQtTableModelAdapter::headerData(int section, Qt::Orientation orientation,
                    int role) const
{
  if (this->noTableCheck())
    {
    return QVariant();
    }

  // For horizontal headers, try to convert the column names to double.
  // If it doesn't work, return a string.
  if (orientation == Qt::Horizontal &&
      (role == Qt::DisplayRole || role == Qt::UserRole))
    {

    QString columnName; 
    if (this->GetSplitMultiComponentColumns())
      {
      columnName = this->ModelColumnNames[section];
      }
    else
      {
      int column = this->ModelColumnToFieldDataColumn(section);
      columnName = this->Table->GetColumnName(column);
      }

    QVariant svar(columnName);
    bool ok;
    double value = svar.toDouble(&ok);
    if (ok)
      {
      return QVariant(value);
      }
    return svar;
    }

  // For vertical headers, return values in the first column if
  // KeyColumn is valid.
  if (orientation == Qt::Vertical && this->KeyColumn >= 0 &&
      (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    vtkVariant v = this->Table->GetValue(section, this->KeyColumn);
    if (v.IsNumeric())
      {
      return QVariant(v.ToDouble());
      }
    return QVariant(v.ToString().c_str());
    }

  return QVariant();
}

QModelIndex vtkQtTableModelAdapter::index(int row, int column,
                  const QModelIndex & vtkNotUsed(parentIdx)) const
{
  return createIndex(row, column, row);
}

QModelIndex vtkQtTableModelAdapter::parent(const QModelIndex & vtkNotUsed(idx)) const
{
  return QModelIndex();
}

int vtkQtTableModelAdapter::rowCount(const QModelIndex & mIndex) const
{
  if (this->noTableCheck())
    {
    return 0;
    }
  if (mIndex == QModelIndex())
    {
    return this->Table->GetNumberOfRows();
    }
  return 0;
}

int vtkQtTableModelAdapter::columnCount(const QModelIndex &) const
{
  if (this->noTableCheck())
    {
    return 0;
    }

  // If we are splitting multi-component columns, then just return the
  // number of entries in the QHash map that stores column names.
  if (this->GetSplitMultiComponentColumns())
    {
    return this->ModelColumnNames.size();
    }

  // The number of columns in the qt model depends on the current ViewType
  switch (this->ViewType)
    {
    case FULL_VIEW:
      return this->Table->GetNumberOfColumns();;
    case DATA_VIEW:
      return this->DataEndColumn - this->DataStartColumn + 1;;
    default:
      vtkGenericWarningMacro("vtkQtTreeModelAdapter: Bad view type.");
    };
  return 0;
}
