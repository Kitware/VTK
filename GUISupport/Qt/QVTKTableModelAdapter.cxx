// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "QVTKTableModelAdapter.h"

#include <vtkCharArray.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkLongLongArray.h>
#include <vtkStringArray.h>
#include <vtkType.h>
#include <vtkUnsignedIntArray.h>
#include <vtkUnsignedLongLongArray.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define vtk_qVariantType(variant) variant.type()
#define vtk_qMetaType(name) QVariant::name
#define vtk_qMetaType_Q(name) vtk_qMetaType(name)
#else
#define vtk_qVariantType(variant) variant.typeId()
#define vtk_qMetaType(name) QMetaType::name
#define vtk_qMetaType_Q(name) vtk_qMetaType(Q##name)
#endif

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
QVTKTableModelAdapter::QVTKTableModelAdapter(QObject* parent)
  : QObject(parent)
{
}

//------------------------------------------------------------------------------
QVTKTableModelAdapter::QVTKTableModelAdapter(QAbstractItemModel* model, QObject* parent)
  : QObject(parent)
{
  this->SetItemModel(model);
}

//------------------------------------------------------------------------------
QVariant QVTKTableModelAdapter::modelData(int row, int col)
{
  return this->ItemModel->data(this->ItemModel->index(row, col), Qt::DisplayRole);
}

//------------------------------------------------------------------------------
vtkAbstractArray* QVTKTableModelAdapter::NewArray(const QVariant& type)
{
  // if type is not valid then return a default type
  if (!type.isValid())
  {
    return vtkDoubleArray::New();
  }

  switch (vtk_qVariantType(type))
  {
    case vtk_qMetaType(Double):
      return vtkDoubleArray::New();
    case vtk_qMetaType(Char):
      return vtkCharArray::New();
    case vtk_qMetaType(Int):
      return vtkIntArray::New();
    case vtk_qMetaType(UInt):
      return vtkUnsignedIntArray::New();
    case vtk_qMetaType(LongLong):
      return vtkLongLongArray::New();
    case vtk_qMetaType(ULongLong):
      return vtkUnsignedLongLongArray::New();
    case vtk_qMetaType_Q(String):
      return vtkStringArray::New();
    default:
      // default: return a vtkDoubleArray for unsupported types
      QString warning =
        QString("Unsupported QVariant::Type '%1' in QVTKTableModelAdapter::NewArray - "
                "default to returning vtkDoubleArray::New()")
          .arg(type.typeName());
      vtkGenericWarningMacro(<< warning.toStdString());

      return vtkDoubleArray::New();
  }
}

