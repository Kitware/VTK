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
#include "vtkDoubleArray.h"
#include "vtkTable.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkVariant.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QHash>
#include <QMap>
#include <QPair>
#include <QPixmap>

#include <vtkstd/set>

//----------------------------------------------------------------------------
class vtkQtTableModelAdapter::vtkInternal {
public:

  vtkInternal()
    {
    }
  ~vtkInternal()
    {
    }

  QHash<QModelIndex, QVariant>                       IndexToDecoration;
  QHash<int, QPair<vtkIdType, int> >                 ModelColumnToTableColumn;
  QHash<int, QString>                                 ModelColumnNames;
  QHash<vtkIdType, vtkSmartPointer<vtkDoubleArray> > MagnitudeColumns;

};

//----------------------------------------------------------------------------
vtkQtTableModelAdapter::vtkQtTableModelAdapter(QObject* p)
  : vtkQtAbstractModelAdapter(p)
{
  this->Internal = new vtkInternal;
  this->Table = NULL;
  this->SplitMultiComponentColumns = false;
} 

//----------------------------------------------------------------------------
vtkQtTableModelAdapter::vtkQtTableModelAdapter(vtkTable* t, QObject* p)
  : vtkQtAbstractModelAdapter(p), Table(t)
{
  this->Internal = new vtkInternal;
  this->SplitMultiComponentColumns = false;
  if (this->Table != NULL)
    {
    this->Table->Register(0);
    }
}

