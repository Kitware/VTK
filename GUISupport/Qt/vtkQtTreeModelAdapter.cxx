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
#include "vtkAnnotation.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkConvertSelection.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkUnicodeStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

#include <algorithm>
#include <QIcon>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QMimeData>

#include <vtksys/ios/sstream>
#include <set>

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

void vtkQtTreeModelAdapter::SetColorColumnName(const char* name)
{
  if (name == 0)
    {
    this->ColorColumn = -1;
    }
  else
    {
    this->ColorColumn = -1;
    for (int i = 0; i < this->Tree->GetVertexData()->GetNumberOfArrays(); i++)
      {
      if (!strcmp(name, this->Tree->GetVertexData()->GetAbstractArray(i)->GetName()))
        {
        this->ColorColumn = i;
        break;
        }
      }
    }
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

unsigned long vtkQtTreeModelAdapter::GetVTKDataObjectMTime() const
{
  return this->TreeMTime;
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
      this->VTKIndexToQtModelIndex.clear();
      this->VTKIndexToQtModelIndex.resize(this->Tree->GetNumberOfVertices());
      if (root >= 0)
        {
        this->GenerateVTKIndexToQtModelIndex(root,
          this->createIndex(0, 0, static_cast<int>(root)));
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
  this->VTKIndexToQtModelIndex.clear();
  if (this->Tree->GetNumberOfVertices() > 0)
    {
    vtkIdType root = this->Tree->GetRoot();
    this->VTKIndexToQtModelIndex.resize(this->Tree->GetNumberOfVertices());
    this->GenerateVTKIndexToQtModelIndex(root,
      this->createIndex(0, 0, static_cast<int>(root)));
    }
  this->TreeMTime = this->Tree->GetMTime();
  emit reset();
}

// Description:
// Selection conversion from VTK land to Qt land
vtkSelection* vtkQtTreeModelAdapter::QModelIndexListToVTKIndexSelection(
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
  std::set<int> unique_ids;
  for (int i = 0; i < qmil.size(); i++)
    {
    unique_ids.insert(qmil.at(i).internalId());
    }

  std::set<int>::iterator iter;
  for (iter = unique_ids.begin(); iter != unique_ids.end(); ++iter)
    {
    index_arr->InsertNextValue(*iter);
    }

  return IndexSelection;
}

QItemSelection vtkQtTreeModelAdapter::VTKIndexSelectionToQItemSelection(
  vtkSelection *vtksel) const
{
  QItemSelection qis_list;
  for(unsigned int j=0; j<vtksel->GetNumberOfNodes(); ++j)
    {
    vtkSelectionNode* node = vtksel->GetNode(j);
    if (node && node->GetFieldType() == vtkSelectionNode::VERTEX)
      {
      vtkIdTypeArray* arr = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      if (arr)
        {
        for (vtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
          {
          vtkIdType vtk_index = arr->GetValue(i);
          QModelIndex qmodel_index = this->VTKIndexToQtModelIndex[vtk_index];
          qis_list.select(qmodel_index, qmodel_index);
          }
        }
      }
    }
  return qis_list;
}

void vtkQtTreeModelAdapter::GenerateVTKIndexToQtModelIndex(vtkIdType vtk_index, QModelIndex qmodel_index)
{

  // Store the QModelIndex for selection conversions later
  this->VTKIndexToQtModelIndex.replace(vtk_index, qmodel_index);

  // Iterate through the children of this tree nodes
  vtkAdjacentVertexIterator* it = vtkAdjacentVertexIterator::New();
  this->Tree->GetChildren(vtk_index, it);
  int i = 0;
  while (it->HasNext())
    {
    vtkIdType vtk_child_index = it->Next();
    this->GenerateVTKIndexToQtModelIndex(vtk_child_index,
      this->createIndex(i, 0, static_cast<int>(vtk_child_index)));
    ++i;
    }
  it->Delete();
}

//----------------------------------------------------------------------------
QVariant vtkQtTreeModelAdapterArrayValue(vtkAbstractArray* arr, vtkIdType i, vtkIdType j)
{
  int comps = arr->GetNumberOfComponents();
  if(vtkDataArray* const data = vtkDataArray::SafeDownCast(arr))
    {
    return QVariant(data->GetComponent(i, j));
    }

  if(vtkStringArray* const data = vtkStringArray::SafeDownCast(arr))
    {
    return QVariant(data->GetValue(i*comps + j));
    }

  if(vtkUnicodeStringArray* const data = vtkUnicodeStringArray::SafeDownCast(arr))
    {
    return QVariant(QString::fromUtf8(data->GetValue(i*comps + j).utf8_str()));
    }

  if(vtkVariantArray* const data = vtkVariantArray::SafeDownCast(arr))
    {
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

  //if (role == Qt::DecorationRole)
  //  {
  //  return this->IndexToDecoration[idx];
  //  }

  vtkIdType vertex = static_cast<vtkIdType>(idx.internalId());
  int column = this->ModelColumnToFieldDataColumn(idx.column());
  vtkAbstractArray* arr = this->Tree->GetVertexData()->GetAbstractArray(column);
  if (role == Qt::DisplayRole)
    {
    return QString::fromUtf8(arr->GetVariantValue(vertex).ToUnicodeString().utf8_str()).trimmed();
    }
  else if (role == Qt::UserRole)
    {
    return vtkQtTreeModelAdapterArrayValue(arr, vertex, 0);
    }

  if(this->ColorColumn >= 0)
    {
    int colorColumn = this->ModelColumnToFieldDataColumn(this->ColorColumn);
    vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(this->Tree->GetVertexData()->GetAbstractArray(colorColumn));
    if (!colors)
      {
      return QVariant();
      }

    const int nComponents = colors->GetNumberOfComponents();
    if(nComponents < 3)
      {
      return QVariant();
      }

    unsigned char rgba[4];
    colors->GetTupleValue(vertex, rgba);
    int rgb[3];
    rgb[0] = static_cast<int>(0x0ff & rgba[0]);
    rgb[1] = static_cast<int>(0x0ff & rgba[1]);
    rgb[2] = static_cast<int>(0x0ff & rgba[2]);

    if(role == Qt::DecorationRole)
      {
      QPixmap pixmap(12, 12);
      pixmap.fill(QColor(0, 0, 0, 0));
      QPainter painter(&pixmap);
      painter.setRenderHint(QPainter::Antialiasing);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QBrush(QColor(rgb[0],rgb[1],rgb[2])));
      if (this->rowCount(idx) > 0)
        {
        painter.drawEllipse(0, 0, 11, 11);
        }
      else
        {
        painter.drawEllipse(2, 2, 7, 7);
        }
      return QVariant(pixmap);
      }
    else if(role == Qt::TextColorRole)
      {
      //return QVariant(QColor(rgb[0],rgb[1],rgb[2]));
      }
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

  Qt::ItemFlags itemFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if(!this->hasChildren(idx))
    {
    itemFlags = itemFlags | Qt::ItemIsDragEnabled;
    }

  return itemFlags;
}

QVariant vtkQtTreeModelAdapter::headerData(int section, Qt::Orientation orientation,
                    int role) const
{

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
    return QVariant(this->Tree->GetVertexData()->GetArrayName(this->KeyColumn));
    }

  return QVariant();
}

QModelIndex vtkQtTreeModelAdapter::index(int row, int column,
                  const QModelIndex &parentIdx) const
{
  if (!this->Tree)
    {
    return QModelIndex();
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
    return 1;
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

  int numArrays = this->Tree->GetVertexData()->GetNumberOfArrays();
  int numDataArrays = this->DataEndColumn - this->DataStartColumn + 1;
  switch (this->ViewType)
    {
    case FULL_VIEW:
      return numArrays;
    case DATA_VIEW:
      return numDataArrays;
    default:
      vtkGenericWarningMacro("vtkQtTreeModelAdapter: Bad view type.");
    };
  return 0;
}

QStringList vtkQtTreeModelAdapter::mimeTypes() const
{
  QStringList types;
  types << "vtk/selection";
  return types;
}

QMimeData *vtkQtTreeModelAdapter::mimeData(const QModelIndexList &indexes) const
{
  // Only supports dragging single item right now ...

  if(indexes.size() == 0)
    {
    return 0;
    }

  vtkSmartPointer<vtkSelection> indexSelection = vtkSmartPointer<vtkSelection>::New();
  indexSelection.TakeReference(QModelIndexListToVTKIndexSelection(indexes));

  vtkSmartPointer<vtkSelection> pedigreeIdSelection = vtkSmartPointer<vtkSelection>::New();
  pedigreeIdSelection.TakeReference(vtkConvertSelection::ToSelectionType(indexSelection, this->Tree, vtkSelectionNode::PEDIGREEIDS));

  if(pedigreeIdSelection->GetNode(0) == 0 || pedigreeIdSelection->GetNode(0)->GetSelectionList()->GetNumberOfTuples() == 0)
    {
    return 0;
    }

  vtksys_ios::ostringstream buffer;
  buffer << pedigreeIdSelection;

  QMimeData *mime_data = new QMimeData();
  mime_data->setData("vtk/selection", buffer.str().c_str());

  return mime_data;
}

Qt::DropActions vtkQtTreeModelAdapter::supportedDragActions() const
{
   return Qt::CopyAction;
}