//------------------------------------------------------------------------------
bool QVTKTableModelAdapter::HasCorrectColumnArrays()
{
  if (!this->ItemModel)
  {
    return false;
  }
  bool correct = true;
  for (int c = 0; c < this->Table->GetNumberOfColumns(); c++)
  {
    QVariant t = this->modelData(0, c);
    vtkAbstractArray* arr = this->Table->GetColumn(c);
    switch (vtk_qVariantType(t))
    {
      case vtk_qMetaType(Double):
        correct = correct && vtkDoubleArray::SafeDownCast(arr);
        break;
      case vtk_qMetaType(Char):
        correct = correct && vtkCharArray::SafeDownCast(arr);
        break;
      case vtk_qMetaType(Int):
        correct = correct && vtkIntArray::SafeDownCast(arr);
        break;
      case vtk_qMetaType(UInt):
        correct = correct && vtkUnsignedIntArray::SafeDownCast(arr);
        break;
      case vtk_qMetaType(LongLong):
        correct = correct && vtkLongLongArray::SafeDownCast(arr);
        break;
      case vtk_qMetaType(ULongLong):
        correct = correct && vtkUnsignedLongLongArray::SafeDownCast(arr);
        break;
      case vtk_qMetaType_Q(String):
        correct = correct && vtkStringArray::SafeDownCast(arr);
        break;
      default:
        // continue with the current array type; proper conversion may not be possible
        continue;
    }
    if (!correct)
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::SetCellValue(int row, int column, const QVariant& data)
{
  if ((row < 0) || (row >= this->Table->GetNumberOfRows()))
  {
    return;
  }
  if ((column < 0) || (column >= this->Table->GetNumberOfColumns()))
  {
    return;
  }
  vtkDataArray* data_arr = vtkDataArray::SafeDownCast(this->Table->GetColumn(column));
  if (data_arr)
  {
    switch (vtk_qVariantType(data))
    {
      case vtk_qMetaType(Double):
        data_arr->SetTuple1(row, data.toDouble());
        break;
      case vtk_qMetaType(Char):
        data_arr->SetTuple1(row, data.toInt());
        break;
      case vtk_qMetaType(Int):
        data_arr->SetTuple1(row, data.toInt());
        break;
      case vtk_qMetaType(UInt):
        data_arr->SetTuple1(row, data.toUInt());
        break;
      case vtk_qMetaType(LongLong):
        data_arr->SetTuple1(row, data.toLongLong());
        break;
      case vtk_qMetaType(ULongLong):
        data_arr->SetTuple1(row, data.toULongLong());
        break;
      default:
        // for unhandled QVariant::Type use toDouble() conversion
        data_arr->SetTuple1(row, data.toDouble());
        break;
    }
    return;
  }
  vtkStringArray* strArr = vtkStringArray::SafeDownCast(this->Table->GetColumn(column));
  if (strArr)
  {
    strArr->SetValue(row, data.toString().toStdString());
    return;
  }
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::UpdateTable(int row0, int column0, int row1, int column1)
{
  row0 = std::max<int>(0, std::min<int>(row0, this->Table->GetNumberOfRows() - 1));
  row1 = std::max<int>(0, std::min<int>(row1, this->Table->GetNumberOfRows() - 1));
  column0 = std::max<int>(0, std::min<int>(column0, this->Table->GetNumberOfColumns() - 1));
  column1 = std::max<int>(0, std::min<int>(column1, this->Table->GetNumberOfColumns() - 1));
  for (int c = column0; c <= column1; c++)
  {
    for (int r = row0; r <= row1; r++)
    {
      this->SetCellValue(r, c, this->modelData(r, c));
    }
  }
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::SetItemModel(QAbstractItemModel* model)
{
  if (this->ItemModel)
  {
    this->ItemModel->disconnect(this);
  }
  this->ItemModel = model;

  QObject::connect(this->ItemModel, SIGNAL(destroyed(QObject*)), this, SLOT(onModelReset()));

  QObject::connect(this->ItemModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)),
    this, SLOT(onDataChanged(QModelIndex, QModelIndex, QVector<int>)));

  QObject::connect(this->ItemModel, SIGNAL(headerDataChanged(Qt::Orientation, int, int)), this,
    SLOT(onHeaderDataChanged(Qt::Orientation, int, int)));

  QObject::connect(this->ItemModel,
    SIGNAL(layoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)), this,
    SLOT(onLayoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));

  QObject::connect(this->ItemModel, SIGNAL(rowsInserted(QModelIndex, int, int)), this,
    SLOT(onRowsInserted(QModelIndex, int, int)));

  QObject::connect(this->ItemModel, SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
    SLOT(onRowsRemoved(QModelIndex, int, int)));

  QObject::connect(this->ItemModel, SIGNAL(columnsInserted(QModelIndex, int, int)), this,
    SLOT(onColumnsInserted(QModelIndex, int, int)));

  QObject::connect(this->ItemModel, SIGNAL(columnsRemoved(QModelIndex, int, int)), this,
    SLOT(onColumnsRemoved(QModelIndex, int, int)));

  QObject::connect(this->ItemModel, SIGNAL(modelReset()), this, SLOT(onModelReset()));

  QObject::connect(this->ItemModel, SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)),
    this, SLOT(onRowsMoved(QModelIndex, int, int, QModelIndex, int)));

  QObject::connect(this->ItemModel, SIGNAL(columnsMoved(QModelIndex, int, int, QModelIndex, int)),
    this, SLOT(onColumnsMoved(QModelIndex, int, int, QModelIndex, int)));

  // rebuild the table
  this->onModelReset();
}

