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
  Table = NULL;
} 
  
vtkQtTableModelAdapter::vtkQtTableModelAdapter(vtkTable* t, QObject* p)
  : vtkQtAbstractModelAdapter(p), Table(t)
{
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
  node->SetFieldType(vtkSelectionNode::VERTEX);
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
