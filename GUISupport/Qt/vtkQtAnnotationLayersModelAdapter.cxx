/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtAnnotationLayersModelAdapter.cxx

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
#include "vtkQtAnnotationLayersModelAdapter.h"

#include "vtkDataSetAttributes.h"
#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkVariant.h"
#include "vtkDoubleArray.h"

#include <QIcon>
#include <QPixmap>
#include <QHash>
#include <QMap>


//----------------------------------------------------------------------------
vtkQtAnnotationLayersModelAdapter::vtkQtAnnotationLayersModelAdapter(QObject* p)
  : vtkQtAbstractModelAdapter(p)
{
  this->Annotations = NULL;
}

//----------------------------------------------------------------------------
vtkQtAnnotationLayersModelAdapter::vtkQtAnnotationLayersModelAdapter(vtkAnnotationLayers* t, QObject* p)
  : vtkQtAbstractModelAdapter(p), Annotations(t)
{
  if (this->Annotations != NULL)
    {
    this->Annotations->Register(0);
    }
}

//----------------------------------------------------------------------------
vtkQtAnnotationLayersModelAdapter::~vtkQtAnnotationLayersModelAdapter()
{
  if (this->Annotations != NULL)
    {
    this->Annotations->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkQtAnnotationLayersModelAdapter::SetKeyColumnName(
  const char *vtkNotUsed(name))
{
/*
  if (name == 0)
    {
    this->KeyColumn = -1;
    }
  else
    {
    this->KeyColumn = -1;
    for (int i = 0; i < static_cast<int>(this->Annotations->GetNumberOfColumns()); i++)
      {
      if (!strcmp(name, this->Annotations->GetColumn(i)->GetName()))
        {
        this->KeyColumn = i;
        break;
        }
      }
    }
    */
}

// ----------------------------------------------------------------------------
void vtkQtAnnotationLayersModelAdapter::SetColorColumnName(
  const char *vtkNotUsed(name))
{
}

//----------------------------------------------------------------------------
void vtkQtAnnotationLayersModelAdapter::SetVTKDataObject(vtkDataObject *obj)
{
  vtkAnnotationLayers *t = vtkAnnotationLayers::SafeDownCast(obj);
  if (obj && !t)
    {
    qWarning("vtkQtAnnotationLayersModelAdapter needs a vtkAnnotationLayers for SetVTKDataObject");
    return;
    }

  // Okay it's a table so set it :)
  this->setAnnotationLayers(t);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkQtAnnotationLayersModelAdapter::GetVTKDataObject() const
{
  return this->Annotations;
}

//----------------------------------------------------------------------------
void vtkQtAnnotationLayersModelAdapter::setAnnotationLayers(vtkAnnotationLayers* t)
{
  if (this->Annotations != NULL)
    {
    this->Annotations->Delete();
    }
  this->Annotations = t;
  if (this->Annotations != NULL)
    {
    this->Annotations->Register(0);

    // When setting a table, update the QHash tables for column mapping.
    // If SplitMultiComponentColumns is disabled, this call will just clear
    // the tables and return.
    //this->updateModelColumnHashTables();

    // We will assume the table is totally
    // new and any views should update completely
    emit this->reset();
    }
}

//----------------------------------------------------------------------------
bool vtkQtAnnotationLayersModelAdapter::noAnnotationsCheck() const
{
  if (this->Annotations == NULL)
    {
    // It's not necessarily an error to have a null pointer for the
    // table.  It just means that the model is empty.
    return true;
    }
  if (this->Annotations->GetNumberOfAnnotations() == 0)
    {
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
// Description:
// Selection conversion from VTK land to Qt land
vtkAnnotationLayers* vtkQtAnnotationLayersModelAdapter::QModelIndexListToVTKAnnotationLayers(
  const QModelIndexList qmil) const
{
  // Create vtk index selection
  vtkAnnotationLayers* annotations = vtkAnnotationLayers::New(); // Caller needs to delete

  // Run through the QModelIndexList pulling out vtk indexes
  for (int i = 0; i < qmil.size(); i++)
    {
    vtkIdType vtk_index = qmil.at(i).internalId();
    //annotations->AddLayer();
    annotations->AddAnnotation(this->Annotations->GetAnnotation(vtk_index));
    }
  return annotations;
}

//----------------------------------------------------------------------------
QItemSelection vtkQtAnnotationLayersModelAdapter::VTKAnnotationLayersToQItemSelection(
  vtkAnnotationLayers *vtkNotUsed(vtkann)) const
{

  QItemSelection qis_list;
  /*
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
  */
  return qis_list;
}

//----------------------------------------------------------------------------
// Description:
// Selection conversion from VTK land to Qt land
vtkSelection* vtkQtAnnotationLayersModelAdapter::QModelIndexListToVTKIndexSelection(
  const QModelIndexList vtkNotUsed(qmil)) const
{
/*
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
  */
  return 0;
}

//----------------------------------------------------------------------------
QItemSelection vtkQtAnnotationLayersModelAdapter::VTKIndexSelectionToQItemSelection(
  vtkSelection *vtkNotUsed(vtksel)) const
{

  QItemSelection qis_list;
  /*
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
    */
  return qis_list;
}


//----------------------------------------------------------------------------
QVariant vtkQtAnnotationLayersModelAdapter::data(const QModelIndex &idx, int role) const
{
  if (this->noAnnotationsCheck())
    {
    return QVariant();
    }
  if (!idx.isValid())
    {
    return QVariant();
    }
  if(idx.row() >= static_cast<int>(this->Annotations->GetNumberOfAnnotations()))
    {
    return QVariant();
    }

  vtkAnnotation *a = this->Annotations->GetAnnotation(idx.row());
  int numItems = 0;
  vtkSelection *s = a->GetSelection();
  if(s)
    {
    for(unsigned int i=0; i<s->GetNumberOfNodes(); ++i)
      {
      numItems += s->GetNode(i)->GetSelectionList()->GetNumberOfTuples();
      }
    }

  double *color = a->GetInformation()->Get(vtkAnnotation::COLOR());
  int annColor[3];
  annColor[0] = static_cast<int>(255*color[0]);
  annColor[1] = static_cast<int>(255*color[1]);
  annColor[2] = static_cast<int>(255*color[2]);

  if (role == Qt::DisplayRole)
    {
    switch(idx.column())
      {
      case 1:
        return QVariant(numItems);
      case 2:
        return QVariant(a->GetInformation()->Get(vtkAnnotation::LABEL()));
      default:
        return QVariant();
      }
    }
  else if (role == Qt::DecorationRole)
    {
    switch(idx.column())
      {
      case 0:
        return QColor(annColor[0], annColor[1], annColor[2]);
      default:
        return QVariant();
      }
    }

  return QVariant();
}

//----------------------------------------------------------------------------
bool vtkQtAnnotationLayersModelAdapter::setData(const QModelIndex &vtkNotUsed(idx),
                                                const QVariant &vtkNotUsed(value),
                                                int vtkNotUsed(role))
{
/*
  if (role == Qt::DecorationRole)
    {
    this->Internal->IndexToDecoration[idx] = value;
    emit this->dataChanged(idx, idx);
    return true;
    }
 */
  return false;
}

//----------------------------------------------------------------------------
Qt::ItemFlags vtkQtAnnotationLayersModelAdapter::flags(const QModelIndex &idx) const
{
  if (!idx.isValid())
    {
    return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
}

//----------------------------------------------------------------------------
QVariant vtkQtAnnotationLayersModelAdapter::headerData(int section, Qt::Orientation orientation,
                    int role) const
{
  if (this->noAnnotationsCheck())
    {
    return QVariant();
    }

  if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
    switch(section)
      {
      case 0:
        return QVariant("C");
      case 1:
        return QVariant("# Items");
      case 2:
        return QVariant("Label");
      default:
        return QVariant();
      }
    }

  return QVariant();
}

//----------------------------------------------------------------------------
QModelIndex vtkQtAnnotationLayersModelAdapter::index(int row, int column,
                  const QModelIndex & vtkNotUsed(parentIdx)) const
{
  return createIndex(row, column, row);
}

//----------------------------------------------------------------------------
QModelIndex vtkQtAnnotationLayersModelAdapter::parent(const QModelIndex & vtkNotUsed(idx)) const
{
  return QModelIndex();
}

//----------------------------------------------------------------------------
int vtkQtAnnotationLayersModelAdapter::rowCount(const QModelIndex & mIndex) const
{
  if (this->noAnnotationsCheck())
    {
    return 0;
    }
  if (mIndex == QModelIndex())
    {
    return this->Annotations->GetNumberOfAnnotations();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkQtAnnotationLayersModelAdapter::columnCount(const QModelIndex &) const
{
  if (this->noAnnotationsCheck())
    {
    return 0;
    }

  return 3;
}
/*
Qt::DropActions vtkQtAnnotationLayersModelAdapter::supportedDropActions() const
{
   return Qt::MoveAction;
}

Qt::DropActions vtkQtAnnotationLayersModelAdapter::supportedDragActions() const
{
   return Qt::MoveAction;
}

bool vtkQtAnnotationLayersModelAdapter::insertRows(int row, int count, const QModelIndex &p)
{
  emit this->beginInsertRows(p,row,row+count-1);
  for(int i=0; i<count; ++i)
    {
    this->Annotations->InsertLayer(row);
    }
  emit this->endInsertRows();

  return true;
}

bool vtkQtAnnotationLayersModelAdapter::removeRows(int row, int count, const QModelIndex &p)
{
  emit this->beginRemoveRows(p,row,row+count-1);
  for(int i=0; i<count; ++i)
    {
    this->Annotations->RemoveAnnotation(this->Annotations->GetAnnotation(row));
    }
  emit this->endRemoveRows();

  return true;
}

bool vtkQtAnnotationLayersModelAdapter::dropMimeData(const QMimeData *data,
     Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  if (action == Qt::IgnoreAction)
    return true;

  if (!data->hasFormat("application/vnd.text.list"))
    return false;

  if (column > 0)
    return false;


}

QStringList vtkQtAnnotationLayersModelAdapter::mimeTypes() const
{
  QStringList types;
  types << "application/x-color" << "text/plain";
  return types;
}

QMimeData *vtkQtAnnotationLayersModelAdapter::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  foreach (QModelIndex index, indexes) {
     if (index.isValid()) {
         stream << data(index, Qt::DisplayRole).toByteArray();
     }
  }

  mimeData->setData("application/vnd.text.list", encodedData);
  return mimeData;
}
*/