//------------------------------------------------------------------------------
QAbstractItemModel* QVTKTableModelAdapter::GetItemModel() const
{
  return this->ItemModel;
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onModified()
{
  this->Table->Modified();
  Q_EMIT(tableChanged());
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onModelReset()
{
  this->Table->RemoveAllColumns();
  this->onModified();

  if (!this->ItemModel)
  {
    return;
  }

  int ncol = this->ItemModel->columnCount();
  int nrow = this->ItemModel->rowCount();

  for (int c = 0; c < ncol; c++)
  {
    vtkAbstractArray* array = this->NewArray(this->modelData(0, c));
    array->SetName(this->ItemModel->headerData(c, Qt::Horizontal, Qt::DisplayRole)
                     .toString()
                     .toStdString()
                     .c_str());
    this->Table->AddColumn(array);
    // for correct reference counting, see vtkObjectBase::Delete()
    array->Delete();
  }

  this->Table->SetNumberOfRows(nrow);
  this->UpdateTable(0, 0, nrow - 1, ncol - 1);

  this->onModified();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onDataChanged(
  const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
  if (!this->ItemModel)
  {
    return;
  }

  // only update the table if Qt::DisplayRole has changed
  // if roles is empty then this is implied
  if (roles.count() && (roles.indexOf(Qt::DisplayRole) < 0))
  {
    return;
  }

  this->UpdateTable(topLeft.row(), topLeft.column(), bottomRight.row(), bottomRight.column());
  this->onModified();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
  if (!this->ItemModel)
  {
    return;
  }
  if (orientation != Qt::Horizontal)
  {
    return;
  }
  for (int c = first; c <= last; c++)
  {
    this->Table->GetColumn(c)->SetName(
      this->ItemModel->headerData(c, Qt::Horizontal, Qt::DisplayRole)
        .toString()
        .toStdString()
        .c_str());
  }
  this->onModified();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onLayoutChanged(const QList<QPersistentModelIndex>& vtkNotUsed(parents),
  QAbstractItemModel::LayoutChangeHint vtkNotUsed(hint))
{
  // Resetting the table in that case is a reasonable choice
  this->onModelReset();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onRowsInserted(
  const QModelIndex& vtkNotUsed(parent), int first, int last)
{
  // if the table currently has no rows then the arrays in there may be of the wrong type
  // hence here is a good point to set the correct array types
  if (this->Table->GetNumberOfRows() == 0)
  {
    if (!this->HasCorrectColumnArrays())
    {
      // perform a complete reset, which will create the correct column arrays
      this->onModelReset();
      return;
    }
  }

  if (!this->ItemModel)
  {
    return;
  }

  this->Table->InsertRows(first, last - first + 1);
  this->UpdateTable(first, 0, last, this->ItemModel->columnCount() - 1);
  this->onModified();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onRowsRemoved(
  const QModelIndex& vtkNotUsed(parent), int first, int last)
{
  if (!this->ItemModel)
  {
    return;
  }
  this->Table->RemoveRows(first, last - first + 1);
  this->Table->SqueezeRows();
  this->onModified();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onRowsMoved(const QModelIndex& vtkNotUsed(parent),
  int vtkNotUsed(start), int vtkNotUsed(end), const QModelIndex& vtkNotUsed(destination),
  int vtkNotUsed(row))
{
  // Resetting the table in that case is a reasonable choice
  this->onModelReset();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onColumnsInserted(
  const QModelIndex& vtkNotUsed(parent), int first, int last)
{
  if (!this->ItemModel)
  {
    return;
  }
  int nrow = this->ItemModel->rowCount();

  for (int c = first; c <= last; c++)
  {
    vtkAbstractArray* array = this->NewArray(this->modelData(0, c));
    QString array_name = this->ItemModel->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
    array->SetName(array_name.toStdString().c_str());
    array->SetNumberOfTuples(nrow);
    this->Table->InsertColumn(array, c);
    this->UpdateTable(0, c, nrow - 1, c);
    // for correct reference counting, see vtkObjectBase::Delete()
    array->Delete();
  }
  this->onModified();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onColumnsRemoved(
  const QModelIndex& vtkNotUsed(parent), int first, int last)
{
  if (!this->ItemModel)
  {
    return;
  }

  for (int c = first; c <= last; c++)
  {
    this->Table->RemoveColumn(first);
  }
  this->onModified();
}

//------------------------------------------------------------------------------
void QVTKTableModelAdapter::onColumnsMoved(const QModelIndex& vtkNotUsed(parent),
  int vtkNotUsed(start), int vtkNotUsed(end), const QModelIndex& vtkNotUsed(destination),
  int vtkNotUsed(column))
{
  // Resetting the table in that case is a reasonable choice
  this->onModelReset();
}

//------------------------------------------------------------------------------
vtkTable* QVTKTableModelAdapter::GetTable() const
{
  return this->Table;
}
VTK_ABI_NAMESPACE_END
