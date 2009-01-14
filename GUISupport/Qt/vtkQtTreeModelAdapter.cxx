/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTreeModelAdapter.cxx

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
#include "vtkQtTreeModelAdapter.h"

#include "vtkAdjacentVertexIterator.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkPointData.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkVariantArray.h"

#include <QIcon>
#include <QPixmap>

vtkQtTreeModelAdapter::vtkQtTreeModelAdapter(QObject* p, vtkTree* t)
  : vtkQtAbstractModelAdapter(p)
{
  this->TreeMTime = 0;
  this->Tree = 0;
  this->setTree(t);
  this->ChildIterator = vtkAdjacentVertexIterator::New();
}

vtkQtTreeModelAdapter::~vtkQtTreeModelAdapter()
{
  if (this->Tree)
    {
    this->Tree->Delete();
    }
  this->ChildIterator->Delete();
}

void vtkQtTreeModelAdapter::SetKeyColumnName(const char* name)
{
  if (name == 0)
    {
    this->KeyColumn = -1;
    }
  else
    {
    this->KeyColumn = -1;
    for (int i = 0; i < this->Tree->GetVertexData()->GetNumberOfArrays(); i++)
      {
      if (!strcmp(name, this->Tree->GetVertexData()->GetAbstractArray(i)->GetName()))
        {
        this->KeyColumn = i;
        break;
        }
      }
    }
}

void vtkQtTreeModelAdapter::SetVTKDataObject(vtkDataObject *obj)
{
  vtkTree *t = vtkTree::SafeDownCast(obj);
  if (obj && !t)
    {
    cerr << "vtkQtTreeModelAdapter needs a vtkTree for SetVTKDataObject" << endl;
    return;
    }
    
  // Okay it's a tree so set it :)
  this->setTree(t);
}

vtkDataObject* vtkQtTreeModelAdapter::GetVTKDataObject() const
{
  return this->Tree;
}

void vtkQtTreeModelAdapter::setTree(vtkTree* t)
{
  if (!t || (t != this->Tree))
    {
    vtkTree* tempSGMacroVar = this->Tree;
    this->Tree = t;
    if (this->Tree != NULL) 
      {
      this->Tree->Register(0);
      vtkIdType root = this->Tree->GetRoot();
      this->IdToPedigreeHash.clear();
      this->PedigreeToIndexHash.clear();
      this->IndexToIdHash.clear();
      if (root >= 0)
        {
        vtkIdType row = 0;
        this->GenerateHashMap(row, root, this->createIndex(0, 0, static_cast<int>(root)));
        }
      this->TreeMTime = this->Tree->GetMTime();
      }
    if (tempSGMacroVar != NULL)
      {
      tempSGMacroVar->UnRegister(0);
      }
    emit reset();
    }
    
  // Okay it's the same pointer but the contents
  // of the tree might have been modified so
  // check for that condition
  else if (this->Tree->GetMTime() != this->TreeMTime)
    {
    this->treeModified();
    }
}


void vtkQtTreeModelAdapter::treeModified()
{
  this->IdToPedigreeHash.clear();
  this->PedigreeToIndexHash.clear();
  this->IndexToIdHash.clear();
  this->RowToPedigreeHash.clear();
  if (this->Tree->GetNumberOfVertices() > 0)
    {
    vtkIdType root = this->Tree->GetRoot();
    vtkIdType row = 0;
    this->GenerateHashMap(row, root, this->createIndex(0, 0, static_cast<int>(root)));
    }
  this->TreeMTime = this->Tree->GetMTime();
  emit reset();
}

void vtkQtTreeModelAdapter::GenerateHashMap(vtkIdType & row, vtkIdType id, QModelIndex idx)
{
  vtkAbstractArray *pedigreeIds = this->Tree->GetVertexData()->GetPedigreeIds();
  vtkIdType pedigree = -1;
  if (pedigreeIds != NULL)
    {
      vtkVariant v(0);
      switch (pedigreeIds->GetDataType())
        {
        vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(pedigreeIds->GetVoidPointer(id)));
        }
      pedigree = v.ToInt();
    }
  else
    {
    pedigree = id;
    }
  this->IdToPedigreeHash[id] = pedigree;
  this->PedigreeToIndexHash[pedigree] = idx;
  this->IndexToIdHash[idx] = id;
  this->RowToPedigreeHash[row] = pedigree;
  vtkAdjacentVertexIterator* it = vtkAdjacentVertexIterator::New();
  this->Tree->GetChildren(id, it);
  int i = 0;
  while (it->HasNext())
    {
    ++row;
    vtkIdType v = it->Next();
    this->GenerateHashMap(row, v, this->createIndex(i, 0, static_cast<int>(v)));
    ++i;
    }
  it->Delete();
}

