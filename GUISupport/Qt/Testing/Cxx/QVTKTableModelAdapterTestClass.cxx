// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "QVTKTableModelAdapterTestClass.h"

#include <iostream>

#include <QCoreApplication>

#include "QVTKTableModelAdapter.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

QVTKTableModelAdapterTestClass::QVTKTableModelAdapterTestClass(QObject* parent)
  : QAbstractTableModel(parent)
{
  this->Errors = 0;

  this->TargetAdapter = new QVTKTableModelAdapter(this, this);
}

void QVTKTableModelAdapterTestClass::processEvents()
{
  QCoreApplication::processEvents();
}

void QVTKTableModelAdapterTestClass::testClearTable()
{
  this->removeColumns(0, columnCount());

  this->processEvents();

  if (this->TargetAdapter->GetTable()->GetNumberOfColumns() != 0)
  {
    cerr << "[TestClassQVTKTableModelAdapter::testClearTable] ERROR: Number of columns not zero."
         << endl;
    this->Errors++;
  }

  if (this->TargetAdapter->GetTable()->GetNumberOfRows() != 0)
  {
    cerr << "[TestClassQVTKTableModelAdapter::testClearTable] ERROR: Number of rows not zero."
         << endl;
    this->Errors++;
  }
}

void QVTKTableModelAdapterTestClass::testChangeHeader(int column, const QString& name)
{
  ColumnNames[column] = name;
  Q_EMIT headerDataChanged(Qt::Horizontal, column, column);

  this->processEvents();

  if (this->TargetAdapter->GetTable()->GetColumn(column)->GetName() != name)
  {
    cerr
      << "[TestClassQVTKTableModelAdapter::testChangeHeader] ERROR: Change of header data failed."
      << endl;
    this->Errors++;
  }
}

void QVTKTableModelAdapterTestClass::testColumnInsertion(int column, const QStringList& names)
{
  int old_ncols = this->TargetAdapter->GetTable()->GetNumberOfColumns();

  this->insertColumns(column, names.count());
  for (int i = 0; i < names.count(); i++)
  {
    this->testChangeHeader(column + i, names[i]);
  }

  this->processEvents();

  int new_ncols = this->TargetAdapter->GetTable()->GetNumberOfColumns();
  if (new_ncols != old_ncols + names.count())
  {
    cerr << "[TestClassQVTKTableModelAdapter::testColumnInsertion] ERROR: Mismatch in number of "
            "columns."
         << endl;
    this->Errors++;
  }
}

void QVTKTableModelAdapterTestClass::testColumnRemoval(int column, int n)
{
  int old_ncols = this->TargetAdapter->GetTable()->GetNumberOfColumns();

  this->removeColumns(column, n);

  this->processEvents();

  int new_ncols = this->TargetAdapter->GetTable()->GetNumberOfColumns();
  if (new_ncols != old_ncols - n)
  {
    cerr
      << "[TestClassQVTKTableModelAdapter::testColumnRemoval] ERROR: Mismatch in number of columns."
      << endl;
    this->Errors++;
  }
}

void QVTKTableModelAdapterTestClass::testRowInsertion(int row, int n)
{
  int old_nrows = this->TargetAdapter->GetTable()->GetNumberOfRows();
  this->insertRows(row, n);

  this->processEvents();

  int new_nrows = this->TargetAdapter->GetTable()->GetNumberOfRows();
  if (new_nrows != old_nrows + n)
  {
    cerr << "[TestClassQVTKTableModelAdapter::testRowInsertion] ERROR: Mismatch in number of rows."
         << endl;
    this->Errors++;
  }
}

void QVTKTableModelAdapterTestClass::testRowRemoval(int row, int n)
{
  int old_nrows = this->TargetAdapter->GetTable()->GetNumberOfRows();
  this->removeRows(row, n);

  this->processEvents();

  int new_nrows = this->TargetAdapter->GetTable()->GetNumberOfRows();
  if (new_nrows != old_nrows - n)
  {
    cerr << "[TestClassQVTKTableModelAdapter::testRowRemoval] ERROR: Mismatch in number of rows."
         << endl;
    this->Errors++;
  }
}

void QVTKTableModelAdapterTestClass::testInsertRemoveColumns()
{
  this->testClearTable();
  this->testColumnInsertion(0, QStringList() << "0"); // insert into empty
  this->testColumnInsertion(1, QStringList() << "3"); // insert at end
  this->testColumnInsertion(1,
    QStringList() << "1"
                  << "2"); // insert two column in the middle

  QStringList headers;
  QStringList target = QStringList() << "0"
                                     << "1"
                                     << "2"
                                     << "3";
  for (int i = 0; i < this->TargetAdapter->GetTable()->GetNumberOfColumns(); i++)
  {
    headers += this->TargetAdapter->GetTable()->GetColumn(i)->GetName();
  }
  if (headers != target)
  {
    cerr << "[TestClassQVTKTableModelAdapter::testInsertRemoveColumns] ERROR: Mismatch in column "
            "header names after inserting columns."
         << endl;
    this->Errors++;
  }

  // now remove 2 middle columns
  this->testColumnRemoval(1, 2);
  target.removeAt(2);
  target.removeAt(1);

  this->processEvents();

  headers.clear();
  for (int i = 0; i < this->TargetAdapter->GetTable()->GetNumberOfColumns(); i++)
  {
    headers += this->TargetAdapter->GetTable()->GetColumn(i)->GetName();
  }
  if (headers != target)
  {
    cerr << "[TestClassQVTKTableModelAdapter::testInsertRemoveColumns] ERROR: Mismatch in column "
            "header names after removing columns."
         << endl;
    this->Errors++;
  }
}

