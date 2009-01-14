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
#include "vtkStdString.h"
#include "vtkVariant.h"

#include <QIcon>
#include <QPixmap>

vtkQtTableModelAdapter::vtkQtTableModelAdapter(QObject* p)
  : vtkQtAbstractModelAdapter(p)
{
  Table = NULL;
} 
  
vtkQtTableModelAdapter::vtkQtTableModelAdapter(vtkTable* t, QObject* p)
  : vtkQtAbstractModelAdapter(p), Table(t)
{
  if (this->Table != NULL)
    {
    this->Table->Register(0);
    this->GenerateHashMap();
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

void vtkQtTableModelAdapter::GenerateHashMap()
{
  vtkAbstractArray *pedigreeIds = this->Table->GetRowData()->GetPedigreeIds();

  this->IdToPedigreeHash.clear();
  this->PedigreeToIndexHash.clear();
  this->IndexToIdHash.clear();
  for (vtkIdType i = 0; i < this->Table->GetNumberOfRows(); i++)
    {
    QModelIndex idx = this->createIndex(i, 0, static_cast<int>(i));
    vtkIdType pedigree = -1;
    if (pedigreeIds != NULL)
      {
      vtkVariant v(0);
      switch (pedigreeIds->GetDataType())
        {
        vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(pedigreeIds->GetVoidPointer(i)));
        }
      pedigree = v.ToInt();
      }
    else
      {
      pedigree = i;
      }
    this->IdToPedigreeHash[i] = pedigree;
    this->PedigreeToIndexHash[pedigree] = idx;
    this->IndexToIdHash[idx] = i;
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

void vtkQtTableModelAdapter::setTable(vtkTable* t) 
{
  if (t == this->Table)
    {
    return;
    }
  if (this->Table != NULL)
    {
    this->Table->Delete();
    }
  this->Table = t;
  if (this->Table != NULL)
    {
    this->Table->Register(0);
    this->GenerateHashMap();

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
//    qWarning("Using vtkQtTableModelAdapter without a table input set!");
    return true;
    }
  if (this->Table->GetNumberOfRows() == 0)
    {
    return true;
    }
  return false;
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

  if (!this->ViewRows && (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    vtkAbstractArray* arr = this->Table->GetColumn(this->DataStartColumn + idx.row());
    return QVariant(arr->GetName());
    }
    
  // Get the item
  int row = idx.row();
  int column = this->ModelColumnToFieldDataColumn(idx.column());
  vtkVariant v = this->Table->GetValue(row, column);
  
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

  if (!this->ViewRows && orientation == Qt::Horizontal &&
      (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    return QVariant("");
    }
  if (!this->ViewRows && orientation == Qt::Vertical &&
    (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    vtkAbstractArray* arr = this->Table->GetColumn(this->DataStartColumn + section);
    return QVariant(arr->GetName());
    }

  // For horizontal headers, try to convert the column names to double.
  // If it doesn't work, return a string.
  if (orientation == Qt::Horizontal &&
      (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    int column = this->ModelColumnToFieldDataColumn(section);
    QVariant svar(this->Table->GetColumnName(column));
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
    if (this->ViewRows)
      {
      return this->Table->GetNumberOfRows();
      }
    else
      {
      return (this->DataEndColumn - this->DataStartColumn + 1);
      }
    }
  return 0;
}

int vtkQtTableModelAdapter::columnCount(const QModelIndex &) const
{
  if (this->noTableCheck())
    {
    return 0;
    }

  if (!this->ViewRows)
    {
    return 1;
    }

  int numArrays = this->Table->GetNumberOfColumns();
  int numDataArrays = this->DataEndColumn - this->DataStartColumn + 1;
  switch (this->ViewType)
    {
    case FULL_VIEW:
      return numArrays;
    case DATA_VIEW:
      return numDataArrays;
    case METADATA_VIEW:
      return numArrays - numDataArrays;
    default:
      vtkGenericWarningMacro("vtkQtTreeModelAdapter: Bad view type.");
    };
  return 0;
}

vtkIdType vtkQtTableModelAdapter::IdToPedigree(vtkIdType id) const
{
  if (this->ViewRows)
    {
    return this->IdToPedigreeHash[id];
    }
  return id;
}

vtkIdType vtkQtTableModelAdapter::PedigreeToId(vtkIdType pedigree) const
{
  if (this->ViewRows)
    {
    return this->IndexToIdHash[this->PedigreeToIndexHash[pedigree]];
    }
  return pedigree;
}

QModelIndex vtkQtTableModelAdapter::PedigreeToQModelIndex(vtkIdType pedigree) const
{
  if (this->ViewRows)
    {
    return this->PedigreeToIndexHash[pedigree];
    }
  return this->index(static_cast<int>(pedigree), 0);
}

vtkIdType vtkQtTableModelAdapter::QModelIndexToPedigree(QModelIndex idx) const
{
  if (this->ViewRows)
    {
    return this->IdToPedigreeHash[this->IndexToIdHash[idx]];
    }
  return idx.row();
}