//----------------------------------------------------------------------------
vtkQtTableModelAdapter::~vtkQtTableModelAdapter()
{
  if (this->Table != NULL)
    {
    this->Table->Delete();
    }
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkQtTableModelAdapter::SetColorColumnName(const char* name)
{
  int color_column = this->ColorColumn;
  if (name == 0 || !this->Table)
    {
    this->ColorColumn = -1;
    }
  else if (this->SplitMultiComponentColumns)
    {
    this->ColorColumn = -1;
    int color_index=0;
    foreach(QString columnname, this->Internal->ModelColumnNames)
      {
      if (columnname == name)
        {
        this->ColorColumn = color_index;
        break;
        }
      color_index++;
      }
    }
  else
    {
    this->ColorColumn = -1;
    for (int i = 0; i < static_cast<int>(this->Table->GetNumberOfColumns()); i++)
      {
      if (!strcmp(name, this->Table->GetColumn(i)->GetName()))
        {
        this->ColorColumn = i;
        break;
        }
      }
    }
  if (this->ColorColumn != color_column)
    {
    emit this->reset();
    }
}

//----------------------------------------------------------------------------
void vtkQtTableModelAdapter::SetKeyColumnName(const char* name)
{
  int key_column = this->KeyColumn;
  if (name == 0 || !this->Table)
    {
    this->KeyColumn = -1;
    }
  else if (this->SplitMultiComponentColumns)
    {
    this->KeyColumn = -1;
    int key_index=0;
    foreach(QString columnname, this->Internal->ModelColumnNames)
      {
      if (columnname == name)
        {
        this->KeyColumn = key_index;
        break;
        }
      key_index++;
      }
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
  if (this->KeyColumn != key_column)
    {
    emit this->reset();
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkDataObject* vtkQtTableModelAdapter::GetVTKDataObject() const
{
  return this->Table;
}

//----------------------------------------------------------------------------
void vtkQtTableModelAdapter::updateModelColumnHashTables() 
{
  // Clear the hash tables
  this->Internal->ModelColumnToTableColumn.clear();
  this->Internal->ModelColumnNames.clear();
  this->Internal->MagnitudeColumns.clear();

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
    const int nComponents = this->Table->GetColumn(tableColumn)->GetNumberOfComponents();
    for (int c = 0; c < nComponents; ++c)
      {
      QString columnName = this->Table->GetColumnName(tableColumn);
      if (nComponents != 1)
        {
        columnName = QString("%1 (%2)").arg(columnName).arg(c);
        }
      this->Internal->ModelColumnNames[modelColumn] = columnName;
      this->Internal->ModelColumnToTableColumn[modelColumn++] =
        QPair<vtkIdType, int>(tableColumn, c);
      }

    // If number of components is greater than 1, create a new column for Magnitude
    vtkDataArray* dataArray = vtkDataArray::SafeDownCast(this->Table->GetColumn(tableColumn));
    if (nComponents > 1 && dataArray)
      {
      vtkSmartPointer<vtkDoubleArray> magArray = vtkSmartPointer<vtkDoubleArray>::New();
      magArray->SetNumberOfComponents(1);
      const vtkIdType nTuples = dataArray->GetNumberOfTuples();
      for (vtkIdType i = 0; i < nTuples; ++i)
        {
        double mag = 0; 
        for (int j = 0; j < nComponents; ++j)
          {
          double tmp = dataArray->GetComponent(i,j);
          mag += tmp*tmp;
          }
        mag = sqrt(mag);
        magArray->InsertNextValue(mag);
        }

      // Create a name for this column and add it to the ModelColumnNames map
      QString columnName = this->Table->GetColumnName(tableColumn);
      columnName = QString("%1 (Magnitude)").arg(columnName);
      this->Internal->ModelColumnNames[modelColumn] = columnName;

      // Store the magnitude column mapped to its corresponding column in the vtkTable
      this->Internal->MagnitudeColumns[tableColumn] = magArray;

      // Add a pair that has the vtkTable column and component, but use a component value
      // that is out of range to signal that this column represents magnitude
      this->Internal->ModelColumnToTableColumn[modelColumn++] =
        QPair<vtkIdType, int>(tableColumn, nComponents);
      }
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
  vtkstd::set<int> unique_ids;
  for (int i = 0; i < qmil.size(); i++)
    {
    unique_ids.insert(qmil.at(i).internalId());
    }  
  vtkstd::set<int>::iterator iter;
  for (iter = unique_ids.begin(); iter != unique_ids.end(); ++iter)
    {
    index_arr->InsertNextValue(*iter);
    }  

  return IndexSelection;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
bool vtkQtTableModelAdapter::GetSplitMultiComponentColumns() const
{
  return this->SplitMultiComponentColumns;
}

//----------------------------------------------------------------------------
void vtkQtTableModelAdapter::SetSplitMultiComponentColumns(bool value)
{
  if (value != this->SplitMultiComponentColumns)
    {
    this->SplitMultiComponentColumns = value;
    this->updateModelColumnHashTables();
    }
}

//----------------------------------------------------------------------------
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
    return this->Internal->IndexToDecoration[idx];
    }

  // Map the qt model column to a column in the vtk table and
  // get the value from the table as a vtkVariant
  vtkVariant v;
  this->getValue(idx.row(), idx.column(), v); 
  
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
      return QString::fromUtf8(v.ToUnicodeString().utf8_str()).trimmed();
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

//----------------------------------------------------------------------------
bool vtkQtTableModelAdapter::setData(const QModelIndex &idx, const QVariant &value, int role)
{
  if (role == Qt::DecorationRole)
    {
    this->Internal->IndexToDecoration[idx] = value;
    emit this->dataChanged(idx, idx);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
Qt::ItemFlags vtkQtTableModelAdapter::flags(const QModelIndex &idx) const
{
  if (!idx.isValid())
    {
    return Qt::ItemIsEnabled;
    }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

//----------------------------------------------------------------------------
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
      columnName = this->Internal->ModelColumnNames[section];
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
  if (orientation == Qt::Vertical)
    {
    if(role == Qt::DisplayRole || role == Qt::UserRole)
      {
      if(this->KeyColumn >= 0)
        {
        vtkVariant v;
        this->getValue(section, this->KeyColumn, v);
        if (v.IsNumeric())
          {
          return QVariant(v.ToDouble());
          }
        return QVariant(v.ToString().c_str());
        }
      }
    else if(role == Qt::DecorationRole && this->ColorColumn >= 0)
      {
      int column;
      if (this->GetSplitMultiComponentColumns())
        {
        column = this->Internal->ModelColumnToTableColumn[this->ColorColumn].first;
        }
      else
        {
        column = this->ModelColumnToFieldDataColumn(this->ColorColumn);
        }
      vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(this->Table->GetColumn(column));
      if (!colors)
        {
        return QVariant();
        }

      const int nComponents = colors->GetNumberOfComponents();
      if(nComponents >= 3)
        {
        unsigned char rgba[4];
        colors->GetTupleValue(section, rgba);
        int rgb[3];
        rgb[0] = static_cast<int>(0x0ff & rgba[0]);
        rgb[1] = static_cast<int>(0x0ff & rgba[1]);
        rgb[2] = static_cast<int>(0x0ff & rgba[2]);

        QPixmap pixmap(16, 16);
        pixmap.fill(QColor(rgb[0],rgb[1],rgb[2]));
        return QVariant(pixmap);
        }
      }
    }

  return QVariant();
}

//----------------------------------------------------------------------------
void vtkQtTableModelAdapter::getValue(int row, int in_column, vtkVariant& v) const
{
  // Map the qt model column to a column in the vtk table
  int column;
  if (this->GetSplitMultiComponentColumns())
    {
    column = this->Internal->ModelColumnToTableColumn[in_column].first;
    }
  else
    {
    column = this->ModelColumnToFieldDataColumn(in_column);
    }

  // Get the value from the table as a vtkVariant
  // We don't use vtkTable::GetValue() since for multi-component arrays, it can
  // be slow due to the use of vtkDataArray in the vtkVariant.
  vtkAbstractArray* array = this->Table->GetColumn(column);
  if (!array)
    {
    return;
    }
  
  const int nComponents = array->GetNumberOfComponents();

  // Special case- if the variant is an array it means the column data
  // has multiple components
  if (nComponents == 1)
    {
    v = array->GetVariantValue(row);
    }
  else if (nComponents > 1)
    {
    if (this->GetSplitMultiComponentColumns())
      {
      // Map the qt model column to the corresponding component in the vtkTable column
      const int component = this->Internal->ModelColumnToTableColumn[in_column].second;
      if (component < nComponents)
        {
        // If component is in range, then fetch the component's value
        v = array->GetVariantValue(nComponents*row + component);
        }
      else
        {
        // If component is out of range this signals that we should return the
        // value from the magnitude column
        v = this->Internal->MagnitudeColumns[column]->GetValue(row);
        }
      }
    else // don't split columns.
      {
      QString strValue;
      for (int i = 0; i < nComponents; ++i)
        {
        strValue.append(QString("%1, ").arg(
            array->GetVariantValue(row*nComponents + i).ToString().c_str()));
        }
      strValue = strValue.remove(strValue.size()-2, 2); // remove the last comma

      // Reconstruct the variant using this string value
      v = vtkVariant(strValue.toAscii().data());
      }
    }
}

//----------------------------------------------------------------------------
QModelIndex vtkQtTableModelAdapter::index(int row, int column,
                  const QModelIndex & vtkNotUsed(parentIdx)) const
{
  return createIndex(row, column, row);
}

//----------------------------------------------------------------------------
QModelIndex vtkQtTableModelAdapter::parent(const QModelIndex & vtkNotUsed(idx)) const
{
  return QModelIndex();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
    return this->Internal->ModelColumnNames.size();
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