void QVTKTableModelAdapterTestClass::testInsertRemoveRows()
{
  this->testClearTable();
  this->testColumnInsertion(0, QStringList() << "x");

  vtkTable* table = this->TargetAdapter->GetTable();
  vtkDoubleArray* x_col = vtkDoubleArray::SafeDownCast(table->GetColumnByName("x"));

  this->testRowInsertion(0, 1); // insert "0" at front
  this->setData(index(0, 0), 0.0, Qt::EditRole);
  this->processEvents();

  this->testRowInsertion(1, 1); // insert "3" at back
  this->setData(index(1, 0), 3.0, Qt::EditRole);
  this->processEvents();

  this->testRowInsertion(1, 2); // insert "1, 2" in middle
  this->setData(index(1, 0), 1.0, Qt::EditRole);
  this->setData(index(2, 0), 2.0, Qt::EditRole);
  this->processEvents();

  for (int i = 0; i < x_col->GetNumberOfTuples(); i++)
  {
    if (x_col->GetTuple1(i) != i)
    {
      cerr << "[TestClassQVTKTableModelAdapter::testInsertRemoveRows] ERROR: Mismatch in row data "
              "after inserting rows."
           << endl;
      this->Errors++;
    }
  }

  // now remove 2 middle rows
  this->testRowRemoval(1, 2);

  if ((x_col->GetTuple1(0) != 0) || (x_col->GetTuple1(1) != 3))
  {
    cerr << "[TestClassQVTKTableModelAdapter::testInsertRemoveRows] ERROR: Mismatch in row data "
            "after removing rows."
         << endl;
    this->Errors++;
  }
}

int QVTKTableModelAdapterTestClass::rowCount(const QModelIndex& vtkNotUsed(parent)) const
{
  if (this->ColumnData.isEmpty())
  {
    return 0;
  }
  return ColumnData[0].count();
}

int QVTKTableModelAdapterTestClass::columnCount(const QModelIndex& vtkNotUsed(parent)) const
{
  return this->ColumnData.count();
}

bool QVTKTableModelAdapterTestClass::insertRows(int row, int count, const QModelIndex& parent)
{
  beginInsertRows(parent, row, row + count - 1);
  for (int j = 0; j < count; j++)
  {
    for (int i = 0; i < this->ColumnData.count(); i++)
    {
      this->ColumnData[i].insert(row, 0);
    }
  }
  endInsertRows();
  return true;
}

bool QVTKTableModelAdapterTestClass::removeRows(int row, int count, const QModelIndex& parent)
{
  beginRemoveRows(parent, row, row + count - 1);
  for (int j = 0; j < count; j++)
  {
    for (int i = 0; i < this->ColumnData.count(); i++)
    {
      if (this->ColumnData[i].count() > row)
      {
        this->ColumnData[i].removeAt(row);
      }
    }
  }
  endRemoveRows();
  return true;
}

bool QVTKTableModelAdapterTestClass::insertColumns(int column, int count, const QModelIndex& parent)
{
  int nrows = 0;
  if (this->ColumnData.count())
  {
    nrows = this->ColumnData[0].count();
  }
  beginInsertColumns(parent, column, column + count - 1);
  for (int i = column; i < column + count; i++)
  {
    QVector<double> nv;
    nv.resize(nrows);
    this->ColumnData.insert(i, nv);
    // find a new unique column name
    for (int j = 0; j < std::numeric_limits<int>::max(); j++)
    {
      QString new_col = QString("col %1").arg(j);
      if (this->ColumnNames.indexOf(new_col) < 0)
      {
        this->ColumnNames.insert(i, new_col);
        break;
      }
    }
  }
  endInsertColumns();
  return false;
}

bool QVTKTableModelAdapterTestClass::removeColumns(int column, int count, const QModelIndex& parent)
{
  beginRemoveColumns(parent, column, column + count - 1);
  for (int i = 0; i < count; i++)
  {
    this->ColumnData.removeAt(column);
    this->ColumnNames.removeAt(column);
  }
  endRemoveColumns();
  return true;
}

QVariant QVTKTableModelAdapterTestClass::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
  {
    return QVariant();
  }
  if ((role != Qt::EditRole) && (role != Qt::DisplayRole))
  {
    return QVariant();
  }
  return this->ColumnData[index.column()][index.row()];
}

bool QVTKTableModelAdapterTestClass::setData(
  const QModelIndex& index, const QVariant& value, int role)
{
  if (!index.isValid())
  {
    return false;
  }
  if (role != Qt::EditRole)
  {
    return false;
  }
  this->ColumnData[index.column()][index.row()] = value.toDouble();
  Q_EMIT dataChanged(index, index);
  return true;
}

QVariant QVTKTableModelAdapterTestClass::headerData(
  int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole)
  {
    return QVariant();
  }
  if (orientation == Qt::Vertical)
  {
    return section;
  }
  return this->ColumnNames[section];
}

void QVTKTableModelAdapterTestClass::runTests()
{
  this->testInsertRemoveColumns();
  this->testInsertRemoveRows();

  QCoreApplication::exit(this->Errors);
}