//----------------------------------------------------------------------------
QVariant vtkQtTreeModelAdapterArrayValue(vtkAbstractArray* arr, vtkIdType i, vtkIdType j)
{
  int comps = arr->GetNumberOfComponents();
  if (vtkDataArray::SafeDownCast(arr))
    {
    vtkDataArray* data = vtkDataArray::SafeDownCast(arr);
    return QVariant(data->GetComponent(i, j));
    }
  else if (vtkStringArray::SafeDownCast(arr))
    {
    vtkStringArray* data = vtkStringArray::SafeDownCast(arr);
    return QVariant(data->GetValue(i*comps + j));
    }
  else if (vtkVariantArray::SafeDownCast(arr))
    {
    vtkVariantArray* data = vtkVariantArray::SafeDownCast(arr);
    return QVariant(QString(data->GetValue(i*comps + j).ToString().c_str()));
    }
  
  vtkGenericWarningMacro("Unknown array type in vtkQtTreeModelAdapterArrayValue.");
  return QVariant();
}

QVariant vtkQtTreeModelAdapter::data(const QModelIndex &idx, int role) const
{
  if (!this->Tree)
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

  // If viewing columns, just return the column name.
  if (!this->ViewRows && (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    vtkAbstractArray* arr = this->Tree->GetVertexData()->GetAbstractArray(this->DataStartColumn + idx.row());
    return QVariant(arr->GetName());
    }

  vtkIdType vertex = static_cast<vtkIdType>(idx.internalId());
  int column = this->ModelColumnToFieldDataColumn(idx.column());
  vtkAbstractArray* arr = this->Tree->GetVertexData()->GetAbstractArray(column);
  if (role == Qt::DisplayRole)
    {
    vtkStdString s;
    int comps = arr->GetNumberOfComponents();
    for (int i = 0; i < comps; i++)
      {
      if (i > 0)
        {
        s += ", ";
        }
      s += vtkQtTreeModelAdapterArrayValue(arr, vertex, i).toString().toStdString();
      }
    const char* const whitespace = " \t\r\n\v\f";
    s.erase(0, s.find_first_not_of(whitespace));
    s.erase(s.find_last_not_of(whitespace) + 1);
    return QVariant(s.c_str());
    }
  else if (role == Qt::UserRole)
    {
    return vtkQtTreeModelAdapterArrayValue(arr, vertex, 0);
    }

  return QVariant();
}

bool vtkQtTreeModelAdapter::setData(const QModelIndex &idx, const QVariant &value, int role)
{
  if (role == Qt::DecorationRole)
    {
    this->IndexToDecoration[idx] = value;
    emit this->dataChanged(idx, idx);
    return true;
    }
  return false;
}

Qt::ItemFlags vtkQtTreeModelAdapter::flags(const QModelIndex &idx) const
{
  if (!idx.isValid())
    {
    return Qt::ItemIsEnabled;
    }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant vtkQtTreeModelAdapter::headerData(int section, Qt::Orientation orientation,
                    int role) const
{
  if (!this->ViewRows && orientation == Qt::Horizontal && 
      (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    return QVariant("");
    }

  if (!this->ViewRows && orientation == Qt::Vertical && 
      (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    vtkAbstractArray* arr = this->Tree->GetVertexData()->GetAbstractArray(this->DataStartColumn + section);
    return QVariant(arr->GetName());
    }

  // For horizontal headers, try to convert the column names to double.
  // If it doesn't work, return a string.
  if (orientation == Qt::Horizontal &&
      (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    section = this->ModelColumnToFieldDataColumn(section);
    QVariant svar(this->Tree->GetVertexData()->GetArrayName(section));
    bool ok;
    double value = svar.toDouble(&ok);
    if (ok)
      {
      return QVariant(value);
      }
    return svar;
    }

  // For vertical headers, return values in the key column if
  // KeyColumn is valid.
  if (orientation == Qt::Vertical && this->KeyColumn != -1 &&
      (role == Qt::DisplayRole || role == Qt::UserRole))
    {
    vtkIdType pedigree = this->RowToPedigreeHash[section];
    vtkAbstractArray* arr = this->Tree->GetVertexData()->GetAbstractArray(this->KeyColumn);
    return vtkQtTreeModelAdapterArrayValue(arr, pedigree, 0);
    }

  return QVariant();
}

QModelIndex vtkQtTreeModelAdapter::index(vtkIdType item) const
{
  return this->PedigreeToIndexHash[item];
}

QModelIndex vtkQtTreeModelAdapter::index(int row, int column,
                  const QModelIndex &parentIdx) const
{
  if (!this->Tree)
    {
    return QModelIndex();
    }

  if (!this->ViewRows)
    {
    return createIndex(row, column, row);
    }
  
  vtkIdType parentItem;

  if (!parentIdx.isValid())
    {
    if (row == 0)
      {
      return createIndex(row, column, static_cast<int>(this->Tree->GetRoot()));
      }
    else
      {
      return QModelIndex();
      }
    }
  else
    {
    parentItem = static_cast<vtkIdType>(parentIdx.internalId());
    }

  this->Tree->GetChildren(parentItem, this->ChildIterator);
  if (row < this->Tree->GetNumberOfChildren(parentItem))
    {
    vtkIdType child = this->ChildIterator->Next();
    for (int i = 0; i < row; ++i)
      {
      child = this->ChildIterator->Next();
      }
    return createIndex(row, column, static_cast<int>(child));
    }
  else
    {
    return QModelIndex();
    }
}

QModelIndex vtkQtTreeModelAdapter::parent(const QModelIndex &idx) const
{
  if (!this->Tree)
    {
    return QModelIndex();
    }
  
  if (!idx.isValid())
    {
    return QModelIndex();
    }

  if (!this->ViewRows)
    {
    return QModelIndex();
    }

  vtkIdType child = static_cast<vtkIdType>(idx.internalId());

  if (child == this->Tree->GetRoot())
    {
    return QModelIndex();
    }

  vtkIdType parentId = this->Tree->GetParent(child);

  if (parentId == this->Tree->GetRoot())
    {
    return createIndex(0, 0, static_cast<int>(parentId));
    }

  vtkIdType grandparentId = this->Tree->GetParent(parentId);

  vtkIdType row = -1;
  this->Tree->GetChildren(grandparentId, this->ChildIterator);
  int i = 0;
  while (this->ChildIterator->HasNext())
    {
    if (this->ChildIterator->Next() == parentId)
      {
      row = i;
      break;
      }
    ++i;
    }

  return createIndex(row, 0, static_cast<int>(parentId));
}

int vtkQtTreeModelAdapter::rowCount(const QModelIndex &idx) const
{
  if (!this->Tree)
    {
    return 1;
    }
  
  if (!idx.isValid())
    {
    if (!this->ViewRows)
      {
      return (this->DataEndColumn - this->DataStartColumn + 1);
      }
    else
      {
      return 1;
      }
    }

  if (!this->ViewRows)
    {
    return 0;
    }

  vtkIdType parentId = static_cast<vtkIdType>(idx.internalId());
  return this->Tree->GetNumberOfChildren(parentId);
}

int vtkQtTreeModelAdapter::columnCount(const QModelIndex & vtkNotUsed(parentIdx)) const
{
  if (!this->Tree)
    {
    return 0;
    }

  if (!this->ViewRows)
    {
    return 1;
    }

  int numArrays = this->Tree->GetVertexData()->GetNumberOfArrays();
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

vtkIdType vtkQtTreeModelAdapter::IdToPedigree(vtkIdType id) const
{
  if (this->ViewRows)
    {
    return this->IdToPedigreeHash[id];
    }
  return id;
}

vtkIdType vtkQtTreeModelAdapter::PedigreeToId(vtkIdType pedigree) const
{
  if (this->ViewRows)
    {
    return this->IndexToIdHash[this->PedigreeToIndexHash[pedigree]];
    }
  return pedigree;
}

QModelIndex vtkQtTreeModelAdapter::PedigreeToQModelIndex(vtkIdType pedigree) const
{
  if (this->ViewRows)
    {
    return this->PedigreeToIndexHash[pedigree];
    }
  return this->index(static_cast<int>(pedigree), 0);
}

vtkIdType vtkQtTreeModelAdapter::QModelIndexToPedigree(QModelIndex idx) const
{
  if (this->ViewRows)
    {
    return this->IdToPedigreeHash[this->IndexToIdHash[idx]];
    }
  return idx.row();
}
